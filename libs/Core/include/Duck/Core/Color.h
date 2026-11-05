#pragma once

#include <Duck/Core/Math.h>

namespace Duck
{
    // matches vulkan format VK_FORMAT_R8G8B8A8_UNORM
    using Rgba8 = std::uint32_t;

    constexpr Rgba8 PackRgba8(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
    {
        return static_cast<Rgba8>(r) | (static_cast<Rgba8>(g) << 8) | (static_cast<Rgba8>(b) << 16) |
               (static_cast<Rgba8>(a) << 24);
    }

    inline Rgba8 PackRgba8(const Vec4& color)
    {
        const Vec4 c = glm::clamp(color, 0.0f, 1.0f) * 255.0f + 0.5f;
        return PackRgba8(static_cast<std::uint8_t>(c.r), static_cast<std::uint8_t>(c.g), static_cast<std::uint8_t>(c.b),
                         static_cast<std::uint8_t>(c.a));
    }

    inline Rgba8 PackRgba8(const Vec3& color) { return PackRgba8(Vec4(color, 1.0f)); }

    inline float LinearToSrgb(float linear)
    {
        linear = glm::clamp(linear, 0.0f, 1.0f);
        return linear <= 0.0031308f ? linear * 12.92f : 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
    }

    inline Vec3 LinearToSrgb(const Vec3& linear)
    {
        return {LinearToSrgb(linear.r), LinearToSrgb(linear.g), LinearToSrgb(linear.b)};
    }

}