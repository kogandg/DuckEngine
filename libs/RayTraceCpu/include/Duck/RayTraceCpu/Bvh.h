#pragma once

#include <Duck/Core/Aabb.h>
#include <Duck/Core/Assert.h>
#include <Duck/Core/Ray.h>
#include <Duck/RayTraceCpu/RayStats.h>

#include <array>
#include <span>
#include <utility>
#include <vector>

namespace Duck::RayTraceCpu
{
    enum class BvhSplitMethod
    {
        Midpoint, // halve the lognest side
        SAH,      // binned surface area heuristic
    };

    struct BvhBuildSettings
    {
        BvhSplitMethod Split = BvhSplitMethod::SAH;
        int MaxLeafSize = 4;
        int BinCount = 16; // SAH only
    };

    struct BvhNode
    {
        Aabb Bounds;
        int First = 0; // interior: index of left child, right cild is first + 1
                       // leaf: where is primitive start
        int Count = 0; // leaf: how many primatives. 0 marks interior node
        bool IsLeaf() const { return Count > 0; }
    };
    static_assert(sizeof(BvhNode) == 32, "Two nodes per 64 byte cache line");

    class Bvh
    {
    public:
        static constexpr int MaxDepth = 64;
        static constexpr int MaxBins = 32;

        // relative costs SAH weights to split with
        static constexpr float TraversalCost = 1.0f;
        static constexpr float IntersectionCost = 1.0f;

        Bvh() = default;
        explicit Bvh(std::span<const Aabb> primitiveBounds, const BvhBuildSettings& settings = {});

        bool IsEmpty() const { return nodes.empty(); }
        const BvhNode& Root() const { return nodes.front(); }
        std::span<const BvhNode> Nodes() const { return nodes; }
        std::span<const int> PrimitiveIndices() const { return primitiveIndices; }

        float SahCost() const;

        template <typename TestPrimitive>
        void ClosestHit(const Ray& ray, float tMin, float tMax, TraversalStats& stats,
                        TestPrimitive&& testPrimitive) const;

        template <typename TestPrimitive>
        bool AnyHit(const Ray& ray, float tMin, float tMax, TraversalStats& stats, TestPrimitive&& testPrimitive) const;

    private:
        struct BuildInput;

        void UpdateBounds(BvhNode& node, const BuildInput& input) const;
        void Subdivide(int nodeIndex, int depth, const BuildInput& input);

        int SplitMidpoint(const BvhNode& node, const Aabb& centroidBounds, const BuildInput& input);
        int SplitSah(const BvhNode& node, const Aabb& centroidBounds, const BuildInput& input);

        std::vector<BvhNode> nodes; // nodes[0] is the root
        std::vector<int> primitiveIndices;
    };

    template <typename TestPrimitive>
    void Bvh::ClosestHit(const Ray& ray, float tMin, float tMax, TraversalStats& stats,
                         TestPrimitive&& testPrimitive) const
    {
        if (nodes.empty())
        {
            return;
        }
        const TraversalRay traversalRay(ray);

        // nodes whose box the ray enters, with where it enters. boxes are tested before theyre pushed, so the
        // nearer child can go on top and be visited first
        struct StackEntry
        {
            int Node;
            float Entry;
        };
        // each level leaves at most one sibling waiting, so depth + 1 slots are enough
        std::array<StackEntry, MaxDepth + 1> stack;
        int stackSize = 0;

        stats.NodeTests++;
        const float rootEntry = nodes[0].Bounds.EntryDistance(traversalRay, tMin, tMax);
        if (rootEntry == Infinity)
        {
            return;
        }
        stack[stackSize] = {0, rootEntry};
        stackSize++;

        while (stackSize > 0)
        {
            stackSize--;
            const StackEntry entry = stack[stackSize];
            if (entry.Entry > tMax)
            {
                continue; // a closer hit was found after this was pushed: everything in it is further away
            }
            const BvhNode& node = nodes[static_cast<size_t>(entry.Node)];

            if (node.IsLeaf())
            {
                for (int i = node.First; i < node.First + node.Count; i++)
                {
                    stats.PrimitiveTests++;
                    tMax = testPrimitive(primitiveIndices[static_cast<size_t>(i)], tMax);
                }
                continue;
            }

            StackEntry nearChild{node.First,
                                 nodes[static_cast<size_t>(node.First)].Bounds.EntryDistance(traversalRay, tMin, tMax)};
            StackEntry farChild{node.First + 1, nodes[static_cast<size_t>(node.First + 1)].Bounds.EntryDistance(
                                                    traversalRay, tMin, tMax)};
            stats.NodeTests += 2;
            if (farChild.Entry < nearChild.Entry)
            {
                std::swap(nearChild, farChild);
            }

            // far first, so the near child ends up on top. a hit in the near child often lowers tMax enough that
            // the far one is skipped when it's popped
            DUCK_ASSERT(stackSize + 2 <= static_cast<int>(stack.size()), "BVH deeper than the traversal stack");
            if (farChild.Entry != Infinity)
            {
                stack[stackSize] = farChild;
                stackSize++;
            }
            if (nearChild.Entry != Infinity)
            {
                stack[stackSize] = nearChild;
                stackSize++;
            }
        }
    }

    template <typename TestPrimitive>
    bool Bvh::AnyHit(const Ray& ray, float tMin, float tMax, TraversalStats& stats, TestPrimitive&& testPrimitive) const
    {
        if (nodes.empty())
        {
            return false;
        }
        const TraversalRay traversalRay(ray);

        std::array<int, MaxDepth + 1> stack;
        int stackSize = 0;
        stack[stackSize] = 0;
        stackSize++;

        while (stackSize > 0)
        {
            stackSize--;
            const BvhNode& node = nodes[static_cast<size_t>(stack[stackSize])];
            stats.NodeTests++;
            if (node.Bounds.EntryDistance(traversalRay, tMin, tMax) == Infinity)
            {
                continue;
            }

            if (node.IsLeaf())
            {
                for (int i = node.First; i < node.First + node.Count; i++)
                {
                    stats.PrimitiveTests++;
                    if (testPrimitive(primitiveIndices[static_cast<size_t>(i)]))
                    {
                        return true;
                    }
                }
            }
            else
            {
                DUCK_ASSERT(stackSize + 2 <= static_cast<int>(stack.size()), "BVH deeper than the traversal stack");
                stack[stackSize] = node.First + 1;
                stackSize++;
                stack[stackSize] = node.First;
                stackSize++;
            }
        }
        return false;
    }
}