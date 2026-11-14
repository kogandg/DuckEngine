#pragma once

#include <Duck/Core/Framebuffer.h>
#include <Duck/Core/ThreadPool.h>
#include <Duck/RayTraceCpu/Camera.h>
#include <Duck/RayTraceCpu/Film.h>
#include <Duck/RayTraceCpu/Sampler.h>

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>
#include <memory>

namespace Duck::RayTraceCpu
{
    class RenderScene;
    class IIntegrator;

    // renders passes on its own thread (plus a thread pool) so the caller's loop never waits on rendering
    // the caller asks for a job with Restart and picks up finished images with TakeLatest
    class BackgroundRenderer
    {
    public:
        // threadCount: the render thread plus pool workers. leave at least one hardware thread for the caller
        BackgroundRenderer(int width, int height, unsigned threadCount);

        BackgroundRenderer(const BackgroundRenderer&) = delete;
        BackgroundRenderer& operator=(const BackgroundRenderer&) = delete;
        BackgroundRenderer(BackgroundRenderer&&) = delete;
        BackgroundRenderer& operator=(BackgroundRenderer&&) = delete;

        // throws away the current image and starts over. the camera is copied; scene and integrator are
        // referenced, so they must outlive the renderer and must not change while it uses them
        void Restart(std::shared_ptr<const RenderScene> newScene, const Camera& camera, IIntegrator& integrator,
                     std::shared_ptr<const ISampler> sampler = std::make_shared<IndependentSampler>());

        // swaps the newest finished image into display if there is one. returns whether display changed
        // display must be width x height
        bool TakeLatest(Framebuffer& display);

        struct Progress
        {
            int Passes = 0;
            int TargetPasses = 0;
            float LastPassMs = 0.0f;
            bool Done() const { return Passes >= TargetPasses; }
        };
        Progress GetProgress() const;

    private:
        void Loop(std::stop_token stop);

        ThreadPool pool;

        // only the render thread touches these
        Film film;
        Framebuffer back;

        mutable std::mutex mutex; // guards everything below
        std::condition_variable_any changed;
        std::optional<Camera> camera;
        IIntegrator* integrator = nullptr;
        std::uint64_t generation = 0; // goes up on every Restart
        std::stop_source jobStop; // stopped by Restart, so a pass of the old job ends early
        Framebuffer shared;           // newest finished image
        bool fresh = false;           // shared has an image the caller hasn't taken yet
        Progress progress;
        std::shared_ptr<const RenderScene> scene;
        std::shared_ptr<const ISampler> sampler;

        // last: started after everything above exists, and stopped and joined before any of it is destroyed
        std::jthread thread;
    };
}