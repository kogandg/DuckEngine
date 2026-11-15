#pragma once

#include <Duck/Core/Ray.h>
#include <Duck/RayTraceCpu/Environment.h>
#include <Duck/RayTraceCpu/Geometry.h>
#include <Duck/RayTraceCpu/Lights.h>
#include <Duck/RayTraceCpu/Materials.h>
#include <Duck/RayTraceCpu/RayStats.h>
#include <Duck/RayTraceCpu/SceneGeometry.h>

#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace Duck::RayTraceCpu
{
    struct Scene;

    // a geometry placed in the world. rays are moved into the geometrys own space instead of moving the geometry
    struct Instance
    {
        int GeometryId = 0;
        Mat4 ObjectToWorld{1.0f};
        Mat4 WorldToObject{1.0f}; // inverted once when the scene is built, not per ray
        Mat3 NormalToWorld{1.0f}; // inverse transpose: keeps normals perpendicular under non uniform scale
        bool IsIdentity = true;   // placed as is: no transforms needed at all
        Aabb WorldBounds;
        int MaterialOverride = -1;
        float VolumeScale = 1.0f; // |determinant| of ObjectToWorld: how much is scales volumes

        // the same ray in the geometrys space. the direction is not normalized, so t means the same point in both
        // spaces and tMax can be passed straight through
        Ray ToObject(const Ray& ray) const
        {
            return {Vec3(WorldToObject * Vec4(ray.Origin, 1.0f)), Vec3(WorldToObject * Vec4(ray.Direction, 0.0f))};
        }

        // FrontFace and T need no change: dot(M d, M^-T n) = dot(d, n), so which side was hit is the same in both
        void ToWorld(Hit& hit) const
        {
            hit.Point = Vec3(ObjectToWorld * Vec4(hit.Point, 1.0f));
            hit.Normal = glm::normalize(NormalToWorld * hit.Normal);
            hit.GeometricNormal = glm::normalize(NormalToWorld * hit.GeometricNormal);
        }
    };

    // which area light a shading point samples, and the chance it was that one
    struct LightChoice
    {
        int Index = -1;
        float Probability = 0.0f;
    };

    class RenderScene
    {
    public:
        // geometry 0 holds the scenes loose spheres and quads, then one geometry per mesh, in order. instances
        // place them: the loose shapes once as they are (if there are any), then the scenes mesh instances in order
        explicit RenderScene(const Scene& scene);

        std::optional<Hit> ClosestHit(const Ray& ray, float tMin, float tMax, TraversalStats* stats = nullptr) const;

        bool Occluded(const Ray& ray, float tMin, float tMax) const;

        const Material& GetMaterial(int id) const { return materials[static_cast<size_t>(id)]; }
        std::span<const Geometry> GetGeometries() const { return geometries; }
        const Environment& GetEnvironment() const { return environment; }
        std::span<const Instance> GetInstances() const { return instances; }
        const Bvh& GetTopLevelBvh() const { return topLevel; }

        // the scenes own lights: directional and point lights
        std::span<const Light> GetLights() const { return lights; }
        // one per emissive instance (and emmisive material on it)
        std::span<const AreaLight> GetAreaLights() const { return areaLights; }

        // picks the one area light a shading point samples, from a uniform number in [0, 1), in proportion to how
        // much light each gives off. empty if there are none
        std::optional<LightChoice> ChooseAreaLight(float u) const;
        // the chance ChooseAreaLight picks this light. MIS needs it for lights a ray hits by itself
        float AreaLightProbability(int light) const;
        // a point on an area light in world space, spread evenly over its surface
        AreaSample SampleAreaLightSurface(int light, Vec2 u) const;
        // the light arriving at point from a random point on an area light, with its pdf per solid angle
        LightSample SampleAreaLight(int light, const Vec3& point, ISampler& sampler) const;
        //the area light hit is on, or -1 if it isnt on one
        int AreaLightAt(const Hit& hit) const;
        // the density, per solid angle seen from `from`, of light sampling picking the point hit on an area light:
        // the chance ChooseAreaLight picks it, times the chance SampleAreaLight lands on that point
        float AreaLightPdf(int light, const Hit& hit, const Vec3& from) const;



    private:
        void BuildAreaLightCdf();
        double WorldAreaEstimate(int light) const;

        std::vector<Material> materials;
        std::vector<Geometry> geometries;
        std::vector<Instance> instances;
        Bvh topLevel;
        std::vector<Light> lights;
        Environment environment;

        std::vector<AreaLight> areaLights;
        std::vector<float> areaLightCdf;

        std::vector<int> firstAreaLight; // instance i's lights are [firstAreaLight[i], firstAreaLight[i + 1])
    };

    inline std::shared_ptr<const RenderScene> BuildScene(const Scene& scene)
    {
        return std::make_shared<const RenderScene>(scene);
    }
}
