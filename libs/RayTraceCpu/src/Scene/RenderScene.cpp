#include <Duck/RayTraceCpu/Scene/RenderScene.h>

#include <Duck/Core/Assert.h>
#include <Duck/Core/Color.h>
#include <Duck/Core/Log.h>
#include <Duck/Core/Stopwatch.h>
#include <Duck/RayTraceCpu/Scene/Scene.h>

#include <algorithm>
#include <utility>
#include <variant>

namespace Duck::RayTraceCpu
{
    namespace
    {
        Instance MakeInstance(int geometryId, const Geometry& geometry, const Mat4& transform, int materialOverride)
        {
            Instance instance;
            instance.GeometryId = geometryId;
            instance.IsIdentity = transform == Mat4(1.0f);
            const Aabb& objectBounds = GeometryBvh(geometry).Root().Bounds;
            if (instance.IsIdentity)
            {
                instance.WorldBounds = objectBounds;
                return instance;
            }

            DUCK_VERIFY(std::abs(glm::determinant(Mat3(transform))) > 0.0f,
                        "Instance transform flattens its mesh (determinant 0) and cant be inverted");
            instance.ObjectToWorld = transform;
            instance.WorldToObject = glm::inverse(transform);
            instance.NormalToWorld = glm::transpose(glm::inverse(Mat3(transform)));
            instance.VolumeScale = std::abs(glm::determinant(Mat3(transform)));
            instance.WorldBounds = objectBounds.Transformed(transform);
            instance.MaterialOverride = materialOverride;
            return instance;
        }
    }

