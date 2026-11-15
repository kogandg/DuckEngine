#pragma once

#include <Duck/Core/Assert.h>
#include <Duck/Core/Math.h>
#include <Duck/Core/Ray.h>
#include <Duck/Core/Aabb.h>
#include <Duck/RayTraceCpu/Scene/Hit.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

namespace Duck::RayTraceCpu
{
    struct Sphere
    {
        Vec3 Center{0.0f};
        float Radius = 1.0f; // negative turns it inside out with front faces inward
        int MaterialId = 0;

        float Sign() const { return Radius < 0.0f ? -1.0f : 1.0f; }

        // only hits with tMin <= t <= tMax count
        std::optional<Hit> Intersect(const Ray& ray, float tMin, float tMax) const
        {
            // |origin + t * direction - center|^2 = r^2 is a * t^2 + 2 * b * t + c = 0
            const float a = glm::dot(ray.Direction, ray.Direction);
            const Vec3 oc = ray.Origin - Center;
            const float b = glm::dot(oc, ray.Direction);
            // b^2 - a * c written as a * (r^2 - |closest approach|^2), which keeps its precision when the ray
            // passes far from the sphere
            const Vec3 perpendicular = oc - (b / a) * ray.Direction;
            const float discriminant = Radius * Radius - glm::dot(perpendicular, perpendicular);
            if (discriminant < 0.0f)
            {
                return std::nullopt;
            }

            const float root = std::sqrt(a * discriminant);
            float t = (-b - root) / a;
            if (t < tMin || t > tMax)
            {
                t = (-b + root) / a;
                if (t < tMin || t > tMax)
                {
                    return std::nullopt;
                }
            }

            Hit hit;
            hit.T = t;
            hit.MaterialId = MaterialId;
            hit.Point = ray.At(t);

            // normalize instead of dividing by Radius: after rounding the hit point is only near the surface,
            // and a normal that isn't unit length makes reflected/refracted directions that aren't either, which
            // puts the next hit further off the surface. the error grows every bounce
            // the sign keeps negative radii (inside-out spheres) working
            const Vec3 fromCenter = glm::normalize(hit.Point - Center);
            const Vec3 outward = fromCenter * Sign();

            hit.FrontFace = glm::dot(ray.Direction, outward) < 0.0f;
            hit.Normal = hit.FrontFace ? outward : -outward;
            hit.GeometricNormal = hit.Normal;

            hit.Uv = Vec2(0.5f + std::atan2(fromCenter.z, fromCenter.x) / TwoPi,
                          std::acos(std::clamp(fromCenter.y, -1.0f, 1.0f)) / Pi);

            return hit;
        }

        Aabb Bounds() const
        {
            const Vec3 extent(std::abs(Radius));
            return {Center - extent, Center + extent};
        }

        float Area() const { return 2.0f * TwoPi * Radius * Radius; } // 4pir^2

        SurfaceSample SampleSurface(const Vec2& u) const
        {
            const float z = 1.0f - 2.0f * u.x;
            const float ring = std::sqrt(std::max(0.0f, 1.0f - z * z));
            const float phi = TwoPi * u.y;
            const Vec3 direction(ring * std::cos(phi), ring * std::sin(phi), z);
            return {Center + std::abs(Radius) * direction, direction * Sign()};
        }
    };

    // parallelogram: corner + a * edgeU + b*edgeV for a, b in [0, 1]
    class Quad
    {
    public:
        Quad() : Quad(Vec3(0.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), 0) {}

        Quad(const Vec3& corner, const Vec3& edgeU, const Vec3& edgeV, int materialId)
            : Corner(corner), EdgeU(edgeU), EdgeV(edgeV), MaterialId(materialId)
        {
            const Vec3 crossUV = glm::cross(EdgeU, EdgeV);
            area = glm::length(crossUV);
            if (area > 0.0f)
            {
                normal = crossUV / area;
                w = crossUV / (area * area);
            }
        }

        Vec3 Corner{0.0f};
        Vec3 EdgeU{1.0f, 0.0f, 0.0f};
        Vec3 EdgeV{0.0f, 0.0f, 1.0f};
        int MaterialId = 0;

