#ifndef SHADER
#define SHADER

#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdlib.h>

namespace VulkanWrappers
{
    class Device; 

    class Shader
    {
    public:
        Shader() {}
        Shader(const char* spirvFilePath, VkShaderStageFlagBits stage, VkShaderStageFlags nextStage = 0x0);

        static void Bind(VkCommandBuffer commandBuffer, const Shader& shader);

        inline void ReleaseByteCode()                 { free(m_SPIRVByteCode); }
        inline VkShaderCreateInfoEXT* GetCreateInfo() { return &m_VKShaderCreateInfo; }
        inline VkShaderEXT* GetPrimitivePtr()         { return &m_VKShader; }
        inline VkShaderEXT  GetPrimitive() const      { return m_VKShader;  }
        inline VkShaderStageFlagBits GetStage() const { return m_VKStageFlagBits; }

    private:
        VkShaderCreateInfoEXT m_VKShaderCreateInfo;
        VkShaderStageFlagBits m_VKStageFlagBits;
        VkShaderEXT           m_VKShader;
        void*                 m_SPIRVByteCode;
    };
}

#endif//SHADER