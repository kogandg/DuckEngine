#pragma once

#include <Duck/Core/Sampling.h>
#include <Duck/RayTraceCpu/Materials/BxDFs/BxDF.h>
#include <Duck/RayTraceCpu/Materials/BxDFs/Microfacet.h>
#include <Duck/RayTraceCpu/Materials/Optics.h>

namespace Duck::RayTraceCpu
{
    // a dielectric coat over a Lambertian base like plastic or paint
    struct CoatedDiffuseBxDF
    {
        Spectrum Reflectance{0.0f};   // reflectance of the base
        float CoatF0 = 0.04f;         // the coats reflectance head-on from its ior
        float CoatWeight = 1.0f;      // glTFs specular factor, scales the coats reflection
        GgxDistribution Distribution; // of the coat

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
            BxDFFlags coat = BxDFFlags::Unset;
            if (CoatWeight > 0.0f)
            {
                coat = Distribution.EffectivelySmooth() ? BxDFFlags::SpecularReflection : BxDFFlags::GlossyReflection;
            }
            return MaxComponent(Reflectance) > 0.0f ? coat | BxDFFlags::DiffuseReflection : coat;
        }

        Spectrum BaseValue(const Vec3& wo, const Vec3& wi) const
        {
            return SameHemisphere(wo, wi) ? Reflectance * InvPi * (1.0f - CoatReflectance(wo)) : Spectrum(0.0f);
        }

        // the rough coat on its own. 0 when smooth: a mirror is a delta
        float CoatValue(const Vec3& wo, const Vec3& wi) const
        {
            if (Distribution.EffectivelySmooth() || !SameHemisphere(wo, wi))
            {
                return 0.0f;
            }
            const float cosO = AbsCosTheta(wo);
            const float cosI = AbsCosTheta(wi);
            Vec3 wm = wo + wi;
            if (cosO == 0.0f || cosI == 0.0f || glm::dot(wm, wm) == 0.0f)
            {
                return 0.0f;
            }
            wm = glm::normalize(wm);
            const float f = CoatWeight * SchlickReflectance(CoatF0, std::abs(glm::dot(wo, wm)));
            return Distribution.D(wm) * f * Distribution.G(wo, wi) / (4.0f * cosO * cosI);
        }

        float CoatPdf(const Vec3& wo, const Vec3& wi) const
        {
            if (Distribution.EffectivelySmooth() || !SameHemisphere(wo, wi))
            {
                return 0.0f;
            }
            Vec3 wm = wo + wi;
            if (glm::dot(wm, wm) == 0.0f)
            {
                return 0.0f;
            }
            wm = FaceForward(glm::normalize(wm), Vec3(0.0f, 0.0f, 1.0f));
            return Distribution.VisibleD(wo, wm) / (4.0f * std::abs(glm::dot(wo, wm)));
        }

        // every non-delta part: the base, plus the coat when its rough
        Spectrum Evaluate(const Vec3& wo, const Vec3& wi, TransportMode) const
        {
            return BaseValue(wo, wi) + Spectrum(CoatValue(wo, wi));
        }

        std::optional<BxDFSample> Sample(const Vec3& wo, float uc, const Vec2& u, TransportMode mode) const
        {
            const float chance = CoatChance(wo);
            Vec3 wi;
            BxDFFlags picked;
            if (uc < chance)
            {
                if (Distribution.EffectivelySmooth())
                {
                    wi = MirrorLocal(wo);
                    return BxDFSample{.F = Spectrum(CoatReflectance(wo) / AbsCosTheta(wi)),
                                      .Wi = wi,
                                      .Pdf = chance,
                                      .Flags = BxDFFlags::SpecularReflection};
                }
                if (wo.z == 0.0f)
                {
                    return std::nullopt;
                }
                wi = ReflectAbout(wo, Distribution.SampleVisibleNormal(wo, u));
                picked = BxDFFlags::GlossyReflection;
            }
            else
            {
                wi = CosineHemisphereLocal(u.x, u.y);
                if (wo.z < 0.0f)
                {
                    wi.z = -wi.z;
                }
                picked = BxDFFlags::DiffuseReflection;
            }
            if (!SameHemisphere(wo, wi))
            {
                return std::nullopt;
            }
            // with a rough coat both parts could have made wi, so the value and pdf include both
            const float pdf = Pdf(wo, wi, mode);
            if (pdf <= 0.0f)
            {
                return std::nullopt;
            }
            return BxDFSample{.F = Evaluate(wo, wi, mode), .Wi = wi, .Pdf = pdf, .Flags = picked};
        }

        float Pdf(const Vec3& wo, const Vec3& wi, TransportMode) const
        {
            if (!SameHemisphere(wo, wi))
            {
                return 0.0f;
            }
            const float chance = CoatChance(wo);
            return chance * CoatPdf(wo, wi) + (1.0f - chance) * AbsCosTheta(wi) * InvPi;
        }

        Spectrum DiffuseAlbedo(const Vec3& wo) const { return Reflectance * (1.0f - CoatReflectance(wo)); }
    };
}