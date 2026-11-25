#pragma once

#include <Duck/Core/Math.h>
#include <Duck/RayTraceCpu/Materials/Bsdf.h>
#include <Duck/RayTraceCpu/Materials/Optics.h>
#include <Duck/RayTraceCpu/Sampler.h>
#include <Duck/RayTraceCpu/Scene/Geometry.h>
#include <Duck/RayTraceCpu/Scene/Hit.h>

#include <cmath>
#include <concepts>
#include <type_traits>
#include <variant>

namespace Duck::RayTraceCpu
{
    // glTF metallic-roughness material, plus KHR_materials_transmission, _ior, _specular, _emissive_strength, _volume
    //
    // metal (chance Metallic): ConductorBxDF, F0 = BaseColor
    // glass (chance (1 - Metallic) * Transmission): DielectricBxDF tinted by BaseColor, absorbed by the attenuation
    // coated plastic (the rest): CoatedDiffuseBxDF, coat F = Specular * Schlick(F0 from Ior), BaseColor underneath
    struct GltfMaterial
    {
        Rgb BaseColor{0.8f};
        float Metallic = 0.0f;
        float Roughness = 0.5f;    // perceptual: 0 = mirror smooth, 1 = fully rough
        float Transmission = 0.0f; // 0 = opaque, 1 = everything under the coat refracts (glass)
        float Ior = 1.5f;
        float Specular = 1.0f; // scales the clear coat. 0 = pure Lambert
        Rgb Emission{0.0f};   // emitted radiance, W/(m^2 sr)

        // light is abosorbed so that after AttenuationDistance, only AttenuationColor is left
        Rgb AttenuationColor{1.0f};
        float AttenuationDistance{Infinity};

        // stand-in for a base color texture until hits have UVs: checker on the XZ plane
        Rgb CheckerColor{0.0f};
        float CheckerSize = 0.0f;

        Rgb BaseColorAt(const Vec3& point) const
        {
            if (CheckerSize <= 0.0f)
            {
                return BaseColor;
            }
            const int cellX = static_cast<int>(std::floor(point.x / CheckerSize));
            const int cellZ = static_cast<int>(std::floor(point.z / CheckerSize));
            return ((cellX + cellZ) & 1) == 0 ? BaseColor : CheckerColor;
        }

        Bsdf GetBsdf(const Hit& hit, ISampler& sampler) const
        {
            const Spectrum base = FromRgb(BaseColorAt(hit.Point));
            const GgxDistribution distribution{GgxDistribution::AlphaFromRoughness(Roughness)};
            if (Choose(Metallic, sampler))
            {
                return Bsdf(hit.Normal, ConductorBxDF{.F0 = base, .Distribution = distribution});
            }
            if (Choose(Transmission, sampler))
            {
                return Bsdf(hit.Normal, DielectricBxDF{.Eta = hit.FrontFace ? Ior : 1.0f / Ior,
                                                       .Tint = base,
                                                       .Distribution = distribution});
            }
            if (Specular <= 0.0f)
            {
                return Bsdf(hit.Normal, DiffuseBxDF{.Reflectance = base});
            }
            return Bsdf(hit.Normal, CoatedDiffuseBxDF{.Reflectance = base,
                                                      .CoatF0 = F0FromIor(Ior),
                                                      .CoatWeight = Specular,
                                                      .Distribution = distribution});
        }

        Spectrum EmissionAt(const Hit& hit) const { return hit.FrontFace ? Emission : Spectrum(0.0f); }

        Rgb UniformEmission() const { return Emission; }

        // fraction of light left after traveling distance through the inside.
        // sigma_a = -ln(AttenuationColor) / AttenuationDistance, so exp(-sigma_a * d) = color^(d / distance)
        Spectrum InteriorTransmittance(float distance) const
        {
            const bool transmissive = (1.0f - Metallic) * Transmission > 0.0f;
            if (!transmissive || std::isinf(AttenuationDistance))
            {
                return Vec3(1.0f);
            }
            return glm::pow(FromRgb(AttenuationColor), Spectrum(distance / AttenuationDistance));
        }

        // shorthands for common materials
        static GltfMaterial Lambert(const Vec3& color)
        {
            return {.BaseColor = color, .Roughness = 1.0f, .Specular = 0.0f};
        }
        static GltfMaterial Plastic(const Vec3& color, float roughness)
        {
            return {.BaseColor = color, .Roughness = roughness};
        }
        static GltfMaterial Metal(const Vec3& color, float roughness = 0.0f)
        {
            return {.BaseColor = color, .Metallic = 1.0f, .Roughness = roughness};
        }
        static GltfMaterial Glass(float ior = 1.5f, const Vec3& tint = Vec3(1.0f))
        {
            return {.BaseColor = tint, .Roughness = 0.0f, .Transmission = 1.0f, .Ior = ior};
        }
        static GltfMaterial Emissive(const Vec3& radiance)
        {
            return {.BaseColor = Vec3(0.0f), .Roughness = 1.0f, .Specular = 0.0f, .Emission = radiance};
        }
        // clear surface, colored by absorption: after 'distance' inside, 'color' is left
        static GltfMaterial ColoredGlass(const Vec3& color, float distance, float ior = 1.5f)
        {
            return {.BaseColor = Vec3(1.0f),
                    .Roughness = 0.0f,
                    .Transmission = 1.0f,
                    .Ior = ior,
                    .AttenuationColor = color,
                    .AttenuationDistance = distance};
        }

        // true with probability chance. 0 and 1 need no random number
        static bool Choose(float chance, ISampler& sampler)
        {
            return chance >= 1.0f || (chance > 0.0f && sampler.Get1D() < chance);
        }
    };

    // ---------- interface every Material provides ---------

    template <typename T>
    concept MaterialType = requires(const T& material, const Hit& hit, ISampler& sampler, float distance) {
        { material.GetBsdf(hit, sampler) } -> std::same_as<Bsdf>;
        { material.EmissionAt(hit) } -> std::same_as<Spectrum>;
        { material.UniformEmission() } -> std::same_as<Rgb>;
        { material.InteriorTransmittance(distance) } -> std::same_as<Spectrum>;
    };

    using Material = std::variant<GltfMaterial>;

    template <typename TVariant>
    struct AllMaterialTypes;
    template <typename... TMaterials>
    struct AllMaterialTypes<std::variant<TMaterials...>> : std::bool_constant<(MaterialType<TMaterials> && ...)>
    {
    };
    static_assert(AllMaterialTypes<Material>::value, "Every type in Material must satisfy MaterialType");

    inline Bsdf GetBsdf(const Material& material, const Hit& hit, ISampler& sampler)
    {
        return std::visit([&](const auto& typed) { return typed.GetBsdf(hit, sampler); }, material);
    }

    inline Spectrum EmissionAt(const Material& material, const Hit& hit)
    {
        return std::visit([&](const auto& typed) { return typed.EmissionAt(hit); }, material);
    }

    inline Rgb UniformEmission(const Material& material)
    {
        return std::visit([](const auto& typed) { return typed.UniformEmission(); }, material);
    }

    inline Spectrum InteriorTransmittance(const Material& material, float distance)
    {
        return std::visit([&](const auto& typed) { return typed.InteriorTransmittance(distance); }, material);
    }

    // light coming back along the ray from hit. if the ray hit this surface from the inside, it crossed the
    // objects interior to get there and was partly absorbed on the way. (no nested media)
    inline Spectrum SegmentTransmittance(const Material& material, const Hit& hit)
    {
        return hit.FrontFace ? Spectrum(1.0f) : InteriorTransmittance(material, hit.T);
    }
}