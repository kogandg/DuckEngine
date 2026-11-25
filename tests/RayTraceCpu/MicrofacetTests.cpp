#include "TestHelpers.h"

#include <Duck/RayTraceCpu/Materials/Bsdf.h>
#include <Duck/RayTraceCpu/Materials/BxDFs/Microfacet.h>
#include <Duck/RayTraceCpu/Sampler.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

using namespace Duck;
using namespace Duck::RayTraceCpu;
using namespace Duck::Test;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{
    Vec3 RandomDirectionAbove(Random& random)
    {
        const Vec3 w = RandomUnitVector(random);
        return w.z < 0.0f ? -w : w;
    }

    // a direction above the surface, but not so close to grazing that float error dominates
    Vec3 RandomViewDirection(Random& random)
    {
        while (true)
        {
            const Vec3 w = RandomDirectionAbove(random);
            if (w.z > 0.05f)
            {
                return w;
            }
        }
    }
}

TEST_CASE("GGX normals are a distribution: D weighted by cos integrates to 1", "[microfacet]")
{
    const float alpha = GENERATE(0.05f, 0.2f, 0.5f, 1.0f);
    CAPTURE(alpha);
    const GgxDistribution ggx{alpha};

    // isotropic, so integrate over cos(theta) only: 2 pi * integral of D * cos over cos from 0 to 1
    constexpr int Steps = 1000000;
    double sum = 0.0;
    for (int i = 0; i < Steps; i++)
    {
        const float c = (static_cast<float>(i) + 0.5f) / Steps;
        const Vec3 wm(std::sqrt(1.0f - c * c), 0.0f, c);
        sum += static_cast<double>(ggx.D(wm)) * c;
    }
    CHECK_THAT(2.0 * Pi * sum / Steps, WithinAbs(1.0, 1e-3));
}

TEST_CASE("GGX visible normals are a distribution for any view direction", "[microfacet]")
{
    const float alpha = GENERATE(0.2f, 0.5f, 1.0f);
    CAPTURE(alpha);
    const GgxDistribution ggx{alpha};
    Random random(3u);
    for (int v = 0; v < 4; v++)
    {
        const Vec3 wo = RandomViewDirection(random);
        CAPTURE(wo.z);
        // integrate over the hemisphere of normals on a (cos theta, phi) grid. normals facing away from wo are
        // hidden, so they're left out. VisibleD itself uses |wo . wm| like pbrt, but is only asked about visible ones
        constexpr int CosSteps = 2000;
        constexpr int PhiSteps = 256;
        double sum = 0.0;
        for (int i = 0; i < CosSteps; i++)
        {
            const float c = (static_cast<float>(i) + 0.5f) / CosSteps;
            const float s = std::sqrt(1.0f - c * c);
            for (int j = 0; j < PhiSteps; j++)
            {
                const float phi = TwoPi * (static_cast<float>(j) + 0.5f) / PhiSteps;
                const Vec3 wm(s * std::cos(phi), s * std::sin(phi), c);
                if (glm::dot(wo, wm) > 0.0f)
                {
                    sum += ggx.VisibleD(wo, wm);
                }
            }
        }
        CHECK_THAT(sum * TwoPi / (CosSteps * PhiSteps), WithinAbs(1.0, 5e-3));
    }
}

TEST_CASE("Rough BxDFs: Pdf and Evaluate match what Sample returns", "[microfacet][bsdf][mis]")
{
    // MIS weighs a light sample by Pdf(wo, wi) and a BSDF sample by the pdf Sample reported, so they must agree
    const GgxDistribution rough{0.3f};
    const BxDF bxdf = GENERATE_COPY(BxDF{ConductorBxDF{.F0 = Vec3(0.9f, 0.6f, 0.3f), .Distribution = rough}},
                                    BxDF{DielectricBxDF{.Eta = 1.5f, .Distribution = rough}},
                                    BxDF{DielectricBxDF{.Eta = 1.0f / 1.5f, .Distribution = rough}},
                                    BxDF{CoatedDiffuseBxDF{.Reflectance = Vec3(0.5f), .Distribution = rough}});
    Random random(11u);
    IndependentSampler sampler(11u);
    int checked = 0;
    for (int i = 0; i < 2000; i++)
    {
        const Vec3 normal = RandomUnitVector(random);
        const Bsdf bsdf(normal, bxdf);
        const Vec3 view = RandomUnitVector(random);
        if (glm::dot(view, normal) < 0.05f)
        {
            continue; // below or grazing
        }
        const auto sample = bsdf.Sample(view, sampler.Get1D(), sampler.Get2D());
        if (!sample)
        {
            continue; // a lost sample: reflected below the surface
        }
        REQUIRE_FALSE(sample->IsSpecular());
        checked++;
        CHECK_THAT(bsdf.Pdf(view, sample->Wi), WithinRel(sample->Pdf, 1e-3f));
        CHECK_THAT(bsdf.Evaluate(view, sample->Wi),
                   Vec3Near(sample->F, 1e-3f * std::max(1.0f, MaxComponent(sample->F))));
    }
    CHECK(checked > 500);
}

