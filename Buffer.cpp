#include <VulkanWrappers/Buffer.h>

using namespace VulkanWrappers;

Buffer::Buffer(VkDeviceSize             size, 
               VkBufferUsageFlags       useFlags, 
               VkMemoryPropertyFlags    memFlags, 
               VmaAllocationCreateFlags allocFlags)
{
    m_VKBufferInfo =
    {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = useFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    m_VMAAllocationInfo = 
    {
        .flags         = allocFlags,
        .usage         = VMA_MEMORY_USAGE_UNKNOWN,
        .requiredFlags = memFlags,
    };   
}