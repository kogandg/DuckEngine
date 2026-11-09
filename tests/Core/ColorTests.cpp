#include <Duck/Core/Color.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace Duck;
using Catch::Matchers::WithinAbs;

TEST_CASE("PackRgba8 matches R8G8B8A8_UNORM byte order", "[color]")
{
    // little-endian: red is the lowest byte
    CHECK(PackRgba8(0x11, 0x22, 0x33, 0x44) == 0x44332211u);
    CHECK(PackRgba8(Vec3(1.0f, 0.0f, 0.0f)) == 0xFF0000FFu);
}

TEST_CASE("PackRgba8 clamps and rounds", "[color]")
{
    // 2 -> 255, -1 -> 0, 0.5 -> 127.5 rounds to 128
    CHECK(PackRgba8(Vec3(2.0f, -1.0f, 0.5f)) == 0xFF8000FFu);
}

TEST_CASE("LinearToSrgb follows the sRGB curve", "[color]")
{
    CHECK(LinearToSrgb(0.0f) == 0.0f);
    CHECK_THAT(LinearToSrgb(1.0f), WithinAbs(1.0f, 1e-6f));
    CHECK_THAT(LinearToSrgb(0.5f), WithinAbs(0.735357f, 1e-5f));
    CHECK_THAT(LinearToSrgb(0.18f), WithinAbs(0.461356f, 1e-5f)); // middle gray

    // out of range input is clamped
    CHECK(LinearToSrgb(-1.0f) == 0.0f);
    CHECK_THAT(LinearToSrgb(2.0f), WithinAbs(1.0f, 1e-6f));

    // the linear and power pieces meet at the breakpoint
    CHECK_THAT(LinearToSrgb(0.0031309f), WithinAbs(LinearToSrgb(0.0031308f), 1e-5f));
}

TEST_CASE("LinearToSrgb never decreases", "[color]")
{
    int decreases = 0;
    float previous = LinearToSrgb(0.0f);
    for (int i = 1; i <= 10000; i++)
    {
        const float value = LinearToSrgb(static_cast<float>(i) / 10000.0f);
        decreases += value < previous ? 1 : 0;
        previous = value;
    }
    CHECK(decreases == 0);
}