        std::optional<Hit> Intersect(const Ray& ray, float tMin, float tMax) const
        {
            // ray vs the quads plane
            const float denominator = glm::dot(normal, ray.Direction);
            if (std::abs(denominator) < 1e-8f)
            {
                return std::nullopt; // parallel to the plane or degen quad
            }
            const float t = glm::dot(normal, Corner - ray.Origin) / denominator;
            if (t < tMin || t > tMax)
            {
                return std::nullopt;
            }

            // write the hit as offset = a * EdgeU + b * EdgeV and check both are in [0, 1]
            // crossing offset with one edge leaves only the other edges term, scaled by crossUV
            const Vec3 point = ray.At(t);
            const Vec3 offset = point - Corner;
            const float a = glm::dot(w, glm::cross(offset, EdgeV));
            const float b = glm::dot(w, glm::cross(EdgeU, offset));
            if (a < 0.0f || a > 1.0f || b < 0.0f || b > 1.0f)
            {
                return std::nullopt;
            }

            Hit hit;
            hit.T = t;
            hit.MaterialId = MaterialId;
            hit.Point = point;
            hit.Uv = Vec2(a, b);
            hit.FrontFace = denominator < 0.0f; // front is the side cross(EdgeU, EdgeV) points to
            hit.Normal = hit.FrontFace ? normal : -normal;
            hit.GeometricNormal = hit.Normal;
            return hit;
        }

        Aabb Bounds() const
        {
            Aabb bounds;
            bounds.Grow(Corner);
            bounds.Grow(Corner + EdgeU);
            bounds.Grow(Corner + EdgeV);
            bounds.Grow(Corner + EdgeU + EdgeV);
            return bounds;
        }

        float Area() const { return area; }

        SurfaceSample SampleSurface(const Vec2& u) const { return {Corner + u.x * EdgeU + u.y * EdgeV, normal}; }

    private:
        Vec3 normal{0.0f}; // unit cross(EdgeU, EdgeV)
        Vec3 w{0.0f};      // cross(EdgeU, EdgeV)  / |cross(EdgeU, EdgeV)|^2 for the in quad test
        float area = 0.0f;
    };

    struct Mesh
    {
        std::vector<Vec3> Positions;
        std::vector<Vec3> Normals;
        std::vector<Vec2> Uvs;
        std::vector<int> Indices;
        int MaterialId = 0;

        int TriangleCount() const { return static_cast<int>(Indices.size()) / 3; }
    };

    // where a ray crosses a triangle. the weight of corner 0 is 1 - B1 - B2
    struct TriangleHit
    {
        float T = 0.0f;
        float B1 = 0.0f; // weight of corner 1
        float B2 = 0.0f; // weight of corner 2
    };

    // one triangle by its corners, with no mesh attached, so it can be copied anywhere: MeshGeometry stores them,
    // and area lights keep their own. vertex normals and uvs belong to the mesh (MeshGeometry::MakeHit)
    struct Triangle
    {
        Triangle() = default;
        Triangle(const Vec3& p0, const Vec3& p1, const Vec3& p2, int materialId)
            : P0(p0), Edge1(p1 - p0), Edge2(p2 - p0), MaterialId(materialId)
        {
        }

        // precomputed for the intersection test
        Vec3 P0{0.0f};
        Vec3 Edge1{0.0f}; // p1 - p0
        Vec3 Edge2{0.0f}; // p2 - p0
        int MaterialId = 0;

        Vec3 FrontNormal() const { return glm::normalize(glm::cross(Edge1, Edge2)); }

        // Moller Trumbore, solves origin + t * direction = p0 + b1 * (p1-p0) + b2 * (p2-p0) directly
        // only finds where the ray crosses, which is all a closest hit search needs for the candidates it rejects
        std::optional<TriangleHit> IntersectBarycentric(const Ray& ray, float tMin, float tMax) const
        {
            const Vec3 pvec = glm::cross(ray.Direction, Edge2);
            const float determinant = glm::dot(Edge1, pvec);
            if (determinant == 0.0f)
            {
                return std::nullopt; // parallel to the triangles plane
            }
            const float inverse = 1.0f / determinant;

            const Vec3 tvec = ray.Origin - P0;
            const float b1 = glm::dot(tvec, pvec) * inverse;
            if (b1 < 0.0f || b1 > 1.0f)
            {
                return std::nullopt;
            }
            const Vec3 qvec = glm::cross(tvec, Edge1);
            const float b2 = glm::dot(ray.Direction, qvec) * inverse;
            if (b2 < 0.0f || b1 + b2 > 1.0f)
            {
                return std::nullopt;
            }
            const float t = glm::dot(Edge2, qvec) * inverse;
            if (t < tMin || t > tMax)
            {
                return std::nullopt;
            }
            return TriangleHit{t, b1, b2};
        }

