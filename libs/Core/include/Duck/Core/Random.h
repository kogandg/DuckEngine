#pragma once

#include <cstdint>

namespace Duck
{
    using std::uint32_t;
    using std::uint64_t;

    // splitMix64
    inline uint64_t HashU64(uint64_t x)
    {
        x += 0x9E3779B97F4A7C15ULL;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
        return x ^ (x >> 31);
    }

    // PCG#2
    // NOT THREAD SAFE, use one Random per thread
    class Random
    {
    public:
        Random() = default;
        Random(uint64_t seed, uint64_t sequence = 0) { Seed(seed, sequence); }

        void Seed(uint64_t seed, uint64_t sequence = 0)
        {
            state = 0;
            increment = (sequence << 1u) | 1u;
            NextUint();
            state += seed;
            NextUint();
        }

        uint32_t NextUint()
        {
            const uint64_t old = state;
            state = old * 6364136223846793005ULL + increment;
            const uint32_t xorShifted = static_cast<uint32_t>(((old >> 18u) ^ old) >> 27u);
            const uint32_t rotation = static_cast<uint32_t>(old >> 59u);
            return (xorShifted >> rotation) | (xorShifted << ((32u - rotation) & 31u));
        }

        // uniform in [0, 1). uses the top 24 bits, which is all a float can represent
        static constexpr float ToUnitFloat(uint32_t bits)
        {
            return static_cast<float>(bits >> 8u) * (1.0f / 16777216.0f);
        }

        float NextFloat() { return ToUnitFloat(NextUint()); }

    private:
        uint64_t state = 0;
        uint64_t increment = 1;
    };
}