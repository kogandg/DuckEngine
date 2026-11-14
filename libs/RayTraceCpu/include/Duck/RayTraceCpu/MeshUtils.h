#pragma once

#include <Duck/RayTraceCpu/Geometry.h>
#include <Duck/Core/Math.h>

namespace Duck::RayTraceCpu
{
    Mesh MakeUvSphereMesh(const Vec3& center, float radius, int segments, int rings, int materialId,
                          bool smooth = true);

    Mesh MakeBoxMesh(const Vec3& min, const Vec3& max, int materialId);

    void TransformMesh(Mesh& mesh, const Mat4& transform);
}