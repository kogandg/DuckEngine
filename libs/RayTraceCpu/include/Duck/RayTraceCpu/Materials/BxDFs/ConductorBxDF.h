#pragma once

#include <Duck/RayTraceCpu/Materials/BxDFs/BxDF.h>
#include <Duck/RayTraceCpu/Materials/Optics.h>

namespace Duck::RayTraceCpu
{
    struct ConductorBxDF
    {
        Spectrum F0{1.0f};
        float Roughness = 0.0f;

        Spectrum Fresnel(float cosine) const { return SchlickReflectance(F0, cosine); }

        BxDFFlags Flags() const { return BxDFFlags::SpecularReflection; }

        Spectrum Evaluate(const Vec3&, const Vec3&, TransportMode) const { return Spectrum(0.0f); }

        std::optional<BxDFSample> Sample(const Vec3& wo, float, const Vec2&, TransportMode) const
        {
            const Vec3 wi = MirrorLocal(wo);
            const float cosine = AbsCosTheta(wi);
            if (cosine <= 0.0f)
            {
                return std::nullopt;
            }
            return BxDFSample{
                .F = Fresnel(cosine) / cosine, .Wi = wi, .Pdf = 1.0f, .Flags = BxDFFlags::SpecularReflection};
        }

        float Pdf(const Vec3&, const Vec3&, TransportMode) const { return 0.0f; }

        Spectrum DiffuseAlbedo(const Vec3&) const { return Spectrum(0.0f); }
    };
}