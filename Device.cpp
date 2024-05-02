#include <VulkanWrappers/Device.h>
#include <VulkanWrappers/Window.h>
#include <VulkanWrappers/Shader.h>
#include <VulkanWrappers/Buffer.h>
#include <VulkanWrappers/Image.h>

#include <GLFW/glfw3.h>
#include <vector>
#include <assert.h>

using namespace VulkanWrappers;

#define DECLARE_VK_FUNC(func) PFN_##func Device::func = nullptr;
#define GET_VK_FUNC(func) Device::func = reinterpret_cast<PFN_##func> (vkGetDeviceProcAddr(m_VKDeviceLogical, #func)); assert(func != nullptr);

DECLARE_VK_FUNC(vkCreateShadersEXT);
DECLARE_VK_FUNC(vkDestroyShaderEXT);
DECLARE_VK_FUNC(vkCmdBindShadersEXT);
DECLARE_VK_FUNC(vkCmdSetRasterizationSamplesEXT);
DECLARE_VK_FUNC(vkCmdSetSampleMaskEXT);
DECLARE_VK_FUNC(vkCmdSetAlphaToCoverageEnableEXT);
DECLARE_VK_FUNC(vkCmdSetPolygonModeEXT);
DECLARE_VK_FUNC(vkCmdSetColorBlendEnableEXT);
DECLARE_VK_FUNC(vkCmdSetColorWriteMaskEXT);
DECLARE_VK_FUNC(vkCmdSetColorBlendEquationEXT);

