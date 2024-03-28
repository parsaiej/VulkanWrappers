#include <VulkanWrappers/Image.h>
#include <VulkanWrappers/Device.h>

using namespace VulkanWrappers;

void Image::TransferBackbufferToWrite(VkCommandBuffer commandBuffer, Device* device, VkImage vkImage)
{
    VkImageMemoryBarrier2KHR imageBarrier
    {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
        .srcStageMask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .image         = vkImage,

        .subresourceRange = 
        {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        }
    };

    VkDependencyInfo dependencyInfo
    {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1u,
        .pImageMemoryBarriers    = &imageBarrier
    };

    device->Barrier(commandBuffer, &dependencyInfo);
}

void Image::TransferBackbufferToPresent(VkCommandBuffer commandBuffer, Device* device, VkImage vkImage)
{
    VkImageMemoryBarrier2KHR imageBarrier
    {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
        .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image         = vkImage,

        .subresourceRange = 
        {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        }
    };

    VkDependencyInfo dependencyInfo
    {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1u,
        .pImageMemoryBarriers    = &imageBarrier
    };

    device->Barrier(commandBuffer, &dependencyInfo);
}