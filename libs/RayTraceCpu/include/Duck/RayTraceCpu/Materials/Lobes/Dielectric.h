#pragma once

#include <Duck/RayTraceCpu/Materials/Lobes/LobeCommon.h>
#include <Duck/RayTraceCpu/Materials/Optics.h>
#include <Duck/Core/Sampling.h>

namespace Duck::RayTraceCpu
{
    // smooth glass surface: Fresnel-weighted reflection and refraction
    // Eta = index on the far side / index on wo's side, so it already knows whether the ray enters or leaves
    struct DielectricLobe
    {
        float Eta = 1.5f;
        Vec3 Tint{1.0f};    // refracted light is multiplied by this
        float Scale = 1.0f; // weight in the mix

        struct Split
        {
            Vec3 Reflected{0.0f};
            std::optional<Vec3> Refracted; // nullopt on total internal reflection
            float Reflectance = 1.0f;
        };

        Split SplitAt(const Vec3& wo) const
        {
            Split split;
            split.Reflected = Vec3(-wo.x, -wo.y, wo.z);
            split.Refracted = Refract(-wo, Vec3(0.0f, 0.0f, 1.0f), 1.0f / Eta);
            if (split.Refracted)
            {
                // Schlick uses the angle on the lower-index side: wo's when entering denser glass,
                // the refracted one when leaving
                const float cosine = Eta > 1.0f ? wo.z : -split.Refracted->z;
                split.Reflectance = SchlickFresnel(cosine, 1.0f, Eta);
            }
            return split;
        }

        Vec3 Evaluate(const Vec3&, const Vec3&) const { return Vec3(0.0f); }

        std::optional<LobeSample> Sample(const Vec3& wo, ISampler& sampler) const
        {
            const Split split = SplitAt(wo);
            const float r = split.Reflectance;
            const float u = sampler.Get1D();
            if (!split.Refracted || u < r)
            {
                const Vec3 wi = split.Reflected;
                return LobeSample{wi, Vec3(Scale * r / std::abs(wi.z)), r, true};
            }
            const Vec3 wi = *split.Refracted;
            return LobeSample{
                .Wi = wi, .Value = Scale * (1.0f - r) * Tint / std::abs(wi.z), .Pdf = 1.0f - r, .IsDelta = true};
        }

        float Pdf(const Vec3&, const Vec3&) const { return 0.0f; }

        float SampleWeight(const Vec3&) const { return Scale; }

        bool IsDelta() const { return true; }
    };
}