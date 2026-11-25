#include "TestHelpers.h"

#include <Duck/RayTraceCpu/Materials/Bsdf.h>
#include <Duck/RayTraceCpu/Materials/Materials.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace Duck;
using namespace Duck::RayTraceCpu;
using namespace Duck::Test;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{
    // light carried by one sample : f* | cos | / pdf, with the eta ^ 2 refraction squeezes radiance by put back,
    // so anything that conserves energy carries exactly 1
    float Throughput(const BsdfSample& sample, const Vec3& normal)
    {
        const float etaScale = sample.IsTransmission() ? sample.Eta * sample.Eta : 1.0f;
        return (sample.F * std::abs(glm::dot(sample.Wi, normal)) / sample.Pdf).x * etaScale;
    }

    // the BxDF divides by its local cos, the check multiplies by the world-space cos. both are accurate to
    // ~1e-7, so the relative error is ~1e-7 / cos and grows near grazing
    float GrazingTolerance(float cosine) { return std::max(1e-4f, 1e-6f / cosine); }

    Vec3 RandomDirectionAbove(const Vec3& normal, Random& random)
    {
        const Vec3 w = RandomUnitVector(random);
        return glm::dot(w, normal) < 0.0f ? -w : w;
    }
}

TEST_CASE("White BxDFs keep all the light", "[bsdf][furnace]")
{
    const BxDF bxdf = GENERATE(BxDF{DiffuseBxDF{Vec3(1.0f)}}, BxDF{ConductorBxDF{.F0 = Vec3(1.0f)}},
                               BxDF{DielectricBxDF{.Eta = 1.5f}},        // entering glass
                               BxDF{DielectricBxDF{.Eta = 1.0f / 1.5f}}, // leaving, with total internal reflection
                               BxDF{CoatedDiffuseBxDF{.Reflectance = Vec3(1.0f), .CoatF0 = 0.04f}});
    Random random(41u);
    IndependentSampler sampler(41u);
    for (int i = 0; i < 1000; i++)
    {
        const Vec3 normal = RandomUnitVector(random);
        const Bsdf bsdf(normal, bxdf);
        const Vec3 wo = RandomDirectionAbove(normal, random);
        const auto sample = bsdf.Sample(wo, sampler.Get1D(), sampler.Get2D());
        REQUIRE(sample.has_value());

        const float cosine = std::abs(glm::dot(sample->Wi, normal));
        REQUIRE_THAT(Throughput(*sample, normal), WithinAbs(1.0f, GrazingTolerance(cosine)));
    }
}

TEST_CASE("BxDF Pdf and Evaluate match what Sample returns", "[bsdf][mis]")
{
    // MIS weighs a light sample by Pdf(wo, wi) and a BSDF sample by the pdf Sample reported, so the two must agree
    const BxDF bxdf = GENERATE(BxDF{DiffuseBxDF{Vec3(0.7f, 0.5f, 0.3f)}},
                               BxDF{CoatedDiffuseBxDF{.Reflectance = Vec3(0.7f, 0.5f, 0.3f), .CoatF0 = 0.04f}},
                               BxDF{CoatedDiffuseBxDF{.Reflectance = Vec3(0.2f), .CoatF0 = 0.1f, .CoatWeight = 0.5f}});
    Random random(7u);
    IndependentSampler sampler(7u);
    int nonSpecular = 0;
    for (int i = 0; i < 1000; i++)
    {
        const Vec3 normal = RandomUnitVector(random);
        const Bsdf bsdf(normal, bxdf);
        const Vec3 wo = RandomDirectionAbove(normal, random);
        const auto sample = bsdf.Sample(wo, sampler.Get1D(), sampler.Get2D());
        REQUIRE(sample.has_value());
        if (sample->IsSpecular())
        {
            continue; // a delta: Pdf and Evaluate only describe the rest
        }
        nonSpecular++;
        CHECK_THAT(bsdf.Pdf(wo, sample->Wi), WithinRel(sample->Pdf, 1e-4f));
        CHECK_THAT(bsdf.Evaluate(wo, sample->Wi), Vec3Near(sample->F, 1e-6f));
    }
    CHECK(nonSpecular > 500);
}

TEST_CASE("Refraction squeezes radiance by eta squared, but not importance", "[bsdf][glass]")
{
    const Vec3 normal(0.0f, 0.0f, 1.0f);
    const Vec3 wo = glm::normalize(Vec3(0.3f, 0.0f, 1.0f));
    const float eta = 1.5f;
    const Bsdf bsdf(normal, DielectricBxDF{.Eta = eta});
    const float reflectance = FrDielectric(wo.z, eta);

    // uc above the reflectance always refracts
    const auto radiance = bsdf.Sample(wo, 0.999f, Vec2(0.5f), TransportMode::Radiance);
    const auto importance = bsdf.Sample(wo, 0.999f, Vec2(0.5f), TransportMode::Importance);
    REQUIRE(radiance.has_value());
    REQUIRE(importance.has_value());
    CHECK(radiance->IsTransmission());
    CHECK(radiance->Eta == eta);
    const auto carried = [&](const BsdfSample& sample)
    { return (sample.F * std::abs(glm::dot(sample.Wi, normal)) / sample.Pdf).x; };
    CHECK_THAT(carried(*importance), WithinRel(1.0f, 1e-5f)); // (1 - R) / (1 - R)
    CHECK_THAT(carried(*radiance), WithinRel(1.0f / (eta * eta), 1e-5f));
    CHECK_THAT(radiance->Pdf, WithinRel(1.0f - reflectance, 1e-5f));
}

TEST_CASE("White glTF materials keep all the light", "[bsdf][material]")
{
    // with a white base, every BxDF a material can pick carries exactly 1 per sample
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

        for (int i = 0; i < 200; i++)
        {
            const Bsdf bsdf = material.GetBsdf(hit, sampler); // picks metal, glass or coated plastic
            const auto sample = bsdf.Sample(wo, sampler.Get1D(), sampler.Get2D());
            CAPTURE(material.Metallic, material.Transmission, material.Ior, material.Specular);
            REQUIRE(sample.has_value());
            REQUIRE_THAT(Throughput(*sample, hit.Normal), WithinAbs(1.0f, 1e-4f));
        }
    }
}