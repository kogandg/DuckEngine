#include <Duck/RayTraceCpu/Bvh.h>

#include <Duck/Core/Assert.h>

#include <algorithm>
#include <numeric>

namespace Duck::RayTraceCpu
{
    Bvh::Bvh(std::span<const Aabb> primitiveBounds, int maxLeafSize)
    {
        DUCK_VERIFY(maxLeafSize >= 1, "A BVH leaf must be able to hold a primitive");
        const int count = static_cast<int>(primitiveBounds.size());
        if (count == 0)
        {
            return;
        }

        primitiveIndices.resize(primitiveBounds.size());
        std::iota(primitiveIndices.begin(), primitiveIndices.end(), 0);

        // primitives are sorted by the center of their box
        std::vector<Vec3> centroids(primitiveBounds.size());
        for (size_t i = 0; i < primitiveBounds.size(); i++)
        {
            DUCK_VERIFY(!primitiveBounds[i].IsEmpty(), "Primitive {} has an empty box", i);
            centroids[i] = 0.5f * (primitiveBounds[i].Min + primitiveBounds[i].Max);
        }

        // a binary tree whose leaves hold at least one primitive has at most 2n - 1 nodes
        nodes.reserve(2 * primitiveBounds.size() - 1);
        BvhNode& root = nodes.emplace_back();
        root.First = 0;
        root.Count = count;
        UpdateBounds(root, primitiveBounds);
        Subdivide(0, 0, primitiveBounds, centroids, maxLeafSize);

        nodes.shrink_to_fit();
    }

    void Bvh::UpdateBounds(BvhNode& node, std::span<const Aabb> primitiveBounds) const
    {
        node.Bounds = {};
        for (int i = node.First; i < node.First + node.Count; i++)
        {
            node.Bounds.Grow(primitiveBounds[static_cast<size_t>(primitiveIndices[static_cast<size_t>(i)])]);
        }
    }

    void Bvh::Subdivide(int nodeIndex, int depth, std::span<const Aabb> primitiveBounds,
                        std::span<const Vec3> centroids, int maxLeafSize)
    {
        const int first = nodes[static_cast<size_t>(nodeIndex)].First;
        const int count = nodes[static_cast<size_t>(nodeIndex)].Count;
        if (count <= maxLeafSize || depth >= MaxDepth)
        {
            return;
        }

        // split across the longest side of the box around the centroids, not around the primitives: big
        // primitives would stretch that box without helping to separate anything
        Aabb centroidBounds;
        for (int i = first; i < first + count; i++)
        {
            centroidBounds.Grow(centroids[static_cast<size_t>(primitiveIndices[static_cast<size_t>(i)])]);
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
            return; // every centroid in the same place: no plane can separate them, so this leaf stays big
        }

        // midpoint split: everything with its centroid below the middle goes left
        const float split = centroidBounds.Min[axis] + 0.5f * extent[axis];
        const auto begin = primitiveIndices.begin() + first;
        const auto middle = std::partition(begin, begin + count, [&](int primitive)
                                           { return centroids[static_cast<size_t>(primitive)][axis] < split; });
        const int leftCount = static_cast<int>(middle - begin);
        if (leftCount == 0 || leftCount == count)
        {
            return; // rounding put everything on one side: splitting again would never end
        }

        const int left = static_cast<int>(nodes.size());
        nodes.push_back({.Bounds = {}, .First = first, .Count = leftCount});
        nodes.push_back({.Bounds = {}, .First = first + leftCount, .Count = count - leftCount});
        UpdateBounds(nodes[static_cast<size_t>(left)], primitiveBounds);
        UpdateBounds(nodes[static_cast<size_t>(left + 1)], primitiveBounds);

        BvhNode& node = nodes[static_cast<size_t>(nodeIndex)]; // taken after the push_backs
        node.First = left;
        node.Count = 0; // now interior

        Subdivide(left, depth + 1, primitiveBounds, centroids, maxLeafSize);
        Subdivide(left + 1, depth + 1, primitiveBounds, centroids, maxLeafSize);
    }
}