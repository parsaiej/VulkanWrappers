# Vulkan Demo Wrappers

These wrappers attempt to provide a dead-simple cross-platform functionality for demo-style applications and prototyping with Vulkan. 

It simplifies the creation of an OS-agnostic window with a triple-buffered swapchain, and calls two user-defined callbacks for resource initialization and command recording. 
## Usage (main.cpp)

```

#include <VulkanWrappers/Device.h>
#include <VulkanWrappers/Window.h>
#include <VulkanWrappers/Image.h>
#include <VulkanWrappers/Shader.h>

#include <memory>

using namespace VulkanWrappers;

// Resources
// ---------------------

static std::unique_ptr<Shader> s_TriangleVert;
static std::unique_ptr<Shader> s_TriangleFrag;

void CreateResources(Device& device)
{
    s_TriangleVert = std::make_unique<Shader>("assets/TriangleVert.spv", VK_SHADER_STAGE_VERTEX_BIT,   VK_SHADER_STAGE_FRAGMENT_BIT);
    s_TriangleFrag = std::make_unique<Shader>("assets/TriangleFrag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, 0x0);

    device.CreateShaders ({ s_TriangleVert.get(), s_TriangleFrag.get() });
}

void ReleaseResources(Device& device)
{
    device.ReleaseShaders ({ s_TriangleVert.get(), s_TriangleFrag.get() });
}

// Implementation
// ---------------------

int main()
{
    // Create window. 
    Window window("Sample", 800, 600);

    // Initialize graphics device usage with the window. 
    Device device(&window);

    // Build all the runtime resources. 
    CreateResources(device);

    // Handle to current frame to write commands to. 
    Frame frame;

    while (window.NextFrame(&device, &frame))
    {
        auto cmd = frame.commandBuffer;

        Image::TransferBackbufferToWrite(cmd, &device, frame.backBuffer);

        VkRenderingAttachmentInfoKHR colorAttachment
        {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView   = frame.backBufferView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue.color = { 0, 0, 0, 1 }
        };

        VkRenderingInfoKHR renderInfo
        {
            .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .renderArea           = window.GetScissor(),
            .layerCount           = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &colorAttachment,
            .pDepthAttachment     = nullptr,
            .pStencilAttachment   = nullptr
        };

        // Write commands for this frame. 
        Device::vkCmdBeginRenderingKHR(cmd, &renderInfo);

        Shader::Bind(cmd, *s_TriangleVert);
        Shader::Bind(cmd, *s_TriangleFrag);
        Device::SetDefaultRenderState(cmd);
        vkCmdDraw(cmd, 3u, 1u, 0u, 0u);

        Device::vkCmdEndRenderingKHR(cmd);

        Image::TransferBackbufferToPresent(cmd, &device, frame.backBuffer);

        window.SubmitFrame(&device, &frame);
    }

    ReleaseResources(device);

    return 0;
}

```
