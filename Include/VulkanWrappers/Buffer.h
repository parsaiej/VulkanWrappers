#ifndef BUFFER
#define BUFFER

#include <VulkanWrappers/VmaUsage.h>

namespace VulkanWrappers
{
    class Device;

    class Buffer
    {
    public:
        Buffer(VkDeviceSize             size, 
               VkBufferUsageFlags       useFlags, 
               VkMemoryPropertyFlags    memFlags, 
               VmaAllocationCreateFlags allocFlags = 0x0);

        static void SetData(Device* device, Buffer* buffer, void* srcPtr, uint32_t size);

    private:
        VkBufferCreateInfo      m_VKBufferInfo;
        VkBuffer                m_VKBuffer;

        VmaAllocationCreateInfo m_VMAAllocationInfo;
        VmaAllocation           m_VMAAllocation;
    };
}

#endif