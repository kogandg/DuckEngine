#include <Duck/Core/Bvh.h>

#include <Duck/Core/Assert.h>
#include <Duck/Core/Stopwatch.h>

#include <algorithm>
#include <numeric>

namespace Duck
{
    struct Bvh::BuildInput
    {
        std::span<const Aabb> PrimitiveBounds;
        std::vector<Vec3> Centroids;
        BvhBuildSettings Settings;

        const Aabb& BoundsOf(int primitive) const { return PrimitiveBounds[static_cast<size_t>(primitive)]; }
        const Vec3& CentroidOf(int primitive) const { return Centroids[static_cast<size_t>(primitive)]; }
    };

    Bvh::Bvh(std::span<const Aabb> primitiveBounds, const BvhBuildSettings& settings)
    {
        const Stopwatch stopwatch;

        DUCK_VERIFY(settings.MaxLeafSize >= 1, "A BVH leaf must be able to hold a primitive");
        DUCK_VERIFY(settings.BinCount >= 2 && settings.BinCount <= MaxBins, "BVH bin count {} not in [2, {}]",
                    settings.BinCount, MaxBins);
        const int count = static_cast<int>(primitiveBounds.size());
        if (count == 0)
        {
            return;
        }

        primitiveIndices.resize(primitiveBounds.size());
        std::iota(primitiveIndices.begin(), primitiveIndices.end(), 0);

        // primitives are sorted by the center of their box
        BuildInput input{primitiveBounds, std::vector<Vec3>(primitiveBounds.size()), settings};
        for (size_t i = 0; i < primitiveBounds.size(); i++)
        {
            DUCK_VERIFY(!primitiveBounds[i].IsEmpty(), "Primitive {} has an empty box", i);
            input.Centroids[i] = 0.5f * (primitiveBounds[i].Min + primitiveBounds[i].Max);
        }

        // a binary tree whose leaves hold at least one primitive has at most 2n - 1 nodes
        nodes.reserve(2 * primitiveBounds.size() - 1);
        BvhNode& root = nodes.emplace_back();
        root.First = 0;
        root.Count = count;
        UpdateBounds(root, input);
        Subdivide(0, 0, input);

        nodes.shrink_to_fit();

        CollectBuildStats(stopwatch.ElapsedMs());
    }

    void Bvh::CollectBuildStats(double buildMs)
    {
        buildStats = {};
        buildStats.BuildMs = buildMs;
        buildStats.Primitives = static_cast<int>(primitiveIndices.size());
        buildStats.Nodes = static_cast<int>(nodes.size());
        buildStats.SahCost = SahCost();

        std::vector<std::pair<int, int>> stack{{0, 0}}; // node, depth
        while (!stack.empty())
        {
            const auto [index, depth] = stack.back();
            stack.pop_back();
            const BvhNode& node = nodes[static_cast<size_t>(index)];
            if (node.IsLeaf())
            {
                buildStats.Leaves++;
                buildStats.MaxDepth = std::max(buildStats.MaxDepth, depth);
                buildStats.LargestLeaf = std::max(buildStats.LargestLeaf, node.Count);
            }
            else
            {
                stack.push_back({node.First, depth + 1});
                stack.push_back({node.First + 1, depth + 1});
            }
        }
    }

    void Bvh::UpdateBounds(BvhNode& node, const BuildInput& input) const
    {
        node.Bounds = {};
        for (int i = node.First; i < node.First + node.Count; i++)
        {
            node.Bounds.Grow(input.BoundsOf(primitiveIndices[static_cast<size_t>(i)]));
        }
    }

    void Bvh::Subdivide(int nodeIndex, int depth, const BuildInput& input)
    {
        const BvhNode& node = nodes[static_cast<size_t>(nodeIndex)];
        if (node.Count == 1 || depth >= MaxDepth)
        {
            return;
        }

        // split across the longest side of the box around the centroids, not around the primitives: big
        // primitives would stretch that box without helping to separate anything
        Aabb centroidBounds;
        for (int i = node.First; i < node.First + node.Count; i++)
        {
            centroidBounds.Grow(input.CentroidOf(primitiveIndices[static_cast<size_t>(i)]));
        }

        const int leftCount = input.Settings.Split == BvhSplitMethod::SAH ? SplitSah(node, centroidBounds, input)
                                                                          : SplitMidpoint(node, centroidBounds, input);
        if (leftCount == 0)
        {
            return; // this node is a leaf
        }
        DUCK_ASSERT(leftCount < node.Count, "A split must leave primitives on both sides");

        const int first = node.First;
        const int count = node.Count;
        const int left = static_cast<int>(nodes.size());
        nodes.push_back({.Bounds = {}, .First = first, .Count = leftCount}); // invalidates node
        nodes.push_back({.Bounds = {}, .First = first + leftCount, .Count = count - leftCount});
        UpdateBounds(nodes[static_cast<size_t>(left)], input);
        UpdateBounds(nodes[static_cast<size_t>(left + 1)], input);

        BvhNode& parent = nodes[static_cast<size_t>(nodeIndex)]; // taken after the push_backs
        parent.First = left;
        parent.Count = 0; // now interior

        Subdivide(left, depth + 1, input);
        Subdivide(left + 1, depth + 1, input);
    }

