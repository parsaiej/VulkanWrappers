#include <VulkanWrappers/Shader.h>
#include <VulkanWrappers/Device.h>

#include <iostream>

using namespace VulkanWrappers;

Shader::Shader(const char* spirvFilePath, VkShaderStageFlagBits stage, VkShaderStageFlags nextStage)
    : m_VKStageFlagBits(stage)
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
    m_SPIRVByteCode = malloc(byteCodeSize + 1);

    if (!m_SPIRVByteCode)
        throw std::runtime_error("failed to allocate memory for shader bytecode.");

    // Read in the file to memory.
    size_t result = fread(m_SPIRVByteCode, 1, byteCodeSize, file);

    if (result != byteCodeSize)
    {
        printf("failed to read bytecode.");
        free(m_SPIRVByteCode);
        fclose(file);
    }

    // Close file. 
    fclose(file);

    // VK Shader Object Info
    // -------------------

	m_VKShaderCreateInfo.sType                  = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
	m_VKShaderCreateInfo.pNext                  = nullptr;
	m_VKShaderCreateInfo.flags                  = 0;
	m_VKShaderCreateInfo.stage                  = stage;
	m_VKShaderCreateInfo.nextStage              = nextStage;
	m_VKShaderCreateInfo.codeType               = VK_SHADER_CODE_TYPE_SPIRV_EXT;
	m_VKShaderCreateInfo.codeSize               = byteCodeSize;
	m_VKShaderCreateInfo.pCode                  = m_SPIRVByteCode;
	m_VKShaderCreateInfo.pName                  = "main";
	m_VKShaderCreateInfo.setLayoutCount         = 0;
	m_VKShaderCreateInfo.pSetLayouts            = nullptr;
	m_VKShaderCreateInfo.pushConstantRangeCount = 0;
	m_VKShaderCreateInfo.pPushConstantRanges    = nullptr;
	m_VKShaderCreateInfo.pSpecializationInfo    = nullptr;
}

void Shader::Bind(VkCommandBuffer commandBuffer, const Shader& shader)
{
    auto shaderObject = shader.GetPrimitive();
    auto shaderStage  = shader.GetStage();
    Device::vkCmdBindShadersEXT(commandBuffer, 1u, &shaderStage, &shaderObject);
}