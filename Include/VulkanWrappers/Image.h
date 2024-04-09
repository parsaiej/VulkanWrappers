#ifndef IMAGE
#define IMAGE

#include <vulkan/vulkan.h>
#include <GL/glew.h>

#include <VulkanWrappers/VmaUsage.h>

namespace VulkanWrappers
{
    class Device;

    class Image
    {
        struct Info
        {
            VkImageCreateInfo image;
            VkImageViewCreateInfo view;
            VmaAllocationCreateInfo allocation;
        };

        struct Data
        {
            VkImage image;
            VkImageView view;
            VmaAllocation allocation;
        };

    public:

        Image() {}
        Image(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect);

        inline Info* GetInfo() { return &m_Info; }
        inline Data* GetData() { return &m_Data; }

        static void TransferUnknownToWrite       (VkCommandBuffer commandBuffer, VkImage vkImage);
        static void TransferWriteToPresent       (VkCommandBuffer commandBuffer, VkImage vkImage);
        static void TransferWriteToSource        (VkCommandBuffer commandBuffer, VkImage vkImage);
        static void TransferUnknownToDestination (VkCommandBuffer commandBuffer, VkImage vkImage);
        static void TransferDestinationToPresent (VkCommandBuffer commandBuffer, VkImage vkImage);

    private:
        Data m_Data;
        Info m_Info;
    };
}

#endif