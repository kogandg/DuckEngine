#pragma once

#include <Duck/Core/Assert.h>
#include <Duck/Core/Color.h>
#include <Duck/Core/Math.h>

#include <algorithm>
#include <span>
#include <cstddef>
#include <vector>

namespace Duck
{
    template <typename TPixel>
    class BasicFramebuffer
    {
    public:
        BasicFramebuffer() = default;
        BasicFramebuffer(int width, int height) { Resize(width, height); }

        void Resize(int width, int height)
        {
            DUCK_ASSERT(width >= 0 && height >= 0, "Invalid framebuffer size {}x{}", width, height);
            this->width = width;
            this->height = height;
            this->pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), TPixel{});
        }

        void Fill(const TPixel& value) { std::ranges::fill(pixels, value); }
        void Clear() { Fill(TPixel{}); }

        int Width() const noexcept { return width; }
        int Height() const noexcept { return height; }
        bool Empty() const noexcept { return pixels.empty(); }
        size_t PixelCount() const noexcept { return pixels.size(); }
        size_t RowBytes() const noexcept { return static_cast<size_t>(width) * sizeof(TPixel); }
        size_t SizeBytes() const noexcept { return pixels.size() * sizeof(TPixel); }
        float AspectRatio() const noexcept
        {
            return height == 0 ? 1.0f : static_cast<float>(width) / static_cast<float>(height);
        }

        TPixel* Data() noexcept { return pixels.data(); }
        const TPixel* Data() const noexcept { return pixels.data(); }

        std::span<TPixel> Pixels() noexcept { return pixels; }
        std::span<const TPixel> Pixels() const noexcept { return pixels; }

        std::span<TPixel> Row(int y)
        {
            DUCK_ASSERT(y < height, "Row {} out of bounds (height {})", y, height);
            return {pixels.data() + Index(0, y), static_cast<size_t>(width)};
        }
        std::span<const TPixel> Row(int y) const
        {
            DUCK_ASSERT(y < height, "Row {} out of bounds (height {})", y, height);
            return {pixels.data() + Index(0, y), static_cast<size_t>(width)};
        }

        TPixel& operator[](int x, int y)
        {
            DUCK_ASSERT(x < width && y < height, "Pixel ({}, {}) out of bounds for {}x{}", x, y, width, height);
            return pixels[Index(x, y)];
        }
        const TPixel& operator[](int x, int y) const
        {
            DUCK_ASSERT(x < width && y < height, "Pixel ({}, {}) out of bounds for {}x{}", x, y, width, height);
            return pixels[Index(x, y)];
        }

    private:
        size_t Index(int x, int y) const noexcept
        {
            return static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
        }

        int width = 0;
        int height = 0;
        std::vector<TPixel> pixels;
    };

    using Framebuffer = BasicFramebuffer<Rgba8>;   // 8-bit for present
    using HdrFramebuffer = BasicFramebuffer<Vec3>; // linear float for accumulation
}