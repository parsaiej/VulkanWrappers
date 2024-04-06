#include <VulkanWrappers/Window.h>
#include <VulkanWrappers/Device.h>

#if __APPLE__
    // Native-access to macOS cocoa window.
    #define GLFW_EXPOSE_NATIVE_COCOA
    
    // Objective-C wrapper to bind a KHR surface to native macOS window. 
    #include "MetalUtility.h"
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

    VkSwapchainCreateInfoKHR createInfo =
    {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = m_VKSurface,
        .minImageCount         = imageCount,
        .imageExtent           = m_VKSurfaceExtent,
        .imageArrayLayers      = 1,
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform          = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = ChooseSwapPresentMode(swapChainSupport),
        .clipped               = VK_TRUE,
        .oldSwapchain          = VK_NULL_HANDLE,
        .imageFormat           = m_VKSurfaceFormat.format,
        .imageColorSpace       = m_VKSurfaceFormat.colorSpace,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = NULL
    };

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

        VkImageViewCreateInfo backBufferViewInfo =
        {
            .sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image        = m_Frames[i].backBuffer,
            .viewType     = VK_IMAGE_VIEW_TYPE_2D,
            .format       = m_VKSurfaceFormat.format,

            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,

            .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel   = 0,
            .subresourceRange.levelCount     = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount     = 1
        };

        if (vkCreateImageView(device->GetLogical(), &backBufferViewInfo, NULL, &m_Frames[i].backBufferView) != VK_SUCCESS)
            throw std::runtime_error("failed to create swap chain image view.");
    }

    for (size_t i = 0; i < NUM_FRAMES_IN_FLIGHT; ++i)
    {
        // Synchronization primitives (graphics).

        VkSemaphoreCreateInfo binarySemaphoreInfo
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        vkCreateSemaphore(device->GetLogical(), &binarySemaphoreInfo, nullptr, &m_GraphicsQueueCompleteSemaphores[i]);
        vkCreateSemaphore(device->GetLogical(), &binarySemaphoreInfo, nullptr, &m_ImageAcquireSemaphores[i]);

        VkFenceCreateInfo fenceInfo
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            .pNext = nullptr
        };

        vkCreateFence(device->GetLogical(), &fenceInfo, nullptr, &m_GraphicsQueueCompleteFences[i]);

        // Command buffer.

        VkCommandBufferAllocateInfo commandAllocateInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = device->GetCommandPool(),
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        if (vkAllocateCommandBuffers(device->GetLogical(), &commandAllocateInfo, &m_VKCommandBuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate command buffer.");
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
    VkCommandBufferBeginInfo commandBegin
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags            = 0,
        .pInheritanceInfo = nullptr
    };

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

    VkSubmitInfo submitInfo
    {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount   = 1u,
        .pCommandBuffers      = &frame->commandBuffer,
        .waitSemaphoreCount   = 1u,
        .pWaitSemaphores      = &m_ImageAcquireSemaphores[m_FrameIndex],
        .pWaitDstStageMask    = backBufferWaitStage,
        .signalSemaphoreCount = 1u,
        .pSignalSemaphores    = &m_GraphicsQueueCompleteSemaphores[m_FrameIndex]
    };

    // Submit the graphics queue and signal both the presentation semaphore and the next frame's fence when done. 
    vkQueueSubmit(device->GetGraphicsQueue(), 1u, &submitInfo, m_GraphicsQueueCompleteFences[m_FrameIndex]);

    // Present.

    VkPresentInfoKHR presentInfo
    {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount     = 1u,
        .pSwapchains        = &m_VKSwapchain,
        .pImageIndices      = &m_VKSwapchainImageIndex,
        .pResults           = nullptr,
        .waitSemaphoreCount = 1u,
        .pWaitSemaphores    = &m_GraphicsQueueCompleteSemaphores[m_FrameIndex]
    };

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