#pragma once

#include <Duck/Core/Math.h>
#include <Duck/Core/Random.h>

#include <catch2/catch_tostring.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <string>

// lets Catch print Vec3 values in failure messages and CAPTURE
namespace Catch
{
    template <>
    struct StringMaker<Duck::Vec3>
    {
        static std::string convert(const Duck::Vec3& v) { return std::format("({}, {}, {})", v.x, v.y, v.z); }
    };
}

namespace Duck::Test
{
    // how many random inputs each property test checks
    inline constexpr int PropertySamples = 10000;

    // passes when every component is within tolerance of expected
    class Vec3Near : public Catch::Matchers::MatcherGenericBase
    {
    public:
        Vec3Near(const Vec3& expectedValue, float toleranceValue) : expected(expectedValue), tolerance(toleranceValue)
        {
        }

        bool match(const Vec3& actual) const
        {
            return glm::all(glm::lessThanEqual(glm::abs(actual - expected), Vec3(tolerance)));
        }

        std::string describe() const override
        {
            return std::format("is within {} of {}", tolerance, Catch::StringMaker<Vec3>::convert(expected));
        }

    private:
        Vec3 expected;
        float tolerance;
    };

    // uniform direction on the unit sphere
    inline Vec3 RandomUnitVector(Random& random)
    {
        const float z = 1.0f - 2.0f * random.NextFloat();
        const float radius = std::sqrt(std::max(0.0f, 1.0f - z * z));
        const float phi = TwoPi * random.NextFloat();
        return {radius * std::cos(phi), radius * std::sin(phi), z};
    }
}