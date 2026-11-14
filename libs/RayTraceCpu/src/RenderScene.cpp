#include <Duck/RayTraceCpu/RenderScene.h>

#include <Duck/Core/Assert.h>
#include <Duck/RayTraceCpu/Scene.h>

#include <algorithm>
#include <variant>

namespace Duck::RayTraceCpu
{
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

        geometries.reserve(1 + scene.Meshes.size());
        geometries.emplace_back(ShapeList(std::move(looseShapes)));
        for (const Mesh& mesh : scene.Meshes)
        {
            geometries.emplace_back(MeshGeometry(mesh));
        }

        for (Light& light : lights)
        {
            if (auto* sun = std::get_if<DirectionalLight>(&light))
            {
                DUCK_VERIFY(glm::dot(sun->Direction, sun->Direction) > 0.0f, "Directional light has no direction");
                sun->Direction = glm::normalize(sun->Direction);
            }
        }

        for (size_t g = 0; g < geometries.size(); g++)
        {
            for (int p = 0; p < GeometryPrimitiveCount(geometries[g]); p++)
            {
                const Shape primitive = GeometryPrimitive(geometries[g], p);
                const int materialId = ShapeMaterialId(primitive);
                DUCK_VERIFY(materialId >= 0 && materialId < static_cast<int>(materials.size()),
                            "Shape uses material {} but the scene has {} materials", materialId, materials.size());

                // every primitive with an emissive material is also an area light
                const Vec3 emission = UniformEmission(materials[static_cast<size_t>(materialId)]);
                if (MaxComponent(emission) > 0.0f)
                {
                    lights.push_back(AreaLight{primitive, emission, static_cast<int>(g), p});
                }
            }
        }
    }

    std::optional<Hit> RenderScene::ClosestHit(const Ray& ray, float tMin, float tMax, TraversalStats* stats) const
    {
        std::optional<Hit> closest;
        int primitiveTests = 0;
        for (size_t g = 0; g < geometries.size(); g++)
        {
            if (auto hit = GeometryClosestHit(geometries[g], ray, tMin, tMax, primitiveTests))
            {
                hit->GeometryId = static_cast<int>(g);
                tMax = hit->T; // later geometries only need to beat this one
                closest = hit;
            }
        }

        RayStats::Counters& counters = RayStats::ThreadCounters;
        counters.ClosestHitRays++;
        counters.PrimitiveTests += static_cast<uint64_t>(primitiveTests);
        if (stats)
        {
            stats->PrimitiveTests += primitiveTests;
        }
        return closest;
    }

    bool RenderScene::Occluded(const Ray& ray, float tMin, float tMax) const
    {
        int primitiveTests = 0;
        bool blocked = false;
        for (const Geometry& geometry : geometries)
        {
            if (GeometryOccluded(geometry, ray, tMin, tMax, primitiveTests))
            {
                blocked = true;
                break;
            }
        }

        RayStats::Counters& counters = RayStats::ThreadCounters;
        counters.OcclusionRays++;
        counters.PrimitiveTests += static_cast<uint64_t>(primitiveTests);
        return blocked;
    }
}
