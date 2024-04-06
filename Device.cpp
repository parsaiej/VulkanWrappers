#include <VulkanWrappers/Device.h>
#include <VulkanWrappers/Window.h>
#include <VulkanWrappers/Shader.h>
#include <VulkanWrappers/Buffer.h>

#include <GLFW/glfw3.h>
#include <vector>
#include <assert.h>

using namespace VulkanWrappers;

#define DECLARE_VK_FUNC(func) PFN_##func Device::func = nullptr;
#define GET_VK_FUNC(func) Device::func = reinterpret_cast<PFN_##func> (vkGetDeviceProcAddr(m_VKDeviceLogical, #func)); assert(func != nullptr);

DECLARE_VK_FUNC(vkCmdBeginRenderingKHR);
DECLARE_VK_FUNC(vkCmdEndRenderingKHR);
DECLARE_VK_FUNC(vkCmdPipelineBarrier2KHR);
DECLARE_VK_FUNC(vkCreateShadersEXT);
DECLARE_VK_FUNC(vkDestroyShaderEXT);
DECLARE_VK_FUNC(vkCmdBindShadersEXT);
DECLARE_VK_FUNC(vkCmdSetPrimitiveTopologyEXT);
DECLARE_VK_FUNC(vkCmdSetColorWriteMaskEXT);
DECLARE_VK_FUNC(vkCmdSetPrimitiveRestartEnableEXT);
DECLARE_VK_FUNC(vkCmdSetColorBlendEnableEXT);
DECLARE_VK_FUNC(vkCmdSetRasterizerDiscardEnableEXT);
DECLARE_VK_FUNC(vkCmdSetAlphaToCoverageEnableEXT);
DECLARE_VK_FUNC(vkCmdSetPolygonModeEXT);
DECLARE_VK_FUNC(vkCmdSetStencilTestEnableEXT);
DECLARE_VK_FUNC(vkCmdSetDepthTestEnableEXT);
DECLARE_VK_FUNC(vkCmdSetColorBlendEquationEXT);
DECLARE_VK_FUNC(vkCmdSetCullModeEXT);
DECLARE_VK_FUNC(vkCmdSetDepthBiasEnableEXT);
DECLARE_VK_FUNC(vkCmdSetDepthWriteEnableEXT);
DECLARE_VK_FUNC(vkCmdSetFrontFaceEXT);
DECLARE_VK_FUNC(vkCmdSetViewportWithCountEXT);
DECLARE_VK_FUNC(vkCmdSetScissorWithCountEXT);
DECLARE_VK_FUNC(vkCmdSetRasterizationSamplesEXT);
DECLARE_VK_FUNC(vkCmdSetSampleMaskEXT);

Device::Device(Window* window)
    : m_Window(window)
{
    // Create Vulkan Instance

    VkApplicationInfo appInfo =
    {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "Vulkan Instance",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_3
    };

    // Sample GLFW for any extensions it needs. 
    uint32_t extensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    std::vector<const char*> enabledInstanceExtensions;

    // Copy GLFW extensions. 
    for (uint32_t i = 0; i < extensionCount; ++i)
        enabledInstanceExtensions.push_back(glfwExtensions[i]);

#if __APPLE__
    // MoltenVK compatibility. 
    enabledInstanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    enabledInstanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif

    std::vector<const char*> enabledLayers;

#if ENABLE_VALIDATION_LAYERS
    enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif

#if __APPLE__
    // Required in order to create a logical device with shader object extension that doesn't natively support it.
    enabledLayers.push_back("VK_LAYER_KHRONOS_shader_object");
#endif

    VkInstanceCreateInfo instanceCreateInfo =
    {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &appInfo,
        .enabledExtensionCount   = (uint32_t)enabledInstanceExtensions.size(),
        .ppEnabledExtensionNames = enabledInstanceExtensions.data(),
        .enabledLayerCount       = (uint32_t)enabledLayers.size(),
        .ppEnabledLayerNames     = enabledLayers.data(),
    #if __APPLE__
        // For MoltenVK. 
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
    #endif
    };

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_VKInstance) != VK_SUCCESS) 
        throw std::runtime_error("failed to create instance!");

    // Create Window Surface (if needed)
    // ----------------------

    if (m_Window != nullptr)
        m_Window->CreateVulkanSurface(this);

    // Create Physical Device
    // ----------------------

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_VKInstance, &deviceCount, nullptr);

    if (deviceCount == 0)
        throw std::runtime_error("no physical graphics devices found.");

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(m_VKInstance, &deviceCount, physicalDevices.data());

    // Lazily choose the first one. 
    m_VKDevicePhysical = physicalDevices[0];

    // Queue Families
    // ----------------------

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_VKDevicePhysical, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_VKDevicePhysical, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) 
            m_VKQueueGraphicsIndex = i;

        if (window == nullptr)
        {
            // Don't bother handling present support if there's no window being drawn to. 
            continue;
        }
        
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_VKDevicePhysical, i, window->GetVulkanSurface(), &presentSupport);

        if (!presentSupport)
            continue;

        m_VKQueuePresentIndex = i;
    }

    // Create Vulkan Device
    // ----------------------
    
    if (window != nullptr)
    {
        if (m_VKQueueGraphicsIndex == UINT_MAX || m_VKQueuePresentIndex == UINT_MAX)
            throw std::runtime_error("no graphics or present queue for the device.");

        if (m_VKQueueGraphicsIndex != m_VKQueuePresentIndex)
            throw std::runtime_error("no support for different graphics and present queue.");
    }
    else
    {
        if (m_VKQueueGraphicsIndex == UINT_MAX)
            throw std::runtime_error("no graphics queue for the device.");
    }
        
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo =
    {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = m_VKQueueGraphicsIndex,
        .queueCount       = 1,
        .pQueuePriorities = &queuePriority
    };

    // Specify the physical features to use. 
    VkPhysicalDeviceFeatures deviceFeatures = {};

    std::vector<const char*> enabledExtensions;
    {
        enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        enabledExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        enabledExtensions.push_back(VK_EXT_SHADER_OBJECT_EXTENSION_NAME);
        enabledExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
        enabledExtensions.push_back(VK_KHR_MAINTENANCE_2_EXTENSION_NAME);
        enabledExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
    }

