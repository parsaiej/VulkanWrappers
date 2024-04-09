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
        struct Info
        {
            VkShaderCreateInfoEXT shader;
            VkShaderStageFlagBits stages;
        };

        struct Data
        {
            VkShaderEXT shader;
            void*       spirvByteCode;
        };

    public:
        Shader() {}
        Shader(const char* spirvFilePath, VkShaderStageFlagBits stage, VkShaderStageFlags nextStage = 0x0);

        static void Bind(VkCommandBuffer commandBuffer, Shader& shader);

        inline Info* GetInfo() { return &m_Info; }
        inline Data* GetData() { return &m_Data; }

    private:
        Data m_Data;
        Info m_Info;
    };
}

#endif//SHADER