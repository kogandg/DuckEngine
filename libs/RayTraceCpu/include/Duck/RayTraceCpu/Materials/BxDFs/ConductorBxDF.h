#pragma once

#include <Duck/RayTraceCpu/Materials/BxDFs/BxDF.h>
#include <Duck/RayTraceCpu/Materials/BxDFs/Microfacet.h>
#include <Duck/RayTraceCpu/Materials/Optics.h>

namespace Duck::RayTraceCpu
{
    struct ConductorBxDF
    {
        Spectrum F0{1.0f};
        GgxDistribution Distribution;

        Spectrum Fresnel(float cosine) const { return SchlickReflectance(F0, cosine); }

        BxDFFlags Flags() const
        {
            return Distribution.EffectivelySmooth() ? BxDFFlags::SpecularReflection : BxDFFlags::GlossyReflection;
        }

        Spectrum Evaluate(const Vec3& wo, const Vec3& wi, TransportMode) const
        {
            if (Distribution.EffectivelySmooth() || !SameHemisphere(wo, wi))
            {
                return Spectrum(0.0f);
            }
            const float cosO = AbsCosTheta(wo);
            const float cosI = AbsCosTheta(wi);
            Vec3 wm = wi + wo;
            if (cosO == 0.0f || cosI == 0.0f || glm::dot(wm, wm) == 0.0f)
            {
                return Spectrum(0.0f);
            }
            wm = glm::normalize(wm);
            // only the microfacets that mirror wo into wi reflect: D picks them, G hides the shadowed ones
            return Distribution.D(wm) * Fresnel(std::abs(glm::dot(wo, wm))) * Distribution.G(wo, wi) /
                   (4.0f * cosO * cosI);
        }

        std::optional<BxDFSample> Sample(const Vec3& wo, float, const Vec2& u, TransportMode mode) const
        {
            if (Distribution.EffectivelySmooth())
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

            if (wo.z == 0.0f)
            {
                return std::nullopt;
            }
            const Vec3 wm = Distribution.SampleVisibleNormal(wo, u);
            const Vec3 wi = ReflectAbout(wo, wm);
            if (!SameHemisphere(wo, wi))
            {
                return std::nullopt; // the mirror direction points into the surface: this sample is lost
            }
            return BxDFSample{
                .F = Evaluate(wo, wi, mode), .Wi = wi, .Pdf = Pdf(wo, wi, mode), .Flags = BxDFFlags::GlossyReflection};
        }

        float Pdf(const Vec3& wo, const Vec3& wi, TransportMode) const
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
            // the visible normal density, changed from normals to reflected directions
            return Distribution.VisibleD(wo, wm) / (4.0f * std::abs(glm::dot(wo, wm)));
        }

        Spectrum DiffuseAlbedo(const Vec3&) const { return Spectrum(0.0f); }
    };
}