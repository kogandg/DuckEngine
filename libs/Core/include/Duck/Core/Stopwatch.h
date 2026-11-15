#pragma once

#include <chrono>

namespace Duck
{
    class Stopwatch
    {
    public:
        Stopwatch() : start(Clock::now()) {}

        void Restart() { start = Clock::now(); }
        double ElapsedMs() const { return std::chrono::duration<double, std::milli>(Clock::now() - start).count(); }

    private:
        using Clock = std::chrono::steady_clock;
        Clock::time_point start;
    };
}