#pragma once

#include <Duck/Core/Ray.h>

#include <cmath>
#include <cstdint>
#include <optional>

namespace Duck::PathTrace
{
    struct Hit
    {
        Vec3 Point{0.0f};
        Vec3 Normal{0.0f};
        float T = 0.0f;
        bool FrontFace = true; // false if hit from the inside of a surface
        int MaterialId = 0;
    };

    enum class MaterialType
    {
        Diffuse,    // labert + sun + ambient
        Mirror,     // perfect reflection, tinted by albedo
        Dielectric, // glass and water, fresnel weighted reflection + refraction
    };

    struct Material
    {
        MaterialType Type = MaterialType::Diffuse;
        Vec3 Albedo{0.5f}; // diffuse color or mirror tint
        float Ior = 1.5f;  // index of refraction, dielectric only

        // checkersize > 0 alternates between albedo and checker albedo in squares on xz plane
        Vec3 CheckerAlbedo{0.0f};
        float CheckerSize = 0.0f;

        Vec3 AlbedoAt(const Vec3& point) const
        {
            if (CheckerSize <= 0.0f)
            {
                return Albedo;
            }
            const int cellX = static_cast<int>(std::floor(point.x / CheckerSize));
            const int cellZ = static_cast<int>(std::floor(point.z / CheckerSize));
            return ((cellX + cellZ) & 1) == 0 ? Albedo : CheckerAlbedo;
        }
    };

    struct Sphere
    {
        Vec3 Center{0.0f};
        float Radius = 1.0f;
        int MaterialId = 0;

        // only hits with tMin <= t <= tMax count
        std::optional<Hit> Intersect(const Ray& ray, float tMin, float tMax) const
        {
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
            const Vec3 outward = (hit.Point - Center) / Radius;
            hit.FrontFace = glm::dot(ray.Direction, outward) < 0.0f;
            hit.Normal = hit.FrontFace ? outward : -outward;
            return hit;
        }
    };

    //parallelogram: corner + a * edgeU + b*edgeV for a, b in [0, 1]
    struct Quad
    {
        Vec3 Corner{0.0f};
        Vec3 EdgeU{1.0f, 0.0f, 0.0f};
        Vec3 EdgeV{0.0f, 0.0f, 1.0f};
        int MaterialId = 0;

        std::optional<Hit> Intersect(const Ray& ray, float tMin, float tMax) const
        {
            const Vec3 crossUV = glm::cross(EdgeU, EdgeV);
            const Vec3 normal = glm::normalize(crossUV);

            // ray vs the quads plane
            const float denominator = glm::dot(normal, ray.Direction);
            if (std::abs(denominator) < 1e-8f)
            {
                return std::nullopt; // parallel to the plane
            }
            const float t = glm::dot(normal, Corner - ray.Origin) / denominator;
            if (t < tMin || t > tMax)
            {
                return std::nullopt;
            }

            // write the hit as offset = a * EdgeU + b * EdgeV and check both are in [0, 1].
            // crossing offset with one edge leaves only the other edges term, scaled by crossUV
            const Vec3 point = ray.At(t);
            const Vec3 offset = point - Corner;
            const Vec3 w = crossUV / glm::dot(crossUV, crossUV);
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
            hit.FrontFace = denominator < 0.0f; // front is the side cross(EdgeU, EdgeV) points to
            hit.Normal = hit.FrontFace ? normal : -normal;
            return hit;
        }
    };
}