#include <VulkanWrappers/Shader.h>
#include <VulkanWrappers/Device.h>

#include <iostream>

using namespace VulkanWrappers;

Shader::Shader(const char* spirvFilePath, VkShaderStageFlagBits stage, VkShaderStageFlags nextStage)
{
    // Read byte code from file.
    // -------------------

    FILE* file = fopen(spirvFilePath, "rb");

    if (!file)
        throw std::runtime_error("failed to open shader byte code.");

    // Determine the byte code file size.

    fseek(file, 0, SEEK_END);
    long byteCodeSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate the byteCode memory. 
    m_Data.spirvByteCode = malloc(byteCodeSize + 1);

    if (!m_Data.spirvByteCode)
        throw std::runtime_error("failed to allocate memory for shader bytecode.");

    // Read in the file to memory.
    size_t result = fread(m_Data.spirvByteCode, 1, byteCodeSize, file);

    if (result != byteCodeSize)
    {
        printf("failed to read bytecode.");
        free(m_Data.spirvByteCode);
        fclose(file);
    }

    // Close file. 
    fclose(file);

    // VK Shader Object Info
    // -------------------

    m_Info = 
    {
        .stages = stage,
        .shader = 
        {
	        .sType                  = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
	        .pNext                  = nullptr,
	        .flags                  = 0,
	        .stage                  = stage,
	        .nextStage              = nextStage,
	        .codeType               = VK_SHADER_CODE_TYPE_SPIRV_EXT,
	        .codeSize               = (size_t)byteCodeSize,
	        .pCode                  = m_Data.spirvByteCode,
	        .pName                  = "main",
	        .setLayoutCount         = 0,
	        .pSetLayouts            = nullptr,
	        .pushConstantRangeCount = 0,
	        .pPushConstantRanges    = nullptr,
	        .pSpecializationInfo    = nullptr
        }
    };
}

void Shader::Bind(VkCommandBuffer commandBuffer, Shader& shader)
{
    auto shaderObject = shader.GetData()->shader;
    auto shaderStage  = shader.GetInfo()->stages;
    Device::vkCmdBindShadersEXT(commandBuffer, 1u, &shaderStage, &shaderObject);
}