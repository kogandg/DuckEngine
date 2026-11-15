#pragma once

#include <Duck/RayTraceCpu/Environment.h>
#include <Duck/RayTraceCpu/Geometry.h>
#include <Duck/RayTraceCpu/Lights.h>
#include <Duck/RayTraceCpu/Materials.h>

#include <optional>
#include <variant>
#include <vector>

namespace Duck::RayTraceCpu
{
    struct ShapeGroup
    {
        std::vector<Shape> Shapes;
    };

    using SceneGeometry = std::variant<Mesh, ShapeGroup>;

    struct GeometryInstance
    {
        int GeometryId = 0;
        Mat4 Transform{1.0f};
        int MaterialOverride = -1; // -1: use own material
    };

    struct Scene
    {
        std::vector<Material> Materials;

        std::vector<Sphere> Spheres;
        std::vector<Quad> Quads;

        std::vector<SceneGeometry> Geometries;
        std::vector<GeometryInstance> Instances;

        std::vector<Light> Lights;

        RayTraceCpu::Environment Environment;

        // returns new material id
        int AddMaterial(const Material& material)
        {
            Materials.push_back(material);
            return static_cast<int>(Materials.size()) - 1;
        }

        // returns a new mesh id. mesh isnt drawn until AddInstance places it
        int AddMesh(Mesh mesh)
        {
            Geometries.emplace_back(std::move(mesh));
            return static_cast<int>(Geometries.size()) - 1;
        }
        int AddShapes(std::vector<Shape> shapes)
        {
            Geometries.emplace_back(ShapeGroup{std::move(shapes)});
            return static_cast<int>(Geometries.size()) - 1;
        }

        void AddInstance(int geometryId, const Mat4& transform = Mat4(1.0f), int materialOverride = -1)
        {
            Instances.push_back({geometryId, transform, materialOverride});
        }
    };
}