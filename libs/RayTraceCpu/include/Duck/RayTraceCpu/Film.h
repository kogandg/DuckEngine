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

    // when a pixel has enough samples to stop
    struct AdaptiveSettings
    {
        int MinPasses = 0;      // every pixel gets at least min passes samples
        float Threshold = 0.0f; // stop once relative error is below threshold. 0 means adaptive sampling off

        bool Enabled() const { return Threshold > 0.0f; }
    };

    struct Film
    {
        Film(int filmWidth, int filmHeight)
            : Sum(filmWidth, filmHeight), Checkpoint(filmWidth, filmHeight), StoppedAt(filmWidth, filmHeight),
              Error(filmWidth, filmHeight), ActivePixels(filmWidth * filmHeight)
        {
        }

        void Reset();

        void Resolve(Framebuffer& output, ThreadPool& pool, OutputTransform transform = OutputTransform::Srgb) const;

        // call after Passes++. on power of two pass counts, stops every pixel whose error and whose neighbours
        // errors are below the threshold
        void UpdateAdaptive(ThreadPool& pool, const AdaptiveSettings& settings);

        bool IsActive(int x, int y) const { return StoppedAt[x, y] == 0; }
        int SampleCount(int x, int y) const { return IsActive(x, y) ? Passes : StoppedAt[x, y]; }

        HdrFramebuffer Sum; // sum of every pass so far
        int Passes = 0;     // how many passes are in Sum

        HdrFramebuffer Checkpoint;       // Sum at the last power of two pass, i.e. the first half of the samples
        BasicFramebuffer<int> StoppedAt; // pass count a pixel stopped at, 0 while it is still sampling
        BasicFramebuffer<float> Error;   // error estimate from the last check, 0 for stopped pixels
        int ActivePixels = 0;
    };
}