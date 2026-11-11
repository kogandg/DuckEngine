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
        shapes.reserve(scene.Spheres.size() + scene.Quads.size());
        shapes.insert(shapes.end(), scene.Spheres.begin(), scene.Spheres.end());
        shapes.insert(shapes.end(), scene.Quads.begin(), scene.Quads.end());

        for (const Shape& shape : shapes)
        {
            const int materialId = ShapeMaterialId(shape);
            DUCK_VERIFY(materialId >= 0 && materialId < static_cast<int>(materials.size()),
                        "Shape uses material {} but the scene has {} materials", materialId, materials.size());
            DUCK_VERIFY(ShapeArea(shape) > 0.0f, "Shape has no area (zero radius or zero-length edge)");
        }

        for (Light& light : lights)
        {
            if (auto* sun = std::get_if<DirectionalLight>(&light))
            {
                DUCK_VERIFY(glm::dot(sun->Direction, sun->Direction) > 0.0f, "Directional light has no direction");
                sun->Direction = glm::normalize(sun->Direction);
            }
        }
    }

    std::optional<Hit> RenderScene::ClosestHit(const Ray& ray, float tMin, float tMax) const
    {
        std::optional<Hit> closest;
        for (size_t i = 0; i < shapes.size(); i++)
        {
            if (auto hit = IntersectShape(shapes[i], ray, tMin, tMax))
            {
                hit->PrimitiveId = static_cast<int>(i);
                tMax = hit->T; // anything further away cant be the closest now
                closest = hit;
            }
        }
        return closest;
    }

    bool RenderScene::Occluded(const Ray& ray, float tMin, float tMax) const
    {
        return std::ranges::any_of(shapes, [&](const Shape& shape)
                                   { return IntersectShape(shape, ray, tMin, tMax).has_value(); });
    }

}