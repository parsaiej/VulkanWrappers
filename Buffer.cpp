#include <VulkanWrappers/Buffer.h>
#include <VulkanWrappers/Image.h>

using namespace VulkanWrappers;

Buffer::Buffer(VkDeviceSize             size, 
               VkBufferUsageFlags       useFlags, 
               VmaAllocationCreateFlags memFlags): m_Data()
{
    m_Info.buffer ={};

    m_Info.buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    m_Info.buffer.size = size;
    m_Info.buffer.usage = useFlags;
    m_Info.buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    m_Info.view = {};
    m_Info.view .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    m_Info.view.pNext = nullptr;
    m_Info.view.flags = 0;
    m_Info.view.format = VK_FORMAT_R32_UINT;
    m_Info.view.offset = 0;
    m_Info.view.range = size;

    m_Info.allocation = {};
    m_Info.allocation.usage = VMA_MEMORY_USAGE_AUTO;
    m_Info.allocation.flags = memFlags;
    m_Info.allocation.priority = 1.0;
}

void Buffer::CopyImage(VkCommandBuffer cmd, Image* image, Buffer* buffer)
{
    VkBufferImageCopy copyInfo = {};
    copyInfo.bufferOffset = 0u;
    copyInfo.bufferRowLength = 0u;
    copyInfo.bufferImageHeight = 0u;
    copyInfo.imageOffset = {0, 0, 0};
    copyInfo.imageExtent = image->GetInfo()->image.extent;
    copyInfo.imageSubresource ={};
    copyInfo.imageSubresource.aspectMask = image->GetInfo()->view.subresourceRange.aspectMask;
    copyInfo.imageSubresource.baseArrayLayer = image->GetInfo()->view.subresourceRange.baseArrayLayer;
    copyInfo.imageSubresource.layerCount = image->GetInfo()->view.subresourceRange.layerCount;
    copyInfo.imageSubresource.mipLevel = image->GetInfo()->view.subresourceRange.baseMipLevel;

    vkCmdCopyImageToBuffer(cmd, image->GetData()->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer->GetData()->buffer, 1u, &copyInfo);
}
