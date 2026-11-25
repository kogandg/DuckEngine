#pragma once

#include <Duck/Core/Math.h>
#include <Duck/Core/Sampling.h>
#include <Duck/RayTraceCpu/Materials/BxDFs/BxDF.h>
#include <Duck/RayTraceCpu/Materials/BxDFs/CoatedDiffuseBxDF.h>
#include <Duck/RayTraceCpu/Materials/BxDFs/ConductorBxDF.h>
#include <Duck/RayTraceCpu/Materials/BxDFs/DielectricBxDF.h>
#include <Duck/RayTraceCpu/Materials/BxDFs/DiffuseBxDF.h>

#include <optional>
#include <variant>

namespace Duck::RayTraceCpu
{
    using BxDF = std::variant<DiffuseBxDF, ConductorBxDF, DielectricBxDF, CoatedDiffuseBxDF>;

    template <typename TVariant>
    struct AllBxDFTypes;
    template <typename... TBxDFs>
    struct AllBxDFTypes<std::variant<TBxDFs...>> : std::bool_constant<(BxDFType<TBxDFs> && ...)>
    {
    };
    static_assert(AllBxDFTypes<BxDF>::value, "Every type in BxDF must satisfy BxDFType");

    struct ShadingFrame
    {
        Vec3 Tangent{0.0f};
        Vec3 Bitangent{0.0f};
        Vec3 Normal{0.0f};

        explicit ShadingFrame(const Vec3& normal) : Normal(normal) { OrthonormalBasis(normal, Tangent, Bitangent); }

        Vec3 ToLocal(const Vec3& v) const
        {
            return {glm::dot(v, Tangent), glm::dot(v, Bitangent), glm::dot(v, Normal)};
        }
        Vec3 ToWorld(const Vec3& v) const { return v.x * Tangent + v.y * Bitangent + v.z * Normal; }
    };

    // a BxDF sample with its direction in world space
    struct BsdfSample
    {
        Spectrum F{0.0f}; // f(wo, wi). for a specular sample: weight / |cos(wi)|
        Vec3 Wi{0.0f};
        float Pdf = 0.0f;
        BxDFFlags Flags = BxDFFlags::Unset;
        float Eta = 1.0f;

        bool IsReflection() const { return IsReflective(Flags); }
        bool IsTransmission() const { return IsTransmissive(Flags); }
        bool IsSpecular() const { return RayTraceCpu::IsSpecular(Flags); }
    };

    // one BxDF in a shading frame, takes world space directions
    class Bsdf
    {
    public:
        Bsdf(const Vec3& normal, const BxDF& scattering) : frame(normal), bxdf(scattering) {}

        BxDFFlags Flags() const
        {
            return std::visit([](const auto& typed) { return typed.Flags(); }, bxdf);
        }

        Spectrum Evaluate(const Vec3& woWorld, const Vec3& wiWorld, TransportMode mode = TransportMode::Radiance) const
        {
            const Vec3 wo = frame.ToLocal(woWorld);
            if (wo.z == 0.0f)
            {
                return Spectrum(0.0f);
            }
            const Vec3 wi = frame.ToLocal(wiWorld);
            return std::visit([&](const auto& typed) { return typed.Evaluate(wo, wi, mode); }, bxdf);
        }

        // uc picks between branches, u picks a direction
        std::optional<BsdfSample> Sample(const Vec3& woWorld, float uc, const Vec2& u,
                                         TransportMode mode = TransportMode::Radiance) const
        {
            const Vec3 wo = frame.ToLocal(woWorld);
            if (wo.z == 0.0f || Flags() == BxDFFlags::Unset)
            {
                return std::nullopt;
            }
            const auto sample = std::visit([&](const auto& typed) { return typed.Sample(wo, uc, u, mode); }, bxdf);
            if (!sample || sample->Pdf <= 0.0f || MaxComponent(sample->F) <= 0.0f || sample->Wi.z == 0.0f)
            {
                return std::nullopt;
            }
            return BsdfSample{.F = sample->F,
                              .Wi = frame.ToWorld(sample->Wi),
                              .Pdf = sample->Pdf,
                              .Flags = sample->Flags,
                              .Eta = sample->Eta};
        }

        // the density Sample has of returning wiWorld. 0 for specular parts. light sampling needs this for MIS
        float Pdf(const Vec3& woWorld, const Vec3& wiWorld, TransportMode mode = TransportMode::Radiance) const
        {
            const Vec3 wo = frame.ToLocal(woWorld);
            if (wo.z == 0.0f)
            {
                return 0.0f;
            }
            const Vec3 wi = frame.ToLocal(wiWorld);
            return std::visit([&](const auto& typed) { return typed.Pdf(wo, wi, mode); }, bxdf);
        }

        // reflectance of the diffuse part, for Whitteds ambient term
        Spectrum DiffuseAlbedo(const Vec3& woWorld) const
        {
            const Vec3 wo = frame.ToLocal(woWorld);
            return std::visit([&](const auto& typed) { return typed.DiffuseAlbedo(wo); }, bxdf);
        }

    private:
        ShadingFrame frame;
        BxDF bxdf;
    };
}