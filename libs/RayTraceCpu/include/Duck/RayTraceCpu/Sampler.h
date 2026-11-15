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

    namespace SobolDetail
    {
        inline uint32_t ReverseBits(uint32_t x)
        {
            x = ((x >> 1) & 0x55555555u) | ((x & 0x55555555u) << 1);
            x = ((x >> 2) & 0x33333333u) | ((x & 0x33333333u) << 2);
            x = ((x >> 4) & 0x0F0F0F0Fu) | ((x & 0x0F0F0F0Fu) << 4);
            x = ((x >> 8) & 0x00FF00FFu) | ((x & 0x00FF00FFu) << 8);
            return (x >> 16) | (x << 16);
        }

        // the first two Sobol dimensions as 32 bit binary fractions
        // dimension 0 is the van der Corput sequence: the indexs bits mirrored around the binary point
        inline uint32_t Dimension0(uint32_t index) { return ReverseBits(index); }

        // dimension 1 uses the polynomial x + 1, whose direction numbers are rows of Pascals triangle mod 2
        inline uint32_t Dimension1(uint32_t index)
        {
            uint32_t result = 0;
            for (uint32_t direction = 1u << 31; index != 0; index >>= 1, direction ^= direction >> 1)
            {
                if ((index & 1u) != 0)
                {
                    result ^= direction;
                }
            }
            return result;
        }

        // hash where each output bit only depends on the input bits below it (Laine and Karras, Burleys constants)
        inline uint32_t LaineKarrasPermutation(uint32_t x, uint32_t seed)
        {
            x += seed;
            x ^= x * 0x6c50b47cu;
            x ^= x * 0xb82f1e52u;
            x ^= x * 0xc7afe638u;
            x ^= x * 0x8d22f6e6u;
            return x;
        }

        // Owen scrambling: each bit is flipped based only on the bits above it, so values that share their
        // top k bits still share them afterwards. aligned blocks stay aligned blocks, which keeps the
        // stratification while removing the visible structure of plain Sobol
        inline uint32_t OwenScramble(uint32_t x, uint32_t seed)
        {
            return ReverseBits(LaineKarrasPermutation(ReverseBits(x), seed));
        }

        // top 24 bits only, so the result can't round up to 1.0
        inline float ToUnitFloat(uint32_t x) { return static_cast<float>(x >> 8) * 0x1p-24f; }
    }

    // Owen scrambled Sobol (Burley 2020, "Practical Hash-based Owen Scrambling")
    // the first 2^k samples of any pixel always put one point in every cell of a 2^k grid, and in every
    // 2^a x 2^b grid with a + b = k. this holds for any prefix, so it works with progressive passes where the
    // final count isnt known in advance (a stratified grid needs the count up front)
    // every dimension gets its own shuffle of the sample order, so dimensions dont correlate with each other.
    // works best at power of two pass counts, but any count is still at least as good as independent
    class SobolSampler final : public ISampler
    {
    public:
        explicit SobolSampler(uint64_t seed = 0) : seed(seed) {}

        const char* Name() const override { return "Sobol"; }

        void StartPixelSample(IVec2 pixel, int sampleIndex) override
        {
            const uint64_t pixelKey =
                (static_cast<uint64_t>(static_cast<uint32_t>(pixel.y)) << 32) | static_cast<uint32_t>(pixel.x);
            pixelHash = HashU64(pixelKey ^ HashU64(seed));
            index = static_cast<uint32_t>(sampleIndex);
            dimension = 0;
        }

        float Get1D() override
        {
            const uint64_t hash = NextDimensionHash();
            const uint32_t shuffled = SobolDetail::OwenScramble(index, static_cast<uint32_t>(hash));
            return SobolDetail::ToUnitFloat(
                SobolDetail::OwenScramble(SobolDetail::Dimension0(shuffled), static_cast<uint32_t>(hash >> 32)));
        }

        Vec2 Get2D() override
        {
            const uint64_t hash = NextDimensionHash();
            const uint64_t secondHash = HashU64(hash);
            const uint32_t shuffled = SobolDetail::OwenScramble(index, static_cast<uint32_t>(hash));
            const uint32_t x =
                SobolDetail::OwenScramble(SobolDetail::Dimension0(shuffled), static_cast<uint32_t>(hash >> 32));
            const uint32_t y =
                SobolDetail::OwenScramble(SobolDetail::Dimension1(shuffled), static_cast<uint32_t>(secondHash));
            return {SobolDetail::ToUnitFloat(x), SobolDetail::ToUnitFloat(y)};
        }

        Vec2 GetPixel2D() override { return Get2D(); }

        std::unique_ptr<ISampler> Clone() const override { return std::make_unique<SobolSampler>(*this); }

    private:
        uint64_t NextDimensionHash() { return HashU64(pixelHash + dimension++); }

        uint64_t seed = 0;
        uint64_t pixelHash = 0;
        uint32_t index = 0;
        uint32_t dimension = 0;
    };
}
