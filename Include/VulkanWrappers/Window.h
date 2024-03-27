#ifndef WINDOW
#define WINDOW

#if __APPLE__
    // For special molten-vk extension.
    #define VK_USE_PLATFORM_MACOS_MVK
#endif

#include <vulkan/vulkan.h>

#include <array>

struct GLFWwindow;

namespace VulkanWrappers
{
    class Device;

    // Not too many FIF to hide latency. 
    #define NUM_FRAMES_IN_FLIGHT 2u

    struct Frame
    {
        VkCommandBuffer commandBuffer;
        VkImage         backBuffer;
        VkImageView     backBufferView;
    };

    class Window
    {
    public:
        Window(const char* name, uint32_t width, uint32_t height);
        ~Window();

        bool NextFrame(const Device* device, Frame* frame);
        void SubmitFrame(const Device* device, const Frame* frame);

        void CreateVulkanSurface   (const Device* device);
        void CreateVulkanSwapchain (const Device* device);
        void ReleaseVulkanObjects  (const Device* device);
        
        inline VkSurfaceKHR   GetVulkanSurface()   const { return m_VKSurface;   };
        inline VkSwapchainKHR GetVulkanSwapchain() const { return m_VKSwapchain; };

    private:
        uint16_t m_Width;
        uint16_t m_Height;

        GLFWwindow* m_GLFWWindow;

        VkSurfaceKHR       m_VKSurface;
        VkSurfaceFormatKHR m_VKSurfaceFormat;
        VkExtent2D         m_VKSurfaceExtent;
        VkSwapchainKHR     m_VKSwapchain;
        uint32_t           m_VKSwapchainImageCount;
        uint32_t           m_VKSwapchainImageIndex;

        uint64_t                                m_FrameCount;
        uint32_t                                m_FrameIndex;
        std::array<Frame, NUM_FRAMES_IN_FLIGHT> m_Frames;

        // Synchronization Primitives 
        // (these could be merged into Frame but not really useful outside of the render loop). 
        std::array<VkFence,     NUM_FRAMES_IN_FLIGHT> m_GraphicsQueueCompleteFences;
        std::array<VkSemaphore, NUM_FRAMES_IN_FLIGHT> m_GraphicsQueueCompleteSemaphores;  
        std::array<VkSemaphore, NUM_FRAMES_IN_FLIGHT> m_ImageAcquireSemaphores;
    };
}

#endif//WINDOW