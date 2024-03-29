#ifndef DEVICE
#define DEVICE

#if __APPLE__
    // For special molten-vk extension.
    #define VK_USE_PLATFORM_MACOS_MVK
#endif

#include <vulkan/vulkan.h>
#include <VulkanWrappers/VmaUsage.h>
#include <vector>

// Extension Functions
// -----------------------

namespace VulkanWrappers
{
    class Window;
    class Shader;

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

        // Utility
        void CreateShaders  (const std::vector<Shader*>& shaders);
        void ReleaseShaders (const std::vector<Shader*>& shaders);
        
        ~Device();

        // Extension Functions
        #define VK_FUNC_MEMBER(func) static PFN_##func func

        VK_FUNC_MEMBER(vkCmdBeginRenderingKHR);
        VK_FUNC_MEMBER(vkCmdEndRenderingKHR);
        VK_FUNC_MEMBER(vkCmdPipelineBarrier2KHR);
        VK_FUNC_MEMBER(vkCreateShadersEXT);
        VK_FUNC_MEMBER(vkDestroyShaderEXT);
        VK_FUNC_MEMBER(vkCmdBindShadersEXT);
        VK_FUNC_MEMBER(vkCmdSetPrimitiveTopologyEXT);
        VK_FUNC_MEMBER(vkCmdSetColorWriteMaskEXT);
        VK_FUNC_MEMBER(vkCmdSetPrimitiveRestartEnableEXT);
        VK_FUNC_MEMBER(vkCmdSetColorBlendEnableEXT);
        VK_FUNC_MEMBER(vkCmdSetRasterizerDiscardEnableEXT);
        VK_FUNC_MEMBER(vkCmdSetAlphaToCoverageEnableEXT);
        VK_FUNC_MEMBER(vkCmdSetPolygonModeEXT);
        VK_FUNC_MEMBER(vkCmdSetStencilTestEnableEXT);
        VK_FUNC_MEMBER(vkCmdSetDepthTestEnableEXT);
        VK_FUNC_MEMBER(vkCmdSetColorBlendEquationEXT);
        VK_FUNC_MEMBER(vkCmdSetCullModeEXT);
        VK_FUNC_MEMBER(vkCmdSetDepthBiasEnableEXT);
        VK_FUNC_MEMBER(vkCmdSetDepthWriteEnableEXT);
        VK_FUNC_MEMBER(vkCmdSetFrontFaceEXT);
        VK_FUNC_MEMBER(vkCmdSetViewportWithCountEXT);
        VK_FUNC_MEMBER(vkCmdSetScissorWithCountEXT);
        VK_FUNC_MEMBER(vkCmdSetRasterizationSamplesEXT);
        VK_FUNC_MEMBER(vkCmdSetSampleMaskEXT);

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