#pragma once

#include <Duck/Core/Math.h>
#include <Duck/RayTraceCpu/Geometry.h>
#include <Duck/RayTraceCpu/Sampler.h>

#include <cmath>
#include <concepts>
#include <type_traits>
#include <variant>

namespace Duck::RayTraceCpu
{
    // one sample of the light reaching a point from one light
    struct LightSample
    {
        Vec3 Direction{0.0f};      // wi: unit, from the shaded point toward the light
        float Distance = Infinity; // to the sampled point on the light. shadow rays stop just before it
        Vec3 Radiance{0.0f};       // arriving along Direction. for delta lights, the value that goes with Pdf = 1
        float Pdf = 0.0f;          // of having picked Direction, per solid angle. 1 for delta lights
        bool IsDelta = true;       // a single point or direction: can be sampled, but no ray can ever hit it
    };

    // infinitely far away
    struct DirectionalLight
    {
        Vec3 Direction{0.0f, 1.0f, 0.0f}; // unit, points TOWARD the light
        Vec3 Irradiance{Pi}; // W/m^2 on a surface facing the light. Pi lights a white surface to radiance 1

        LightSample Sample(const Vec3&, ISampler&) const
        {
            return {Direction, Infinity, Irradiance, 1.0f, true};
        }
    };

    // light from one point, falls off with 1/distance^2, inverse square law
    struct PointLight
    {
        Vec3 Position{0.0f};
        Vec3 Intensity{1.0f}; // W/sr

        LightSample Sample(const Vec3& point, ISampler&) const
        {
            const Vec3 toLight = Position - point;
            const float distanceSquared = glm::dot(toLight, toLight);
            const float distance = std::sqrt(distanceSquared);
            return {toLight / distance, distance, Intensity / distanceSquared, 1.0f, true}; // inverse-square law
        }
    };

    // light given off of a shapes surface
    // every point on its front side emits Radiance equally in every direction
    struct AreaLight
    {
        Shape Surface;
        Vec3 Radiance{0.0f}; // W/(m^2 sr)
        int PrimitiveId = -1;

        LightSample Sample(const Vec3& point, ISampler& sampler) const
        {
            const SurfaceSample surface = SampleShapeSurface(Surface, sampler.Get2D());

            const Vec3 toLight = surface.Point - point;
            const float distanceSquared = glm::dot(toLight, toLight);
            if (distanceSquared <= 0.0f)
            {
                return {};
            }

            const float distance = std::sqrt(distanceSquared);
            const Vec3 direction = toLight / distance;

            const float cosLight = -glm::dot(surface.Normal, direction);
            if (cosLight <= 0.0f)
            {
                return {}; // back of the light or edge on
            }

            const float pdf = distanceSquared / (cosLight * ShapeArea(Surface));
            return {direction, distance, Radiance, pdf, false};
        }
    };

    // what every light type must provide
    template <typename T>
    concept LightType = requires(const T& light, const Vec3& point, ISampler& sampler) {
        { light.Sample(point, sampler) } -> std::same_as<LightSample>;
    };

    // every light type. a variant keeps them in one list which NNE needs to sample randomly
    using Light = std::variant<DirectionalLight, PointLight, AreaLight>;

    template <typename TVariant>
    struct AllLightTypes;
    template <typename... TLights>
    struct AllLightTypes<std::variant<TLights...>> : std::bool_constant<(LightType<TLights> && ...)>
    {
    };
    static_assert(AllLightTypes<Light>::value, "Every type in Light must statify LightType");

    inline LightSample SampleLight(const Light& light, const Vec3& point, ISampler& sampler)
    {
        return std::visit([&](const auto& typed) { return typed.Sample(point, sampler); }, light);
    }
}