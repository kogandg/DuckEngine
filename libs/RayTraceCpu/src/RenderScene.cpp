#include <Duck/RayTraceCpu/RenderScene.h>

#include <Duck/Core/Assert.h>
#include <Duck/RayTraceCpu/Scene.h>

#include <algorithm>
#include <variant>

namespace Duck::RayTraceCpu
{
    RenderScene::RenderScene(const Scene& scene)
        : materials(scene.Materials), meshes(scene.Meshes), lights(scene.Lights), environment(scene.Environment)
    {
        size_t triangleCount = 0;
        for (const Mesh& mesh : meshes)
        {
            triangleCount += static_cast<size_t>(mesh.TriangleCount());
        }
        shapes.reserve(scene.Spheres.size() + scene.Quads.size() + triangleCount);
        shapes.insert(shapes.end(), scene.Spheres.begin(), scene.Spheres.end());
        shapes.insert(shapes.end(), scene.Quads.begin(), scene.Quads.end());

        for (const Mesh& mesh : meshes)
        {
            const int vertexCount = static_cast<int>(mesh.Positions.size());
            DUCK_VERIFY(mesh.Indices.size() % 3 == 0, "Mesh has {} indices, not a multiple of 3", mesh.Indices.size());
            DUCK_VERIFY(mesh.Normals.empty() || static_cast<int>(mesh.Normals.size()) == vertexCount,
                        "Mesh has {} normals for {} vertices", mesh.Normals.size(), vertexCount);
            DUCK_VERIFY(mesh.Uvs.empty() || static_cast<int>(mesh.Uvs.size()) == vertexCount,
                        "Mesh has {} uvs for {} vertices", mesh.Uvs.size(), vertexCount);
            for (const int index : mesh.Indices)
            {
                DUCK_VERIFY(index >= 0 && index < vertexCount, "Mesh index {} out of range ({} vertices)", index,
                            vertexCount);
            }

            for (int i = 0; i < mesh.TriangleCount(); i++)
            {
                const Triangle triangle {&mesh, i, mesh.MaterialId};
                if (triangle.Area() > 0.0f)
                {
                    shapes.push_back(triangle);
                }
            }
        }

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

        // every shape with an emissive material is also an area light
        for (size_t i = 0; i < shapes.size(); i++)
        {
            const Vec3 emission = UniformEmission(materials[static_cast<size_t>(ShapeMaterialId(shapes[i]))]);
            if (MaxComponent(emission) > 0.0f)
            {
                lights.push_back(AreaLight{shapes[i], emission, static_cast<int>(i)});
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