#ifndef IMAGE
#define IMAGE

#include <vulkan/vulkan.h>

namespace VulkanWrappers
{
    class Device;

    class Image
    {
    public:
        static void TransferBackbufferToWrite   (VkCommandBuffer commandBuffer, Device* device, VkImage vkImage);
        static void TransferBackbufferToPresent (VkCommandBuffer commandBuffer, Device* device, VkImage vkImage);
    };
}

#endif