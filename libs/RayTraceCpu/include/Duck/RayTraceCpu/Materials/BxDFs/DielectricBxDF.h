#pragma once

#include <Duck/RayTraceCpu/Materials/BxDFs/BxDF.h>
#include <Duck/RayTraceCpu/Materials/Optics.h>

namespace Duck::RayTraceCpu
{
    struct DielectricBxDF
    {
        float Eta = 1.5f;
        Spectrum Tint{1.0f};

        struct Split
        {
            float Reflectance = 1.0f;
            std::optional<Vec3> Refracted; // nullopt on total internal reflection
        };

        Split SplitAt(const Vec3& wo) const
        {
            Split split;
            split.Reflectance = FrDielectric(CosTheta(wo), Eta);
            split.Refracted = Refract(-wo, Vec3(0.0f, 0.0f, 1.0f), 1.0f / Eta);
            if (!split.Refracted)
            {
                split.Reflectance = 1.0f;
            }
            return split;
        }

        // refracted radiance is squeezed into a smaller solid angle by the relative index squared, importance is not
        Spectrum TransmittedWeight(float reflectance, TransportMode mode) const
        {
            const Spectrum weight = Tint * (1.0f - reflectance);
            return mode == TransportMode::Radiance ? weight / (Eta * Eta) : weight;
        }

        BxDFFlags Flags() const
        {
            return Eta == 1.0f ? BxDFFlags::SpecularTransmission
                               : BxDFFlags::SpecularReflection | BxDFFlags::SpecularTransmission;
        }

        Spectrum Evaluate(const Vec3&, const Vec3&, TransportMode) const { return Spectrum(0.0f); }

        std::optional<BxDFSample> Sample(const Vec3& wo, float uc, const Vec2&, TransportMode mode) const
        {
            const Split split = SplitAt(wo);
            const float r = split.Reflectance;
            if (uc < r)
            {
                const Vec3 wi = MirrorLocal(wo);
                return BxDFSample{
                    .F = Spectrum(r / AbsCosTheta(wi)), .Wi = wi, .Pdf = r, .Flags = BxDFFlags::SpecularReflection};
            }
            const Vec3 wi = *split.Refracted;
            return BxDFSample{.F = TransmittedWeight(r, mode) / AbsCosTheta(wi),
                              .Wi = wi,
                              .Pdf = 1.0f - r,
                              .Flags = BxDFFlags::SpecularTransmission,
                              .Eta = Eta};
        }

        float Pdf(const Vec3&, const Vec3&, TransportMode) const { return 0.0f; }

        Spectrum DiffuseAlbedo(const Vec3&) const { return Spectrum(0.0f); }
    };
}