#if __APPLE__
    enabledExtensions.push_back("VK_KHR_portability_subset");
#endif

    // Setup for VK_EXT_extended_dynamic_state2

    VkPhysicalDeviceExtendedDynamicState2FeaturesEXT dynamicState2 = 
    {
        .sType                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT,
        .extendedDynamicState2 = VK_TRUE
    };

    // Setup for VK_KHR_synchronization2

    VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2Feature =
    {
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
        .pNext            = &dynamicState2,
        .synchronization2 = VK_TRUE
    };

    // Setup for VK_EXT_shader_object

    VkPhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeature = 
    {
        .sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
        .pNext        = &synchronization2Feature,
        .shaderObject = VK_TRUE
    };

    // Setup for VK_KHR_dynamic_rendering

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature =
    {
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .pNext            = &shaderObjectFeature,
        .dynamicRendering = VK_TRUE
    };

    // Setup to enable timeline semaphores.

    VkPhysicalDeviceVulkan12Features features12 = 
    {
        .sType             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext             = &dynamicRenderingFeature,
        .timelineSemaphore = VK_TRUE
    };
    
    VkDeviceCreateInfo deviceCreateInfo = 
    {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &features12,
        .pQueueCreateInfos       = &queueCreateInfo,
        .queueCreateInfoCount    = 1,
        .pEnabledFeatures        = &deviceFeatures,
        .ppEnabledExtensionNames = enabledExtensions.data(),
        .enabledExtensionCount   = (uint32_t)enabledExtensions.size(),
        .enabledLayerCount       = 0
    };

    if (vkCreateDevice(m_VKDevicePhysical, &deviceCreateInfo, nullptr, &m_VKDeviceLogical) != VK_SUCCESS) 
        throw std::runtime_error("failed to create logical device.");

    // Get Queues
    // ---------------------

    vkGetDeviceQueue(m_VKDeviceLogical, m_VKQueueGraphicsIndex, 0, &m_VKQueueGraphics);
    vkGetDeviceQueue(m_VKDeviceLogical, m_VKQueuePresentIndex,  0, &m_VKQueuePresent);

    // Command Pool
    // ---------------------

    VkCommandPoolCreateInfo commandPoolInfo
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_VKQueueGraphicsIndex
    };

    if (vkCreateCommandPool(m_VKDeviceLogical, &commandPoolInfo, nullptr, &m_VKCommandPool) != VK_SUCCESS)
        throw std::runtime_error("failed to create command pool.");

    // Create swap-chain
    // ---------------------

    if (m_Window != nullptr)
        m_Window->CreateVulkanSwapchain(this);

    // Extension function pointers
    // ---------------------

    if (Device::vkCmdBeginRenderingKHR != nullptr)
        return;

    GET_VK_FUNC(vkCmdBeginRenderingKHR);
    GET_VK_FUNC(vkCmdEndRenderingKHR);
    GET_VK_FUNC(vkCmdPipelineBarrier2KHR);
    GET_VK_FUNC(vkCmdBindShadersEXT);
    GET_VK_FUNC(vkCmdSetPrimitiveTopologyEXT);
    GET_VK_FUNC(vkCreateShadersEXT);
    GET_VK_FUNC(vkDestroyShaderEXT);
    GET_VK_FUNC(vkCmdSetColorWriteMaskEXT);
    GET_VK_FUNC(vkCmdSetPrimitiveRestartEnableEXT);
    GET_VK_FUNC(vkCmdSetColorBlendEnableEXT);
    GET_VK_FUNC(vkCmdSetRasterizerDiscardEnableEXT);
    GET_VK_FUNC(vkCmdSetAlphaToCoverageEnableEXT);
    GET_VK_FUNC(vkCmdSetPolygonModeEXT);
    GET_VK_FUNC(vkCmdSetStencilTestEnableEXT);
    GET_VK_FUNC(vkCmdSetDepthTestEnableEXT);
    GET_VK_FUNC(vkCmdSetColorBlendEquationEXT);
    GET_VK_FUNC(vkCmdSetCullModeEXT);
    GET_VK_FUNC(vkCmdSetDepthBiasEnableEXT);
    GET_VK_FUNC(vkCmdSetDepthWriteEnableEXT);
    GET_VK_FUNC(vkCmdSetFrontFaceEXT);
    GET_VK_FUNC(vkCmdSetViewportWithCountEXT);
    GET_VK_FUNC(vkCmdSetScissorWithCountEXT);
    GET_VK_FUNC(vkCmdSetRasterizationSamplesEXT);
    GET_VK_FUNC(vkCmdSetSampleMaskEXT);
}