    RenderScene::RenderScene(const Scene& scene)
        : materials(scene.Materials), lights(scene.Lights), environment(scene.Environment)
    {
        std::vector<Shape> looseShapes;
        looseShapes.reserve(scene.Spheres.size() + scene.Quads.size());
        looseShapes.insert(looseShapes.end(), scene.Spheres.begin(), scene.Spheres.end());
        looseShapes.insert(looseShapes.end(), scene.Quads.begin(), scene.Quads.end());
        for (const Shape& shape : looseShapes)
        {
            DUCK_VERIFY(ShapeArea(shape) > 0.0f, "Shape has no area (zero radius or zero-length edge)");
        }

        const Stopwatch stopwatch;

        geometries.reserve(1 + scene.Geometries.size());
        geometries.emplace_back(ShapeList(std::move(looseShapes)));
        for (const SceneGeometry& described : scene.Geometries)
        {
            if (const auto* mesh = std::get_if<Mesh>(&described))
            {
                geometries.emplace_back(MeshGeometry(*mesh));
                continue;
            }
            const ShapeGroup& group = std::get<ShapeGroup>(described);
            for (const Shape& shape : group.Shapes)
            {
                DUCK_VERIFY(ShapeArea(shape) > 0.0f, "Shape has no area (zero radius or zero-length edge)");
            }
            geometries.emplace_back(ShapeList(group.Shapes));
        }

        // an empty geometry has no box to put in the top level BVH, and nothing to hit anyway
        if (GeometryPrimitiveCount(geometries[0]) > 0)
        {
            instances.push_back(MakeInstance(0, geometries[0], Mat4(1.0f), -1));
        }
        for (const GeometryInstance& placed : scene.Instances)
        {
            DUCK_VERIFY(placed.GeometryId >= 0 && placed.GeometryId < static_cast<int>(scene.Geometries.size()),
                        "Instance places geometry {} but the scene has {}", placed.GeometryId, scene.Geometries.size());
            DUCK_VERIFY(placed.MaterialOverride >= -1 && placed.MaterialOverride < static_cast<int>(materials.size()),
                        "Instance overrides with material {} but the scene has {} materials", placed.MaterialOverride,
                        materials.size());
            const int geometryId = 1 + placed.GeometryId;
            const Geometry& geometry = geometries[static_cast<size_t>(geometryId)];
            if (GeometryPrimitiveCount(geometry) > 0)
            {
                instances.push_back(MakeInstance(geometryId, geometry, placed.Transform, placed.MaterialOverride));
            }
        }

        std::vector<Aabb> instanceBounds;
        instanceBounds.reserve(instances.size());
        for (const Instance& instance : instances)
        {
            instanceBounds.push_back(instance.WorldBounds);
        }
        topLevel = Bvh(instanceBounds);

        double bvhMs = topLevel.BuildStats().BuildMs;
        int primitives = 0;
        int bvhNodes = topLevel.BuildStats().Nodes;
        for (size_t g = 0; g < geometries.size(); g++)
        {
            const BvhBuildStats& bvh = GeometryBvh(geometries[g]).BuildStats();
            bvhMs += bvh.BuildMs;
            primitives += bvh.Primitives;
            bvhNodes += bvh.Nodes;
            Log::Debug("  geometry {}: {} primitives, {} nodes, depth {}, largest leaf {}, SAH cost {:.1f}, {:.2f} ms",
                       g, bvh.Primitives, bvh.Nodes, bvh.MaxDepth, bvh.LargestLeaf, bvh.SahCost, bvh.BuildMs);
        }
        Log::Info("Scene built in {:.1f} ms: {} geometries ({} primitives) placed by {} instances, {} BVH nodes "
                  "({:.1f} ms of it BVHs)",
                  stopwatch.ElapsedMs(), geometries.size(), primitives, instances.size(), bvhNodes, bvhMs);

        for (Light& light : lights)
        {
            if (auto* sun = std::get_if<DirectionalLight>(&light))
            {
                DUCK_VERIFY(glm::dot(sun->Direction, sun->Direction) > 0.0f, "Directional light has no direction");
                sun->Direction = glm::normalize(sun->Direction);
            }
        }

        for (const Geometry& geometry : geometries)
        {
            for (int p = 0; p < GeometryPrimitiveCount(geometry); p++)
            {
                const int materialId = ShapeMaterialId(GeometryPrimitive(geometry, p));
                DUCK_VERIFY(materialId >= 0 && materialId < static_cast<int>(materials.size()),
                            "Shape uses material {} but the scene has {} materials", materialId, materials.size());
            }
        }

        // emissive primitives become area lights: one per instance and emissive material, sampled as one surface,
        // however many primitives it has
        for (size_t i = 0; i < instances.size(); i++)
        {
            const Instance& instance = instances[i];
            const Geometry& geometry = geometries[static_cast<size_t>(instance.GeometryId)];
            const size_t firstLight = areaLights.size(); // this instances lights start here
            firstAreaLight.push_back(static_cast<int>(firstLight));
            for (int p = 0; p < GeometryPrimitiveCount(geometry); p++)
            {
                const Shape primitive = GeometryPrimitive(geometry, p);
                const int materialId =
                    instance.MaterialOverride >= 0 ? instance.MaterialOverride : ShapeMaterialId(primitive);
                const Vec3 emission = UniformEmission(materials[static_cast<size_t>(materialId)]);
                if (MaxComponent(emission) <= 0.0f)
                {
                    continue;
                }

                // an instance almost always has one emissive material, so a short search beats a map
                size_t light = firstLight;
                while (light < areaLights.size() && areaLights[light].MaterialId != materialId)
                {
                    light++;
                }
                if (light == areaLights.size())
                {
                    AreaLight added;
                    added.InstanceId = static_cast<int>(i);
                    added.MaterialId = materialId;
                    added.Radiance = emission;
                    areaLights.push_back(std::move(added));
                }

                AreaLight& areaLight = areaLights[light];
                areaLight.TotalArea += ShapeArea(primitive);
                areaLight.Primitives.push_back(p);
                areaLight.AreaCdf.push_back(areaLight.TotalArea);
            }
        }
        firstAreaLight.push_back(static_cast<int>(areaLights.size()));

        BuildAreaLightCdf();
    }

    std::optional<Hit> RenderScene::ClosestHit(const Ray& ray, float tMin, float tMax, TraversalStats* stats) const
    {
        std::optional<Hit> closest;
        int closestInstance = -1;
        TraversalStats local;         // inside the geometries
        TraversalStats instanceStats; // in the top level

        topLevel.ClosestHit(ray, tMin, tMax, instanceStats,
                            [&](int index, float currentMax)
                            {
                                const Instance& instance = instances[static_cast<size_t>(index)];
                                const Geometry& geometry = geometries[static_cast<size_t>(instance.GeometryId)];
                                auto hit =
                                    GeometryClosestHit(geometry, instance.IsIdentity ? ray : instance.ToObject(ray),
                                                       tMin, currentMax, local);
                                if (!hit)
                                {
                                    return currentMax;
                                }
                                closest = hit;
                                closestInstance = index;
                                return hit->T;
                            });

        if (closest)
        {
            // only the winner goes back to world space
            const Instance& instance = instances[static_cast<size_t>(closestInstance)];
            if (!instance.IsIdentity)
            {
                instance.ToWorld(*closest);
            }
            closest->InstanceId = closestInstance;
            closest->GeometryId = instance.GeometryId;
            if (instance.MaterialOverride > -1)
            {
                closest->MaterialId = instance.MaterialOverride;
            }
        }

        RayStats::Counters& counters = RayStats::ThreadCounters;
        counters.ClosestHitRays++;
        counters.NodeTests += static_cast<uint64_t>(local.NodeTests + instanceStats.NodeTests);
        counters.PrimitiveTests += static_cast<uint64_t>(local.PrimitiveTests);
        if (stats)
        {
            stats->NodeTests += local.NodeTests + instanceStats.NodeTests;
            stats->PrimitiveTests += local.PrimitiveTests;
        }
        return closest;
    }

