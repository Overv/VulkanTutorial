We've talked a lot about framebuffers in the past few chapters and we've set up
the render pass to expect a single framebuffer with the same format as the swap
chain images, but we haven't actually created any yet.

The attachments specified during render pass creation are bound by wrapping them
into a `VkFramebuffer` object. A framebuffer object references all of the
`VkImageView` objects that represent the attachments. In our case that will be
only a single one: the color attachment. However, the image that we have to use
for the attachment depends on which image the swap chain returns when we retrieve one
for presentation. That means that we have to create a framebuffer for all of the
images in the swap chain and use the one that corresponds to the retrieved image
at drawing time.

To that end, create another `std::vector` class member to hold the framebuffers:

```c++
std::vector<VkFramebuffer> swapChainFramebuffers;
```

We'll create the objects for this array in a new function `createFramebuffers`
that is called from `initVulkan` right after creating the graphics pipeline:

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
    createFramebuffers();
}

...

void createFramebuffers() {

}
```

Start by resizing the container to hold all of the framebuffers:

```c++
void createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());
}
```

We'll then iterate through the image views and create framebuffers from them:

```c++
for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    VkImageView attachments[] = {
        swapChainImageViews[i]
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
    }
}
```

As you can see, creation of framebuffers is quite straightforward. We first need
to specify with which `renderPass` the framebuffer needs to be compatible. You
can only use a framebuffer with the render passes that it is compatible with,
which roughly means that they use the same number and type of attachments.

The `attachmentCount` and `pAttachments` parameters specify the `VkImageView`
objects that should be bound to the respective attachment descriptions in
the render pass `pAttachment` array.

The `width` and `height` parameters are self-explanatory and `layers` refers to
the number of layers in image arrays. Our swap chain images are single images,
so the number of layers is `1`.

We should delete the framebuffers before the image views and render pass that
they are based on, but only after we've finished rendering:

```c++
void cleanup() {
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    ...
}
```

We've now reached the milestone where we have all of the objects that are
required for rendering. In the next chapter we're going to write the first
actual drawing commands.

[C++ code](/code/13_framebuffers.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
