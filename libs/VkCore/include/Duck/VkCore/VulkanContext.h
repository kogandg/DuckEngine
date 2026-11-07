#pragma once

#include <volk.h>

#include <cstdint>

struct VmaAllocator_T;

namespace Duck
{
    using std::uint32_t;

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

        VkInstance Instance() const noexcept { return instance; }
        VkSurfaceKHR Surface() const noexcept { return surface; }
        VkPhysicalDevice PhysicalDevice() const noexcept { return physicalDevice; }
        VkDevice Device() const noexcept { return device; }
        VkQueue Queue() const noexcept { return queue; }
        uint32_t QueueFamily() const noexcept { return queueFamily; }
        VmaAllocator_T* Allocator() const noexcept { return allocator; }
        const VkPhysicalDeviceProperties& DeviceProperties() const noexcept { return deviceProperties; }

    private:
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties deviceProperties{};
        VkDevice device = VK_NULL_HANDLE;
        VkQueue queue = VK_NULL_HANDLE;
        uint32_t queueFamily = 0;
        VmaAllocator_T* allocator = nullptr;
    };
}