    bool RenderScene::Occluded(const Ray& ray, float tMin, float tMax) const
    {
        TraversalStats local;
        TraversalStats instanceStats;
        const bool blocked = topLevel.AnyHit(
            ray, tMin, tMax, instanceStats,
            [&](int index)
            {
                const Instance& instance = instances[static_cast<size_t>(index)];
                return GeometryOccluded(geometries[static_cast<size_t>(instance.GeometryId)],
                                        instance.IsIdentity ? ray : instance.ToObject(ray), tMin, tMax, local);
            });

        RayStats::Counters& counters = RayStats::ThreadCounters;
        counters.OcclusionRays++;
        counters.NodeTests += static_cast<uint64_t>(local.NodeTests + instanceStats.NodeTests);
        counters.PrimitiveTests += static_cast<uint64_t>(local.PrimitiveTests);
        return blocked;
    }

    std::optional<LightChoice> RenderScene::ChooseAreaLight(float u) const
    {
        if (areaLights.empty())
        {
            return std::nullopt;
        }
        // the first light whose running total passes u: each light owns a slice of [0, 1) as wide as its chance
        const auto found = std::upper_bound(areaLightCdf.begin(), areaLightCdf.end(), u);
        const int index =
            std::min(static_cast<int>(found - areaLightCdf.begin()), static_cast<int>(areaLights.size()) - 1);
        return LightChoice{index, AreaLightProbability(index)};
    }

    float RenderScene::AreaLightProbability(int light) const
    {
        const size_t index = static_cast<size_t>(light);
        return areaLightCdf[index] - (index > 0 ? areaLightCdf[index - 1] : 0.0f);
    }

    AreaSample RenderScene::SampleAreaLightSurface(int lightIndex, Vec2 u) const
    {
        const AreaLight& light = areaLights[static_cast<size_t>(lightIndex)];

        // pick a primitive in proportion to its area. u.x lands somewhere inside that primitives slice of the
        // running total, and where exactly is uniform again, so it's stretched back to [0, 1) and reused
        const float target = u.x * light.TotalArea;
        const auto found = std::upper_bound(light.AreaCdf.begin(), light.AreaCdf.end(), target);
        const size_t chosen = std::min(static_cast<size_t>(found - light.AreaCdf.begin()), light.AreaCdf.size() - 1);
        const float sliceStart = chosen > 0 ? light.AreaCdf[chosen - 1] : 0.0f;
        u.x = std::clamp((target - sliceStart) / (light.AreaCdf[chosen] - sliceStart), 0.0f, 1.0f);

        const Instance& instance = instances[static_cast<size_t>(light.InstanceId)];
        const Shape primitive =
            GeometryPrimitive(geometries[static_cast<size_t>(instance.GeometryId)], light.Primitives[chosen]);
        const SurfaceSample surface = SampleShapeSurface(primitive, u);

        // area(primitive) / total to pick it, times 1 / area(primitive) for the point on it: 1 / total, uniform
        const float objectPdf = 1.0f / light.TotalArea;
        if (instance.IsIdentity)
        {
            return {surface.Point, surface.Normal, objectPdf};
        }

        // a transform can stretch some parts of the surface more than others (a sphere scaled into an ellipsoid).
        // by Nansons formula a small patchs area grows by |det M| * |M^-T n|, so the density shrinks by that
        const Vec3 normal = instance.NormalToWorld * surface.Normal;
        const float normalLength = glm::length(normal);
        const float areaScale = instance.VolumeScale * normalLength;
        return {Vec3(instance.ObjectToWorld * Vec4(surface.Point, 1.0f)), normal / normalLength, objectPdf / areaScale};
    }

