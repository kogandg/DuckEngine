#pragma once

#include <Duck/RayTraceCpu/Materials/Lobes/LobeCommon.h>
#include <Duck/RayTraceCpu/Materials/Optics.h>
#include <Duck/Core/Sampling.h>

namespace Duck::RayTraceCpu
{
    // ideal diffuse reflection
    struct LambertLobe
    {
        Vec3 Reflectance{0.0f};

        Vec3 Evaluate(const Vec3&, const Vec3& wi) const { return wi.z > 0.0f ? Reflectance * InvPi : Vec3(0.0f); }

        std::optional<LobeSample> Sample(const Vec3& wo, ISampler& sampler) const
        {
            const Vec2 u = sampler.Get2D();
            const Vec3 wi = CosineHemisphereLocal(u.x, u.y);
            if (wi.z <= 0.0f)
            {
                return std::nullopt;
            }
            return LobeSample{.Wi = wi, .Value = Evaluate(wo, wi), .Pdf = wi.z * InvPi, .IsDelta = false};
        }

        float Pdf(const Vec3&, const Vec3& wi) const { return wi.z > 0.0f ? wi.z * InvPi : 0.0f; }

        float SampleWeight(const Vec3&) const { return MaxComponent(Reflectance); }

        bool IsDelta() const { return false; }
    };
}