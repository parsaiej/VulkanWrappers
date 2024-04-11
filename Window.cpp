#include <VulkanWrappers/Window.h>
#include <VulkanWrappers/Device.h>

#include "algorithm"
#if __APPLE__
    // Native-access to macOS cocoa window.
    #define GLFW_EXPOSE_NATIVE_COCOA
    
    // Objective-C wrapper to bind a KHR surface to native macOS window. 
    #include "MetalUtility.h"
#else
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <vulkan/vulkan_win32.h>
#endif

#include <GLFW/glfw3.h>
// In case of macOS, will include the cocoa native implementation.
#include <GLFW/glfw3native.h>

#include <iostream>

using namespace VulkanWrappers;

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);

    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);

    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());

    return details;
}

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(SwapChainSupportDetails swapChainInfo)
{
    for (auto format : swapChainInfo.formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return swapChainInfo.formats[0];
}

VkPresentModeKHR ChooseSwapPresentMode(SwapChainSupportDetails swapChainInfo)
{
    for (auto presentMode : swapChainInfo.presentModes)
    {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return presentMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR* capabilities, GLFWwindow* window)
{
    if ((*capabilities).currentExtent.width != 0xFFFFFFFFu)
        return (*capabilities).currentExtent;
    else 
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = 
        {
            (uint32_t)(width),
            (uint32_t)(height)
        };

        actualExtent.width  = std::clamp(actualExtent.width,  (*capabilities).minImageExtent.width,  (*capabilities).maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, (*capabilities).minImageExtent.height, (*capabilities).maxImageExtent.height);

        return actualExtent;
    }
}

Window::Window(const char* name, uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height), m_FrameCount(0), m_FrameIndex(0)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,  GLFW_FALSE);
    
    m_GLFWWindow = glfwCreateWindow(m_Width, m_Height, name, nullptr, nullptr);
}

Window::~Window()
{
    glfwDestroyWindow(m_GLFWWindow);
    glfwTerminate();
}

void Window::CreateVulkanSurface(const Device* device)
{
    // Create an OS-compatible Surface. 
    // ----------------------
    
#if __APPLE__
    {  
        // macOS-compatibility
        // ----------------------
        
        VkMacOSSurfaceCreateInfoMVK createInfo =
        {
            .sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
            .pNext = nullptr,
            .flags = 0,
            .pView = GetMetalLayer(m_GLFWWindow)
        };

        PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK;
        vkCreateMacOSSurfaceMVK = (PFN_vkCreateMacOSSurfaceMVK)vkGetInstanceProcAddr(device->GetInstance(), "vkCreateMacOSSurfaceMVK");

        if (!vkCreateMacOSSurfaceMVK) 
        {
            // Note: This call will fail unless we add VK_MVK_MACOS_SURFACE_EXTENSION_NAME extension. 
            throw std::runtime_error("Unabled to get pointer to function: vkCreateMacOSSurfaceMVK");
        }

        if (vkCreateMacOSSurfaceMVK(device->GetInstance(), &createInfo, nullptr, &m_VKSurface)!= VK_SUCCESS) 
            throw std::runtime_error("failed to create surface.");
    }
#else
	VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.hwnd = glfwGetWin32Window(m_GLFWWindow);
    // .pView = (m_GLFWWindow)
        
    PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
    
    vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR )vkGetInstanceProcAddr(device->GetInstance(), "vkCreateWin32SurfaceKHR");

    if (!vkCreateWin32SurfaceKHR) 
    {
        // Note: This call will fail unless we add VK_MVK_MACOS_SURFACE_EXTENSION_NAME extension. 
        throw std::runtime_error("Unabled to get pointer to function: VkWin32SurfaceCreateInfoKHR");
    }

    if (vkCreateWin32SurfaceKHR(device->GetInstance(), &createInfo, nullptr, &m_VKSurface)!= VK_SUCCESS) 
        throw std::runtime_error("failed to create surface.");
