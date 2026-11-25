#pragma once

#include <Duck/Core/Sampling.h>
#include <Duck/RayTraceCpu/Materials/BxDFs/BxDF.h>
#include <Duck/RayTraceCpu/Materials/Optics.h>

namespace Duck::RayTraceCpu
{
    // a smooth dielectric coat over a Lambertian base like plastic or paint
    struct CoatedDiffuseBxDF
    {
        Spectrum Reflectance{0.0f}; // reflectance of the base
        float CoatF0 = 0.04f;    // the coats reflectance head-on from its ior
        float CoatWeight = 1.0f; // glTFs specular factor, scales the coats reflection
        float Roughness = 0.0f;  // of the coat. stored for the GGX distribution

        float CoatReflectance(const Vec3& wo) const { return CoatWeight * SchlickReflectance(CoatF0, AbsCosTheta(wo)); }

        // chance Sample reflects off the coat in proportion to how much light each layer sends back
        float CoatChance(const Vec3& wo) const
        {
            const float coat = CoatReflectance(wo);
            const float base = MaxComponent(Reflectance) * (1.0f - coat);
            return coat + base > 0.0f ? coat / (coat + base) : 0.0f;
        }

        BxDFFlags Flags() const
        {
            const BxDFFlags coat = CoatWeight > 0.0f ? BxDFFlags::SpecularReflection : BxDFFlags::Unset;
            return MaxComponent(Reflectance) > 0.0f ? coat | BxDFFlags::DiffuseReflection : coat;
        }

        // the base only: the coat is a delta
        Spectrum Evaluate(const Vec3& wo, const Vec3& wi, TransportMode) const
        {
            return SameHemisphere(wo, wi) ? Reflectance * InvPi * (1.0f - CoatReflectance(wo)) : Spectrum(0.0f);
        }

        std::optional<BxDFSample> Sample(const Vec3& wo, float uc, const Vec2& u, TransportMode mode) const
        {
            const float chance = CoatChance(wo);
            if (uc < chance)
            {
                const Vec3 wi = MirrorLocal(wo);
                return BxDFSample{.F = Vec3(CoatReflectance(wo) / AbsCosTheta(wi)),
                                  .Wi = wi,
                                  .Pdf = chance,
                                  .Flags = BxDFFlags::SpecularReflection};
            }
            Vec3 wi = CosineHemisphereLocal(u.x, u.y);
            if (wo.z < 0.0f)
            {
                wi.z = -wi.z;
            }
            const float pdf = (1.0f - chance) * AbsCosTheta(wi) * InvPi;
            if (pdf <= 0.0f)
            {
                return std::nullopt;
            }
            return BxDFSample{.F = Evaluate(wo, wi, mode), .Wi = wi, .Pdf = pdf, .Flags = BxDFFlags::DiffuseReflection};
        }

        float Pdf(const Vec3& wo, const Vec3& wi, TransportMode) const
        {
            return SameHemisphere(wo, wi) ? (1.0f - CoatChance(wo)) * AbsCosTheta(wi) * InvPi : 0.0f;
        }

        Spectrum DiffuseAlbedo(const Vec3& wo) const { return Reflectance * (1.0f - CoatReflectance(wo)); }
    };
}