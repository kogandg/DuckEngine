#include <Duck/RayTraceCpu/BackgroundRenderer.h>

#include <Duck/Core/Assert.h>
#include <Duck/RayTraceCpu/Integrator.h>
#include <Duck/RayTraceCpu/RenderScene.h>

#include <chrono>
#include <utility>

namespace Duck::RayTraceCpu
{
    BackgroundRenderer::BackgroundRenderer(int width, int height, unsigned threadCount)
        : pool(threadCount), film(width, height), back(width, height), shared(width, height),
          thread([this](std::stop_token stop) { Loop(stop); })
    {
    }

    void BackgroundRenderer::Restart(std::shared_ptr<const RenderScene> newScene, const Camera& newCamera,
                                     IIntegrator& newIntegrator, std::shared_ptr<const ISampler> newSampler)
    {
        {
            std::lock_guard lock(mutex);
            scene = std::move(newScene);
            camera = newCamera;
            integrator = &newIntegrator;
            sampler = std::move(newSampler);
            generation++;
            jobStop.request_stop(); // the pass running now belongs to the old job
            jobStop = {};           // fresh source for the new job
            progress = {0, newIntegrator.RecommendedPasses()};
        }
        changed.notify_one();
    }

    bool BackgroundRenderer::TakeLatest(Framebuffer& display)
    {
        std::lock_guard lock(mutex);
        if (!fresh)
        {
            return false;
        }
        DUCK_ASSERT(display.Width() == shared.Width() && display.Height() == shared.Height());
        std::swap(shared, display); // the caller's old buffer becomes the next spare
        fresh = false;
        return true;
    }

    BackgroundRenderer::Progress BackgroundRenderer::GetProgress() const
    {
        std::lock_guard lock(mutex);
        return progress;
    }

    void BackgroundRenderer::Loop(std::stop_token stop)
    {
        std::uint64_t rendering = 0; // the generation the film belongs to
        while (true)
        {
            std::shared_ptr<const RenderScene> jobScene;
            IIntegrator* jobIntegrator = nullptr;
            std::optional<Camera> jobCamera;
            std::shared_ptr<const ISampler> jobSampler;
            std::stop_token jobCancel;
            {
                std::unique_lock lock(mutex);
                // sleep until there's a new job, or the current one still wants passes. a stop request wakes it too
                const bool hasWork = changed.wait(
                    lock, stop,
                    [&]
                    {
                        return integrator && (generation != rendering || film.Passes < integrator->RecommendedPasses());
                    });
                if (!hasWork)
                {
                    return; // stop requested
                }
                if (generation != rendering)
                {
                    film.Reset(); // the accumulated passes belong to the old job
                    rendering = generation;
                }
                jobScene = scene;
                jobCamera = camera;
                jobIntegrator = integrator;
                jobSampler = sampler;
                jobCancel = jobStop.get_token();
            }

            // the slow part, without the lock: the main thread keeps running
            const auto start = std::chrono::steady_clock::now();
            jobIntegrator->RenderPass(*jobScene, *jobCamera, film, pool, *jobSampler, jobCancel);
            if (jobCancel.stop_requested())
            {
                continue;
            }
            film.Passes++;
            film.Resolve(back, pool, jobIntegrator->GetOutputTransform());
            const float passMs =
                std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();

            std::lock_guard lock(mutex);
            if (generation == rendering) // otherwise Restart was called mid-pass: this image is outdated
            {
                std::swap(back, shared);
                fresh = true;
                progress = {film.Passes, jobIntegrator->RecommendedPasses(), passMs};
            }
        }
    }
}