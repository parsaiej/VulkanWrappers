#include <VulkanWrappers/Image.h>
#include <VulkanWrappers/Device.h>

using namespace VulkanWrappers;

Image::Image(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect)
{
    m_Info.image = 
    {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .extent.width  = width,
        .extent.height = height,
        .extent.depth  = 1,
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .format        = format,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage         = usage,
        .samples       = VK_SAMPLE_COUNT_1_BIT
    };

    m_Info.view = 
    {
        .sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image        = nullptr,
        .viewType     = VK_IMAGE_VIEW_TYPE_2D,
        .format       = format,

        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,

        .subresourceRange.aspectMask     = aspect,
        .subresourceRange.baseMipLevel   = 0,
        .subresourceRange.levelCount     = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount     = 1
    };
    
    m_Info.allocation = 
    {
        .usage    = VMA_MEMORY_USAGE_AUTO,
        .flags    = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .priority = 1.0
    };
}

// Transition Utilities
// ----------------------------------------

struct TransitionArgs
{
    VkCommandBuffer       cmd;
    VkImage               image;
    VkPipelineStageFlags2 srcStage; 
    VkPipelineStageFlags2 dstStage;
    VkAccessFlags2        srcAccess;
    VkAccessFlags2        dstAccess;
    VkImageLayout         oldLayout;
    VkImageLayout         newLayout;
    VkImageAspectFlags    aspect;
};

static void Transition(TransitionArgs args)
{
    VkImageMemoryBarrier2KHR imageBarrier
    {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
        .srcStageMask  = args.srcStage,
        .dstStageMask  = args.dstStage,
        .srcAccessMask = args.srcAccess,
        .dstAccessMask = args.dstAccess,
        .oldLayout     = args.oldLayout,
        .newLayout     = args.newLayout,
        .image         = args.image,

        .subresourceRange = 
        {
            .aspectMask     = args.aspect,
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

    Device::vkCmdPipelineBarrier2KHR(args.cmd, &dependencyInfo);
}

void Image::TransferUnknownToWrite(VkCommandBuffer commandBuffer, VkImage vkImage)
{
    TransitionArgs args = 
    {
        .cmd       = commandBuffer,
        .image     = vkImage,
        .srcStage  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        .dstStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .aspect    = VK_IMAGE_ASPECT_COLOR_BIT
    };

    Transition(args);
}

void Image::TransferWriteToSource(VkCommandBuffer commandBuffer, VkImage vkImage)
{
    TransitionArgs args = 
    {
        .cmd       = commandBuffer,
        .image     = vkImage,
        .srcStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStage  = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccess = VK_ACCESS_2_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .aspect    = VK_IMAGE_ASPECT_COLOR_BIT
    };

    Transition(args);
}

void Image::TransferWriteToPresent(VkCommandBuffer commandBuffer, VkImage vkImage)
{
    TransitionArgs args = 
    {
        .cmd       = commandBuffer,
        .image     = vkImage,
        .srcStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStage  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        .srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .aspect    = VK_IMAGE_ASPECT_COLOR_BIT
    };

    Transition(args);
}

void Image::TransferUnknownToDestination(VkCommandBuffer commandBuffer, VkImage vkImage)
{
    TransitionArgs args = 
    {
        .cmd       = commandBuffer,
        .image     = vkImage,
        .srcStage  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        .dstStage  = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .aspect    = VK_IMAGE_ASPECT_COLOR_BIT
    };

    Transition(args);
}

void Image::TransferDestinationToPresent(VkCommandBuffer commandBuffer, VkImage vkImage)
{
    TransitionArgs args = 
    {
        .cmd       = commandBuffer,
        .image     = vkImage,
        .srcStage  = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstStage  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        .srcAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .aspect    = VK_IMAGE_ASPECT_COLOR_BIT
    };

    Transition(args);
}