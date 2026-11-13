#include "TestHelpers.h"

#include <Duck/RayTraceCpu/Bsdf.h>
#include <Duck/RayTraceCpu/Materials.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace Duck;
using namespace Duck::RayTraceCpu;
using namespace Duck::Test;
using Catch::Matchers::WithinAbs;

namespace
{
    // light carried by one sample: f * |cos| / pdf
    float Throughput(const BsdfSample& sample, const Vec3& normal)
    {
        return (sample.Value * std::abs(glm::dot(sample.Direction, normal)) / sample.Pdf).x;
    }

    // the lobe divides by its local cos, the check multiplies by the world-space cos. both are accurate to
    // ~1e-7, so the relative error is ~1e-7 / cos and grows near grazing
    float GrazingTolerance(float cosine) { return std::max(1e-4f, 1e-6f / cosine); }


    Vec3 RandomDirectionAbove(const Vec3& normal, Random& random)
    {
        const Vec3 w = RandomUnitVector(random);
        return glm::dot(w, normal) < 0.0f ? -w : w;
    }
}

TEST_CASE("White lobes keep all the light", "[bsdf][furnace]")
{
    const Lobe lobe =
        GENERATE(Lobe{LambertLobe{Vec3(1.0f)}}, Lobe{SpecularReflectionLobe{.F0 = Vec3(1.0f), .Scale = 1.0f}},
                 Lobe{SpecularDielectricLobe{.Eta = 1.5f}},         // entering glass
                 Lobe{SpecularDielectricLobe{.Eta = 1.0f / 1.5f}}); // leaving, with total internal reflection
    Random random(41u);
    IndependentSampler sampler(41u);
    for (int i = 0; i < 1000; i++)
    {
        const Vec3 normal = RandomUnitVector(random);
        Bsdf bsdf(normal);
        bsdf.Add(lobe);
        const Vec3 wo = RandomDirectionAbove(normal, random);
        const auto sample = bsdf.Sample(wo, sampler);

        const float cosine = std::abs(glm::dot(sample->Direction, normal));
        const float throughput = Throughput(*sample, normal);

        REQUIRE(sample.has_value());
        REQUIRE_THAT(throughput, WithinAbs(1.0f, GrazingTolerance(cosine)));
    }
}

TEST_CASE("White glTF materials keep all the light", "[bsdf][material]")
{
    // with a white base, metal + coat + diffuse + glass add up to exactly 1, and each sample carries exactly 1
    Random random(61u);
    IndependentSampler sampler(61u);
    for (int m = 0; m < 50; m++)
    {
        const GltfMaterial material{
            .BaseColor = Vec3(1.0f),
            .Metallic = random.NextFloat(),
            .Roughness = 0.0f,
            .Transmission = random.NextFloat(),
            .Ior = 1.0f + 2.0f * random.NextFloat(),
            .Specular = random.NextFloat(),
        };
        Hit hit;
        hit.Normal = RandomUnitVector(random);
        const Vec3 wo = RandomDirectionAbove(hit.Normal, random);
        const Bsdf bsdf = material.GetBsdf(hit, wo);

        for (int i = 0; i < 200; i++)
        {
            const auto sample = bsdf.Sample(wo, sampler);
            CAPTURE(material.Metallic, material.Transmission, material.Ior, material.Specular);
            REQUIRE(sample.has_value());
            REQUIRE_THAT(Throughput(*sample, hit.Normal), WithinAbs(1.0f, 1e-4f));
        }
    }
}

TEST_CASE("Absorption leaves AttenuationColor after AttenuationDistance", "[material][absorption]")
{
    const GltfMaterial glass = GltfMaterial::ColoredGlass(Vec3(0.8f, 0.5f, 0.2f), 2.0f);
    CHECK_THAT(glass.InteriorTransmittance(0.0f), Vec3Near(Vec3(1.0f), 1e-6f));
    CHECK_THAT(glass.InteriorTransmittance(2.0f), Vec3Near(Vec3(0.8f, 0.5f, 0.2f), 1e-6f));
    CHECK_THAT(glass.InteriorTransmittance(4.0f), Vec3Near(Vec3(0.64f, 0.25f, 0.04f), 1e-6f)); // twice as far: squared
    CHECK_THAT(GltfMaterial::Glass().InteriorTransmittance(100.0f), Vec3Near(Vec3(1.0f), 0.0f)); // clear
}