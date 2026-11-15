#include <Duck/RayTraceCpu/SceneGeometry.h>

#include <Duck/Core/Assert.h>

#include <array>

namespace Duck::RayTraceCpu
{
    // --------- ShapeList --------

    std::optional<Hit> ShapeList::ClosestHit(const Ray& ray, float tMin, float tMax, TraversalStats& stats) const
    {
        std::optional<Hit> closest;
        for (size_t i = 0; i < shapes.size(); i++)
        {
            stats.PrimitiveTests++;
            if (auto hit = IntersectShape(shapes[i], ray, tMin, tMax))
            {
                hit->PrimitiveId = static_cast<int>(i);
                tMax = hit->T; // anything further away cant be the closest now
                closest = hit;
            }
        }
        return closest;
    }

    bool ShapeList::Occluded(const Ray& ray, float tMin, float tMax, TraversalStats& stats) const
    {
        for (const Shape& shape : shapes)
        {
            stats.PrimitiveTests++;
            if (IntersectShape(shape, ray, tMin, tMax))
            {
                return true; // any hit will do
            }
        }
        return false;
    }

    // ------ MeshGeometry --------

    MeshGeometry::MeshGeometry(const Mesh& mesh) : normals(mesh.Normals), uvs(mesh.Uvs)
    {
        const int vertexCount = static_cast<int>(mesh.Positions.size());
        DUCK_VERIFY(mesh.Indices.size() % 3 == 0, "Mesh has {} indices, not a multiple of 3", mesh.Indices.size());
        DUCK_VERIFY(mesh.Normals.empty() || static_cast<int>(mesh.Normals.size()) == vertexCount,
                    "Mesh has {} normals for {} vertices", mesh.Normals.size(), vertexCount);
        DUCK_VERIFY(mesh.Uvs.empty() || static_cast<int>(mesh.Uvs.size()) == vertexCount,
                    "Mesh has {} uvs for {} vertices", mesh.Uvs.size(), vertexCount);
        for (const int index : mesh.Indices)
        {
            DUCK_VERIFY(index >= 0 && index < vertexCount, "Mesh index {} out of range ({} vertices)", index,
                        vertexCount);
        }

        triangles.reserve(static_cast<size_t>(mesh.TriangleCount()));
        corners.reserve(static_cast<size_t>(mesh.TriangleCount()));
        for (int i = 0; i < mesh.TriangleCount(); i++)
        {
            const auto index = [&](int corner) { return mesh.Indices[static_cast<size_t>(3 * i + corner)]; };
            const IVec3 corner(index(0), index(1), index(2));
            const auto position = [&](int c) { return mesh.Positions[static_cast<size_t>(corner[c])]; };

            const Triangle triangle(position(0), position(1), position(2), mesh.MaterialId);
            if (triangle.Area() > 0.0f) // uv sphere poles have some with two corners in the same place
            {
                triangles.push_back(triangle);
                corners.push_back(corner);
            }
        }

        std::vector<Aabb> bounds;
        bounds.reserve(triangles.size());
        for (const Triangle& triangle : triangles)
        {
            bounds.push_back(triangle.Bounds());
        }
        bvh = Bvh(bounds);
    }

    std::optional<Hit> MeshGeometry::ClosestHit(const Ray& ray, float tMin, float tMax, TraversalStats& stats) const
    {
        if (bvh.IsEmpty())
        {
            return std::nullopt;
        }

        const TraversalRay traversalRay(ray);
        const std::span<const BvhNode> nodes = bvh.Nodes();
        const std::span<const int> primitives = bvh.PrimitiveIndices();

        // find the closest crossing first, and only build the full hit for that one
        int closest = -1;
        TriangleHit closestWhere;

        std::array<int, Bvh::MaxDepth + 1> stack;
        int stackSize = 0;
        stack[stackSize] = 0; // root
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
                    const int triangle = primitives[static_cast<size_t>(i)];
                    stats.PrimitiveTests++;
                    if (const auto where = triangles[static_cast<size_t>(triangle)].IntersectBarycentric(ray, tMin, tMax))
                    {
                        closest = triangle;
                        closestWhere = *where;
                        tMax = where->T;
                    }
                }
            }
            else
            {
                DUCK_ASSERT(stackSize + 2 <= static_cast<int>(stack.size()), "BVH deeper than the traversal stack");
                stack[stackSize] = node.First + 1;
                stackSize++;
                stack[stackSize] = node.First; // the left child is on top so its visited next
                stackSize++;
            }
        }

        if (closest < 0)
        {
            return std::nullopt;
        }
        return MakeHit(closest, ray, closestWhere);
    }

    bool MeshGeometry::Occluded(const Ray& ray, float tMin, float tMax, TraversalStats& stats) const
    {
        if (bvh.IsEmpty())
        {
            return false;
        }

        const TraversalRay traversalRay(ray);
        const std::span<const BvhNode> nodes = bvh.Nodes();
        const std::span<const int> primitives = bvh.PrimitiveIndices();

        std::array<int, Bvh::MaxDepth + 1> stack;
        int stackSize = 0;
        stack[stackSize++] = 0;

        while (stackSize > 0)
        {
            const BvhNode& node = nodes[static_cast<size_t>(stack[--stackSize])];
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
                    if (triangles[static_cast<size_t>(primitives[static_cast<size_t>(i)])].IntersectBarycentric(
                            ray, tMin, tMax))
                    {
                        return true; // any hit will do
                    }
                }
            }
            else
            {
                DUCK_ASSERT(stackSize + 2 <= static_cast<int>(stack.size()), "BVH deeper than the traversal stack");
                stack[stackSize++] = node.First + 1;
                stack[stackSize++] = node.First;
            }
        }
        return false;
    }

    Hit MeshGeometry::MakeHit(int triangle, const Ray& ray, const TriangleHit& where) const
    {
        const size_t index = static_cast<size_t>(triangle);
        Hit hit = triangles[index].MakeFlatHit(ray, where);
        hit.PrimitiveId = triangle;

        const IVec3& corner = corners[index];
        const float b0 = 1.0f - where.B1 - where.B2;

        if (!normals.empty())
        {
            const auto normal = [&](int c) { return normals[static_cast<size_t>(corner[c])]; };
            const Vec3 blended = b0 * normal(0) + where.B1 * normal(1) + where.B2 * normal(2);
            if (glm::dot(blended, blended) > 0.0f)
            {
                const Vec3 front = hit.FrontFace ? hit.GeometricNormal : -hit.GeometricNormal;
                Vec3 shading = glm::normalize(blended);
                if (glm::dot(shading, front) < 0.0f)
                {
                    shading = -shading; // vertex normals disagree with the winding: the winding wins
                }
                hit.Normal = hit.FrontFace ? shading : -shading;
            }
        }

        if (!uvs.empty())
        {
            const auto uv = [&](int c) { return uvs[static_cast<size_t>(corner[c])]; };
            hit.Uv = b0 * uv(0) + where.B1 * uv(1) + where.B2 * uv(2);
        }
        return hit;
    }
}
