#include "GraphicsResource.h"

#include <stdio.h>
#include <stdlib.h>

#include <iostream>

using namespace Wrappers;

RenderInstance* GraphicsResource::s_RenderInstance = nullptr;

#define SAFE_RELEASE(func, value) \
    if (value != VK_NULL_HANDLE)  \
        func(s_RenderInstance->GetDevice(), value, nullptr);

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

    TryCreateShaderModule(s_RenderInstance->GetDevice(), &m_VKShaderVS, filePathVertex);
    TryCreateShaderModule(s_RenderInstance->GetDevice(), &m_VKShaderFS, filePathFragment);
}

void Pipeline::SetShaderProgram(const char* filePathCompute)
{
    // Most recent caller defines whether to create a VkComputePipeline or not.
    m_IsCompute = true;

    TryCreateShaderModule(s_RenderInstance->GetDevice(), &m_VKShaderCS, filePathCompute);
}

void Pipeline::Release()
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

    if (vkCreatePipelineLayout(s_RenderInstance->GetDevice(), &pipelineLayoutInfo, nullptr, &m_VKPipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("failed to create pipeline layout.");

    if (m_IsCompute)
    {
        VkComputePipelineCreateInfo pipelineInfo
        {
            .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage  = shaderStages[0],
            .layout = m_VKPipelineLayout
        };

        if (vkCreateComputePipelines(s_RenderInstance->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VKPipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create compute pipeline.");
    }
    else
    {
        // Vertex Layout

        VkPipelineVertexInputStateCreateInfo vertexInputInfo =
        {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = (uint32_t)m_VKVertexBindings.size(),
            .pVertexBindingDescriptions      = m_VKVertexBindings.data(),
            .vertexAttributeDescriptionCount = (uint32_t)m_VKVertexAttributes.size(),
            .pVertexAttributeDescriptions    = m_VKVertexAttributes.data()
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
            .pColorAttachmentFormats = m_ColorRTFormats.data(),
            .depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT,
        };

        VkPipelineDepthStencilStateCreateInfo depthStencilInfo
        {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable       = VK_TRUE,
            .depthWriteEnable      = VK_TRUE,
            .depthCompareOp        = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .minDepthBounds        = 0.0,
            .maxDepthBounds        = 1.0,
            .stencilTestEnable     = VK_FALSE,
            .front                 = {},
            .back                  = {}
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
            .pDepthStencilState  = &depthStencilInfo,
            .pColorBlendState    = &colorBlending,
            .pDynamicState       = nullptr,
            .layout              = m_VKPipelineLayout,
            .basePipelineHandle  = VK_NULL_HANDLE,
            .basePipelineIndex   = -1,

            // None due to dynamic rendering extension. 
            .renderPass          = nullptr,
            .subpass             = 0
        };

        if (vkCreateGraphicsPipelines(s_RenderInstance->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VKPipeline) != VK_SUCCESS) 
            throw std::runtime_error("failed to create graphics pipeline.");
    }
}

// Buffer Implementation
// ----------------------------------

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags bufferFlags, VkMemoryPropertyFlags memoryFlags, VmaAllocationCreateFlags allocFlags)
{
    VkBufferCreateInfo bufferInfo
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = size,
        .usage = bufferFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaAllocationCreateInfo allocInfo = 
    {
        .flags         = allocFlags,
        .usage         = VMA_MEMORY_USAGE_UNKNOWN,
        .requiredFlags = memoryFlags,
    };

    vmaCreateBuffer(s_RenderInstance->GetMemoryAllocator(), &bufferInfo, &allocInfo, &m_VKBuffer, &m_VMAAllocation, nullptr);
}

void Buffer::SetData(void* srcPtr, uint32_t size)
{
    // Create a staging buffer resource to map data. 

    Buffer stagingBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    // Map data to device. 
    void* dstPtr;
    vmaMapMemory(s_RenderInstance->GetMemoryAllocator(), stagingBuffer.m_VMAAllocation, &dstPtr);
    memcpy(dstPtr, srcPtr, size);
    vmaUnmapMemory(s_RenderInstance->GetMemoryAllocator(), stagingBuffer.m_VMAAllocation);

    // Create and dispatch  a copy command 
    VkCommandBufferAllocateInfo commandAllocInfo
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool        = s_RenderInstance->GetCommandPool(),
        .commandBufferCount = 1
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(s_RenderInstance->GetDevice(), &commandAllocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    VkBufferCopy copyRegion
    {
        .srcOffset = 0,
        .dstOffset = 0,
        .size      = size
    };
    vkCmdCopyBuffer(commandBuffer, stagingBuffer.m_VKBuffer, m_VKBuffer, 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    vkQueueSubmit(s_RenderInstance->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(s_RenderInstance->GetGraphicsQueue());
    vkFreeCommandBuffers(s_RenderInstance->GetDevice(), s_RenderInstance->GetCommandPool(), 1, &commandBuffer);

    stagingBuffer.Release();
}

void Buffer::Release()
{
    vmaDestroyBuffer(s_RenderInstance->GetMemoryAllocator(), m_VKBuffer, m_VMAAllocation);
}

// Image Implementation
// ----------------------------------

Image::Image(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage)
{
    VkImageCreateInfo createInfo = 
    {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .extent.width  = width,
        .extent.height = height,
        .extent.depth  = 1,
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .format        = format,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage         = usage,
        .samples       = VK_SAMPLE_COUNT_1_BIT
    };
    
    VmaAllocationCreateInfo allocInfo = 
    {
        .usage    = VMA_MEMORY_USAGE_AUTO,
        .flags    = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .priority = 1.0
    };
    
    vmaCreateImage(s_RenderInstance->GetMemoryAllocator(), &createInfo, &allocInfo, &m_VKImage, &m_VMAAllocation, nullptr);
}

void Image::Release()
{
    vmaDestroyImage(s_RenderInstance->GetMemoryAllocator(), m_VKImage, m_VMAAllocation);
}

// Image View Implementation
// ----------------------------------

ImageView::ImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo createInfo =
    {
        .sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image        = image,
        .viewType     = VK_IMAGE_VIEW_TYPE_2D,
        .format       = format,

        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,

        .subresourceRange.aspectMask     = aspectFlags,
        .subresourceRange.baseMipLevel   = 0,
        .subresourceRange.levelCount     = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount     = 1
    };

    if (vkCreateImageView(s_RenderInstance->GetDevice(), &createInfo, NULL, &m_VKImageView) != VK_SUCCESS)
        throw std::runtime_error("failed to create swap chain image view.");
}

void ImageView::Release()
{
    vkDestroyImageView(s_RenderInstance->GetDevice(), m_VKImageView, nullptr);
}