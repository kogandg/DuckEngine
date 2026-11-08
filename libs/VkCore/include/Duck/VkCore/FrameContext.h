#pragma once

#include <Duck/VkCore/Swapchain.h>

#include <volk.h>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace Duck::Vk
{
    class VulkanContext;

    constexpr uint32_t MaxFramesInFlight = 2;

    struct FrameInfo
    {
        VkCommandBuffer Commands = VK_NULL_HANDLE;
        VkImage Image = VK_NULL_HANDLE; // swapchain image, in layout VK_IMAGE_LAYOUT_UNDEFINED
        VkImageView ImageView = VK_NULL_HANDLE;
        VkExtent2D Extent{};
        uint32_t FrameIndex = 0; // 0 to maxframesinflight-1, for indexing per frame resources
    };

    class FrameContext
    {
    public:
        FrameContext(VulkanContext& contet, Swapchain& swapchain);
        ~FrameContext();

        FrameContext(const FrameContext&) = delete;
        FrameContext& operator=(const FrameContext&) = delete;
        FrameContext(FrameContext&&) = delete;
        FrameContext& operator=(FrameContext&&) = delete;

        // waits for this frames previous work, acquires an image, and starts recording. Returns nullopt if the frame
        // should be skipped ie swapchain rebuilt or window minimized
        std::optional<FrameInfo> BeginFrame();

        // ends recording, submits, and presents, only call after BeginFrame returned a value
        void EndFrame();

    private:
        struct Frame
        {
            VkCommandPool Pool = VK_NULL_HANDLE;
            VkCommandBuffer Commands = VK_NULL_HANDLE;
            VkFence InFlight = VK_NULL_HANDLE;
            VkSemaphore ImageAvailable = VK_NULL_HANDLE;
        };

        void RecreateSwapchain();
        void CreateRenderFinishedSemaphores();
        void DestroyRenderFinishedSemaphores();

        VulkanContext& context;
        Swapchain& swapchain;

        std::array<Frame, MaxFramesInFlight> frames;
        std::vector<VkSemaphore> renderFinished; // one per swapchain image
        uint32_t frameIndex = 0;
        uint32_t imageIndex = 0;
        bool recording = false;
    };
}