#pragma once

#include <Duck/RayTraceCpu/Materials/BxDFs/BxDF.h>
#include <Duck/RayTraceCpu/Materials/BxDFs/Microfacet.h>
#include <Duck/RayTraceCpu/Materials/Optics.h>

namespace Duck::RayTraceCpu
{
    struct DielectricBxDF
    {
        float Eta = 1.5f;
        Spectrum Tint{1.0f};
        GgxDistribution Distribution;

        bool IsSmooth() const { return Eta == 1.0f || Distribution.EffectivelySmooth(); }

        BxDFFlags Flags() const
        {
            const BxDFFlags sides =
                Eta == 1.0f ? BxDFFlags::Transmission : BxDFFlags::Reflection | BxDFFlags::Transmission;
            return sides | (IsSmooth() ? BxDFFlags::Specular : BxDFFlags::Glossy);
        }

        // refracted radiance is squeezed into a smaller solid angle by the relative index squared, importance is not
        static Spectrum Squeeze(const Spectrum& value, float etap, TransportMode mode)
        {
            return mode == TransportMode::Radiance ? value / (etap * etap) : value;
        }

        // ---------- smooth --------

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

        std::optional<BxDFSample> SampleSmooth(const Vec3& wo, float uc, TransportMode mode) const
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
            return BxDFSample{.F = Squeeze(Tint * (1.0f - r), Eta, mode) / AbsCosTheta(wi),
                              .Wi = wi,
                              .Pdf = 1.0f - r,
                              .Flags = BxDFFlags::SpecularTransmission,
                              .Eta = Eta};
        }

        // ------- rough -----------

        // the microfacet normal that sends wo to wi, and the relative index crossed (1 for reflection)
        // empty for pairs no microfacet can connect, or that would need a microfacet facing away from either side
        struct HalfVector
        {
            Vec3 Wm{0.0f};
            float Etap = 1.0f;
            bool Reflect = true;
        };

        std::optional<HalfVector> HalfVectorFor(const Vec3& wo, const Vec3& wi) const
        {
            const float cosO = CosTheta(wo);
            const float cosI = CosTheta(wi);
            if (cosO == 0.0f || cosI == 0.0f)
            {
                return std::nullopt;
            }
            HalfVector half;
            half.Reflect = cosO * cosI > 0.0f;
            half.Etap = half.Reflect ? 1.0f : (cosO > 0.0f ? Eta : 1.0f / Eta);
            // the generalized half vector: for refraction wo + etap * wi is along the microfacet normal
            Vec3 wm = wi * half.Etap + wo;
            if (glm::dot(wm, wm) == 0.0f)
            {
                return std::nullopt;
            }
            wm = FaceForward(glm::normalize(wm), Vec3(0.0f, 0.0f, 1.0f));
            // a microfacet seen from behind by either direction can't be the one that connected them
            if (glm::dot(wm, wi) * cosI < 0.0f || glm::dot(wm, wo) * cosO < 0.0f)
            {
                return std::nullopt;
            }
            half.Wm = wm;
            return half;
        }

        Spectrum Evaluate(const Vec3& wo, const Vec3& wi, TransportMode mode) const
        {
            if (IsSmooth())
            {
                return Spectrum(0.0f);
            }
            const auto half = HalfVectorFor(wo, wi);
            if (!half)
            {
                return Spectrum(0.0f);
            }
            const Vec3& wm = half->Wm;
            const float cosO = CosTheta(wo);
            const float cosI = CosTheta(wi);
            const float f = FrDielectric(glm::dot(wo, wm), Eta);
            const float d = Distribution.D(wm);
            const float g = Distribution.G(wo, wi);
            if (half->Reflect)
            {
                return Spectrum(d * g * f / std::abs(4.0f * cosI * cosO));
            }
            const float denominator = Sqr(glm::dot(wi, wm) + glm::dot(wo, wm) / half->Etap) * cosI * cosO;
            const float ft = d * (1.0f - f) * g * std::abs(glm::dot(wi, wm) * glm::dot(wo, wm) / denominator);
            return Squeeze(Tint * ft, half->Etap, mode);
        }

        std::optional<BxDFSample> Sample(const Vec3& wo, float uc, const Vec2& u, TransportMode mode) const
        {
            if (IsSmooth())
            {
                return SampleSmooth(wo, uc, mode);
            }
            if (wo.z == 0.0f)
            {
                return std::nullopt;
            }

            // pick a microfacet, then reflect or refract through it like a smooth boundary
            const Vec3 wm = Distribution.SampleVisibleNormal(wo, u);
            const float r = FrDielectric(glm::dot(wo, wm), Eta);
            if (uc < r)
            {
                const Vec3 wi = ReflectAbout(wo, wm);
                if (!SameHemisphere(wo, wi))
                {
                    return std::nullopt;
                }
                return BxDFSample{.F = Evaluate(wo, wi, mode),
                                  .Wi = wi,
                                  .Pdf = Pdf(wo, wi, mode),
                                  .Flags = BxDFFlags::GlossyReflection};
            }

            // Eta is relative to the side the shading normal faces. wo is almost always there, but smooth vertex
            // normals can put it just below
            const float etap = CosTheta(wo) > 0.0f ? Eta : 1.0f / Eta;
            const auto refracted = Refract(-wo, FaceForward(wm, wo), 1.0f / etap);
            if (!refracted || SameHemisphere(wo, *refracted) || refracted->z == 0.0f)
            {
                return std::nullopt;
            }
            const Vec3 wi = *refracted;
            return BxDFSample{.F = Evaluate(wo, wi, mode),
                              .Wi = wi,
                              .Pdf = Pdf(wo, wi, mode),
                              .Flags = BxDFFlags::GlossyTransmission,
                              .Eta = etap};
        }

        float Pdf(const Vec3& wo, const Vec3& wi, TransportMode) const
        {
            if (IsSmooth())
            {
                return 0.0f;
            }
            const auto half = HalfVectorFor(wo, wi);
            if (!half)
            {
                return 0.0f;
            }
            const Vec3& wm = half->Wm;
            const float r = FrDielectric(glm::dot(wo, wm), Eta);
            if (half->Reflect)
            {
                return Distribution.VisibleD(wo, wm) / (4.0f * std::abs(glm::dot(wo, wm))) * r;
            }
            // how fast the refracted direction moves as the microfacet normal moves
            const float denominator = Sqr(glm::dot(wi, wm) + glm::dot(wo, wm) / half->Etap);
            const float dwmDwi = std::abs(glm::dot(wi, wm)) / denominator;
            return Distribution.VisibleD(wo, wm) * dwmDwi * (1.0f - r);
        }

        Spectrum DiffuseAlbedo(const Vec3&) const { return Spectrum(0.0f); }
    };
}