#endif

}

void Window::CreateVulkanSwapchain(const Device* device)
{
    // Create Swapchain
    // ----------------------

    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device->GetPhysical(), m_VKSurface);

    // Store the format, extent, scissor, viewport
    m_VKSurfaceFormat   = ChooseSwapSurfaceFormat(swapChainSupport);
    m_VKSurfaceExtent   = ChooseSwapExtent(&swapChainSupport.capabilities, NULL);
    m_VKSurfaceViewport = { 0, 0, (float)m_Width, (float)m_Height, 0.0, 1.0 };
    m_VKSurfaceScissor  = { 0, 0, m_VKSurfaceExtent };

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface               = m_VKSurface;
    createInfo.minImageCount         = imageCount;
    createInfo.imageExtent           = m_VKSurfaceExtent;
    createInfo.imageArrayLayers      = 1;
    createInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.preTransform          = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode           = ChooseSwapPresentMode(swapChainSupport);
    createInfo.clipped               = VK_TRUE;
    createInfo.oldSwapchain          = VK_NULL_HANDLE;
    createInfo.imageFormat           = m_VKSurfaceFormat.format;
    createInfo.imageColorSpace       = m_VKSurfaceFormat.colorSpace;
    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices   = NULL;

    if(vkCreateSwapchainKHR(device->GetLogical(), &createInfo, NULL, &m_VKSwapchain) != VK_SUCCESS)
        throw std::runtime_error("failed to create swap chain.");

    // Fetch image count.
    vkGetSwapchainImagesKHR(device->GetLogical(), m_VKSwapchain, &m_VKSwapchainImageCount, nullptr);

    std::vector<VkImage> swapChainImages(m_VKSwapchainImageCount);
    vkGetSwapchainImagesKHR(device->GetLogical(), m_VKSwapchain, &m_VKSwapchainImageCount, swapChainImages.data());

    m_Frames.resize(m_VKSwapchainImageCount);

    // Create frames.
    for (size_t i = 0; i < m_VKSwapchainImageCount; ++i)
    {
        // Swapchain image. 

        m_Frames[i].backBuffer = swapChainImages[i];

        // Swapchain image view.

        VkImageViewCreateInfo backBufferViewInfo = {};
        backBufferViewInfo.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        backBufferViewInfo.image        = m_Frames[i].backBuffer;
        backBufferViewInfo.viewType     = VK_IMAGE_VIEW_TYPE_2D;
        backBufferViewInfo.format       = m_VKSurfaceFormat.format;

        backBufferViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        backBufferViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        backBufferViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        backBufferViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        backBufferViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        backBufferViewInfo.subresourceRange.baseMipLevel   = 0;
        backBufferViewInfo.subresourceRange.levelCount     = 1;
        backBufferViewInfo.subresourceRange.baseArrayLayer = 0;
        backBufferViewInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(device->GetLogical(), &backBufferViewInfo, NULL, &m_Frames[i].backBufferView) != VK_SUCCESS)
            throw std::runtime_error("failed to create swap chain image view.");
    }

    for (size_t i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
    {
        // Synchronization primitives (graphics).

        VkSemaphoreCreateInfo binarySemaphoreInfo = {};
        binarySemaphoreInfo .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        vkCreateSemaphore(device->GetLogical(), &binarySemaphoreInfo, nullptr, &m_GraphicsQueueCompleteSemaphores[i]);
        vkCreateSemaphore(device->GetLogical(), &binarySemaphoreInfo, nullptr, &m_ImageAcquireSemaphores[i]);

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo .flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fenceInfo.pNext = nullptr;

        vkCreateFence(device->GetLogical(), &fenceInfo, nullptr, &m_GraphicsQueueCompleteFences[i]);

        // Command buffer.
        device->CreateCommandBuffer(&m_VKCommandBuffers[i]);
    }
}

