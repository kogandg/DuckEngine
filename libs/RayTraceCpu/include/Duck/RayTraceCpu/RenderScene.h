#pragma once

#include <Duck/Core/Ray.h>
#include <Duck/RayTraceCpu/Geometry.h>
#include <Duck/RayTraceCpu/Lights.h>
#include <Duck/RayTraceCpu/Materials.h>
#include <Duck/RayTraceCpu/Environment.h>
#include <Duck/RayTraceCpu/RayStats.h>
#include <Duck/RayTraceCpu/SceneGeometry.h>

#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace Duck::RayTraceCpu
{
    struct Scene;

    class RenderScene
    {
    public:
        // geometry 0 holds the scenes loose spheres and quads, then one geometry per mesh, in order
        explicit RenderScene(const Scene& scene);

        std::optional<Hit> ClosestHit(const Ray& ray, float tMin, float tMax, TraversalStats* stats = nullptr) const;

        bool Occluded(const Ray& ray, float tMin, float tMax) const;

        const Material& GetMaterial(int id) const { return materials[static_cast<size_t>(id)]; }
        std::span<const Geometry> GetGeometries() const { return geometries; }
        std::span<const Light> GetLights() const { return lights; }
        const Environment& GetEnvironment() const { return environment; }

    private:
        std::vector<Material> materials;
        std::vector<Geometry> geometries;
        std::vector<Light> lights;
        Environment environment;
    };

    inline std::shared_ptr<const RenderScene>BuildScene(const Scene& scene)
    {
        return std::make_shared<const RenderScene>(scene);
    }
}