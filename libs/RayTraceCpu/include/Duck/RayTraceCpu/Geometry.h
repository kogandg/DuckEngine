#pragma once

#include <Duck/Core/Assert.h>
#include <Duck/Core/Math.h>
#include <Duck/Core/Ray.h>

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <variant>
#include <array>
#include <vector>

namespace Duck::RayTraceCpu
{
    struct Hit
    {
        Vec3 Point{0.0f};
        Vec3 Normal{0.0f}; // unit, facing incoming ray. smooth on meshes with vertex normals
        Vec3 GeometricNormal{0.0f}; // normal rays use to leave surface
        Vec2 Uv{0.0f};     // surface coords for textures
        float T = 0.0f;
        bool FrontFace = true; // false if hit from the inside of a surface
        int MaterialId = 0;
        int PrimitiveId = -1;
    };

    struct Aabb
    {
        Vec3 Min{Infinity};
        Vec3 Max{-Infinity};

        void Grow(const Vec3& point)
        {
            Min = glm::min(Min, point);
            Max = glm::max(Max, point);
        }
        void Grow(const Aabb& other)
        {
            Min = glm::min(Min, other.Min);
            Max = glm::max(Max, other.Max);
        }

        bool IsEmpty() const { return glm::any(glm::greaterThan(Min, Max)); }

        bool Contains(const Vec3& point, float tolerance = 0.0f) const
        {
            return glm::all(glm::greaterThanEqual(point, Min - tolerance)) &&
                   glm::all(glm::lessThanEqual(point, Max + tolerance));
        }
    };

    struct SurfaceSample
    {
        Vec3 Point{0.0f};
        Vec3 Normal{0.0f}; // unit vector on the front side
    };

    struct Sphere
    {
        Vec3 Center{0.0f};
        float Radius = 1.0f; // negative turns it inside out with front faces inward
        int MaterialId = 0;

        float Sign() const { return Radius < 0.0f ? -1.0f : 1.0f; }

        // only hits with tMin <= t <= tMax count
        std::optional<Hit> Intersect(const Ray& ray, float tMin, float tMax) const
        {
            DUCK_ASSERT(std::abs(glm::dot(ray.Direction, ray.Direction) - 1.0f) < 1e-4f,
                        "Ray direction must be normalized");

            const Vec3 oc = ray.Origin - Center;
            const float b = glm::dot(oc, ray.Direction);
            const Vec3 perpendicular = oc - b * ray.Direction; // closest approach offset
            const float discriminant = Radius * Radius - glm::dot(perpendicular, perpendicular);
            if (discriminant < 0.0f)
            {
                return std::nullopt;
            }

            const float root = std::sqrt(discriminant);
            float t = -b - root;
            if (t < tMin || t > tMax)
            {
                t = -b + root;
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

    struct Triangle
    {
        const Mesh* Source = nullptr;
        int Index = 0; //which triangle of source
        int MaterialId = 0;

        int VertexIndex(int corner) const { return Source->Indices[static_cast<size_t>(3 * Index + corner)]; }
        const Vec3& Position(int corner) const { return Source->Positions[static_cast<size_t>(VertexIndex(corner))]; }

        Vec3 FrontNormal() const
        {
            return glm::normalize(glm::cross(Position(1) - Position(0), Position(2) - Position(0)));
        }

        // Moller Trumbore, solves origin + t * direction = p0 + b1 * (p1-p0) + b2 * (p2-p0) directly
        std::optional<Hit> Intersect(const Ray& ray, float tMin, float tMax) const
        {
            const Vec3& p0 = Position(0);
            const Vec3& p1 = Position(1);
            const Vec3& p2 = Position(2);
            const Vec3 edge1 = p1 - p0;
            const Vec3 edge2 = p2 - p0;

            const Vec3 pvec = glm::cross(ray.Direction, edge2);
            const float determinant = glm::dot(edge1, pvec);
            if (determinant == 0.0f)
            {
                return std::nullopt; // parallel to the triangles plane
            }
            const float inverse = 1.0f / determinant;

            const Vec3 tvec = ray.Origin - p0;
            const float b1 = glm::dot(tvec, pvec) * inverse;
            if (b1 < 0.0f || b1 > 1.0f)
            {
                return std::nullopt;
            }
            const Vec3 qvec = glm::cross(tvec, edge1);
            const float b2 = glm::dot(ray.Direction, qvec) * inverse;
            if (b2 < 0.0f || b1 + b2 > 1.0f)
            {
                return std::nullopt;
            }
            const float t = glm::dot(edge2, qvec) * inverse;
            if (t < tMin || t > tMax)
            {
                return std::nullopt;
            }
            const float b0 = 1.0f - b1 - b2;

            Hit hit;
            hit.T = t;
            hit.MaterialId = MaterialId;
            hit.Point = b0 * p0 + b1 * p1 + b2 * p2; // on the triangle exactly, rather than ray.At(t)

            const Vec3 front = glm::normalize(glm::cross(edge1, edge2));
            hit.FrontFace = glm::dot(ray.Direction, front) < 0.0f;
            hit.GeometricNormal = hit.FrontFace ? front : -front;

            Vec3 shading = front;
            if (!Source->Normals.empty())
            {
                const auto normal = [&](int corner)
                { return Source->Normals[static_cast<size_t>(VertexIndex(corner))]; };
                const Vec3 blended = b0 * normal(0) + b1 * normal(1) + b2 * normal(2);
                if (glm::dot(blended, blended) > 0.0f)
                {
                    shading = glm::normalize(blended);
                    if (glm::dot(shading, front) < 0.0f)
                    {
                        shading = -shading; // vertex normals disagree with the winding: the winding wins
                    }
                }
            }
            hit.Normal = hit.FrontFace ? shading : -shading;

            if (Source->Uvs.empty())
            {
                hit.Uv = Vec2(b1, b2);
            }
            else
            {
                const auto uv = [&](int corner) { return Source->Uvs[static_cast<size_t>(VertexIndex(corner))]; };
                hit.Uv = b0 * uv(0) + b1 * uv(1) + b2 * uv(2);
            }
            return hit;
        }

        Aabb Bounds() const
        {
            Aabb bounds;
            bounds.Grow(Position(0));
            bounds.Grow(Position(1));
            bounds.Grow(Position(2));
            return bounds;
        }

        float Area() const
        {
            return 0.5f * glm::length(glm::cross(Position(1) - Position(0), Position(2) - Position(0)));
        }

        SurfaceSample SampleSurface(const Vec2& u) const
        {
            // uniform over the triangle: the square root keeps samples from bunching up at corner 0
            const float root = std::sqrt(u.x);
            const float b0 = 1.0f - root;
            const float b1 = u.y * root;
            return {b0 * Position(0) + b1 * Position(1) + (1.0f - b0 - b1) * Position(2), FrontNormal()};
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