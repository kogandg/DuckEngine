#pragma once


#include <concepts>
#include <optional>
#include <type_traits>
#include <variant>
#include <array>

// lobes work in local frame where shading normal is +z and wo is on the +z side
// wo points towords the viewr, wi toward where the light comes from
// a delta lobe returns value = weight / |cos(wi)| from Sample, so Value * |cos| / pdf = weight
namespace Duck::RayTraceCpu
{
    struct LobeSample
    {
        Vec3 Wi{0.0f};
        Vec3 Value{0.0f}; // f(wo, wi)
        float Pdf = 0.0f;
        bool IsDelta = false; // one exact direction, cant be evaluedted for an arbitrary wi
    };

    template <typename T>
    concept LobeType = requires(const T& lobe, const Vec3& wo, const Vec3& wi, ISampler& sampler) {
        { lobe.Evaluate(wo, wi) } -> std::same_as<Vec3>; // 0 for delta lobes
        { lobe.Sample(wo, sampler) } -> std::same_as<std::optional<LobeSample>>;
        { lobe.SampleWeight(wo) } -> std::same_as<float>;     // rough share of the light
        { lobe.IsDelta() } -> std::same_as<bool>;             // only reachable by sampling,
        { lobe.Pdf(wo, wi) } -> std::same_as<float>;          // of Sample returning wi, 0 for delta lobes
    };

    template <typename TVariant>
    struct AllLobeTypes;
    template <typename... TLobes>
    struct AllLobeTypes<std::variant<TLobes...>> : std::bool_constant<(LobeType<TLobes> && ...)>
    {
    };
}