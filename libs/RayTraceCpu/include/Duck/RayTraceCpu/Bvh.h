#pragma once

#include <Duck/Core/Aabb.h>

#include <span>
#include <vector>

namespace Duck::RayTraceCpu
{
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

        Bvh() = default;
        explicit Bvh(std::span<const Aabb> primitiveBounds, int maxLeafSize = 4);

        bool IsEmpty() const { return nodes.empty(); }
        const BvhNode& Root() const { return nodes.front(); }
        std::span<const BvhNode> Nodes() const { return nodes; }
        std::span<const int> PrimitiveIndices() const { return primitiveIndices; }

    private:
        void UpdateBounds(BvhNode& node, std::span<const Aabb> primitiveBounds) const;
        void Subdivide(int nodeIndex, int depth, std::span<const Aabb> primitiveBounds, std::span<const Vec3> centroids,
                       int maxLeafSize);

        std::vector<BvhNode> nodes; // nodes[0] is the root
        std::vector<int> primitiveIndices;
    };
}