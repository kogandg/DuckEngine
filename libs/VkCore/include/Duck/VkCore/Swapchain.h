#pragma once

#include <volk.h>

#include <cstdint>
#include <vector>

namespace Duck
{
    class Window;
}

namespace Duck::Vk
{
    using std::uint32_t;

    class VulkanContext;

    enum SwapchainStatus
    {
        Ok,
        OutOfDate, // the swapchain no longer matches the window, should call recreate and skip the frame
    };

    class Swapchain
    {
    public:
        Swapchain(VulkanContext& context, Window& window);
        ~Swapchain();

        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;
        Swapchain(Swapchain&&) = delete;
        Swapchain& operator=(Swapchain&&) = delete;

        // rebuilds for the current window size, returns false and changes nothing while the window is minimized
        bool Recreate();

        // gets the next image to render into, signalSemaphore is signaled when the image is ready to be written
        SwapchainStatus Acquire(VkSemaphore signalSemaphore, uint32_t& imageIndex);
        // 2ueues the image for display once waitSemaphore is signaled
        SwapchainStatus Present(VkSemaphore waitSemaphore, uint32_t imageIndex);

        VkSwapchainKHR Handle() const noexcept { return this->swapchain; }
        VkFormat Format() const noexcept { return this->format; }
        VkExtent2D Extent() const noexcept { return this->extent; }
        uint32_t ImageCount() const noexcept { return static_cast<uint32_t>(this->images.size()); }
        VkImage Image(uint32_t index) const { return this->images[index]; }
        VkImageView ImageView(uint32_t index) const { return this->imageViews[index]; }

    private:
        void Build(VkSwapchainKHR oldSwapchain);
        void DestroyImageViews();

        VulkanContext& context;
        Window& window;

        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        VkExtent2D extent{};
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
    };
}