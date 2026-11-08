#include <Duck/VkCore/FrameContext.h>

#include <Duck/Core/Assert.h>
#include <Duck/VkCore/VkCheck.h>
#include <Duck/VkCore/VulkanContext.h>

namespace Duck::Vk
{
    namespace
    {
        VkSemaphore CreateSemaphore(VkDevice device)
        {
            const VkSemaphoreCreateInfo info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            VkSemaphore semaphore = VK_NULL_HANDLE;
            DUCK_VK_CHECK(vkCreateSemaphore(device, &info, nullptr, &semaphore));
            return semaphore;
        }
    }
    FrameContext::FrameContext(VulkanContext& context, Swapchain& swapchain) : context(context), swapchain(swapchain)
    {
        const VkDevice device = context.Device();

        for (Frame& frame : frames)
        {
            const VkCommandPoolCreateInfo poolInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                                   .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                                                   .queueFamilyIndex = context.QueueFamily()};
            DUCK_VK_CHECK(vkCreateCommandPool(device, &poolInfo, nullptr, &frame.Pool));

            const VkCommandBufferAllocateInfo allocInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                        .commandPool = frame.Pool,
                                                        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                        .commandBufferCount = 1};
            DUCK_VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &frame.Commands));

            // created signaled, so the very first wait in BeginFrame returns immediately
            const VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                              .flags = VK_FENCE_CREATE_SIGNALED_BIT};
            DUCK_VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &frame.InFlight));

            frame.ImageAvailable = CreateSemaphore(device);
        }

        CreateRenderFinishedSemaphores();
    }

    FrameContext::~FrameContext()
    {
        const VkDevice device = context.Device();
        vkDeviceWaitIdle(device);

        DestroyRenderFinishedSemaphores();
        for (Frame& frame : frames)
        {
            vkDestroySemaphore(device, frame.ImageAvailable, nullptr);
            vkDestroyFence(device, frame.InFlight, nullptr);
            vkDestroyCommandPool(device, frame.Pool, nullptr); // also frees its command buffer
        }
    }

    std::optional<FrameInfo> FrameContext::BeginFrame()
    {
        DUCK_ASSERT(!recording, "BeginFrame called twice without EndFrame");

        const VkDevice device = context.Device();
        Frame& frame = frames[frameIndex];

        // wait until GPU has finished the last frame that used this slot
        DUCK_VK_CHECK(vkWaitForFences(device, 1, &frame.InFlight, VK_TRUE, UINT64_MAX));

        if (swapchain.Acquire(frame.ImageAvailable, imageIndex) == SwapchainStatus::OutOfDate)
        {
            // nothing was signaled and nothing submitted so the fence is still signaled, just skip the frame
            RecreateSwapchain();
            return std::nullopt;
        }

        // only reset the fence once we know we will submit work that signals it again
        DUCK_VK_CHECK(vkResetFences(device, 1, &frame.InFlight));
        DUCK_VK_CHECK(vkResetCommandPool(device, frame.Pool, 0));

        const VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                                 .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
        DUCK_VK_CHECK(vkBeginCommandBuffer(frame.Commands, &beginInfo));
        recording = true;

        return FrameInfo{.Commands = frame.Commands,
                         .Image = swapchain.Image(imageIndex),
                         .ImageView = swapchain.ImageView(imageIndex),
                         .Extent = swapchain.Extent(),
                         .FrameIndex = frameIndex};
    }

    void FrameContext::EndFrame()
    {
        DUCK_ASSERT(recording, "EndFrame called without a successful BeginFrame");
        recording = false;

        Frame& frame = frames[frameIndex];
        DUCK_VK_CHECK(vkEndCommandBuffer(frame.Commands));

        const VkSemaphore renderDone = renderFinished[imageIndex];

        // ALL_COMMANDS is conservative but works. it works for both rendering and transfer work and its easy to narrow
        // later if profiling shows performace problems
        const VkSemaphoreSubmitInfo waitInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                             .semaphore = frame.ImageAvailable,
                                             .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
        const VkSemaphoreSubmitInfo signalInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                               .semaphore = renderDone,
                                               .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
        const VkCommandBufferSubmitInfo commandInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                                                    .commandBuffer = frame.Commands};
        const VkSubmitInfo2 submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                                       .waitSemaphoreInfoCount = 1,
                                       .pWaitSemaphoreInfos = &waitInfo,
                                       .commandBufferInfoCount = 1,
                                       .pCommandBufferInfos = &commandInfo,
                                       .signalSemaphoreInfoCount = 1,
                                       .pSignalSemaphoreInfos = &signalInfo};
        DUCK_VK_CHECK(vkQueueSubmit2(context.Queue(), 1, &submitInfo, frame.InFlight));

        if (swapchain.Present(renderDone, imageIndex) == SwapchainStatus::OutOfDate)
        {
            RecreateSwapchain();
        }

        frameIndex = (frameIndex + 1) % MaxFramesInFlight;
    }

    void FrameContext::RecreateSwapchain()
    {
        // Swapchain::Recreate waits for the device to go idle so destroying the semaphores afterwards is safe.
        // rebuilt every time rather than only on a count change, because a present that failed as out of date
        // may have left one in an unknown state.
        if (swapchain.Recreate())
        {
            DestroyRenderFinishedSemaphores();
            CreateRenderFinishedSemaphores();
        }
    }

    void FrameContext::CreateRenderFinishedSemaphores()
    {
        renderFinished.resize(swapchain.ImageCount());
        for (VkSemaphore& semaphore : renderFinished)
        {
            semaphore = CreateSemaphore(context.Device());
        }
    }

    void FrameContext::DestroyRenderFinishedSemaphores()
    {
        for (VkSemaphore semaphore : renderFinished)
        {
            vkDestroySemaphore(context.Device(), semaphore, nullptr);
        }
        renderFinished.clear();
    }
}