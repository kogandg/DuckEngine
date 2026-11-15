#pragma once

#include <Duck/RayTraceCpu/Scene/Geometry.h>
#include <Duck/Core/Bvh.h>
#include <Duck/RayTraceCpu/RayStats.h>

#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace Duck::RayTraceCpu
{
    // a group of primitives the render scene intersects as one unit. each will get its own BVH, and later
    // instances will place them in the world (vulkans bottom level acceleration structures)
    // both kinds give primitive ids that are indices into their own list, so the ids never depend on anything else
    // in the scene

    // loose spheres, quads and triangles, tested one after another
    class ShapeList
    {
    public:
        ShapeList() = default;
        explicit ShapeList(std::vector<Shape> shapes);

        int PrimitiveCount() const { return static_cast<int>(shapes.size()); }
        Shape GetPrimitive(int id) const { return shapes[static_cast<size_t>(id)]; }
        std::span<const Shape> Shapes() const { return shapes; }

        const Bvh& GetBvh() const { return bvh; }

        std::optional<Hit> ClosestHit(const Ray& ray, float tMin, float tMax, TraversalStats& stats) const;
        bool Occluded(const Ray& ray, float tMin, float tMax, TraversalStats& stats) const;

    private:
        std::vector<Shape> shapes;
        Bvh bvh;
    };

    // a mesh packaged for intersection, with its triangles precomputed
    // triangles with no area are dropped, so primitive ids are indices into Triangles(), not the meshes triangles
    class MeshGeometry
    {
    public:
        explicit MeshGeometry(const Mesh& mesh);

        int PrimitiveCount() const { return static_cast<int>(triangles.size()); }
        Shape GetPrimitive(int id) const { return triangles[static_cast<size_t>(id)]; }
        std::span<const Triangle> Triangles() const { return triangles; }

        std::optional<Hit> ClosestHit(const Ray& ray, float tMin, float tMax, TraversalStats& stats) const;
        bool Occluded(const Ray& ray, float tMin, float tMax, TraversalStats& stats) const;

        // the full hit on one triangle, with interpolated vertex normals and uvs
        Hit MakeHit(int triangle, const Ray& ray, const TriangleHit& where) const;

        const Bvh& GetBvh() const { return bvh; }

    private:
        std::vector<Triangle> triangles;
        std::vector<IVec3> corners; // each triangles vertex indices, for looking up normals and uvs
        std::vector<Vec3> normals;  // per vertex. empty: flat shaded
        std::vector<Vec2> uvs;      // per vertex. empty: uv = barycentrics

        Bvh bvh;
    };

    using Geometry = std::variant<ShapeList, MeshGeometry>;

    inline int GeometryPrimitiveCount(const Geometry& geometry)
    {
        return std::visit([](const auto& typed) { return typed.PrimitiveCount(); }, geometry);
    }

    inline Shape GeometryPrimitive(const Geometry& geometry, int id)
    {
        return std::visit([&](const auto& typed) { return typed.GetPrimitive(id); }, geometry);
    }

    inline std::optional<Hit> GeometryClosestHit(const Geometry& geometry, const Ray& ray, float tMin, float tMax,
                                                 TraversalStats& stats)
    {
        return std::visit([&](const auto& typed) { return typed.ClosestHit(ray, tMin, tMax, stats); },
                          geometry);
    }

    inline bool GeometryOccluded(const Geometry& geometry, const Ray& ray, float tMin, float tMax,
                                 TraversalStats& stats)
    {
        return std::visit([&](const auto& typed) { return typed.Occluded(ray, tMin, tMax, stats); },
                          geometry);
    }

    inline const Bvh& GeometryBvh(const Geometry& geometry)
    {
        return std::visit([](const auto& typed) -> const Bvh& { return typed.GetBvh(); }, geometry);
    }
}