        // the full hit, flat shaded: face normal, and uv = (b1, b2)
        Hit MakeFlatHit(const Ray& ray, const TriangleHit& where) const
        {
            Hit hit;
            hit.T = where.T;
            hit.MaterialId = MaterialId;
            hit.Point = P0 + where.B1 * Edge1 + where.B2 * Edge2; // on the triangle exactly, rather than ray.At(t)

            const Vec3 front = FrontNormal();
            hit.FrontFace = glm::dot(ray.Direction, front) < 0.0f;
            hit.GeometricNormal = hit.FrontFace ? front : -front;
            hit.Normal = hit.GeometricNormal;
            hit.Uv = Vec2(where.B1, where.B2);
            return hit;
        }

        std::optional<Hit> Intersect(const Ray& ray, float tMin, float tMax) const
        {
            const auto where = IntersectBarycentric(ray, tMin, tMax);
            if (!where)
            {
                return std::nullopt;
            }
            return MakeFlatHit(ray, *where);
        }

        Aabb Bounds() const
        {
            Aabb bounds;
            bounds.Grow(P0);
            bounds.Grow(P0 + Edge1);
            bounds.Grow(P0 + Edge2);
            return bounds;
        }

        float Area() const { return 0.5f * glm::length(glm::cross(Edge1, Edge2)); }

        SurfaceSample SampleSurface(const Vec2& u) const
        {
            // uniform over the triangle: the square root keeps samples from bunching up at corner 0
            const float root = std::sqrt(u.x);
            const float b1 = u.y * root;
            const float b2 = root - b1;
            return {P0 + b1 * Edge1 + b2 * Edge2, FrontNormal()};
        }
    };

    // -------- interface every shape must provide -------------

    template <typename T>
    concept ShapeType = requires(const T& shape, const Ray& ray, float t, const Vec2& u) {
        { shape.Intersect(ray, t, t) } -> std::same_as<std::optional<Hit>>;
        { shape.Bounds() } -> std::same_as<Aabb>;                  // for the BVH
        { shape.Area() } -> std::same_as<float>;                   // for area lights
        { shape.SampleSurface(u) } -> std::same_as<SurfaceSample>; // for area lights
        { shape.MaterialId } -> std::convertible_to<int>;
    };

    using Shape = std::variant<Sphere, Quad, Triangle>;

    template <typename TVariant>
    struct AllShapeTypes;
    template <typename... TShapes>
    struct AllShapeTypes<std::variant<TShapes...>> : std::bool_constant<(ShapeType<TShapes> && ...)>
    {
    };
    static_assert(AllShapeTypes<Shape>::value, "Every type in Shape must satisfy ShapeType");

    inline std::optional<Hit> IntersectShape(const Shape& shape, const Ray& ray, float tMin, float tMax)
    {
        return std::visit([&](const auto& typed) { return typed.Intersect(ray, tMin, tMax); }, shape);
    }

    inline Aabb ShapeBounds(const Shape& shape)
    {
        return std::visit([](const auto& typed) { return typed.Bounds(); }, shape);
    }

    inline float ShapeArea(const Shape& shape)
    {
        return std::visit([](const auto& typed) { return typed.Area(); }, shape);
    }

    inline SurfaceSample SampleShapeSurface(const Shape& shape, const Vec2& u)
    {
        return std::visit([&](const auto& typed) { return typed.SampleSurface(u); }, shape);
    }

    inline int ShapeMaterialId(const Shape& shape)
    {
        return std::visit([](const auto& typed) { return typed.MaterialId; }, shape);
    }
}
