# Vulkan Demo Wrappers

These wrappers attempt to provide a dead-simple cross-platform functionality for demo-style applications and prototyping with Vulkan. 

It simplifies the creation of an OS-agnostic window with a triple-buffered swapchain, and calls two user-defined callbacks for resource initialization and command recording. 
## Usage (main.cpp)

```

#include <RenderInstance.h>
#include <GraphicsResource.h>

#include <iostream>
#include <memory>

using namespace Wrappers;

// Resources
// ----------------------

#define PIPELINE_TRIANGLE "triangle"

// Implementation
// ----------------------

void InitFunc(InitializeContext context)
{
    context.resources[PIPELINE_TRIANGLE] = std::make_unique<Pipeline>();
    {
        auto trianglePipeline = static_cast<Pipeline*>(context.resources[PIPELINE_TRIANGLE].get());

        // Configure the triangle pipeline.
        trianglePipeline->SetShaderProgram("build/vert.spv", "build/frag.spv");
        trianglePipeline->SetScissor(context.backBufferScissor);
        trianglePipeline->SetViewport(context.backBufferViewport);
        trianglePipeline->SetColorTargetFormats( { context.backBufferFormat } );
        trianglePipeline->Commit();
    }
}

void RenderFunc(RenderContext context)
{
    // Shorthand for command.
    auto cmd = context.commandBuffer;

    VkRenderingAttachmentInfoKHR colorAttachment
    {
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView   = context.backBufferView,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue.color = {1, 0, 0, 1}
    };

    VkRenderingInfoKHR renderInfo
    {
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea           = context.backBufferScissor,
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &colorAttachment,
        .pDepthAttachment     = nullptr,
        .pStencilAttachment   = nullptr
    };

    vkCmdBeginRendering(cmd, &renderInfo);

    // Issue native Vulkan commands
    context.resources.at(PIPELINE_TRIANGLE)->Bind(cmd);
    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRendering(cmd);
}

// Entry
// ----------------------

int main(int argc, char *argv[]) 
{
    try 
    {
        RenderInstance renderInstance(800, 600);

        return renderInstance.Execute(InitFunc, RenderFunc);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1; 
    }
}

```
