#include <Duck/VkCore/Swapchain.h>

#include <Duck/Core/Assert.h>
#include <Duck/Core/Cast.h>
#include <Duck/Core/Log.h>
#include <Duck/Platform/Window.h>
#include <Duck/VkCore/VkCheck.h>
#include <Duck/VkCore/VulkanContext.h>

#include <algorithm>
#include <limits>

namespace Duck
{
    namespace
    {
        VkSurfaceFormatKHR ChooseSurfaceFormat(VkPhysicalDevice device, VkSurfaceKHR surface)
        {
            uint32_t count = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
            std::vector<VkSurfaceFormatKHR> formats(count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats.data());

            // UNORM, not SRGB since the framebuffer already holds srgb encoded bytes and a SRGB swapchain would encode
            // them again when blitting
            for (const VkSurfaceFormatKHR& candidate : formats)
            {
                if ((candidate.format == VK_FORMAT_B8G8R8A8_UNORM || candidate.format == VK_FORMAT_R8G8B8A8_UNORM) &&
                    candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    return candidate;
                }
            }

            Log::Warn("No 8-bit UNORM swapchain format found, using the first available");
            return formats[0];
        }
    }

    Swapchain::Swapchain(VulkanContext& context, Window& window) : context(context), window(window)
    {
        DUCK_VERIFY(!window.IsMinimized(), "Cannot create swapchain for a minimized window");
        Build(VK_NULL_HANDLE);
    }

    Swapchain::~Swapchain()
    {
        DestroyImageViews();
        vkDestroySwapchainKHR(context.Device(), swapchain, nullptr);
    }

    bool Swapchain::Recreate()
    {
        if (window.IsMinimized())
        {
            return false;
        }

        // wait so nothing is still be using the old images or views
        vkDeviceWaitIdle(context.Device());

        DestroyImageViews();
        const VkSwapchainKHR old = swapchain;
        Build(old);
        vkDestroySwapchainKHR(context.Device(), old, nullptr);
        return true;
    }

    SwapchainStatus Swapchain::Acquire(VkSemaphore signalSemaphore, uint32_t& imageIndex)
    {
        const VkResult result = vkAcquireNextImageKHR(context.Device(), swapchain, UINT64_MAX,
                                                      signalSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return SwapchainStatus::OutOfDate;
        }
        // SUBOPTIMAL still returned an image and signaled the semaphore, so use it and rebuild after presenting
        DUCK_VERIFY(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "vkAcquireNextImageKHR failed with {}",
                    static_cast<int>(result));
        return SwapchainStatus::Ok;
    }

    SwapchainStatus Swapchain::Present(VkSemaphore waitSemaphore, uint32_t imageIndex)
    {
        const VkPresentInfoKHR info{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &waitSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &imageIndex,
        };
        const VkResult result = vkQueuePresentKHR(context.Queue(), &info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            return SwapchainStatus::OutOfDate;
        }
        DUCK_VERIFY(result == VK_SUCCESS, "vkQueuePresentKHR failed with {}", static_cast<int>(result));
        return SwapchainStatus::Ok;
    }

    void Swapchain::Build(VkSwapchainKHR oldSwapchain)
    {
        const VkPhysicalDevice physicalDevice = context.PhysicalDevice();
        const VkSurfaceKHR surface = context.Surface();

        VkSurfaceCapabilitiesKHR capabilities;
        DUCK_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities));

        const VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(physicalDevice, surface);

        // platforms will dictate the extent, max value means any value works within bounds
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            extent = capabilities.currentExtent;
        }
        else
        {
            const IVec2 size = window.FramebufferSize();
            extent.width =
                std::clamp(CheckedCast<uint32_t>(size.x), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            extent.height =
                std::clamp(CheckedCast<uint32_t>(size.y), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }

        // one more than the min to never waiting on the driver, max of 0 means no limit
        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0)
        {
            imageCount = std::min(imageCount, capabilities.maxImageCount);
        }

        const VkSwapchainCreateInfoKHR info{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = imageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            // COLOR_ATTACHMENT for rendering, TRANSFER_DST for the CPU framebuffer blit
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .preTransform = capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR, // vsync, can also use mailbox but fifo is guaranteed
            .clipped = VK_TRUE,
            .oldSwapchain = oldSwapchain,
        };
        DUCK_VK_CHECK(vkCreateSwapchainKHR(context.Device(), &info, nullptr, &swapchain));

        format = surfaceFormat.format;
        colorSpace = surfaceFormat.colorSpace;

        uint32_t actualCount = 0;
        vkGetSwapchainImagesKHR(context.Device(), swapchain, &actualCount, nullptr);
        images.resize(actualCount);
        vkGetSwapchainImagesKHR(context.Device(), swapchain, &actualCount, images.data());

        imageViews.resize(actualCount);
        for (uint32_t i = 0; i < actualCount; i++)
        {
            const VkImageViewCreateInfo viewInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = format,
                .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            };
            DUCK_VK_CHECK(vkCreateImageView(context.Device(), &viewInfo, nullptr, &imageViews[i]));
        }

        Log::Info("Swapchain: {}x{}, {} images, format {}", extent.width, extent.height, actualCount,
                  static_cast<int>(format));
    }

    void Swapchain::DestroyImageViews()
    {
        for (VkImageView view : imageViews)
        {
            vkDestroyImageView(context.Device(), view, nullptr);
        }
        imageViews.clear();
    }
}