    LightSample RenderScene::SampleAreaLight(int lightIndex, const Vec3& point, ISampler& sampler) const
    {
        const AreaSample surface = SampleAreaLightSurface(lightIndex, sampler.Get2D());

        const Vec3 toLight = surface.Point - point;
        const float distanceSquared = glm::dot(toLight, toLight);
        if (distanceSquared <= 0.0f)
        {
            return {};
        }

        const float distance = std::sqrt(distanceSquared);
        const Vec3 direction = toLight / distance;

        const float cosLight = -glm::dot(surface.Normal, direction);
        if (cosLight <= 0.0f)
        {
            return {}; // back of the light or edge on
        }

        // per area to per solid angle: a patch dA seen from the point covers dA * cos / distance^2 steradians
        const float pdf = surface.Pdf * distanceSquared / cosLight;
        return {direction, distance, areaLights[static_cast<size_t>(lightIndex)].Radiance, pdf, false};
    }

    void RenderScene::BuildAreaLightCdf()
    {
        // a light's power is radiance * area * pi (its front side emits into a whole hemisphere). picking lights in
        // proportion to it spends samples where the light is. the weights only steer which light gets sampled:
        // dividing by the same probability keeps the result right, so the area only needs to be roughly right
        std::vector<double> power(areaLights.size());
        double totalPower = 0.0;
        for (size_t i = 0; i < areaLights.size(); i++)
        {
            power[i] =
                static_cast<double>(Luminance(areaLights[i].Radiance)) * WorldAreaEstimate(static_cast<int>(i)) * Pi;
            totalPower += power[i];
        }

        // every light keeps a small share however dim it is. a slice too thin for a float would never be picked,
        // and a light thats never sampled is light missing from the image, not just noise
        const double count = static_cast<double>(areaLights.size());
        const double floor = totalPower > 0.0 ? 1e-4 * totalPower / count : 1.0; // no power at all: all equal
        double weightSum = 0.0;
        for (double& weight : power)
        {
            weight = std::max(weight, floor);
            weightSum += weight;
        }

        areaLightCdf.resize(areaLights.size());
        double running = 0.0;
        for (size_t i = 0; i < areaLights.size(); i++)
        {
            running += power[i] / weightSum;
            areaLightCdf[i] = static_cast<float>(running);
        }
        if (!areaLightCdf.empty())
        {
            areaLightCdf.back() = 1.0f; // exactly, so every u in [0, 1) finds a light
        }
    }

    double RenderScene::WorldAreaEstimate(int light) const
    {
        // 1 / pdf averages to the area for any correct pdf, so a fixed grid of samples estimates it. exact for
        // lights placed as they are, close for transformed ones (where the stretch varies over the surface)
        constexpr int Grid = 8;
        double sum = 0.0;
        for (int y = 0; y < Grid; y++)
        {
            for (int x = 0; x < Grid; x++)
            {
                const Vec2 u((static_cast<float>(x) + 0.5f) / Grid, (static_cast<float>(y) + 0.5f) / Grid);
                sum += 1.0 / static_cast<double>(SampleAreaLightSurface(light, u).Pdf);
            }
        }
        return sum / (Grid * Grid);
    }

    int RenderScene::AreaLightAt(const Hit& hit) const
    {
        if (hit.InstanceId < 0)
        {
            return -1;
        }
        const size_t instance = static_cast<size_t>(hit.InstanceId);
        for (int light = firstAreaLight[instance]; light < firstAreaLight[instance + 1]; light++)
        {
            if (areaLights[static_cast<size_t>(light)].MaterialId == hit.MaterialId)
            {
                return light;
            }
        }
        return -1;
    }

    float RenderScene::AreaLightPdf(int lightIndex, const Hit& hit, const Vec3& from) const
    {
        const AreaLight& light = areaLights[static_cast<size_t>(lightIndex)];
        const Instance& instance = instances[static_cast<size_t>(light.InstanceId)];

        // the same density SampleAreaLightSurface gives, worked out from the world normal instead of the object
        // one: there areaScale = |det M| * |M^-T n_object|, and |M^-T n_object| = 1 / |M^T n_world|
        float areaPdf = 1.0f / light.TotalArea;
        if (!instance.IsIdentity)
        {
            const Vec3 stretched = glm::transpose(Mat3(instance.ObjectToWorld)) * hit.GeometricNormal;
            areaPdf *= glm::length(stretched) / instance.VolumeScale;
        }

        // per area to per solid angle, as in SampleAreaLight
        const Vec3 toLight = hit.Point - from;
        const float distanceSquared = glm::dot(toLight, toLight);
        const float cosLight = std::abs(glm::dot(hit.GeometricNormal, toLight)) / std::sqrt(distanceSquared);
        if (cosLight <= 0.0f)
        {
            return 0.0f;
        }
        return AreaLightProbability(lightIndex) * areaPdf * distanceSquared / cosLight;
    }
}
