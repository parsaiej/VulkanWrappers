#include <VulkanWrappers/Image.h>
#include <VulkanWrappers/Device.h>

using namespace VulkanWrappers;

Image::Image(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect):
    m_Data()
{
    m_Info = {};
    m_Info.image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    m_Info.image.imageType = VK_IMAGE_TYPE_2D;
    m_Info.image.extent.width = width;
    m_Info.image.extent.height = height;
    m_Info.image.extent.depth = 1;
    m_Info.image.mipLevels = 1;
    m_Info.image.arrayLayers = 1;
    m_Info.image.format = format;
    m_Info.image.tiling = VK_IMAGE_TILING_OPTIMAL;
    m_Info.image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_Info.image.usage = usage;
    m_Info.image.samples = VK_SAMPLE_COUNT_1_BIT;
    m_Info.view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    m_Info.view.image = nullptr;
    m_Info.view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    m_Info.view.format = format;

    m_Info.view.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    m_Info.view.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    m_Info.view.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    m_Info.view.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    m_Info.view.subresourceRange.aspectMask = aspect;
    m_Info.view.subresourceRange.baseMipLevel = 0;
    m_Info.view.subresourceRange.levelCount = 1;
    m_Info.view.subresourceRange.baseArrayLayer = 0;
    m_Info.view.subresourceRange.layerCount = 1;

    m_Info.allocation.usage = VMA_MEMORY_USAGE_AUTO;
    m_Info.allocation.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    m_Info.allocation.priority = 1.0;
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
    VkImageMemoryBarrier2KHR imageBarrier = {};

    imageBarrier.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
    imageBarrier.srcStageMask  = args.srcStage;
    imageBarrier.dstStageMask  = args.dstStage;
    imageBarrier.srcAccessMask = args.srcAccess;
    imageBarrier.dstAccessMask = args.dstAccess;
    imageBarrier.oldLayout     = args.oldLayout;
    imageBarrier.newLayout     = args.newLayout;
    imageBarrier.image         = args.image;

    imageBarrier.subresourceRange = {};
    imageBarrier.subresourceRange.aspectMask     = args.aspect;
    imageBarrier.subresourceRange.baseMipLevel   = 0;
    imageBarrier.subresourceRange.levelCount     = 1;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount     = 1;
    VkDependencyInfo dependencyInfo ={};

    dependencyInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = 1u;
    dependencyInfo.pImageMemoryBarriers    = &imageBarrier;

#if __APPLE__
    Device::vkCmdPipelineBarrier2KHR(args.cmd, &dependencyInfo);
#else
    vkCmdPipelineBarrier2(args.cmd, &dependencyInfo);
#endif
}

void Image::TransferUnknownToWrite(VkCommandBuffer commandBuffer, VkImage vkImage)
{
    TransitionArgs args = {};
    args.cmd = commandBuffer;
    args.image = vkImage;
    args.srcStage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    args.dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    args.dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    args.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    args.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    args.aspect = VK_IMAGE_ASPECT_COLOR_BIT;


    Transition(args);
}

void Image::TransferWriteToSource(VkCommandBuffer commandBuffer, VkImage vkImage)
{
    TransitionArgs args = {};

    args.cmd       = commandBuffer;
    args.image     = vkImage;
    args.srcStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    args.dstStage  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    args.srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    args.dstAccess = VK_ACCESS_2_TRANSFER_READ_BIT;
    args.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    args.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    args.aspect    = VK_IMAGE_ASPECT_COLOR_BIT;

    Transition(args);
}

void Image::TransferWriteToPresent(VkCommandBuffer commandBuffer, VkImage vkImage)
{
    TransitionArgs args = {};
    args.cmd       = commandBuffer;
    args.image     = vkImage;
    args.srcStage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    args.dstStage  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    args.srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    args.dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
    args.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    args.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    args.aspect    = VK_IMAGE_ASPECT_COLOR_BIT;
    Transition(args);
}

void Image::TransferUnknownToDestination(VkCommandBuffer commandBuffer, VkImage vkImage)
{
    TransitionArgs args = {};
    args.cmd       = commandBuffer;
    args.image     = vkImage;
    args.srcStage  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    args.dstStage  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    args.dstAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    args.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    args.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    args.aspect    = VK_IMAGE_ASPECT_COLOR_BIT;
    Transition(args);
}

void Image::TransferDestinationToPresent(VkCommandBuffer commandBuffer, VkImage vkImage)
{
    TransitionArgs args = {};

        args.cmd       = commandBuffer;
        args.image     = vkImage;
        args.srcStage  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        args.dstStage  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        args.srcAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        args.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        args.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        args.aspect    = VK_IMAGE_ASPECT_COLOR_BIT;


    Transition(args);
}