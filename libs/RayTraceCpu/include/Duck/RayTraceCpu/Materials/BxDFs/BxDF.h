#pragma once

#include <Duck/Core/Math.h>
#include <Duck/RayTraceCpu/Spectrum.h>

#include <cmath>
#include <concepts>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <variant>

// local shading frame where shading normal is +z
// wo points toward the viewer, wi toward where the light comes from, both point away from the surface
namespace Duck::RayTraceCpu
{
    enum class BxDFFlags : uint8_t
    {
        Unset = 0,
        Reflection = 1 << 0,
        Transmission = 1 << 1,
        Diffuse = 1 << 2,
        Glossy = 1 << 3,
        Specular = 1 << 4, // perfectly smooth, delta
        DiffuseReflection = Diffuse | Reflection,
        DiffuseTransmission = Diffuse | Transmission,
        GlossyReflection = Glossy | Reflection,
        GlossyTransmission = Glossy | Transmission,
        SpecularReflection = Specular | Reflection,
        SpecularTransmission = Specular | Transmission,
        All = Diffuse | Glossy | Specular | Reflection | Transmission,
    };

    constexpr BxDFFlags operator|(BxDFFlags a, BxDFFlags b)
    {
        return static_cast<BxDFFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }
    constexpr bool HasAny(BxDFFlags flags, BxDFFlags mask)
    {
        return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(mask)) != 0;
    }
    constexpr bool IsReflective(BxDFFlags flags) { return HasAny(flags, BxDFFlags::Reflection); }
    constexpr bool IsTransmissive(BxDFFlags flags) { return HasAny(flags, BxDFFlags::Transmission); }
    constexpr bool IsDiffuse(BxDFFlags flags) { return HasAny(flags, BxDFFlags::Diffuse); }
    constexpr bool IsGlossy(BxDFFlags flags) { return HasAny(flags, BxDFFlags::Glossy); }
    constexpr bool IsSpecular(BxDFFlags flags) { return HasAny(flags, BxDFFlags::Specular); }
    // has a part light sampling can reach: something to evaluate for a lights direction
    constexpr bool IsNonSpecular(BxDFFlags flags) { return HasAny(flags, BxDFFlags::Diffuse | BxDFFlags::Glossy); }

    // which way the path carries light, camera paths carry radiance
    // only refraction cares, radiance is squeezed into a smaller solid angle by eta^2
    enum class TransportMode : uint8_t
    {
        Radiance,
        Importance,
    };

    struct BxDFSample
    {
        Spectrum F{0.0f}; // f(wo, wi). for a specular sample: weight / |cos(wi)|
        Vec3 Wi{0.0f};
        float Pdf = 0.0f; // per solid angle, or for a specular sample the chance of that branch
        BxDFFlags Flags = BxDFFlags::Unset;
        float Eta = 1.0f; // relative index of refraction crossed. 1 unless the sample was transmitted

        bool IsReflection() const { return IsReflective(Flags); }
        bool IsTransmission() const { return IsTransmissive(Flags); }
        bool IsSpecular() const { return RayTraceCpu::IsSpecular(Flags); }
    };

    inline float CosTheta(const Vec3& w) { return w.z; }
    inline float AbsCosTheta(const Vec3& w) { return std::abs(w.z); }
    inline bool SameHemisphere(const Vec3& a, const Vec3& b) { return a.z * b.z > 0.0f; }
    inline Vec3 MirrorLocal(const Vec3& wo) { return {-wo.x, -wo.y, wo.z}; }

    template <typename T>
    concept BxDFType =
        requires(const T& bxdf, const Vec3& wo, const Vec3& wi, float uc, const Vec2& u, TransportMode mode) {
            { bxdf.Flags() } -> std::same_as<BxDFFlags>;
            { bxdf.Evaluate(wo, wi, mode) } -> std::same_as<Spectrum>; // 0 for specular parts
            { bxdf.Sample(wo, uc, u, mode) } -> std::same_as<std::optional<BxDFSample>>;
            { bxdf.Pdf(wo, wi, mode) } -> std::same_as<float>; // of Sample returning wi. 0 for specular parts
            // for Whitted's ambient term
            { bxdf.DiffuseAlbedo(wo) } -> std::same_as<Spectrum>;
        };
}