Device::~Device()
{
    vkDeviceWaitIdle(m_VKDeviceLogical);

    if (m_Window != nullptr)
        m_Window->ReleaseVulkanObjects(this);

    vkDestroyCommandPool(m_VKDeviceLogical, m_VKCommandPool, nullptr);
    vkDestroyDevice(m_VKDeviceLogical, nullptr);
}

void Device::CreateShaders(const std::vector<Shader*>& shaders)
{
    for (auto& shader : shaders)
    {
        // TODO: Do this in one native call. 
        Device::vkCreateShadersEXT(m_VKDeviceLogical, 1u, shader->GetCreateInfo(), nullptr, shader->GetPrimitivePtr());

        shader->ReleaseByteCode();
    }
}

void Device::ReleaseShaders(const std::vector<Shader*>& shaders)
{
    for (auto& shader : shaders)
        Device::vkDestroyShaderEXT(m_VKDeviceLogical, *shader->GetPrimitivePtr(), nullptr);
}

void Device::CreateBuffers(const std::vector<Buffer*>& buffers)
{

}

void Device::ReleaseBuffers(const std::vector<Buffer*>& buffers)
{
    
}

void Device::SetDefaultRenderState(VkCommandBuffer commandBuffer)
{
    static VkColorComponentFlags s_DefaultWriteMask =   VK_COLOR_COMPONENT_R_BIT | 
                                                        VK_COLOR_COMPONENT_G_BIT | 
                                                        VK_COLOR_COMPONENT_B_BIT | 
                                                        VK_COLOR_COMPONENT_A_BIT;

    static VkColorBlendEquationEXT s_DefaultColorBlend = 
    {
        // Color
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,

        // Alpha
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
    };

    static VkBool32     s_DefaultBlendEnable = VK_FALSE;
    static VkViewport   s_DefaultViewport    = { 0, 0, 64, 64, 0.0, 1.0 };
    static VkRect2D     s_DefaultScissor     = { 0, 0, 64, 64 };
    static VkSampleMask s_DefaultSampleMask  = 0xFFFFFFFF;

    Device::vkCmdSetColorBlendEnableEXT       (commandBuffer, 0u, 1u, &s_DefaultBlendEnable);
    Device::vkCmdSetColorWriteMaskEXT         (commandBuffer, 0u, 1u, &s_DefaultWriteMask);
    Device::vkCmdSetColorBlendEquationEXT     (commandBuffer, 0u, 1u, &s_DefaultColorBlend);
    Device::vkCmdSetViewportWithCountEXT      (commandBuffer, 1u, &s_DefaultViewport);
    Device::vkCmdSetScissorWithCountEXT       (commandBuffer, 1u, &s_DefaultScissor);
    Device::vkCmdSetPrimitiveRestartEnableEXT (commandBuffer, VK_FALSE);
    Device::vkCmdSetRasterizerDiscardEnableEXT(commandBuffer, VK_FALSE);
    Device::vkCmdSetAlphaToCoverageEnableEXT  (commandBuffer, VK_FALSE);
    Device::vkCmdSetStencilTestEnableEXT      (commandBuffer, VK_FALSE);
    Device::vkCmdSetDepthTestEnableEXT        (commandBuffer, VK_FALSE);
    Device::vkCmdSetDepthBiasEnableEXT        (commandBuffer, VK_FALSE);
    Device::vkCmdSetDepthWriteEnableEXT       (commandBuffer, VK_FALSE);
    Device::vkCmdSetRasterizationSamplesEXT   (commandBuffer, VK_SAMPLE_COUNT_1_BIT);
    Device::vkCmdSetSampleMaskEXT             (commandBuffer, VK_SAMPLE_COUNT_1_BIT, &s_DefaultSampleMask);
    Device::vkCmdSetFrontFaceEXT              (commandBuffer, VK_FRONT_FACE_CLOCKWISE);
    Device::vkCmdSetPolygonModeEXT            (commandBuffer, VK_POLYGON_MODE_FILL);
    Device::vkCmdSetCullModeEXT               (commandBuffer, VK_CULL_MODE_BACK_BIT);
    Device::vkCmdSetPrimitiveTopologyEXT      (commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
}