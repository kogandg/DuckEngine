#pragma once

#include <Duck/Core/Ray.h>
#include <Duck/PathTraceCpu/Geometry.h>

#include <optional>
#include <vector>

namespace Duck::PathTrace
{
    // infinitely far away
    struct DirectionalLight
    {
        Vec3 Direction{0.0f, 1.0f, 0.0f}; // unit, points TOWARD the light
        Vec3 Irradiance{Pi}; // W/m^2 on a surface facing the light. Pi lights a white surface to radiance 1
    };

    // light from one point, falls off with 1/distance^2, inverse square law
    struct PointLight
    {
        Vec3 Position{0.0f};
        Vec3 Intensity{1.0f}; // W/sr
    };

    struct Scene
    {
        std::vector<Material> Materials;
        std::vector<Sphere> Spheres;
        std::vector<Quad> Quads;

        std::vector<DirectionalLight> DirectionalLights;
        std::vector<PointLight> PointLights;

        // radiance of the background for rays that escape, W/(m^2 sr)
        Vec3 SkyDown{1.0f};
        Vec3 SkyUp{0.5f, 0.7f, 1.0f};

        // whitted ambient: radiance of a surrounding environment, ignoring occlusion
        Vec3 AmbientDown{0.3f};
        Vec3 AmbientUp{0.15f, 0.21f, 0.3f};

        std::optional<Hit> ClosestHit(const Ray& ray, float tMin, float tMax) const;

        // any hit in the interval counts
        bool Occluded(const Ray& ray, float tMin, float tMax) const;

        Vec3 SkyColor(const Ray& ray) const { return glm::mix(SkyDown, SkyUp, 0.5f * (ray.Direction.y + 1.0f)); }
        Vec3 Ambient(const Vec3& normal) const { return glm::mix(AmbientDown, AmbientUp, 0.5f * (normal.y + 1.0f)); }

        // returns new material id
        int AddMaterial(const Material& material)
        {
            Materials.push_back(material);
            return static_cast<int>(Materials.size()) - 1;
        }
    };
}