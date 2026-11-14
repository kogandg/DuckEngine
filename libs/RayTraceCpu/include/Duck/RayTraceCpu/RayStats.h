#pragma once

#include <cstdint>

namespace Duck::RayTraceCpu
{
    struct TraversalStats
    {
        int NodeTests = 0;
        int PrimitiveTests = 0;
    };

    namespace RayStats
    {
        struct Counters
        {
            uint64_t ClosestHitRays = 0;
            uint64_t OcclusionRays = 0;
            uint64_t NodeTests = 0;
            uint64_t PrimitiveTests = 0;

            uint64_t Rays() const { return ClosestHitRays + OcclusionRays; }

            Counters operator-(const Counters& other) const
            {
                return {ClosestHitRays - other.ClosestHitRays, OcclusionRays - other.OcclusionRays,
                        NodeTests - other.NodeTests, PrimitiveTests - other.PrimitiveTests};
            }
        };

        inline thread_local Counters ThreadCounters;

        void FlushThread();

        Counters Totals();
    }
}