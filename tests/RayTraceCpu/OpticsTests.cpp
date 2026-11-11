#include "TestHelpers.h"

#include <Duck/RayTraceCpu/Optics.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace Duck;
using namespace Duck::RayTraceCpu;
using namespace Duck::Test;
using Catch::Matchers::WithinAbs;

namespace
{
    // a random incoming direction and a normal facing it, like Hit::Normal
    struct Incidence
    {
        Vec3 Direction;
        Vec3 Normal;
    };

    Incidence RandomIncidence(Random& random)
    {
        const Vec3 direction = RandomUnitVector(random);
        Vec3 normal = RandomUnitVector(random);
        if (glm::dot(direction, normal) > 0.0f)
        {
            normal = -normal;
        }
        return {direction, normal};
    }

    float SinBetween(const Vec3& a, const Vec3& b) { return glm::length(glm::cross(a, b)); }
}

TEST_CASE("Reflect mirrors about the normal", "[optics]")
{
    Random random(1u);
    for (int i = 0; i < PropertySamples; i++)
    {
        const auto [d, n] = RandomIncidence(random);
        const Vec3 r = Reflect(d, n);
        CAPTURE(d, n, r);
        REQUIRE_THAT(glm::length(r), WithinAbs(1.0f, 1e-5f));
        REQUIRE_THAT(glm::dot(r, n), WithinAbs(-glm::dot(d, n), 1e-5f));               // same angle, other side
        REQUIRE_THAT(r - glm::dot(r, n) * n, Vec3Near(d - glm::dot(d, n) * n, 1e-5f)); // along-surface part kept
    }
}

TEST_CASE("Refract follows Snell's law", "[optics]")
{
    const float eta = GENERATE(1.0f / 1.5f, 1.5f, 1.0f / 1.33f, 1.33f);

    Random random(2u);
    for (int i = 0; i < PropertySamples; i++)
    {
        const auto [d, n] = RandomIncidence(random);
        const float sinIncident = SinBetween(d, n);
        // too close to the critical angle to say which side rounding lands on
        if (std::abs(eta * sinIncident - 1.0f) < 1e-4f)
        {
            continue;
        }

        const auto t = Refract(d, n, eta);
        CAPTURE(eta, d, n);
        REQUIRE(t.has_value() == (eta * sinIncident < 1.0f));
        if (!t)
        {
            continue;
        }

        REQUIRE_THAT(glm::length(*t), WithinAbs(1.0f, 1e-4f));
        REQUIRE(glm::dot(*t, n) < 0.0f);                                      // went through the surface
        REQUIRE_THAT(SinBetween(*t, n), WithinAbs(eta * sinIncident, 1e-4f)); // eta1 sin1 = eta2 sin2
        REQUIRE_THAT(glm::dot(*t, glm::cross(d, n)), WithinAbs(0.0f, 1e-4f)); // stays in the plane

        // light paths are reversible. skipped at grazing angles, where sqrt(k) amplifies rounding
        if (-glm::dot(d, n) > 0.01f)
        {
            const auto back = Refract(-*t, -n, 1.0f / eta);
            REQUIRE(back.has_value());
            REQUIRE_THAT(*back, Vec3Near(-d, 1e-3f));
        }
    }
}

TEST_CASE("Refract reports total internal reflection past the critical angle", "[optics]")
{
    const Vec3 n(0.0f, 1.0f, 0.0f);
    const float eta = 1.5f; // leaving glass into air
    const float critical = std::asin(1.0f / eta);
    auto incoming = [](float angle) { return Vec3(std::sin(angle), -std::cos(angle), 0.0f); };

    CHECK(Refract(incoming(critical - Radians(0.5f)), n, eta).has_value());
    CHECK_FALSE(Refract(incoming(critical + Radians(0.5f)), n, eta).has_value());
}

TEST_CASE("Refract leaves head-on rays unbent", "[optics]")
{
    const Vec3 n(0.0f, 1.0f, 0.0f);
    const Vec3 d(0.0f, -1.0f, 0.0f);
    const float eta = GENERATE(1.0f / 1.5f, 1.5f);
    const auto t = Refract(d, n, eta);
    REQUIRE(t.has_value());
    CHECK_THAT(*t, Vec3Near(d, 1e-6f));
}

TEST_CASE("SchlickFresnel matches known values", "[optics]")
{
    CHECK_THAT(SchlickFresnel(1.0f, 1.0f, 1.5f), WithinAbs(0.04f, 1e-6f)); // head-on glass reflects 4%
    CHECK_THAT(SchlickFresnel(0.0f, 1.0f, 1.5f), WithinAbs(1.0f, 1e-6f));  // grazing is a mirror
    CHECK(SchlickFresnel(0.7f, 1.0f, 1.5f) == SchlickFresnel(0.7f, 1.5f, 1.0f));

    int bad = 0;
    float previous = Infinity;
    for (int i = 0; i <= 1000; i++)
    {
        const float reflectance = SchlickFresnel(static_cast<float>(i) / 1000.0f, 1.0f, 1.5f);
        bad += (reflectance < 0.0f || reflectance > 1.0f + 1e-6f || reflectance > previous) ? 1 : 0;
        previous = reflectance;
    }
    CHECK(bad == 0); // stays in [0, 1] and only falls as the angle gets more head-on
}