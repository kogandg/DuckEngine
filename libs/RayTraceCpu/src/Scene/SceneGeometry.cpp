#include <Duck/RayTraceCpu/Scene/SceneGeometry.h>

#include <Duck/Core/Assert.h>

#include <array>
#include <utility>

namespace Duck::RayTraceCpu
{
    // --------- ShapeList --------

    ShapeList::ShapeList(std::vector<Shape> shapeList) : shapes(std::move(shapeList))
    {
        std::vector<Aabb> bounds;
        bounds.reserve(shapes.size());
        for (const Shape& shape : shapes)
        {
            bounds.push_back(ShapeBounds(shape));
        }
        bvh = Bvh(bounds);
    }

    std::optional<Hit> ShapeList::ClosestHit(const Ray& ray, float tMin, float tMax, TraversalStats& stats) const
    {
        std::optional<Hit> closest;
        bvh.ClosestHit(ray, tMin, tMax, stats,
                       [&](int shape, float currentMax)
                       {
                           auto hit = IntersectShape(shapes[static_cast<size_t>(shape)], ray, tMin, currentMax);
                           if (!hit)
                           {
                               return currentMax;
                           }
                           hit->PrimitiveId = shape;
                           closest = hit;
                           return hit->T;
                       });
        return closest;
    }

    bool ShapeList::Occluded(const Ray& ray, float tMin, float tMax, TraversalStats& stats) const
    {
        return bvh.AnyHit(ray, tMin, tMax, stats, [&](int shape)
                          { return IntersectShape(shapes[static_cast<size_t>(shape)], ray, tMin, tMax).has_value(); });
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
        int closest = -1;
        TriangleHit closestWhere;
        bvh.ClosestHit(ray, tMin, tMax, stats,
                       [&](int triangle, float currentMax)
                       {
                           const auto where =
                               triangles[static_cast<size_t>(triangle)].IntersectBarycentric(ray, tMin, currentMax);
                           if (!where)
                           {
                               return currentMax;
                           }
                           closest = triangle;
                           closestWhere = *where;
                           return where->T;
                       });

        if (closest < 0)
        {
            return std::nullopt;
        }
        return MakeHit(closest, ray, closestWhere);
    }

    bool MeshGeometry::Occluded(const Ray& ray, float tMin, float tMax, TraversalStats& stats) const
    {
        return bvh.AnyHit(
            ray, tMin, tMax, stats, [&](int triangle)
            { return triangles[static_cast<size_t>(triangle)].IntersectBarycentric(ray, tMin, tMax).has_value(); });
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
