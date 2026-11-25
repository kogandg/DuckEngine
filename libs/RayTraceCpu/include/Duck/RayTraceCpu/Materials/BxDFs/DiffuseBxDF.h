#pragma once

#include <Duck/Core/Sampling.h>
#include <Duck/RayTraceCpu/Materials/BxDFs/BxDF.h>

namespace Duck::RayTraceCpu
{
    // ideal diffuse reflection
    struct DiffuseBxDF
    {
        Spectrum Reflectance{0.0f};

        BxDFFlags Flags() const { return MaxComponent(Reflectance) > 0.0f ? BxDFFlags::DiffuseReflection : BxDFFlags::Unset; }

        Spectrum Evaluate(const Vec3& wo, const Vec3& wi, TransportMode) const
        {
            return SameHemisphere(wo, wi) ? Reflectance * InvPi : Spectrum(0.0f);
        }

        std::optional<BxDFSample> Sample(const Vec3& wo, float, const Vec2& u, TransportMode) const
        {
            Vec3 wi = CosineHemisphereLocal(u.x, u.y);
            if (wo.z < 0.0f)
            {
                wi.z = -wi.z;
            }
            const float pdf = AbsCosTheta(wi) * InvPi;
            if (pdf <= 0.0f)
            {
                return std::nullopt;
            }
            return BxDFSample{.F = Reflectance * InvPi, .Wi = wi, .Pdf = pdf, .Flags = BxDFFlags::DiffuseReflection};
        }

        float Pdf(const Vec3& wo, const Vec3& wi, TransportMode) const
        {
            return SameHemisphere(wo, wi) ? AbsCosTheta(wi) * InvPi : 0.0f;
        }

        Spectrum DiffuseAlbedo(const Vec3&) const { return Reflectance; }
    };
}