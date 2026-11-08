#include <Duck/VkCore/CpuFramebufferPresenter.h>

#include <Duck/Core/Assert.h>
#include <Duck/Core/Cast.h>
#include <Duck/VkCore/VkCheck.h>
#include <Duck/VkCore/VulkanContext.h>

#include <vk_mem_alloc.h>

#include <algorithm>
#include <cstring>

namespace Duck::Vk
{
    namespace
    {
        constexpr VkFormat CpuImageFormat = VK_FORMAT_R8G8B8A8_UNORM; // matches rgba8

        constexpr VkImageSubresourceRange ColorRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        constexpr VkImageSubresourceLayers ColorLayers{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};

        // same style as FrameContext submt, discards old contents when 'from' is UNDEFINED.
        void Transition(VkCommandBuffer commands, VkImage image, VkImageLayout from, VkImageLayout to)
        {
            const VkImageMemoryBarrier2 barrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
                .oldLayout = from,
                .newLayout = to,
                .image = image,
                .subresourceRange = ColorRange,
            };
            const VkDependencyInfo dependency{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                              .imageMemoryBarrierCount = 1,
                                              .pImageMemoryBarriers = &barrier};
            vkCmdPipelineBarrier2(commands, &dependency);
        }
    }

    CpuFramebufferPresenter::CpuFramebufferPresenter(VulkanContext& context) : context(context) {}

    CpuFramebufferPresenter::~CpuFramebufferPresenter()
    {
        vkDeviceWaitIdle(context.Device());
        for (Slot& slot : slots)
        {
            DestroySlot(slot);
        }
    }

    void CpuFramebufferPresenter::Present(const FrameInfo& frame, const Framebuffer& framebuffer)
    {
        DUCK_ASSERT(!framebuffer.Empty(), "Cannot present an empty framebuffer");

        Slot& slot = slots[frame.FrameIndex];
        EnsureSlot(slot, framebuffer.Width(), framebuffer.Height());

        // host write, flushing is a no-op on coherent memory, and required on noncoherent memory
        // queue submission makes the write visible to the GPU
        std::memcpy(slot.StagingMapped, framebuffer.Data(), framebuffer.SizeBytes());
        vmaFlushAllocation(context.Allocator(), slot.StagingAllocation, 0, VK_WHOLE_SIZE);

        VkCommandBuffer commands = frame.Commands;

        // staging buffer -> image
        Transition(commands, slot.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        const VkBufferImageCopy copy{.imageSubresource = ColorLayers,
                                     .imageExtent = {CheckedCast<std::uint32_t>(framebuffer.Width()),
                                                     CheckedCast<std::uint32_t>(framebuffer.Height()), 1}};
        vkCmdCopyBufferToImage(commands, slot.StagingBuffer, slot.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &copy);
        Transition(commands, slot.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        // black background, then image scaled into the largest centered rect that keeps the aspect ratio
        Transition(commands, frame.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        const VkClearColorValue black{{0.0f, 0.0f, 0.0f, 1.0f}};
        vkCmdClearColorImage(commands, frame.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &black, 1, &ColorRange);

        const int srcWidth = framebuffer.Width();
        const int srcHeight = framebuffer.Height();
        const int extentWidth = CheckedCast<int>(frame.Extent.width);
        const int extentHeight = CheckedCast<int>(frame.Extent.height);

        const float scale = std::min(static_cast<float>(extentWidth) / static_cast<float>(srcWidth),
                                     static_cast<float>(extentHeight) / static_cast<float>(srcHeight));
        const int width = std::max(1, static_cast<int>(static_cast<float>(srcWidth) * scale));
        const int height = std::max(1, static_cast<int>(static_cast<float>(srcHeight) * scale));
        const int x = (extentWidth - width) / 2;
        const int y = (extentHeight - height) / 2;

        const VkImageBlit blit{
            .srcSubresource = ColorLayers,
            .srcOffsets = {{0, 0, 0}, {srcWidth, srcHeight, 1}},
            .dstSubresource = ColorLayers,
            .dstOffsets = {{x, y, 0}, {x + width, y + height, 1}},
        };
        vkCmdBlitImage(commands, slot.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, frame.Image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, filter);

        Transition(commands, frame.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    void CpuFramebufferPresenter::EnsureSlot(Slot& slot, int width, int height)
    {
        if (slot.Width == width && slot.Height == height && slot.Image != VK_NULL_HANDLE)
        {
            return;
        }

        // safe since FrameContext waited on this slots fence so GPU is done with these resources
        DestroySlot(slot);

        const std::uint32_t w = CheckedCast<std::uint32_t>(width);
        const std::uint32_t h = CheckedCast<std::uint32_t>(height);

        const VmaAllocator allocator = context.Allocator();

        const VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                            .size = static_cast<VkDeviceSize>(w) * h * sizeof(Rgba8),
                                            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
        VmaAllocationCreateInfo bufferAlloc{};
        bufferAlloc.usage = VMA_MEMORY_USAGE_AUTO;
        bufferAlloc.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo allocationInfo{};
        DUCK_VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &bufferAlloc, &slot.StagingBuffer,
                                      &slot.StagingAllocation, &allocationInfo));
        slot.StagingMapped = allocationInfo.pMappedData;

        const VkImageCreateInfo imageInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                                          .imageType = VK_IMAGE_TYPE_2D,
                                          .format = CpuImageFormat,
                                          .extent = {w, h, 1},
                                          .mipLevels = 1,
                                          .arrayLayers = 1,
                                          .samples = VK_SAMPLE_COUNT_1_BIT,
                                          .tiling = VK_IMAGE_TILING_OPTIMAL,
                                          .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
        VmaAllocationCreateInfo imageAlloc{};
        imageAlloc.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        DUCK_VK_CHECK(vmaCreateImage(allocator, &imageInfo, &imageAlloc, &slot.Image, &slot.ImageAllocation, nullptr));

        slot.Width = width;
        slot.Height = height;
    }

    void CpuFramebufferPresenter::DestroySlot(Slot& slot)
    {
        const VmaAllocator allocator = context.Allocator();
        if (slot.Image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(allocator, slot.Image, slot.ImageAllocation);
        }
        if (slot.StagingBuffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(allocator, slot.StagingBuffer, slot.StagingAllocation);
        }
        slot = {};
    }
}