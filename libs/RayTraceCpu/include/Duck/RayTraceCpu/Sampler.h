#pragma once

#include <Duck/Core/Math.h>
#include <Duck/Core/Random.h>

#include <cstdint>
#include <memory>

namespace Duck::RayTraceCpu
{
    // hand out the number for one sample of a pixel
    // not thread safe, every thread works with a clone
    class ISampler
    {
    public:
        virtual ~ISampler() = default;

        virtual const char* Name() const = 0;

        virtual void StartPixelSample(IVec2 pixel, int sampleIndex) = 0;

        virtual float Get1D() = 0;
        virtual Vec2 Get2D() = 0;

        virtual Vec2 GetPixel2D() = 0;

        virtual std::unique_ptr<ISampler> Clone() const = 0;
    };

    class IndependentSampler final : public ISampler
    {
    public:
        explicit IndependentSampler(uint64_t seed = 0) : random(seed) {}

        const char* Name() const override { return "Independent"; }
        void StartPixelSample(IVec2 pixel, int sampleIndex) override
        {
            // seeded from the pixel's coordinates, so nothing depends on the image size
            const uint64_t pixelKey =
                (static_cast<uint64_t>(static_cast<uint32_t>(pixel.y)) << 32) | static_cast<uint32_t>(pixel.x);
            random.Seed(HashU64(pixelKey), HashU64(static_cast<uint64_t>(sampleIndex)));
        }

        float Get1D() override { return random.NextFloat(); }

        Vec2 Get2D() override
        {
            const float u1 = random.NextFloat();
            const float u2 = random.NextFloat();
            return {u1, u2};
        }

        Vec2 GetPixel2D() override { return Get2D(); }

        std::unique_ptr<ISampler> Clone() const override { return std::make_unique<IndependentSampler>(*this); }

    private:
        Random random;
    };
}