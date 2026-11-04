duck_library(VkCore
  DEPS Duck::Core Duck::Platform Vulkan::Headers volk::volk GPUOpen::VulkanMemoryAllocator)

target_compile_definitions(Duck_VkCore PUBLIC
  VK_NO_PROTOTYPES
  VMA_STATIC_VULKAN_FUNCTIONS=0
  VMA_DYNAMIC_VULKAN_FUNCTIONS=1)