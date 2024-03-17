#ifndef RESOURCE
#define RESOURCE

#include <vulkan/vulkan.h>

#include <vector>

class GraphicsResource
{
public:
    virtual ~GraphicsResource() {};

    // Invoke the native VK command to release device memory.
    virtual void Release(VkDevice device) {};

    // Bind the VK primitive to the graphics pipeline. 
    virtual void Bind(VkCommandBuffer command) {};

    static VkDevice* s_VKDevice;
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

        void Release(VkDevice device) override;

        void SetShaderProgram(const char* filePathVertex, const char* filePathFragment);
        void SetShaderProgram(const char* filePathCompute);

        void SetColorTargetFormats(const std::vector<VkFormat>& colorRTs) { m_ColorRTFormats = colorRTs; };
        void SetDepthTargetFormats(const std::vector<VkFormat>& depthRTs) { m_DepthRTFormats = depthRTs; };

        void SetPushConstants(const std::vector<VkPushConstantRange>& pushConstantRanges) { m_PushConstants = pushConstantRanges; };

        void SetScissor(VkRect2D scissor)     { m_VKScissor = scissor;   }
        void SetViewport(VkViewport viewport) { m_VKViewport = viewport; }

    private:
        VkPipeline       m_VKPipeline;
        VkPipelineLayout m_VKPipelineLayout;

        VkShaderModule m_VKShaderVS;
        VkShaderModule m_VKShaderFS;
        VkShaderModule m_VKShaderCS;

        std::vector<VkFormat> m_ColorRTFormats;
        std::vector<VkFormat> m_DepthRTFormats;

        std::vector<VkPushConstantRange> m_PushConstants;

        VkViewport m_VKViewport;
        VkRect2D   m_VKScissor;

        bool m_IsCompute;
    };

    class Buffer : public GraphicsResource
    {
    public:

        void Release(VkDevice device) override { vkDestroyBuffer(device, m_Primitive, nullptr); }

    private:
        VkBuffer m_Primitive;
    };

    class Image : public GraphicsResource
    {
    public:

        void Release(VkDevice device) override { vkDestroyImage(device, m_Primitive, nullptr); }

    private:
        VkImage m_Primitive;
    };
}

#endif//RESOURCE