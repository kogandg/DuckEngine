#pragma once

#include <Duck/Core/Math.h>
#include <Duck/Core/Sampling.h>
#include <Duck/RayTraceCpu/Materials/BxDFs/BxDF.h>

#include <algorithm>
#include <cmath>

namespace Duck::RayTraceCpu
{
    struct GgxDistribution
    {
        float Alpha = 0.0f; // 0 is smooth, 1 is rough

        // below this the surface is treated as a perfect mirror. GGX itself breaks down numerically there, and the
        // delta version is exact and noise free
        static constexpr float SmoothAlpha = 1e-3f;

        // glTF roughness conversion
        static float AlphaFromRoughness(float roughness) { return roughness * roughness; }

        bool EffectivelySmooth() const { return Alpha < SmoothAlpha; }

        // density of microfacet normals: D(wm). integrates to 1 over the hemisphere when weighted by cos(wm)
        float D(const Vec3& wm) const
        {
            const float cos2 = wm.z * wm.z;
            if (cos2 <= 0.0f)
            {
                return 0.0f;
            }
            const float a2 = Alpha * Alpha;
            const float denominator = cos2 * (a2 - 1.0f) + 1.0f;
            return a2 / (Pi * denominator * denominator);
        }

        // Smiths Lambda: how much of the surface the microfacets hide when seen from w
        float Lambda(const Vec3& w) const
        {
            const float cos2 = w.z * w.z;
            if (cos2 <= 0.0f)
            {
                return Infinity;
            }
            const float tan2 = std::max(0.0f, 1.0f - cos2) / cos2;
            return 0.5f * (std::sqrt(1.0f + Alpha * Alpha * tan2) - 1.0f);
        }

        // fraction of microfacets visible from w
        float G1(const Vec3& w) const { return 1.0f / (1.0f + Lambda(w)); }

        // fraction visible from both wo and wi (height-correlated, which is more accurate than G1(wo) * G1(wi))
        float G(const Vec3& wo, const Vec3& wi) const { return 1.0f / (1.0f + Lambda(wo) + Lambda(wi)); }

        // density of the normals seen from w: D weighted by how much of each faces w. VisibleD(w, wm) is the pdf of
        // SampleVisibleNormal
        float VisibleD(const Vec3& w, const Vec3& wm) const
        {
            const float cosW = std::abs(w.z);
            if (cosW <= 0.0f)
            {
                return 0.0f;
            }
            return G1(w) / cosW * D(wm) * std::abs(glm::dot(w, wm));
        }

        // a microfacet normal in proportion to how much of it w sees
        Vec3 SampleVisibleNormal(const Vec3& w, const Vec2& u) const
        {
            Vec3 wh = glm::normalize(Vec3(Alpha * w.x, Alpha * w.y, w.z));
            if (wh.z < 0.0f)
            {
                wh = -wh;
            }
            const Vec3 t1 =
                wh.z < 0.99999f ? glm::normalize(glm::cross(Vec3(0.0f, 0.0f, 1.0f), wh)) : Vec3(1.0f, 0.0f, 0.0f);
            const Vec3 t2 = glm::cross(wh, t1);

            Vec2 p = ConcentricDisk(u.x, u.y);
            // squash the disk to the part of it that faces w
            const float h = std::sqrt(std::max(0.0f, 1.0f - p.x * p.x));
            p.y = glm::mix(h, p.y, 0.5f * (1.0f + wh.z));

            const float pz = std::sqrt(std::max(0.0f, 1.0f - p.x * p.x - p.y * p.y));
            const Vec3 nh = p.x * t1 + p.y * t2 + pz * wh;
            return glm::normalize(Vec3(Alpha * nh.x, Alpha * nh.y, std::max(1e-6f, nh.z)));
        }
    };

    inline float Sqr(float x) { return x * x; }

    // w reflected about the microfacet normal m. both point away from the surface
    inline Vec3 ReflectAbout(const Vec3& w, const Vec3& m) { return -w + 2.0f * glm::dot(w, m) * m; }

    // m flipped to ws side
    inline Vec3 FaceForward(const Vec3& m, const Vec3& w) { return glm::dot(m, w) < 0.0f ? -m : m; }
}