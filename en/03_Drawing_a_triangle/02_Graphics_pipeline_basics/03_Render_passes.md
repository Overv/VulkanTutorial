## Setup

Before we can finish creating the pipeline, we need to tell Vulkan about the
framebuffer attachments that will be used while rendering. We need to specify
how many color and depth buffers there will be, how many samples to use for each
of them and how their contents should be handled throughout the rendering
operations. All of this information is wrapped in a *render pass* object, for
which we'll create a new `createRenderPass` function. Call this function from
`initVulkan` before `createGraphicsPipeline`.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
}

...

void createRenderPass() {

}
```

## Attachment description

In our case we'll have just a single color buffer attachment represented by one
of the images from the swap chain.

```c++
void createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
}
```

The `format` of the color attachment should match the format of the swap chain
images, and we're not doing anything with multisampling yet, so we'll stick to 1
sample.

```c++
colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
```

The `loadOp` and `storeOp` determine what to do with the data in the attachment
before rendering and after rendering. We have the following choices for
`loadOp`:

* `VK_ATTACHMENT_LOAD_OP_LOAD`: Preserve the existing contents of the attachment
* `VK_ATTACHMENT_LOAD_OP_CLEAR`: Clear the values to a constant at the start
* `VK_ATTACHMENT_LOAD_OP_DONT_CARE`: Existing contents are undefined; we don't
care about them

In our case we're going to use the clear operation to clear the framebuffer to
black before drawing a new frame. There are only two possibilities for the
`storeOp`:

* `VK_ATTACHMENT_STORE_OP_STORE`: Rendered contents will be stored in memory and
can be read later
* `VK_ATTACHMENT_STORE_OP_DONT_CARE`: Contents of the framebuffer will be
undefined after the rendering operation

We're interested in seeing the rendered triangle on the screen, so we're going
with the store operation here.

```c++
colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
```

The `loadOp` and `storeOp` apply to color and depth data, and `stencilLoadOp` /
`stencilStoreOp` apply to stencil data. Our application won't do anything with
the stencil buffer, so the results of loading and storing are irrelevant.

```c++
colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
```

Textures and framebuffers in Vulkan are represented by `VkImage` objects with a
certain pixel format, however the layout of the pixels in memory can change
based on what you're trying to do with an image.

Some of the most common layouts are:

* `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`: Images used as color attachment
* `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`: Images to be presented in the swap chain
* `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`: Images to be used as destination for a
memory copy operation

We'll discuss this topic in more depth in the texturing chapter, but what's
important to know right now is that images need to be transitioned to specific
layouts that are suitable for the operation that they're going to be involved in
next.

The `initialLayout` specifies which layout the image will have before the render
pass begins. The `finalLayout` specifies the layout to automatically transition
to when the render pass finishes. Using `VK_IMAGE_LAYOUT_UNDEFINED` for
`initialLayout` means that we don't care what previous layout the image was in.
The caveat of this special value is that the contents of the image are not
guaranteed to be preserved, but that doesn't matter since we're going to clear
it anyway. We want the image to be ready for presentation using the swap chain
after rendering, which is why we use `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` as
`finalLayout`.

## Subpasses and attachment references

A single render pass can consist of multiple subpasses. Subpasses are subsequent
rendering operations that depend on the contents of framebuffers in previous
passes, for example a sequence of post-processing effects that are applied one
after another. If you group these rendering operations into one render pass,
then Vulkan is able to reorder the operations and conserve memory bandwidth for
possibly better performance. For our very first triangle, however, we'll stick
to a single subpass.

Every subpass references one or more of the attachments that we've described
using the structure in the previous sections. These references are themselves
`VkAttachmentReference` structs that look like this:

```c++
VkAttachmentReference colorAttachmentRef{};
colorAttachmentRef.attachment = 0;
colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
```

The `attachment` parameter specifies which attachment to reference by its index
in the attachment descriptions array. Our array consists of a single
`VkAttachmentDescription`, so its index is `0`. The `layout` specifies which
layout we would like the attachment to have during a subpass that uses this
reference. Vulkan will automatically transition the attachment to this layout
when the subpass is started. We intend to use the attachment to function as a
color buffer and the `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL` layout will give
us the best performance, as its name implies.

The subpass is described using a `VkSubpassDescription` structure:

```c++
VkSubpassDescription subpass{};
subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
```

Vulkan may also support compute subpasses in the future, so we have to be
explicit about this being a graphics subpass. Next, we specify the reference to
the color attachment:

```c++
subpass.colorAttachmentCount = 1;
subpass.pColorAttachments = &colorAttachmentRef;
```

The index of the attachment in this array is directly referenced from the
fragment shader with the `layout(location = 0) out vec4 outColor` directive!

The following other types of attachments can be referenced by a subpass:

* `pInputAttachments`: Attachments that are read from a shader
* `pResolveAttachments`: Attachments used for multisampling color attachments
* `pDepthStencilAttachment`: Attachment for depth and stencil data
* `pPreserveAttachments`: Attachments that are not used by this subpass, but for
which the data must be preserved

## Render pass

Now that the attachment and a basic subpass referencing it have been described,
we can create the render pass itself. Create a new class member variable to hold
the `VkRenderPass` object right above the `pipelineLayout` variable:

```c++
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
```

The render pass object can then be created by filling in the
`VkRenderPassCreateInfo` structure with an array of attachments and subpasses.
The `VkAttachmentReference` objects reference attachments using the indices of
this array.

```c++
VkRenderPassCreateInfo renderPassInfo{};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
renderPassInfo.attachmentCount = 1;
renderPassInfo.pAttachments = &colorAttachment;
renderPassInfo.subpassCount = 1;
renderPassInfo.pSubpasses = &subpass;

if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
}
```

Just like the pipeline layout, the render pass will be referenced throughout the
program, so it should only be cleaned up at the end:

```c++
void cleanup() {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    ...
}
```

That was a lot of work, but in the next chapter it all comes together to finally
create the graphics pipeline object!

[C++ code](/code/11_render_passes.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
