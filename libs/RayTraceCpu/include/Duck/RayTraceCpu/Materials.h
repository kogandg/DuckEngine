#pragma once

#include <Duck/Core/Math.h>
#include <Duck/RayTraceCpu/Bsdf.h>
#include <Duck/RayTraceCpu/Geometry.h>
#include <Duck/RayTraceCpu/Optics.h>

#include <cmath>
#include <concepts>
#include <type_traits>
#include <variant>

namespace Duck::RayTraceCpu
{
    // glTF metallic-roughness material, plus KHR_materials_transmission, _ior, _specular, _emissive_strength, _volume
    // lobes:
    // metal (Metallic): mirror reflection, F0 = BaseColor
    // clear coat (opaque dielelectric part): mirror reflection, F = Specular * Schlick(F0 from Ior)
    // diffuse under the coat: Labert, BaseColor * (1 - coat F)
    // transmission: smooth glass tinted by base color and attenuated by attenuation
    // gltfs defaults are white, metallic 1, roughness 1
    struct GltfMaterial
    {
        Vec3 BaseColor{0.8f};
        float Metallic = 0.0f;
        float Roughness = 0.5f;    // perceptual: 0 = mirror smooth, 1 = fully rough
        float Transmission = 0.0f; // 0 = opaque, 1 = everything under the coat refracts (glass)
        float Ior = 1.5f;
        float Specular = 1.0f; // scales the clear coat. 0 = pure Lambert
        Vec3 Emission{0.0f};   // emitted radiance, W/(m^2 sr)

        // light is abosorbed so that after AttenuationDistance, only AttenuationColor is left
        Vec3 AttenuationColor{1.0f};
        float AttenuationDistance{Infinity};

        // stand-in for a base color texture until hits have UVs: checker on the XZ plane
        Vec3 CheckerColor{0.0f};
        float CheckerSize = 0.0f;

        Vec3 BaseColorAt(const Vec3& point) const
        {
            if (CheckerSize <= 0.0f)
            {
                return BaseColor;
            }
            const int cellX = static_cast<int>(std::floor(point.x / CheckerSize));
            const int cellZ = static_cast<int>(std::floor(point.z / CheckerSize));
            return ((cellX + cellZ) & 1) == 0 ? BaseColor : CheckerColor;
        }

        Bsdf GetBsdf(const Hit& hit, const Vec3& wo) const
        {
            Bsdf bsdf(hit.Normal);
            const Vec3 base = BaseColorAt(hit.Point);
            const float dielectric = 1.0f - Metallic;
            const float opaque = dielectric * (1.0f - Transmission);
            const float glass = dielectric * Transmission;
            const float f0 = F0FromIor(Ior);

            if (Metallic > 0.0f)
            {
                bsdf.Add(SpecularReflectionLobe{.F0 = base, .Scale = Metallic, .Roughness = Roughness});
            }
            if (opaque > 0.0f)
            {
                if (Specular > 0.0f)
                {
                    bsdf.Add(
                        SpecularReflectionLobe{.F0 = Vec3(f0), .Scale = opaque * Specular, .Roughness = Roughness});
                }
                // the coat reflected F, the diffuse layer under it gets the rest
                const float coat = Specular * SchlickReflectance(f0, glm::dot(hit.Normal, wo));
                bsdf.Add(LambertLobe{base * (opaque * (1.0f - coat))});
            }
            if (glass > 0.0f)
            {
                bsdf.Add(SpecularDielectricLobe{.Eta = hit.FrontFace ? Ior : 1.0f / Ior, .Tint = base, .Scale = glass});
            }
            return bsdf;
        }

        Vec3 EmissionAt(const Hit&) const { return Emission; }

        // fraction of light left after traveling distance through the inside.
        // sigma_a = -ln(AttenuationColor) / AttenuationDistance, so exp(-sigma_a * d) = color^(d / distance)
        Vec3 InteriorTransmittance(float distance) const
        {
            const bool transmissive = (1.0f - Metallic) * Transmission > 0.0f;
            if (!transmissive || std::isinf(AttenuationDistance))
            {
                return Vec3(1.0f);
            }
            return glm::pow(AttenuationColor, Vec3(distance / AttenuationDistance));
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
    };

    // ---------- interface every Material provides ---------

    template <typename T>
    concept MaterialType = requires(const T& material, const Hit& hit, const Vec3& wo, float distance) {
        { material.GetBsdf(hit, wo) } -> std::same_as<Bsdf>;
        { material.EmissionAt(hit) } -> std::same_as<Vec3>;
        { material.InteriorTransmittance(distance) } -> std::same_as<Vec3>;
    };

    using Material = std::variant<GltfMaterial>;

    template <typename TVariant>
    struct AllMaterialTypes;
    template <typename... TMaterials>
    struct AllMaterialTypes<std::variant<TMaterials...>> : std::bool_constant<(MaterialType<TMaterials> && ...)>
    {
    };
    static_assert(AllMaterialTypes<Material>::value, "Every type in Material must satisfy MaterialType");

    inline Bsdf GetBsdf(const Material& material, const Hit& hit, const Vec3& wo)
    {
        return std::visit([&](const auto& typed) { return typed.GetBsdf(hit, wo); }, material);
    }

    inline Vec3 EmissionAt(const Material& material, const Hit& hit)
    {
        return std::visit([&](const auto& typed) { return typed.EmissionAt(hit); }, material);
    }

    inline Vec3 InteriorTransmittance(const Material& material, float distance)
    {
        return std::visit([&](const auto& typed) { return typed.InteriorTransmittance(distance); }, material);
    }
}