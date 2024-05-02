#ifndef DEVICE
#define DEVICE

#if __APPLE__
    // For special molten-vk extension.
    #define VK_USE_PLATFORM_MACOS_MVK
#endif

#include <vulkan/vulkan.h>
#include <VulkanWrappers/VmaUsage.h>
#include <vector>
#include "stdexcept"
// Extension Functions
// -----------------------

namespace VulkanWrappers
{
    class Window;
    class Shader;
    class Buffer;
    class Image;

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
        inline VmaAllocator GetAllocator()    const { return m_VMAAllocator;     }

        static void SetDefaultRenderState(VkCommandBuffer VkCommandBuffer);

        inline void CreateCommandBuffer(VkCommandBuffer* commandBuffer, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const
        { 
            VkCommandBufferAllocateInfo commandAllocateInfo = {};
            commandAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandAllocateInfo.commandPool        = m_VKCommandPool;
            commandAllocateInfo.level              = level;
            commandAllocateInfo.commandBufferCount = 1;

            if (vkAllocateCommandBuffers(m_VKDeviceLogical, &commandAllocateInfo, commandBuffer) != VK_SUCCESS)
                throw std::runtime_error("failed to allocate command buffer.");
        }

        // Utility
        void CreateShaders  (const std::vector<Shader*>& shaders);
        void ReleaseShaders (const std::vector<Shader*>& shaders);

        void CreateBuffers  (const std::vector<Buffer*>& buffers);
        void ReleaseBuffers (const std::vector<Buffer*>& buffers);

        void CreateImages  (const std::vector<Image*>& images);
        void ReleaseImages (const std::vector<Image*>& images);

        inline Window* GetWindow() { return m_Window; }
        
        ~Device();

        // Extension Functions
        #define VK_FUNC_MEMBER(func) static PFN_##func func

        VK_FUNC_MEMBER(vkCreateShadersEXT);
        VK_FUNC_MEMBER(vkDestroyShaderEXT);
        VK_FUNC_MEMBER(vkCmdBindShadersEXT);
        VK_FUNC_MEMBER(vkCmdSetRasterizationSamplesEXT);
        VK_FUNC_MEMBER(vkCmdSetSampleMaskEXT);
        VK_FUNC_MEMBER(vkCmdSetAlphaToCoverageEnableEXT);
        VK_FUNC_MEMBER(vkCmdSetPolygonModeEXT);
        VK_FUNC_MEMBER(vkCmdSetColorBlendEnableEXT);
        VK_FUNC_MEMBER(vkCmdSetColorWriteMaskEXT);
        VK_FUNC_MEMBER(vkCmdSetColorBlendEquationEXT);

    private:
        VkInstance       m_VKInstance;
        VkPhysicalDevice m_VKDevicePhysical;
        VkDevice         m_VKDeviceLogical;
        VkCommandPool    m_VKCommandPool;

        // Memory Allocation (VMA)
        VmaAllocator m_VMAAllocator;

        // Window Handle
        Window* m_Window;

        // Graphics Queue
        VkQueue  m_VKQueueGraphics;
        uint32_t m_VKQueueGraphicsIndex;

        // Present Queue
        VkQueue  m_VKQueuePresent;
        uint32_t m_VKQueuePresentIndex;
    };
}

#endif//DEVICE