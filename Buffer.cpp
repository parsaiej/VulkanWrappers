#include <VulkanWrappers/Buffer.h>
#include <VulkanWrappers/Image.h>

using namespace VulkanWrappers;

Buffer::Buffer(VkDeviceSize             size, 
               VkBufferUsageFlags       useFlags, 
               VmaAllocationCreateFlags memFlags)
{
    m_Info.buffer =
    {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = useFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    m_Info.view =
    {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
        .pNext  = nullptr,
        .flags  = 0,
        .format = VK_FORMAT_R32_UINT,
        .offset = 0,
        .range  = size
    };

    m_Info.allocation = 
    {
        .usage         = VMA_MEMORY_USAGE_AUTO,
        .flags         = memFlags,
        .priority      = 1.0,
    };   
}

void Buffer::CopyImage(VkCommandBuffer cmd, Image* image, Buffer* buffer)
{
    VkBufferImageCopy copyInfo =
    {
        .bufferOffset      = 0u,
        .bufferRowLength   = 0u,
        .bufferImageHeight = 0u,
        .imageOffset       = { 0, 0, 0},
        .imageExtent       = image->GetInfo()->image.extent,
        .imageSubresource  = 
        {
            .aspectMask     = image->GetInfo()->view.subresourceRange.aspectMask,
            .baseArrayLayer = image->GetInfo()->view.subresourceRange.baseArrayLayer,
            .layerCount     = image->GetInfo()->view.subresourceRange.layerCount,
            .mipLevel       = image->GetInfo()->view.subresourceRange.baseMipLevel,
        }
    };

    vkCmdCopyImageToBuffer(cmd, image->GetData()->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer->GetData()->buffer, 1u, &copyInfo);
}