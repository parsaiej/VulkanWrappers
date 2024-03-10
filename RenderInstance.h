#ifndef RENDER_INSTANCE
#define RENDER_INSTANCE

/*
    GLFW / Vulkan wrapper for consolidation of Window, Device (Logical / Physical), and Swapchain creation
    for simple demo-style applications that draw to one non-resizing triple-buffered viewport. 
*/

#if __APPLE__
    // For special molten-vk extension.
    #define VK_USE_PLATFORM_MACOS_MVK
#endif

#include <vulkan/vulkan.h>

#include <vector>
#include <functional>

// Forward-declare GLFW
struct GLFWwindow;

namespace Wrappers
{
    struct RenderContext
    {
        // Contextual info
        VkCommandBuffer commandBuffer;
        VkImageView     backBufferView;
        VkRect2D        backBufferScissor;
        VkViewport      backBufferViewport;

        // Unfortunately need to pass the function pointers due to extension
        PFN_vkCmdBeginRenderingKHR renderBegin;
        PFN_vkCmdEndRenderingKHR   renderEnd;
    };

    struct Frame
    {
        VkImage         backBuffer;
        VkImageView     backBufferView;
        VkCommandBuffer commandBuffer;
        VkSemaphore     semaphoreWait;
        VkSemaphore     semaphoreSignal;
        VkFence         fenceWait;
    };

    class RenderInstance
    {
    public:

        // Swapchain, OS window, device, instance primitive creation. 
        RenderInstance(int width, int height);

        // Clean up all VK and GLFW objects. 
        ~RenderInstance();

        // Invoke the render-loop with a callback for filling out the current command buffer. 
        int Execute(std::function<void(RenderContext)>);

        void WaitForIdle() { vkDeviceWaitIdle(m_VKDevice); }

    private:

        GLFWwindow* m_GLFW;

        VkInstance       m_VKInstance;
        VkPhysicalDevice m_VKPhysicalDevice;
        VkDevice         m_VKDevice;
        VkSurfaceKHR     m_VKSurface;
        VkCommandPool    m_VKCommandPool;

        // Swapchain image resource / views and synchronizaation primitives. 
        VkSwapchainKHR   m_VKSwapchain;
        uint32_t         m_VKSwapchainImageCount;

        std::vector<Frame> m_Frames;

        VkViewport m_VKViewport;
        VkRect2D   m_VKScissor;

        uint32_t m_QueueFamilyIndexGraphics;
        uint32_t m_QueueFamilyIndexPresent;

        VkQueue m_VKGraphicsQueue;
        VkQueue m_VKPresentQueue;

	    PFN_vkCmdBeginRenderingKHR m_VKCmdBeginRenderingKHR;
	    PFN_vkCmdEndRenderingKHR   m_VKCmdEndRenderingKHR;

        uint32_t m_FrameIndex;
    };
}


#endif//RENDER_INSTANCE