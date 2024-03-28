#ifndef DEVICE
#define DEVICE

#if __APPLE__
    // For special molten-vk extension.
    #define VK_USE_PLATFORM_MACOS_MVK
#endif

#include <vulkan/vulkan.h>

// Extension Functions
// -----------------------

namespace VulkanWrappers
{
    class Window;

    class Device
    {
    public:

        Device(Window* window);
        Device() : Device(nullptr) {};

        inline VkInstance GetInstance()       const { return m_VKInstance;       }
        inline VkPhysicalDevice GetPhysical() const { return m_VKDevicePhysical; }
        inline VkDevice GetLogical()          const { return m_VKDeviceLogical;  }
        inline VkCommandPool GetCommandPool() const { return m_VKCommandPool;    }
        inline VkQueue GetGraphicsQueue()     const { return m_VKQueueGraphics;  }
        inline VkQueue GetPresentQueue()      const { return m_VKQueuePresent;   }

        // Extension shortcuts
        inline void BeginRendering(VkCommandBuffer commandBuffer, const VkRenderingInfo* renderingInfo) { m_VKCmdBeginRenderingKHR(commandBuffer, renderingInfo); }
        inline void EndRendering(VkCommandBuffer commandBuffer) { m_VKCmdEndRenderingKHR(commandBuffer); }
        inline void Barrier(VkCommandBuffer commandBuffer, const VkDependencyInfoKHR* dependencyInfo) { m_VKCmdBarrierKHR(commandBuffer, dependencyInfo); }

        ~Device();

    private:
        VkInstance       m_VKInstance;
        VkPhysicalDevice m_VKDevicePhysical;
        VkDevice         m_VKDeviceLogical;
        VkCommandPool    m_VKCommandPool;

        // Window Handle
        Window* m_Window;

        // Graphics Queue
        VkQueue  m_VKQueueGraphics;
        uint32_t m_VKQueueGraphicsIndex;

        // Present Queue
        VkQueue  m_VKQueuePresent;
        uint32_t m_VKQueuePresentIndex;

        // Extension Functions
        PFN_vkCmdBeginRenderingKHR   m_VKCmdBeginRenderingKHR;
        PFN_vkCmdEndRenderingKHR     m_VKCmdEndRenderingKHR;
        PFN_vkCmdPipelineBarrier2KHR m_VKCmdBarrierKHR;
    };
}

#endif//DEVICE