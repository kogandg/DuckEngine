#include <Duck/RayTraceCpu/MeshUtils.h>

#include <Duck/Core/Assert.h>

namespace Duck::RayTraceCpu
{

    Mesh MakeUvSphereMesh(const Vec3& center, float radius, int segments, int rings, int materialId, bool smooth)
    {
        DUCK_ASSERT(segments >= 3 && rings >= 2, "Sphere mesh needs at least 3 segments and 2 rings");
        Mesh mesh;
        mesh.MaterialId = materialId;

        for (int ring = 0; ring <= rings; ring++)
        {
            const float theta = Pi * static_cast<float>(ring) / static_cast<float>(rings); // 0 at the top
            for (int segment = 0; segment <= segments; segment++) // the seam is doubled so uvs can run to 1
            {
                const float phi = TwoPi * static_cast<float>(segment) / static_cast<float>(segments);
                const Vec3 direction(std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi));
                mesh.Positions.push_back(center + radius * direction);
                if (smooth)
                {
                    mesh.Normals.push_back(direction);
                }
                mesh.Uvs.push_back({static_cast<float>(segment) / static_cast<float>(segments),
                                    static_cast<float>(ring) / static_cast<float>(rings)});
            }
        }

        const int stride = segments + 1;
        for (int ring = 0; ring < rings; ring++)
        {
            for (int segment = 0; segment < segments; segment++)
            {
                const int a = ring * stride + segment; // this ring
                const int b = a + stride;              // the ring below
                // counter-clockwise seen from outside
                // the triangles touching a pole have zero area so the scene skips them
                mesh.Indices.insert(mesh.Indices.end(), {a, a + 1, b, a + 1, b + 1, b});
            }
        }
        return mesh;
    }

    Mesh MakeBoxMesh(const Vec3& min, const Vec3& max, int materialId)
    {
        const Vec3 size = max - min;
        const Vec3 dx(size.x, 0.0f, 0.0f);
        const Vec3 dy(0.0f, size.y, 0.0f);
        const Vec3 dz(0.0f, 0.0f, size.z);
        // a corner and two edges per face, cross(u, v) pointing out of the box
        const std::array<std::array<Vec3, 3>, 6> faces = {{
            {min, dz, dy},
            {Vec3(max.x, min.y, min.z), dy, dz},
            {min, dx, dz},
            {Vec3(min.x, max.y, min.z), dz, dx},
            {min, dy, dx},
            {Vec3(min.x, min.y, max.z), dx, dy},
        }};

        Mesh mesh;
        mesh.MaterialId = materialId;
        for (const auto& [corner, u, v] : faces)
        {
            const int first = static_cast<int>(mesh.Positions.size());
            mesh.Positions.insert(mesh.Positions.end(), {corner, corner + u, corner + u + v, corner + v});
            mesh.Uvs.insert(mesh.Uvs.end(), {Vec2(0.0f, 0.0f), Vec2(1.0f, 0.0f), Vec2(1.0f, 1.0f), Vec2(0.0f, 1.0f)});
            mesh.Indices.insert(mesh.Indices.end(), {first, first + 1, first + 2, first, first + 2, first + 3});
        }
        // no vertex normals, flat faces use the triangles own normals
        return mesh;
    }

    void TransformMesh(Mesh& mesh, const Mat4& transform)
    {
        for (Vec3& position : mesh.Positions)
        {
            position = Vec3(transform * Vec4(position, 1.0f));
        }

        // normals stay perpendicular to the surface only with the inverse transpose. with the plain matrix,
        // non-uniform scaling would tilt them
        const Mat3 normalMatrix = glm::transpose(glm::inverse(Mat3(transform)));
        for (Vec3& normal : mesh.Normals)
        {
            normal = glm::normalize(normalMatrix * normal);
        }

        // a mirroring transform turns counter-clockwise into clockwise: swap two corners to keep fronts outside
        if (glm::determinant(Mat3(transform)) < 0.0f)
        {
            for (size_t i = 0; i + 2 < mesh.Indices.size(); i += 3)
            {
                std::swap(mesh.Indices[i + 1], mesh.Indices[i + 2]);
            }
        }
    }
}