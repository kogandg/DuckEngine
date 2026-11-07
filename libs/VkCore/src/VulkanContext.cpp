#include <Duck/VkCore/VulkanContext.h>

#include <Duck/Core/Assert.h>
#include <Duck/Core/Log.h>
#include <Duck/Platform/Window.h>
#include <Duck/VkCore/VkCheck.h>

#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

#include <algorithm>
#include <cstring>
#include <string_view>
#include <vector>

namespace Duck
{
    namespace
    {
        constexpr const char* ValidationLayerName = "VK_LAYER_KHRONOS_validation";

        VKAPI_ATTR VkBool32 VKAPI_CALL OnDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                      VkDebugUtilsMessageTypeFlagsEXT,
                                                      const VkDebugUtilsMessengerCallbackDataEXT* data, void*)
        {
            if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                Log::Error("[Vulkan] {}", data->pMessage);
            }
            else
            {
                Log::Warn("[Vulkan] {}", data->pMessage);
            }
            return VK_FALSE; // never abort the call that triggered the message
        }

        bool IsValidationAvailable()
        {
            std::uint32_t count = 0;
            vkEnumerateInstanceLayerProperties(&count, nullptr);
            std::vector<VkLayerProperties> layers(count);
            vkEnumerateInstanceLayerProperties(&count, layers.data());

            return std::ranges::any_of(layers, [](const VkLayerProperties& layer)
                                       { return std::string_view(layer.layerName) == ValidationLayerName; });
        }

        bool SupportsSwapchain(VkPhysicalDevice device)
        {
            std::uint32_t count = 0;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
            std::vector<VkExtensionProperties> extensions(count);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());

            return std::ranges::any_of(
                extensions, [](const VkExtensionProperties& extension)
                { return std::string_view(extension.extensionName) == VK_KHR_SWAPCHAIN_EXTENSION_NAME; });
        }

        bool SupportsRequiredFeatures(VkPhysicalDevice device)
        {
            VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
            VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                                                        .pNext = &features12};
            VkPhysicalDeviceFeatures2 features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                                               .pNext = &features13};
            vkGetPhysicalDeviceFeatures2(device, &features);

            return features13.dynamicRendering && features13.synchronization2 && features12.bufferDeviceAddress;
        }

        // single family that does graphics, compute, transfer, and present
        bool FindQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface, std::uint32_t& outFamily)
        {
            std::uint32_t count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
            std::vector<VkQueueFamilyProperties> families(count);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

            for (std::uint32_t i = 0; i < count; ++i)
            {
                VkBool32 canPresent = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &canPresent);

                constexpr VkQueueFlags required = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
                if ((families[i].queueFlags & required) == required && canPresent)
                {
                    outFamily = i;
                    return true;
                }
            }
            return false;
        }
    }

    VulkanContext::VulkanContext(Window& window, const VulkanContextDesc& desc)
    {
        DUCK_VK_CHECK(volkInitialize());
        DUCK_VERIFY(volkGetInstanceVersion() >= VK_API_VERSION_1_3, "The Vulkan loader does not support 1.3");

        // --- Instance ---
        std::uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        DUCK_VERIFY(glfwExtensions != nullptr, "GLFW cannot find a Vulkan loader");
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        std::vector<const char*> layers;
        const bool validation = desc.EnableValidation && IsValidationAvailable();
        if (desc.EnableValidation && !validation)
        {
            Log::Warn("Validation requested but {} is not installed (install the Vulkan SDK)", ValidationLayerName);
        }
        if (validation)
        {
            layers.push_back(ValidationLayerName);
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        const VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                        .pApplicationName = desc.AppName,
                                        .pEngineName = "DuckEngine",
                                        .apiVersion = VK_API_VERSION_1_3};
        const VkInstanceCreateInfo instanceInfo{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                                .pApplicationInfo = &appInfo,
                                                .enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
                                                .ppEnabledLayerNames = layers.data(),
                                                .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
                                                .ppEnabledExtensionNames = extensions.data()};
        DUCK_VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &this->instance));
        volkLoadInstance(this->instance);

        if (validation)
        {
            const VkDebugUtilsMessengerCreateInfoEXT messengerInfo{
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity =
                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = OnDebugMessage};
            DUCK_VK_CHECK(
                vkCreateDebugUtilsMessengerEXT(this->instance, &messengerInfo, nullptr, &this->debugMessenger));
        }

        // --- Surface ---
        DUCK_VK_CHECK(glfwCreateWindowSurface(this->instance, window.Handle(), nullptr, &this->surface));

        // --- Physical device: the best one that meets our requirements ---
        std::uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());

        int bestScore = -1;
        for (VkPhysicalDevice candidate : devices)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(candidate, &properties);

            const char* name = properties.deviceName;
            std::uint32_t family = 0;

            if (properties.apiVersion < VK_API_VERSION_1_3)
            {
                Log::Warn("Skipping {}: Vulkan {}.{} (need 1.3)", name, VK_API_VERSION_MAJOR(properties.apiVersion),
                          VK_API_VERSION_MINOR(properties.apiVersion));
                continue;
            }
            if (!SupportsSwapchain(candidate))
            {
                Log::Warn("Skipping {}: no swapchain support", name);
                continue;
            }
            if (!SupportsRequiredFeatures(candidate))
            {
                Log::Warn("Skipping {}: missing dynamic rendering, synchronization2, or buffer device address", name);
                continue;
            }
            if (!FindQueueFamily(candidate, this->surface, family))
            {
                Log::Warn("Skipping {}: no graphics+compute+present queue family", name);
                continue;
            }

            const int score = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 1000 : 0;
            if (score > bestScore)
            {
                bestScore = score;
                this->physicalDevice = candidate;
                this->deviceProperties = properties;
                this->queueFamily = family;
            }
        }
        DUCK_VERIFY(this->physicalDevice != VK_NULL_HANDLE, "No GPU supports Vulkan 1.4 with the required features");
        Log::Info("Using GPU: {} (Vulkan {}.{}.{})", this->deviceProperties.deviceName,
                  VK_API_VERSION_MAJOR(this->deviceProperties.apiVersion),
                  VK_API_VERSION_MINOR(this->deviceProperties.apiVersion),
                  VK_API_VERSION_PATCH(this->deviceProperties.apiVersion));

        // --- Logical device ---
        VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
                                                    .bufferDeviceAddress = VK_TRUE};
        VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                                                    .pNext = &features12,
                                                    .synchronization2 = VK_TRUE,
                                                    .dynamicRendering = VK_TRUE};

        const float queuePriority = 1.0f;
        const VkDeviceQueueCreateInfo queueInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                .queueFamilyIndex = this->queueFamily,
                                                .queueCount = 1,
                                                .pQueuePriorities = &queuePriority};
        const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        const VkDeviceCreateInfo deviceInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                            .pNext = &features13,
                                            .queueCreateInfoCount = 1,
                                            .pQueueCreateInfos = &queueInfo,
                                            .enabledExtensionCount = 1,
                                            .ppEnabledExtensionNames = deviceExtensions};
        DUCK_VK_CHECK(vkCreateDevice(this->physicalDevice, &deviceInfo, nullptr, &this->device));
        volkLoadDevice(this->device);
        vkGetDeviceQueue(this->device, this->queueFamily, 0, &this->queue);

        // --- VMA ---
        VmaVulkanFunctions functions{};
        functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        allocatorInfo.physicalDevice = this->physicalDevice;
        allocatorInfo.device = this->device;
        allocatorInfo.instance = this->instance;
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorInfo.pVulkanFunctions = &functions;
        DUCK_VK_CHECK(vmaCreateAllocator(&allocatorInfo, &this->allocator));
    }

    VulkanContext::~VulkanContext()
    {
        // clear in reverse order of creation
        vmaDestroyAllocator(this->allocator);
        vkDestroyDevice(this->device, nullptr);
        vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
        if (this->debugMessenger != VK_NULL_HANDLE)
        {
            vkDestroyDebugUtilsMessengerEXT(this->instance, this->debugMessenger, nullptr);
        }
        vkDestroyInstance(this->instance, nullptr);
    }
}