#ifndef DEVICE
#define DEVICE

#if __APPLE__
    // For special molten-vk extension.
    #define VK_USE_PLATFORM_MACOS_MVK
#endif

#include <vulkan/vulkan.h>

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
    };
}

#endif//DEVICE