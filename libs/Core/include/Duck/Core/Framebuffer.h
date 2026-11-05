#pragma once

#include <Duck/Core/Assert.h>
#include <Duck/Core/Color.h>
#include <Duck/Core/Math.h>

#include <algorithm>
#include <span>

namespace Duck
{
    template <typename TPixel>
    class BasicFramebuffer
    {
    public:
        BasicFramebuffer() = default;
        BasicFramebuffer(std::uint32_t width, std::uint32_t height) { Resize(width, height); }

        void Resize(std::uint32_t width, std::uint32_t height)
        {
            this->width = width;
            this->height = height;
            this->pixels.assign(static_cast<std::size_t>(width) * height, TPixel{});
        }

        void Fill(const TPixel& value) { std::ranges::fill(pixels, value); }
        void Clear() { Fill(TPixel{}); }

        std::uint32_t Width() const noexcept { return width; }
        std::uint32_t Height() const noexcept { return height; }
        bool Empty() const noexcept { return pixels.empty(); }
        std::size_t PixelCount() const noexcept { return pixels.size(); }
        std::size_t RowBytes() const noexcept { return static_cast<std::size_t>(width) * sizeof(TPixel); }
        std::size_t SizeBytes() const noexcept { return pixels.size() * sizeof(TPixel); }
        float AspectRatio() const noexcept
        {
            return height == 0 ? 1.0f : static_cast<float>(width) / static_cast<float>(height);
        }

        TPixel* Data() noexcept { return pixels.data(); }
        const TPixel* Data() const noexcept { return pixels.data(); }

        std::span<TPixel> Pixels() noexcept { return pixels; }
        std::span<const TPixel> Pixels() const noexcept { return pixels; }

        std::span<TPixel> Row(std::uint32_t y)
        {
            DUCK_ASSERT(y < height, "Row {} out of bounds (height {})", y, height);
            return {pixels.data() + static_cast<std::size_t>(y) * width, width};
        }
        std::span<const TPixel> Row(std::uint32_t y) const
        {
            DUCK_ASSERT(y < height, "Row {} out of bounds (height {})", y, height);
            return {pixels.data() + static_cast<std::size_t>(y) * width, width};
        }

        TPixel& operator[](std::uint32_t x, std::uint32_t y)
        {
            DUCK_ASSERT(x < width && y < height, "Pixel ({}, {}) out of bounds for {}x{}", x, y, width, height);
            return pixels[static_cast<std::size_t>(y) * width + x];
        }
        const TPixel& operator[](std::uint32_t x, std::uint32_t y) const
        {
            DUCK_ASSERT(x < width && y < height, "Pixel ({}, {}) out of bounds for {}x{}", x, y, width, height);
            return pixels[static_cast<std::size_t>(y) * width + x];
        }

    private:
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::vector<TPixel> pixels;
    };

    using Framebuffer = BasicFramebuffer<Rgba8>;   // 8-bit for present
    using HdrFramebuffer = BasicFramebuffer<Vec3>; // linear float for accumulation
}