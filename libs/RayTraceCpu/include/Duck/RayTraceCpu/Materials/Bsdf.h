#pragma once

#include <Duck/Core/Assert.h>
#include <Duck/Core/Math.h>
#include <Duck/Core/Sampling.h>
#include <Duck/RayTraceCpu/Materials/Optics.h>
#include <Duck/RayTraceCpu/Sampler.h>

#include <Duck/RayTraceCpu/Materials/Lobes/Dielectric.h>
#include <Duck/RayTraceCpu/Materials/Lobes/Lambert.h>

#include <array>
#include <cmath>

namespace Duck::RayTraceCpu
{
    // perfect mirror reflection with Schlick Fresnel: metal (F0 = base color) and the clear coat of dielectrics
    // (F0 from ior)
    struct SpecularReflectionLobe
    {
        Vec3 F0{1.0f};
        float Scale = 1.0f; // weight in the mix
        float Roughness = 0.0f;

        Vec3 Fresnel(float cosine) const { return Scale * SchlickReflectance(F0, cosine); }

        Vec3 Evaluate(const Vec3&, const Vec3&) const { return Vec3(0.0f); }

        std::optional<LobeSample> Sample(const Vec3& wo, ISampler&) const
        {
            if (wo.z <= 0.0f)
            {
                return std::nullopt;
            }
            const Vec3 wi(-wo.x, -wo.y, wo.z);
            return LobeSample{.Wi = wi, .Value = Fresnel(wo.z) / wi.z, .Pdf = 1.0f, .IsDelta = true};
        }

        float Pdf(const Vec3&, const Vec3&) const { return 0.0f; }

        float SampleWeight(const Vec3& wo) const { return MaxComponent(Fresnel(wo.z)); }

        bool IsDelta() const { return true; }
    };

    using Lobe = std::variant<LambertLobe, SpecularReflectionLobe, DielectricLobe>;
    static_assert(AllLobeTypes<Lobe>::value, "Every type in Lobe must satisfy LobeType");

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
        std::optional<BsdfSample> Sample(const Vec3& woWorld, ISampler& sampler) const
        {
            const Vec3 wo = frame.ToLocal(woWorld);
            std::array<float, MaxLobes> weights{};
            const float total = LobeWeights(wo, weights);
            if (total <= 0.0f)
            {
                return std::nullopt;
            }

            float pick = sampler.Get1D() * total;
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

            const auto sample = std::visit([&](const auto& lobe) { return lobe.Sample(wo, sampler); }, LobeAt(chosen));
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

        // the density Sample has of returning wiWorld, the same pdf it reports. delta lobes add nothing: no
        // other direction picked by chance can ever land exactly on theirs. light sampling needs this for MIS
        float Pdf(const Vec3& woWorld, const Vec3& wiWorld) const
        {
            const Vec3 wo = frame.ToLocal(woWorld);
            const Vec3 wi = frame.ToLocal(wiWorld);
            std::array<float, MaxLobes> weights{};
            const float total = LobeWeights(wo, weights);
            if (total <= 0.0f)
            {
                return 0.0f;
            }
            float pdf = 0.0f;
            for (int i = 0; i < count; i++)
            {
                pdf += weights[static_cast<size_t>(i)] / total *
                       std::visit([&](const auto& lobe) { return lobe.Pdf(wo, wi); }, LobeAt(i));
            }
            return pdf;
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

        // how likely Sample is to pick each lobe, unnormalized. returns the total
        float LobeWeights(const Vec3& wo, std::array<float, MaxLobes>& weights) const
        {
            float total = 0.0f;
            for (int i = 0; i < count; i++)
            {
                weights[static_cast<size_t>(i)] =
                    std::visit([&](const auto& lobe) { return lobe.SampleWeight(wo); }, LobeAt(i));
                total += weights[static_cast<size_t>(i)];
            }
            return total;
        }

        ShadingFrame frame;
        std::array<Lobe, MaxLobes> lobes{};
        int count = 0;
    };
}