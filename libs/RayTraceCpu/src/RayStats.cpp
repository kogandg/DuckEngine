#include <Duck/RayTraceCpu/RayStats.h>

#include <atomic>

namespace Duck::RayTraceCpu::RayStats
{
    namespace
    {
        std::atomic<uint64_t> closestHitRays{0};
        std::atomic<uint64_t> occlusionRays{0};
        std::atomic<uint64_t> nodeTests{0};
        std::atomic<uint64_t> primitiveTests{0};
    }

    void FlushThread()
    {
        Counters& counters = ThreadCounters;
        closestHitRays.fetch_add(counters.ClosestHitRays, std::memory_order_relaxed);
        occlusionRays.fetch_add(counters.OcclusionRays, std::memory_order_relaxed);
        nodeTests.fetch_add(counters.NodeTests, std::memory_order_relaxed);
        primitiveTests.fetch_add(counters.PrimitiveTests, std::memory_order_relaxed);
        counters = {};
    }

    Counters Totals()
    {
        return {closestHitRays.load(std::memory_order_relaxed), occlusionRays.load(std::memory_order_relaxed),
                nodeTests.load(std::memory_order_relaxed), primitiveTests.load(std::memory_order_relaxed)};
    }
}