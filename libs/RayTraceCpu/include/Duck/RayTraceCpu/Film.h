#pragma once

#include <Duck/Core/Framebuffer.h>

#include <cstdint>

namespace Duck
{
    class ThreadPool;
}

namespace Duck::RayTraceCpu
{
    enum class OutputTransform
    {
        Srgb,   // linear radiance to sRGB encoding
        Linear, // written as is
    };

    struct Film
    {
        Film(int filmWidth, int filmHeight) : Sum(filmWidth, filmHeight) {}

        void Reset();

        void Resolve(Framebuffer& output, ThreadPool& pool, OutputTransform transform = OutputTransform::Srgb) const;

        HdrFramebuffer Sum; // sum of every pass so far
        int Passes = 0;     // how many passes are in Sum
    };
}