Device::Device(Window* window)
    : m_Window(window)
{
    // Create Vulkan Instance

    VkApplicationInfo appInfo  = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Vulkan Instance";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "No Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_3;
    std::vector<const char*> enabledInstanceExtensions;

    if (window != nullptr)
    {
        // Sample GLFW for any extensions it needs. 
        uint32_t extensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

        // Copy GLFW extensions. 
        for (uint32_t i = 0; i < extensionCount; ++i)
            enabledInstanceExtensions.push_back(glfwExtensions[i]);
    }

#if __APPLE__
    // MoltenVK compatibility. 
    enabledInstanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    if (window != nullptr)
        enabledInstanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif

    std::vector<const char*> enabledLayers;

#if ENABLE_VALIDATION_LAYERS
    enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif

#if __APPLE__
    // Required in order to create a logical device with shader object extension that doesn't natively support it.
    enabledLayers.push_back("VK_LAYER_KHRONOS_shader_object");
#else
    VkInstanceCreateInfo instanceCreateInfo    = {};
    instanceCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo        = &appInfo;
    instanceCreateInfo.enabledExtensionCount   = (uint32_t)enabledInstanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();
    instanceCreateInfo.enabledLayerCount       = (uint32_t)enabledLayers.size();
    instanceCreateInfo.ppEnabledLayerNames     = enabledLayers.data();
#endif
    #if __APPLE__
        // For MoltenVK. 
        instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif


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
    }
    else
    {
        if (m_VKQueueGraphicsIndex == UINT_MAX)
            throw std::runtime_error("no graphics queue for the device.");
    }
        
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_VKQueueGraphicsIndex;
    queueCreateInfo.queueCount       = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    queueCreateInfos.push_back(queueCreateInfo);

    if (m_VKQueueGraphicsIndex != m_VKQueuePresentIndex)
    {
        queueCreateInfo.queueFamilyIndex = m_VKQueuePresentIndex;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Specify the physical features to use. 
    VkPhysicalDeviceFeatures deviceFeatures = {};

    std::vector<const char*> enabledExtensions;
    {
        if (window != nullptr)
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

    VkPhysicalDeviceExtendedDynamicState2FeaturesEXT dynamicState2 = {};
    dynamicState2.sType                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
    dynamicState2.extendedDynamicState2 = VK_TRUE;

    // Setup for VK_KHR_synchronization2

    VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2Feature = {};
    synchronization2Feature.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
    synchronization2Feature.pNext            = &dynamicState2;
    synchronization2Feature.synchronization2 = VK_TRUE;

    // Setup for VK_EXT_shader_object

    VkPhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeature = {};
    shaderObjectFeature.sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT;
    shaderObjectFeature.pNext        = &synchronization2Feature;
    shaderObjectFeature.shaderObject = VK_TRUE;

    // Setup for VK_KHR_dynamic_rendering

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature ={};
    dynamicRenderingFeature.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamicRenderingFeature.pNext            = &shaderObjectFeature;
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;

    // Setup to enable timeline semaphores.

    VkPhysicalDeviceVulkan12Features features12 =  {};
    features12.sType             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.pNext             = &dynamicRenderingFeature;
    features12.timelineSemaphore = VK_TRUE;
    
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext                   = &features12;
    deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount    = (uint32_t)queueCreateInfos.size();
    deviceCreateInfo.pEnabledFeatures        = &deviceFeatures;
    deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
    deviceCreateInfo.enabledExtensionCount   = (uint32_t)enabledExtensions.size();
    deviceCreateInfo.enabledLayerCount       = 0;

    if (vkCreateDevice(m_VKDevicePhysical, &deviceCreateInfo, nullptr, &m_VKDeviceLogical) != VK_SUCCESS) 
        throw std::runtime_error("failed to create logical device.");

    // Get Queues
    // ---------------------

    vkGetDeviceQueue(m_VKDeviceLogical, m_VKQueueGraphicsIndex, 0, &m_VKQueueGraphics);

    if (m_Window != nullptr)
        vkGetDeviceQueue(m_VKDeviceLogical, m_VKQueuePresentIndex,  0, &m_VKQueuePresent);

    // Create Vulkan Memory Allocator
    // ----------------------

    VmaVulkanFunctions vmaVulkanFunctions = {};
    vmaVulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vmaVulkanFunctions.vkGetDeviceProcAddr   = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.flags            = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocatorCreateInfo.physicalDevice   = m_VKDevicePhysical;
    allocatorCreateInfo.device           = m_VKDeviceLogical;
    allocatorCreateInfo.instance         = m_VKInstance;
    allocatorCreateInfo.pVulkanFunctions = &vmaVulkanFunctions;
    vmaCreateAllocator(&allocatorCreateInfo, &m_VMAAllocator);

    // Command Pool
    // ---------------------

    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = m_VKQueueGraphicsIndex;

    if (vkCreateCommandPool(m_VKDeviceLogical, &commandPoolInfo, nullptr, &m_VKCommandPool) != VK_SUCCESS)
        throw std::runtime_error("failed to create command pool.");

    // Create swap-chain
    // ---------------------

    if (m_Window != nullptr)
        m_Window->CreateVulkanSwapchain(this);

    // Extension function pointers
    // ---------------------
    
    GET_VK_FUNC(vkCmdBindShadersEXT);
    GET_VK_FUNC(vkCreateShadersEXT);
    GET_VK_FUNC(vkDestroyShaderEXT);
    GET_VK_FUNC(vkCmdSetRasterizationSamplesEXT);
    GET_VK_FUNC(vkCmdSetSampleMaskEXT);
    GET_VK_FUNC(vkCmdSetAlphaToCoverageEnableEXT);
    GET_VK_FUNC(vkCmdSetPolygonModeEXT);
    GET_VK_FUNC(vkCmdSetColorBlendEnableEXT);  
    GET_VK_FUNC(vkCmdSetColorWriteMaskEXT);
    GET_VK_FUNC(vkCmdSetColorBlendEquationEXT);
}

Device::~Device()
{
    vkDeviceWaitIdle(m_VKDeviceLogical);

    vmaDestroyAllocator(m_VMAAllocator);

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
        vkCreateShadersEXT(m_VKDeviceLogical, 1u, &shader->GetInfo()->shader, nullptr, &shader->GetData()->shader);
    }
}

void Device::ReleaseShaders(const std::vector<Shader*>& shaders)
{
    for (auto& shader : shaders)
    {
        free(shader->GetData()->spirvByteCode);
        vkDestroyShaderEXT(m_VKDeviceLogical, shader->GetData()->shader, nullptr);
    }
}