bool Window::NextFrame(const Device* device, Frame* frame)
{
    if (glfwWindowShouldClose(m_GLFWWindow))
        return false;

    glfwPollEvents();

    // Pause thread until graphics queue finished processing. 
    vkWaitForFences(device->GetLogical(), 1u, &m_GraphicsQueueCompleteFences[m_FrameIndex], VK_TRUE, UINT64_MAX);

    // Reset the fence for this frame.
    vkResetFences(device->GetLogical(), 1u, &m_GraphicsQueueCompleteFences[m_FrameIndex]);

    // Grab the next image in the swap chain and signal the current semaphore when it can be drawn to. 
    vkAcquireNextImageKHR(device->GetLogical(), m_VKSwapchain, UINT64_MAX, m_ImageAcquireSemaphores[m_FrameIndex], VK_NULL_HANDLE, &m_VKSwapchainImageIndex);

    *frame = m_Frames[m_VKSwapchainImageIndex];
    
    // Attach the command buffer for this frame
    frame->commandBuffer = m_VKCommandBuffers[m_FrameIndex];

    // Reset the command buffer for this frame.
    vkResetCommandBuffer(frame->commandBuffer, 0x0);

    // Enable the command buffer into a recording state. 
    VkCommandBufferBeginInfo commandBegin = {};
    commandBegin.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBegin.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    commandBegin.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(frame->commandBuffer, &commandBegin);

    return true;
}

void Window::SubmitFrame(Device* device, const Frame* frame)
{
    // Conclude command buffer recording.
    vkEndCommandBuffer(frame->commandBuffer);

    VkPipelineStageFlags backBufferWaitStage[] = 
    {
        // Backbuffer can be written to once it is in this stage. 
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo submitInfo = {};

    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount   = 1u;
    submitInfo.pCommandBuffers      = &frame->commandBuffer;
    submitInfo.waitSemaphoreCount   = 1u;
    submitInfo.pWaitSemaphores      = &m_ImageAcquireSemaphores[m_FrameIndex];
    submitInfo.pWaitDstStageMask    = backBufferWaitStage;
    submitInfo.signalSemaphoreCount = 1u;
    submitInfo.pSignalSemaphores    = &m_GraphicsQueueCompleteSemaphores[m_FrameIndex];

    // Submit the graphics queue and signal both the presentation semaphore and the next frame's fence when done. 
    vkQueueSubmit(device->GetGraphicsQueue(), 1u, &submitInfo, m_GraphicsQueueCompleteFences[m_FrameIndex]);

    // Present.

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount     = 1u;
    presentInfo.pSwapchains        = &m_VKSwapchain;
    presentInfo.pImageIndices      = &m_VKSwapchainImageIndex;
    presentInfo.pResults           = nullptr;
    presentInfo.waitSemaphoreCount = 1u;
    presentInfo.pWaitSemaphores    = &m_GraphicsQueueCompleteSemaphores[m_FrameIndex];

    vkQueuePresentKHR(device->GetPresentQueue(), &presentInfo);

    // Compute next frame Index.
    m_FrameIndex = (m_FrameIndex + 1) % NUM_FRAMES_IN_FLIGHT;
}

void Window::ReleaseVulkanObjects(const Device* device)
{
    for (auto& frame : m_Frames)
        vkDestroyImageView(device->GetLogical(), frame.backBufferView, nullptr);

    for (auto& fence : m_GraphicsQueueCompleteFences)
        vkDestroyFence(device->GetLogical(), fence, nullptr);

    for (auto& semaphore : m_GraphicsQueueCompleteSemaphores)
        vkDestroySemaphore(device->GetLogical(), semaphore, nullptr);

    for (auto& semaphore : m_ImageAcquireSemaphores)
        vkDestroySemaphore(device->GetLogical(), semaphore, nullptr);

    if (m_VKSwapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(device->GetLogical(), m_VKSwapchain, nullptr);

    if (m_VKSurface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(device->GetInstance(), m_VKSurface, nullptr);
}