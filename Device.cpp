#include <VulkanWrappers/Device.h>
#include <VulkanWrappers/Window.h>

#include <GLFW/glfw3.h>
#include <vector>

using namespace VulkanWrappers;

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
    }

#if __APPLE__
    enabledExtensions.push_back("VK_KHR_portability_subset");
#endif

    // Setup for VK_KHR_synchronization2

    VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2Feature =
    {
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
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

	m_VKCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>  (vkGetDeviceProcAddr(m_VKDeviceLogical, "vkCmdBeginRenderingKHR"));
	m_VKCmdEndRenderingKHR   = reinterpret_cast<PFN_vkCmdEndRenderingKHR>    (vkGetDeviceProcAddr(m_VKDeviceLogical, "vkCmdEndRenderingKHR"  ));
    m_VKCmdBarrierKHR        = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(vkGetDeviceProcAddr(m_VKDeviceLogical, "vkCmdPipelineBarrier2KHR"));
}

Device::~Device()
{
    vkDeviceWaitIdle(m_VKDeviceLogical);

    if (m_Window != nullptr)
        m_Window->ReleaseVulkanObjects(this);

    vkDestroyCommandPool(m_VKDeviceLogical, m_VKCommandPool, nullptr);
    vkDestroyDevice(m_VKDeviceLogical, nullptr);
}