#pragma once

#include <Duck/Core/Math.h>

namespace Duck
{
    // matches vulkan format VK_FORMAT_R8G8B8A8_UNORM
    using Rgba8 = uint32_t;

    constexpr Rgba8 PackRgba8(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
    {
        return static_cast<Rgba8>(r) | (static_cast<Rgba8>(g) << 8) | (static_cast<Rgba8>(b) << 16) |
               (static_cast<Rgba8>(a) << 24);
    }

    inline Rgba8 PackRgba8(const Vec4& color)
    {
        const Vec4 c = glm::clamp(color, 0.0f, 1.0f) * 255.0f + 0.5f;
        return PackRgba8(static_cast<uint8_t>(c.r), static_cast<uint8_t>(c.g), static_cast<uint8_t>(c.b),
                         static_cast<uint8_t>(c.a));
    }

    inline Rgba8 PackRgba8(const Vec3& color) { return PackRgba8(Vec4(color, 1.0f)); }

    // how bright a linear rgb color looks: the eye is most sensitive to green and least to blue (Rec. 709 weights)
    inline float Luminance(const Vec3& linear) { return glm::dot(linear, Vec3(0.2126f, 0.7152f, 0.0722f)); }

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