void Device::CreateBuffers(const std::vector<Buffer*>& buffers)
{
    for (auto& buffer : buffers)
    {
        auto hr = vmaCreateBuffer(m_VMAAllocator, 
                            &buffer->GetInfo()->buffer, 
                            &buffer->GetInfo()->allocation, 
                            &buffer->GetData()->buffer,
                            &buffer->GetData()->allocation,
                            nullptr);

        if (hr != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate buffer.");

        // Patch in the created buffer.
        buffer->GetInfo()->view.buffer = buffer->GetData()->buffer;

        vkCreateBufferView(m_VKDeviceLogical, &buffer->GetInfo()->view, nullptr, &buffer->GetData()->view);
    }

}

void Device::ReleaseBuffers(const std::vector<Buffer*>& buffers)
{
    for (auto& buffer : buffers)
    {
        vmaDestroyBuffer(m_VMAAllocator, buffer->GetData()->buffer, buffer->GetData()->allocation);
        vkDestroyBufferView(m_VKDeviceLogical, buffer->GetData()->view, nullptr);
    }
}

void Device::CreateImages(const std::vector<Image*>& images)
{
    for (auto& image : images)
    {
        auto hr = vmaCreateImage(m_VMAAllocator, 
                    &image->GetInfo()->image, 
                    &image->GetInfo()->allocation, 
                    &image->GetData()->image, 
                    &image->GetData()->allocation, nullptr);

        if (hr != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate image.");

        // Patch in the created image.
        image->GetInfo()->view.image = image->GetData()->image;

        vkCreateImageView(m_VKDeviceLogical, &image->GetInfo()->view, nullptr, &image->GetData()->view);
    }
}

void Device::ReleaseImages(const std::vector<Image*>& images)
{
    for (auto& image : images)
    {
        vmaDestroyImage(m_VMAAllocator, image->GetData()->image, image->GetData()->allocation);
        vkDestroyImageView(m_VKDeviceLogical, image->GetData()->view, nullptr);
    }
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
    static VkRect2D     s_DefaultScissor     = { {0, 0}, {64, 64} };
    static VkSampleMask s_DefaultSampleMask  = 0xFFFFFFFF;

    // See: https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#shaders-objects-state
    vkCmdSetViewportWithCount         (commandBuffer, 1u, &s_DefaultViewport);
    vkCmdSetScissorWithCount          (commandBuffer, 1u, &s_DefaultScissor);
    vkCmdSetRasterizerDiscardEnable   (commandBuffer, VK_FALSE);

    vkCmdSetPrimitiveTopology      (commandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    vkCmdSetPrimitiveRestartEnable (commandBuffer, VK_FALSE);

    vkCmdSetRasterizationSamplesEXT  (commandBuffer, VK_SAMPLE_COUNT_1_BIT);
    vkCmdSetSampleMaskEXT            (commandBuffer, VK_SAMPLE_COUNT_1_BIT, &s_DefaultSampleMask);
    vkCmdSetAlphaToCoverageEnableEXT (commandBuffer, VK_FALSE);
    vkCmdSetPolygonModeEXT           (commandBuffer, VK_POLYGON_MODE_FILL);
    vkCmdSetCullMode                 (commandBuffer, VK_CULL_MODE_BACK_BIT);
    vkCmdSetFrontFace                (commandBuffer, VK_FRONT_FACE_CLOCKWISE);
    vkCmdSetDepthTestEnable          (commandBuffer, VK_FALSE);
    vkCmdSetDepthWriteEnable         (commandBuffer, VK_FALSE);
    vkCmdSetDepthBiasEnable          (commandBuffer, VK_FALSE);
    vkCmdSetStencilTestEnable        (commandBuffer, VK_FALSE);
    vkCmdSetColorBlendEnableEXT      (commandBuffer, 0u, 1u, &s_DefaultBlendEnable);
    vkCmdSetColorWriteMaskEXT        (commandBuffer, 0u, 1u, &s_DefaultWriteMask);
    vkCmdSetColorBlendEquationEXT    (commandBuffer, 0u, 1u, &s_DefaultColorBlend);
}