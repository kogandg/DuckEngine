#pragma once

#include <Duck/Core/Assert.h>
#include <Duck/Core/Math.h>
#include <Duck/Core/Random.h>
#include <Duck/Core/Sampling.h>
#include <Duck/RayTraceCpu/Optics.h>

#include <array>
#include <cmath>
#include <concepts>
#include <optional>
#include <type_traits>
#include <variant>

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

    // direction a perfectly smooth version of a lobe sends light
    struct SpecularPath
    {
        Vec3 Direction{0.0f};
        Vec3 Weight{0.0f};      // fraction of light going this way
        float Roughness = 0.0f; // of the surface
    };

    struct SpecularPaths
    {
        std::array<SpecularPath, 2> Paths{};
        int Count = 0;
        void Add(const SpecularPath& path)
        {
            Paths[static_cast<size_t>(Count)] = path;
            Count++;
        }
    };

    //---------------- lobes -------------

    // ideal diffuse reflection
    struct LambertLobe
    {
        Vec3 Reflectance{0.0f};

        Vec3 Evaluate(const Vec3&, const Vec3& wi) const { return wi.z > 0.0f ? Reflectance * InvPi : Vec3(0.0f); }

        std::optional<LobeSample> Sample(const Vec3& wo, Random& random) const
        {
            const float u1 = random.NextFloat();
            const float u2 = random.NextFloat();
            const Vec3 wi = CosineHemisphereLocal(u1, u2);
            if (wi.z <= 0.0f)
            {
                return std::nullopt;
            }
            return LobeSample{.Wi = wi, .Value = Evaluate(wo, wi), .Pdf = wi.z * InvPi, .IsDelta = false};
        }

        float SampleWeight(const Vec3&) const { return MaxComponent(Reflectance); }
        SpecularPaths Specular(const Vec3&) const { return {}; }

        bool IsDelta() const { return false; }
    };

    // perfect mirror reflection with Schlick Fresnel: metal (F0 = base color) and the clear coat of dielectrics
    // (F0 from ior)
    struct SpecularReflectionLobe
    {
        Vec3 F0{1.0f};
        float Scale = 1.0f; // weight in the mix
        float Roughness = 0.0f;

        Vec3 Fresnel(float cosine) const { return Scale * SchlickReflectance(F0, cosine); }

        Vec3 Evaluate(const Vec3&, const Vec3&) const { return Vec3(0.0f); }

        std::optional<LobeSample> Sample(const Vec3& wo, Random&) const
        {
            if (wo.z <= 0.0f)
            {
                return std::nullopt;
            }
            const Vec3 wi(-wo.x, -wo.y, wo.z);
            return LobeSample{.Wi = wi, .Value = Fresnel(wo.z) / wi.z, .Pdf = 1.0f, .IsDelta = true};
        }

        float SampleWeight(const Vec3& wo) const { return MaxComponent(Fresnel(wo.z)); }

        SpecularPaths Specular(const Vec3& wo) const
        {
            SpecularPaths paths;
            paths.Add({Vec3(-wo.x, -wo.y, wo.z), Fresnel(wo.z), Roughness});
            return paths;
        }

        bool IsDelta() const { return true; }
    };

    // smooth glass surface: Fresnel-weighted reflection and refraction
    // Eta = index on the far side / index on wo's side, so it already knows whether the ray enters or leaves
    struct SpecularDielectricLobe
    {
        float Eta = 1.5f;
        Vec3 Tint{1.0f};    // refracted light is multiplied by this
        float Scale = 1.0f; // weight in the mix

        struct Split
        {
            Vec3 Reflected{0.0f};
            std::optional<Vec3> Refracted; // nullopt on total internal reflection
            float Reflectance = 1.0f;
        };

        Split SplitAt(const Vec3& wo) const
        {
            Split split;
            split.Reflected = Vec3(-wo.x, -wo.y, wo.z);
            split.Refracted = Refract(-wo, Vec3(0.0f, 0.0f, 1.0f), 1.0f / Eta);
            if (split.Refracted)
            {
                // Schlick uses the angle on the lower-index side: wo's when entering denser glass,
                // the refracted one when leaving
                const float cosine = Eta > 1.0f ? wo.z : -split.Refracted->z;
                split.Reflectance = SchlickFresnel(cosine, 1.0f, Eta);
            }
            return split;
        }

        Vec3 Evaluate(const Vec3&, const Vec3&) const { return Vec3(0.0f); }

        std::optional<LobeSample> Sample(const Vec3& wo, Random& random) const
        {
            const Split split = SplitAt(wo);
            const float r = split.Reflectance;
            if (!split.Refracted || random.NextFloat() < r)
            {
                const Vec3 wi = split.Reflected;
                return LobeSample{wi, Vec3(Scale * r / std::abs(wi.z)), r, true};
            }
            const Vec3 wi = *split.Refracted;
            return LobeSample{
                .Wi = wi, .Value = Scale * (1.0f - r) * Tint / std::abs(wi.z), .Pdf = 1.0f - r, .IsDelta = true};
        }

        float SampleWeight(const Vec3&) const { return Scale; }

        SpecularPaths Specular(const Vec3& wo) const
        {
            const Split split = SplitAt(wo);
            SpecularPaths paths;
            paths.Add({split.Reflected, Vec3(Scale * split.Reflectance), 0.0f});
            if (split.Refracted)
            {
                paths.Add({*split.Refracted, Scale * (1.0f - split.Reflectance) * Tint, 0.0f});
            }
            return paths;
        }

        bool IsDelta() const { return true; }
    };

    // --------------- interface every lobe must provide ----------

    template <typename T>
    concept LobeType = requires(const T& lobe, const Vec3& wo, const Vec3& wi, Random& random) {
        { lobe.Evaluate(wo, wi) } -> std::same_as<Vec3>; // 0 for delta lobes
        { lobe.Sample(wo, random) } -> std::same_as<std::optional<LobeSample>>;
        { lobe.SampleWeight(wo) } -> std::same_as<float>;     // rough share of the light
        { lobe.Specular(wo) } -> std::same_as<SpecularPaths>; // for Whitted
        { lobe.IsDelta() } -> std::same_as<bool>;             // only reachable by sampling,
    };

    using Lobe = std::variant<LambertLobe, SpecularReflectionLobe, SpecularDielectricLobe>;

    template <typename TVariant>
    struct AllLobeTypes;
    template <typename... TLobes>
    struct AllLobeTypes<std::variant<TLobes...>> : std::bool_constant<(LobeType<TLobes> && ...)>
    {
    };
    static_assert(AllLobeTypes<Lobe>::value, "Every type in Lobe must satisfy LobeType");

    // --------------- the BSDF at one hit point --------

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

    struct BsdfSample
    {
        Vec3 Direction{0.0f}; // wi, world space
        Vec3 Value{0.0f};     // f(wo, wi), for delta samples: weight / |cos|
        float Pdf = 0.0f;
        bool IsDelta = false;
    };

    // a few weighted lobes in a shading frame
    class Bsdf
    {
    public:
        static constexpr int MaxLobes = 4;

        explicit Bsdf(const Vec3& normal) : frame(normal) {}

        void Add(const Lobe& lobe)
        {
            DUCK_ASSERT(count < MaxLobes, "Too many lobes in one BSDF");
            lobes[static_cast<size_t>(count)] = lobe;
            count++;
        }

        // sum of every lobe for light from wi (delta lobes add nothing)
        Vec3 Evaluate(const Vec3& woWorld, const Vec3& wiWorld) const
        {
            const Vec3 wo = frame.ToLocal(woWorld);
            const Vec3 wi = frame.ToLocal(wiWorld);
            Vec3 sum(0.0f);
            for (int i = 0; i < count; i++)
            {
                sum += std::visit([&](const auto& lobe) { return lobe.Evaluate(wo, wi); }, LobeAt(i));
            }
            return sum;
        }

        // picks one lobe in proportion to its weight and samples it.
        // exact while there's at most one non-delta lobe. once glossy lobes exist, a non-delta sample has
        // to add up f and pdf over every non-delta lobe
        std::optional<BsdfSample> Sample(const Vec3& woWorld, Random& random) const
        {
            const Vec3 wo = frame.ToLocal(woWorld);
            std::array<float, MaxLobes> weights{};
            float total = 0.0f;
            for (int i = 0; i < count; i++)
            {
                weights[static_cast<size_t>(i)] =
                    std::visit([&](const auto& lobe) { return lobe.SampleWeight(wo); }, LobeAt(i));
                total += weights[static_cast<size_t>(i)];
            }
            if (total <= 0.0f)
            {
                return std::nullopt;
            }

            float pick = random.NextFloat() * total;
            int chosen = count - 1;
            for (int i = 0; i < count; i++)
            {
                if (pick < weights[static_cast<size_t>(i)])
                {
                    chosen = i;
                    break;
                }
                pick -= weights[static_cast<size_t>(i)];
            }

            const auto sample = std::visit([&](const auto& lobe) { return lobe.Sample(wo, random); }, LobeAt(chosen));
            if (!sample || sample->Pdf <= 0.0f)
            {
                return std::nullopt;
            }
            // the lobe itself was picked with probability weight / total
            const float choice = weights[static_cast<size_t>(chosen)] / total;
            return BsdfSample{.Direction = frame.ToWorld(sample->Wi),
                              .Value = sample->Value,
                              .Pdf = sample->Pdf * choice,
                              .IsDelta = sample->IsDelta};
        }

        // every perfect reflection / refraction direction, for Whitted
        template <typename TFunc>
        void ForEachSpecularPath(const Vec3& woWorld, TFunc&& func) const
        {
            const Vec3 wo = frame.ToLocal(woWorld);
            for (int i = 0; i < count; i++)
            {
                const SpecularPaths paths = std::visit([&](const auto& lobe) { return lobe.Specular(wo); }, LobeAt(i));
                for (int p = 0; p < paths.Count; p++)
                {
                    SpecularPath path = paths.Paths[static_cast<size_t>(p)];
                    path.Direction = frame.ToWorld(path.Direction);
                    func(path);
                }
            }
        }

        // total Lambert reflectance, for Whitted's ambient term
        Vec3 DiffuseReflectance() const
        {
            Vec3 sum(0.0f);
            for (int i = 0; i < count; i++)
            {
                if (const auto* lambert = std::get_if<LambertLobe>(&LobeAt(i)))
                {
                    sum += lambert->Reflectance;
                }
            }
            return sum;
        }

        // false when every lobe is delta (smooth glass, perfect mirrors): light sampling can't contribute then
        bool HasNonDeltaLobes() const
        {
            for (int i = 0; i < count; i++)
            {
                if (!std::visit([](const auto& lobe) { return lobe.IsDelta(); }, LobeAt(i)))
                {
                    return true;
                }
            }
            return false;
        }

    private:
        const Lobe& LobeAt(int index) const { return lobes[static_cast<size_t>(index)]; }

        ShadingFrame frame;
        std::array<Lobe, MaxLobes> lobes{};
        int count = 0;
    };
}