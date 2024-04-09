#ifndef BUFFER
#define BUFFER

#include <VulkanWrappers/VmaUsage.h>

namespace VulkanWrappers
{
    class Device;
    class Image;

    class Buffer
    {
        struct Info
        {
            VkBufferCreateInfo      buffer;
            VkBufferViewCreateInfo  view;
            VmaAllocationCreateInfo allocation;
        };

        struct Data
        {
            VkBuffer      buffer;
            VkBufferView  view;
            VmaAllocation allocation;
        };

    public:

        Buffer() {}
        Buffer(VkDeviceSize             size, 
               VkBufferUsageFlags       useFlags, 
               VmaAllocationCreateFlags memFlags);

        static void SetData(Device* device, Buffer* buffer, void* srcPtr, uint32_t size);

        // Copies an image resource into a buffer resource. 
        static void CopyImage(VkCommandBuffer cmd, Image* image, Buffer* buffer);

        inline Info* GetInfo() { return &m_Info; }
        inline Data* GetData() { return &m_Data; }

    private:
        Info m_Info;
        Data m_Data;
    };
}

#endif