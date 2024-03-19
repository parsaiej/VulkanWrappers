#ifndef RESOURCE
#define RESOURCE

#include <vulkan/vulkan.h>

#include "VmaUsage.h"
#include "RenderInstance.h"

#include <vector>

class GraphicsResource
{
public:
    virtual ~GraphicsResource() {};

    // Invoke the native VK command to release device memory.
    virtual void Release() {};

    // Bind the VK primitive to the graphics pipeline. 
    virtual void Bind(VkCommandBuffer command) {};

    // Keep handle to render instance for simpler resource creation. 
    static Wrappers::RenderInstance* s_RenderInstance;
};

namespace Wrappers
{
    class Pipeline : public GraphicsResource
    {
    public:

        Pipeline() {};

        // Re-create a pipeline based on the currently configured state of the object. 
        void Commit();

        void UpdateLayout(VkCommandBuffer cmd, void* constants, uint32_t constantsSize);

        void Bind(VkCommandBuffer command) override
        {
            auto bindPoint = m_IsCompute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
            vkCmdBindPipeline(command, bindPoint, m_VKPipeline);
        }

        void Release() override;

        void SetShaderProgram(const char* filePathVertex, const char* filePathFragment);
        void SetShaderProgram(const char* filePathCompute);

        inline void SetColorTargetFormats(const std::vector<VkFormat>& colorRTs) { m_ColorRTFormats = colorRTs; };
        inline void SetDepthTargetFormats(const std::vector<VkFormat>& depthRTs) { m_DepthRTFormats = depthRTs; };
 
        inline void SetPushConstants(const std::vector<VkPushConstantRange>& pushConstantRanges) { m_PushConstants = pushConstantRanges; };
 
        inline void SetVertexInputBindings(const std::vector<VkVertexInputBindingDescription>& inputBindings)       { m_VKVertexBindings   = inputBindings;   };
        inline void SetVertexInputAttributes(const std::vector<VkVertexInputAttributeDescription>& inputAttributes) { m_VKVertexAttributes = inputAttributes; };
 
        inline void SetScissor(VkRect2D scissor)     { m_VKScissor = scissor;   }
        inline void SetViewport(VkViewport viewport) { m_VKViewport = viewport; }

    private:
        VkPipeline       m_VKPipeline;
        VkPipelineLayout m_VKPipelineLayout;

        VkShaderModule m_VKShaderVS;
        VkShaderModule m_VKShaderFS;
        VkShaderModule m_VKShaderCS;

        std::vector<VkFormat> m_ColorRTFormats;
        std::vector<VkFormat> m_DepthRTFormats;

        std::vector<VkPushConstantRange> m_PushConstants;

        std::vector<VkVertexInputBindingDescription>   m_VKVertexBindings;
        std::vector<VkVertexInputAttributeDescription> m_VKVertexAttributes;

        VkViewport m_VKViewport;
        VkRect2D   m_VKScissor;

        bool m_IsCompute;
    };

    class Buffer : public GraphicsResource
    {
    public:

        Buffer(VkDeviceSize size, VkBufferUsageFlags bufferFlags, VkMemoryPropertyFlags memoryFlags, VmaAllocationCreateFlags allocFlags = 0x0);

        VkBuffer Get() { return m_VKBuffer; }

        void SetData(void* data, uint32_t size);
        void Release() override;

    private:
        VkBuffer       m_VKBuffer;
        VmaAllocation  m_VMAAllocation;
    };

    class Image : public GraphicsResource
    {
    public:

        Image(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);

        void Release() override;
        
        inline VkImage Get() { return m_VKImage; }

    private:
        VkImage       m_VKImage;
        VmaAllocation m_VMAAllocation;
    };

    class ImageView : public GraphicsResource
    {
    public:

        ImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

        void Release() override;

        inline VkImageView Get() { return m_VKImageView; }

    private:
        VkImageView m_VKImageView;
    };
}

#endif//RESOURCE