TEST_CASE("Rough BxDFs: Pdf integrates to the chance that Sample succeeds", "[microfacet][bsdf]")
{
    // a sample is lost when a microfacet sends it below the surface, so the pdf over all directions adds up to
    // the share of samples that survive. a sampling routine that doesn't match its pdf breaks this
    const GgxDistribution rough{0.4f};
    const BxDF bxdf = GENERATE_COPY(BxDF{ConductorBxDF{.F0 = Vec3(1.0f), .Distribution = rough}},
                                    BxDF{DielectricBxDF{.Eta = 1.5f, .Distribution = rough}},
                                    BxDF{DielectricBxDF{.Eta = 1.0f / 1.5f, .Distribution = rough}},
                                    BxDF{CoatedDiffuseBxDF{.Reflectance = Vec3(0.5f), .Distribution = rough}});
    const Bsdf bsdf(Vec3(0.0f, 0.0f, 1.0f), bxdf);
    IndependentSampler sampler(5u);
    for (const float cosO : {0.9f, 0.5f, 0.2f})
    {
        CAPTURE(cosO);
        const Vec3 wo(std::sqrt(1.0f - cosO * cosO), 0.0f, cosO);

        constexpr int Count = 400000; // ~5e-4 standard error on the share that survives
        int survived = 0;
        for (int i = 0; i < Count; i++)
        {
            survived += bsdf.Sample(wo, sampler.Get1D(), sampler.Get2D()).has_value() ? 1 : 0;
        }

        // the pdf over every direction, on a fine (cos theta, phi) grid. refraction into glass is too peaked to
        // integrate well from random directions
        constexpr int CosSteps = 4000;
        constexpr int PhiSteps = 512;
        double pdfSum = 0.0;
        for (int i = 0; i < CosSteps; i++)
        {
            const float z = -1.0f + 2.0f * (static_cast<float>(i) + 0.5f) / CosSteps;
            const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
            for (int j = 0; j < PhiSteps; j++)
            {
                const float phi = TwoPi * (static_cast<float>(j) + 0.5f) / PhiSteps;
                pdfSum += bsdf.Pdf(wo, Vec3(r * std::cos(phi), r * std::sin(phi), z));
            }
        }
        const double integral = pdfSum * 4.0 * Pi / (CosSteps * PhiSteps);
        CHECK_THAT(integral, WithinAbs(static_cast<double>(survived) / Count, 0.005));
    }
}

TEST_CASE("Rough metal reflects the same both ways", "[microfacet][bsdf]")
{
    const ConductorBxDF metal{.F0 = Vec3(0.9f, 0.6f, 0.3f), .Distribution = GgxDistribution{0.35f}};
    Random random(17u);
    for (int i = 0; i < 1000; i++)
    {
        const Vec3 wo = RandomDirectionAbove(random);
        const Vec3 wi = RandomDirectionAbove(random);
        const Spectrum forward = metal.Evaluate(wo, wi, TransportMode::Radiance);
        const Spectrum backward = metal.Evaluate(wi, wo, TransportMode::Radiance);
        // relative: near grazing the 1 / (4 cos cos) makes values large
        CHECK_THAT(forward, Vec3Near(backward, 1e-5f * std::max(1.0f, MaxComponent(backward))));
    }
}

TEST_CASE("Rough white BxDFs never add light, and lose little when nearly smooth", "[microfacet][bsdf][furnace]")
{
    // with visible normal sampling each sample carries F * G(wo, wi) / G1(wo): never above 1 for white
    // surfaces. single scattering GGX loses the light that bounces between microfacets, so rough surfaces come
    // out below 1, but a nearly smooth one keeps almost all of it
    const float alpha = GENERATE(0.01f, 0.3f, 1.0f);
    CAPTURE(alpha);
    const GgxDistribution ggx{alpha};
    const BxDF bxdf = GENERATE_COPY(BxDF{ConductorBxDF{.F0 = Vec3(1.0f), .Distribution = ggx}},
                                    BxDF{DielectricBxDF{.Eta = 1.5f, .Distribution = ggx}});
    const Bsdf bsdf(Vec3(0.0f, 0.0f, 1.0f), bxdf);
    Random random(23u);
    IndependentSampler sampler(23u);
    double sum = 0.0;
    constexpr int Count = 20000;
    for (int i = 0; i < Count; i++)
    {
        const Vec3 wo = RandomViewDirection(random);
        const auto sample = bsdf.Sample(wo, sampler.Get1D(), sampler.Get2D());
        if (!sample)
        {
            continue; // lost: carries 0
        }
        const float etaScale = sample->IsTransmission() ? sample->Eta * sample->Eta : 1.0f;
        const float carried = (sample->F * std::abs(sample->Wi.z) / sample->Pdf).x * etaScale;
        REQUIRE(carried <= 1.0f + 1e-3f);
        sum += carried;
    }
    const double average = sum / Count;
    CHECK(average <= 1.0);
    if (alpha <= 0.01f)
    {
        CHECK(average > 0.99);
    }
}