    int Bvh::SplitMidpoint(const BvhNode& node, const Aabb& centroidBounds, const BuildInput& input)
    {
        if (node.Count <= input.Settings.MaxLeafSize)
        {
            return 0; // small enough
        }

        const Vec3 extent = centroidBounds.Max - centroidBounds.Min;
        int axis = 0;
        if (extent.y > extent.x)
        {
            axis = 1;
        }
        if (extent.z > extent[axis])
        {
            axis = 2;
        }
        if (extent[axis] <= 0.0f)
        {
            return 0; // every centroid in the same place: no plane can separate them
        }

        // everything with its centroid below the middle goes left
        const float split = centroidBounds.Min[axis] + 0.5f * extent[axis];
        const auto begin = primitiveIndices.begin() + node.First;
        const auto middle = std::partition(begin, begin + node.Count,
                                           [&](int primitive) { return input.CentroidOf(primitive)[axis] < split; });
        const int leftCount = static_cast<int>(middle - begin);
        return leftCount == node.Count ? 0 : leftCount; // rounding put everything on one side
    }

    int Bvh::SplitSah(const BvhNode& node, const Aabb& centroidBounds, const BuildInput& input)
    {
        const int binCount = input.Settings.BinCount;
        const Vec3 extent = centroidBounds.Max - centroidBounds.Min;

        // the bin a centroid falls in along one axis. used both to fill the bins and to split, so the two always
        // agree on which side every primitive is
        const auto binOf = [&](int axis, int primitive)
        {
            const float scale = static_cast<float>(binCount) / extent[axis];
            const int bin = static_cast<int>((input.CentroidOf(primitive)[axis] - centroidBounds.Min[axis]) * scale);
            return std::min(bin, binCount - 1); // the largest centroid lands exactly on binCount
        };

        struct Bin
        {
            Aabb Bounds; // around the primitives in it, not their centroids
            int Count = 0;
        };

        // best split found: bins [0, bestBin] go left, the rest go right. cost is sum of count * area of the two
        // sides, the part of the SAH cost that depends on where the split is
        float bestCost = Infinity;
        int bestAxis = -1;
        int bestBin = -1;

        for (int axis = 0; axis < 3; axis++)
        {
            if (extent[axis] <= 0.0f)
            {
                continue; // every centroid at the same coordinate: nothing to separate along this axis
            }

            std::array<Bin, MaxBins> bins{};
            for (int i = node.First; i < node.First + node.Count; i++)
            {
                const int primitive = primitiveIndices[static_cast<size_t>(i)];
                Bin& bin = bins[static_cast<size_t>(binOf(axis, primitive))];
                bin.Count++;
                bin.Bounds.Grow(input.BoundsOf(primitive));
            }

            // sweep from the left: whats left of the plane after bin i. growing by an empty bin changes nothing
            std::array<float, MaxBins> leftArea{};
            std::array<int, MaxBins> leftCount{};
            Aabb leftBox;
            int leftSum = 0;
            for (int i = 0; i < binCount - 1; i++)
            {
                leftBox.Grow(bins[static_cast<size_t>(i)].Bounds);
                leftSum += bins[static_cast<size_t>(i)].Count;
                leftCount[static_cast<size_t>(i)] = leftSum;
                leftArea[static_cast<size_t>(i)] = leftSum > 0 ? leftBox.SurfaceArea() : 0.0f;
            }

            // sweep from the right, and price each plane on the way
            Aabb rightBox;
            int rightSum = 0;
            for (int i = binCount - 1; i > 0; i--)
            {
                rightBox.Grow(bins[static_cast<size_t>(i)].Bounds);
                rightSum += bins[static_cast<size_t>(i)].Count;

                const int plane = i - 1; // between bins i - 1 and i
                const int leftSide = leftCount[static_cast<size_t>(plane)];
                if (leftSide == 0 || rightSum == 0)
                {
                    continue; // everything on one side isnt a split
                }
                const float cost = static_cast<float>(leftSide) * leftArea[static_cast<size_t>(plane)] +
                                   static_cast<float>(rightSum) * rightBox.SurfaceArea();
                if (cost < bestCost)
                {
                    bestCost = cost;
                    bestAxis = axis;
                    bestBin = plane;
                }
            }
        }

        if (bestAxis < 0)
        {
            return 0; // every centroid in the same place
        }

        // splitting costs a traversal step, then each sides primitives weighted by the chance a ray reaching this
        // node goes into that side (area ratio). staying a leaf means testing all of them. both are multiplied by
        // this nodes area, which saves a division and changes nothing about which is smaller
        const float area = node.Bounds.SurfaceArea();
        const float splitCost = TraversalCost * area + IntersectionCost * bestCost;
        const float leafCost = IntersectionCost * static_cast<float>(node.Count) * area;
        if (node.Count <= input.Settings.MaxLeafSize && leafCost <= splitCost)
        {
            return 0; // splitting wouldnt pay for itself
        }

        const auto begin = primitiveIndices.begin() + node.First;
        const auto middle = std::partition(begin, begin + node.Count,
                                           [&](int primitive) { return binOf(bestAxis, primitive) <= bestBin; });
        return static_cast<int>(middle - begin);
    }

    float Bvh::SahCost() const
    {
        if (nodes.empty())
        {
            return 0.0f;
        }
        // a random ray hitting the root reaches each node with probability area(node) / area(root), and pays for
        // stepping into it (interior) or testing its primitives (leaf)
        const float rootArea = nodes.front().Bounds.SurfaceArea();
        if (rootArea <= 0.0f)
        {
            return IntersectionCost * static_cast<float>(primitiveIndices.size()); // flat as a line: no areas to weigh
        }
        float cost = 0.0f;
        for (const BvhNode& node : nodes)
        {
            const float reached = node.Bounds.SurfaceArea() / rootArea;
            cost +=
                node.IsLeaf() ? IntersectionCost * static_cast<float>(node.Count) * reached : TraversalCost * reached;
        }
        return cost;
    }
}