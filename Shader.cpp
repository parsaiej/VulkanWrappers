#include <VulkanWrappers/Shader.h>
#include <VulkanWrappers/Device.h>

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
    m_Info = {};
    m_Info.stages = stage;

    m_Info.shader = {};
    m_Info.shader.sType                  = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
    m_Info.shader.pNext                  = nullptr;
    m_Info.shader.flags                  = 0;
    m_Info.shader.stage                  = stage;
    m_Info.shader.nextStage              = nextStage;
    m_Info.shader.codeType               = VK_SHADER_CODE_TYPE_SPIRV_EXT;
    m_Info.shader.codeSize               = (size_t)byteCodeSize;
    m_Info.shader.pCode                  = m_Data.spirvByteCode;
    m_Info.shader.pName                  = "main";
    m_Info.shader.setLayoutCount         = 0;
    m_Info.shader.pSetLayouts            = nullptr;
    m_Info.shader.pushConstantRangeCount = 0;
    m_Info.shader.pPushConstantRanges    = nullptr;
    m_Info.shader.pSpecializationInfo    = nullptr;

}

void Shader::Bind(VkCommandBuffer commandBuffer, Shader& shader)
{
    auto shaderObject = shader.GetData()->shader;
    auto shaderStage  = shader.GetInfo()->stages;
    vkCmdBindShadersEXT(commandBuffer, 1u, &shaderStage, &shaderObject);
}