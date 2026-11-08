#pragma once
#include <Duck/Core/Framebuffer.h>
#include <Duck/VkCore/FrameContext.h>

#include <volk.h>

#include <array>
#include <cstddef>
#include <cstdint>

struct VmaAllocation_T;

namespace Duck::Vk
{
    class VulkanContext;

    class CpuFramebufferPresenter
    {
    public:
        explicit CpuFramebufferPresenter(VulkanContext& context);
        ~CpuFramebufferPresenter();

        CpuFramebufferPresenter(const CpuFramebufferPresenter&) = delete;
        CpuFramebufferPresenter& operator=(const CpuFramebufferPresenter&) = delete;
        CpuFramebufferPresenter(CpuFramebufferPresenter&&) = delete;
        CpuFramebufferPresenter& operator=(CpuFramebufferPresenter&&) = delete;

        // records the upload and the blit into frame.Commands, the framebuffer is copied immediately so the caller may
        // modify it as soon as this returns
        void Present(const FrameInfo& frame, const Framebuffer& framebuffer);

        // Nearest, default, keeps pixels sharp, linear smooths when scaling.
        void SetLinearFiltering(bool linear) noexcept { this->filter = linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST; }

    private:
        struct Slot
        {
            VkBuffer StagingBuffer = VK_NULL_HANDLE;
            VmaAllocation_T* StagingAllocation = nullptr;
            void* StagingMapped = nullptr;

            VkImage Image = VK_NULL_HANDLE;
            VmaAllocation_T* ImageAllocation = nullptr;

            int Width = 0;
            int Height = 0;
        };

        void EnsureSlot(Slot& slot, int width, int height);
        void DestroySlot(Slot& slot);

        VulkanContext& context;
        std::array<Slot, MaxFramesInFlight> slots;
        VkFilter filter = VK_FILTER_NEAREST;
    };

}