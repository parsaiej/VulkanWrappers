#include "GraphicsResource.h"

#include <stdio.h>
#include <stdlib.h>

#include <iostream>

using namespace Wrappers;

VkDevice* GraphicsResource::s_VKDevice = nullptr;

#define SAFE_RELEASE(func, value) \
    if (value != VK_NULL_HANDLE)  \
        func(*s_VKDevice, value, nullptr);

// NOTE: This implementation was re-used from an old C project.
void TryCreateShaderModule(VkDevice device, VkShaderModule* module, const char* filePath)
{
    // Read byte code from file.
    // -------------------

    FILE* file = fopen(filePath, "rb");

    if (!file)
        throw std::runtime_error("failed to open shader byte code.");

    // Determine the byte code file size.

    fseek(file, 0, SEEK_END);
    long byteCodeSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate the byteCode memory. 
    char* byteCode = (char*)malloc(byteCodeSize + 1);

    if (!byteCode)
        throw std::runtime_error("failed to allocate memory for shader bytecode.");

    // Read in the file to memory.
    size_t result = fread(byteCode, 1, byteCodeSize, file);

    if (result != byteCodeSize)
    {
        printf("failed to read bytecode.");
        free(byteCode);
        fclose(file);
    }

    // Close file. 
    fclose(file);

    VkShaderModuleCreateInfo moduleInfo =
    {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = (size_t)byteCodeSize,
        .pCode    = (uint32_t*)byteCode,
    };

    if (*module != VK_NULL_HANDLE)
    {
        // Release if needed. 
        vkDestroyShaderModule(device, *module, nullptr);
    }

    if (vkCreateShaderModule(device, &moduleInfo, NULL, module) != VK_SUCCESS)
        throw std::runtime_error("failed to create shader module.");
}

// Pipeline Implementation
// ----------------------------------

void Pipeline::SetShaderProgram(const char* filePathVertex, const char* filePathFragment)
{
    // Most recent caller defines whether to create a VkComputePipeline or not.
    m_IsCompute = false;

    TryCreateShaderModule(*s_VKDevice, &m_VKShaderVS, filePathVertex);
    TryCreateShaderModule(*s_VKDevice, &m_VKShaderFS, filePathFragment);
}

void Pipeline::SetShaderProgram(const char* filePathCompute)
{
    // Most recent caller defines whether to create a VkComputePipeline or not.
    m_IsCompute = true;

    TryCreateShaderModule(*s_VKDevice, &m_VKShaderCS, filePathCompute);
}

void Pipeline::Release(VkDevice device)
{
    SAFE_RELEASE(vkDestroyShaderModule,   m_VKShaderVS);
    SAFE_RELEASE(vkDestroyShaderModule,   m_VKShaderFS);
    SAFE_RELEASE(vkDestroyShaderModule,   m_VKShaderCS);
    SAFE_RELEASE(vkDestroyPipelineLayout, m_VKPipelineLayout);
    SAFE_RELEASE(vkDestroyPipeline,       m_VKPipeline);
}

void Pipeline::UpdateLayout(VkCommandBuffer cmd, void* constants, uint32_t constantsSize)
{
    vkCmdPushConstants(cmd, m_VKPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, constantsSize, constants);
}

void Pipeline::Commit()
{
    // Destroy any pre-existing objects (not shader modules).
    SAFE_RELEASE(vkDestroyPipelineLayout, m_VKPipelineLayout);
    SAFE_RELEASE(vkDestroyPipeline,       m_VKPipeline);

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    auto PushShaderStage = [&](VkShaderStageFlagBits stageBits, VkShaderModule module) -> void
    {
        VkPipelineShaderStageCreateInfo shaderStageInfo
        {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext  = nullptr,
            .stage  = stageBits,
            .module = module,
            .pName  = "main",
            .flags  = 0,
            .pSpecializationInfo = NULL
        };

        shaderStages.push_back(shaderStageInfo);
    };

    if (m_IsCompute)
       PushShaderStage(VK_SHADER_STAGE_COMPUTE_BIT, m_VKShaderCS);
    else
    {
       PushShaderStage(VK_SHADER_STAGE_VERTEX_BIT,   m_VKShaderVS);
       PushShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, m_VKShaderFS);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo =
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = (uint32_t)m_PushConstants.size(),
        .pPushConstantRanges    = m_PushConstants.data()
    };

    if (vkCreatePipelineLayout(*s_VKDevice, &pipelineLayoutInfo, nullptr, &m_VKPipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("failed to create pipeline layout.");

    if (m_IsCompute)
    {
        VkComputePipelineCreateInfo pipelineInfo
        {
            .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage  = shaderStages[0],
            .layout = m_VKPipelineLayout
        };

        if (vkCreateComputePipelines(*s_VKDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VKPipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create compute pipeline.");
    }
    else
    {
        // Vertex Layout

        VkPipelineVertexInputStateCreateInfo vertexInputInfo =
        {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 0,
            .pVertexBindingDescriptions      = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions    = nullptr
        };

        // Input Assembly

        VkPipelineInputAssemblyStateCreateInfo inputAssembly =
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        };

        // Rasterizer

        VkPipelineRasterizationStateCreateInfo rasterizer =
        {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable        = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode             = VK_POLYGON_MODE_FILL,
            .lineWidth               = 1.0f,
            .cullMode                = VK_CULL_MODE_NONE,
            .frontFace               = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable         = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp          = 0.0f, 
            .depthBiasSlopeFactor    = 0.0f 
        };

        // MSAA

        VkPipelineMultisampleStateCreateInfo multisampling =
        {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .sampleShadingEnable   = VK_FALSE,
            .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
            .minSampleShading      = 1.0f,
            .pSampleMask           = nullptr, 
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable      = VK_FALSE,   
        };

        // Blend State

        VkPipelineColorBlendAttachmentState colorBlendAttachment =
        {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable    = VK_FALSE
        };

        VkPipelineColorBlendStateCreateInfo colorBlending =
        {
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable   = VK_FALSE,
            .attachmentCount = 1,
            .pAttachments    = &colorBlendAttachment
        };

        // Viewport

        VkPipelineViewportStateCreateInfo viewportState =
        {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .scissorCount  = 1,
            .pScissors     = &m_VKScissor,
            .viewportCount = 1,
            .pViewports    = &m_VKViewport
        };

        VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo = 
        {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
            .colorAttachmentCount    = (uint32_t)m_ColorRTFormats.size(),
            .pColorAttachmentFormats = m_ColorRTFormats.data()
        };

        VkGraphicsPipelineCreateInfo pipelineInfo = 
        {
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext               = &pipelineRenderingCreateInfo,
            .stageCount          = 2,
            .pStages             = shaderStages.data(),
            .pVertexInputState   = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState      = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pDepthStencilState  = nullptr,
            .pColorBlendState    = &colorBlending,
            .pDynamicState       = nullptr,
            .layout              = m_VKPipelineLayout,
            .basePipelineHandle  = VK_NULL_HANDLE,
            .basePipelineIndex   = -1,

            // None due to dynamic rendering extension. 
            .renderPass          = nullptr,
            .subpass             = 0
        };

        if (vkCreateGraphicsPipelines(*s_VKDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VKPipeline) != VK_SUCCESS) 
            throw std::runtime_error("failed to create graphics pipeline.");
    }
}

// Buffer Implementation
// ----------------------------------

// Image Implementation
// ----------------------------------