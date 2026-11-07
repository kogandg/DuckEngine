#pragma once

#include <volk.h>

#include <cstdint>

struct VmaAllocator_T;

namespace Duck
{
    class Window;

    struct VulkanContextDesc
    {
        const char* AppName = "DuckEngine";
        bool EnableValidation = true;
    };

    // owns instance, surface, device, queue, and memory allocator
    class VulkanContext
    {
    public:
        explicit VulkanContext(Window& window, const VulkanContextDesc& desc = {});
        ~VulkanContext();

        VulkanContext(const VulkanContext&) = delete;
        VulkanContext& operator=(const VulkanContext&) = delete;
        VulkanContext(VulkanContext&&) = delete;
        VulkanContext& operator=(VulkanContext&&) = delete;

        VkInstance Instance() const noexcept { return this->instance; }
        VkSurfaceKHR Surface() const noexcept { return this->surface; }
        VkPhysicalDevice PhysicalDevice() const noexcept { return this->physicalDevice; }
        VkDevice Device() const noexcept { return this->device; }
        VkQueue Queue() const noexcept { return this->queue; }
        std::uint32_t QueueFamily() const noexcept { return this->queueFamily; }
        VmaAllocator_T* Allocator() const noexcept { return this->allocator; }
        const VkPhysicalDeviceProperties& DeviceProperties() const noexcept { return this->deviceProperties; }

    private:
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties deviceProperties{};
        VkDevice device = VK_NULL_HANDLE;
        VkQueue queue = VK_NULL_HANDLE;
        std::uint32_t queueFamily = 0;
        VmaAllocator_T* allocator = nullptr;
    };
}