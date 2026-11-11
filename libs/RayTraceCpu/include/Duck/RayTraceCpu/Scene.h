#pragma once

#include <Duck/RayTraceCpu/Geometry.h>
#include <Duck/RayTraceCpu/Lights.h>
#include <Duck/RayTraceCpu/Materials.h>
#include <dUCK/RayTraceCpu/Environment.h>

#include <optional>
#include <vector>

namespace Duck::RayTraceCpu
{
    struct Scene
    {
        std::vector<Material> Materials;
        std::vector<Sphere> Spheres;
        std::vector<Quad> Quads;
        std::vector<Light> Lights;

        RayTraceCpu::Environment Environment;

        // returns new material id
        int AddMaterial(const Material& material)
        {
            Materials.push_back(material);
            return static_cast<int>(Materials.size()) - 1;
        }
    };
}