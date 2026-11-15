#pragma once

#include <Duck/Core/Math.h>
#include <Duck/Core/Ray.h>

namespace Duck::RayTraceCpu
{
    struct Hit
    {
        Vec3 Point{0.0f};
        Vec3 Normal{0.0f};          // unit, facing incoming ray. smooth on meshes with vertex normals
        Vec3 GeometricNormal{0.0f}; // normal rays use to leave surface
        Vec2 Uv{0.0f};              // surface coords for textures
        float T = 0.0f;
        bool FrontFace = true; // false if hit from the inside of a surface
        int MaterialId = 0;
        int InstanceId = -1;  // which placement of a geometry was hit
        int GeometryId = -1;  // which of the render scenes geometries was hit
        int PrimitiveId = -1; // which primitive inside that geometry
    };

    struct SurfaceSample
    {
        Vec3 Point{0.0f};
        Vec3 Normal{0.0f}; // unit vector on the front side
    };

    // rays leaving a surface ignore hits closer than this
    inline constexpr float RayEpsilon = 1e-6f;

    // how far SpawnRay pushes an origin off it surface
    inline constexpr float RayOffset = 1e-4f;

    // a ray leaving the surface at hit. the one place to add an offset along the normal later
    inline Ray SpawnRay(const Hit& hit, const Vec3& direction)
    {
        const float side = glm::dot(direction, hit.GeometricNormal) > 0.0f ? 1.0f : -1.0f;

        return {hit.Point + side * RayOffset * hit.GeometricNormal, direction};
    }
}