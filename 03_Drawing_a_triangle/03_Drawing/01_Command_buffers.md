Commands in Vulkan, like drawing operations and memory transfers, are not
executed directly using function calls. You have to record all of the operations
you want to perform in command buffer objects. The advantage of this is that all
of the hard work of setting up the drawing commands can be done in advance and
in multiple threads. After that, you just have to tell Vulkan to execute the
commands in the main loop.

## Command pools

We have to create a command pool before we can create command buffers. Command
pools manage the memory that is used to store the buffers and command buffers
are allocated from them. Add a new class member to store a `VkCommandPool`:

```c++
VkCommandPool commandPool;
```

Then create a new function `createCommandPool` and call it from `initVulkan`
after the framebuffers were created.

```c++
void initVulkan() {
    createInstance();
    setupDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
}

...

void createCommandPool() {

}
```

Command pool creation only takes two parameters:

```c++
QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

VkCommandPoolCreateInfo poolInfo = {};
poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
poolInfo.flags = 0; // Optional
```

Command buffers are executed by submitting them on one of the device queues,
like the graphics and presentation queues we retrieved. Each command pool can
only allocate command buffers that are submitted on a single type of queue.
We're going to record commands for drawing, which is why we've chosen the
graphics queue family.

There are two possible flags for command pools:

* `VK_COMMAND_POOL_CREATE_TRANSIENT_BIT`: Hint that command buffers are
rerecorded with new commands very often (may change memory allocation behavior)
* `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`: Allow command buffers to be
rerecorded individually, without this flag they all have to be reset together

We will only record the command buffers at the beginning of the program and then
execute them many times in the main loop, so we're not going to use either of
these flags.

```c++
if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
}
```

Finish creating the command pool using the `vkCreateCommandPool` function. It
doesn't have any special parameters. Commands will be used throughout the
program to draw things on the screen, so the pool should only be destroyed at
the end:

```c++
void cleanup() {
    vkDestroyCommandPool(device, commandPool, nullptr);

    ...
}
```

## Command buffer allocation

We can now start allocating command buffers and recording drawing commands in
them. Because one of the drawing commands involves binding the right
`VkFramebuffer`, we'll actually have to record a command buffer for every image
in the swap chain once again. To that end, create a list of `VkCommandBuffer`
objects as class member. Command buffers will be automatically freed when their
command pool is destroyed, so we don't need an explicit cleanup.

```c++
std::vector<VkCommandBuffer> commandBuffers;
```

We'll now start working on a `createCommandBuffers` function that allocates and
records the commands for each swap chain image.

```c++
void initVulkan() {
    createInstance();
    setupDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
}

...

void createCommandBuffers() {
    commandBuffers.resize(swapChainFramebuffers.size());
}
```

Command buffers are allocated with the `vkAllocateCommandBuffers` function,
which takes a `VkCommandBufferAllocateInfo` struct as parameter that specifies
the command pool and number of buffers to allocate:

```c++
VkCommandBufferAllocateInfo allocInfo = {};
allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
allocInfo.commandPool = commandPool;
allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
}
```

The `level` parameter specifies if the allocated command buffers are primary or
secondary command buffers.

* `VK_COMMAND_BUFFER_LEVEL_PRIMARY`: Can be submitted to a queue for execution,
but cannot be called from other command buffers.
* `VK_COMMAND_BUFFER_LEVEL_SECONDARY`: Cannot be submitted directly, but can be
called from primary command buffers.

We won't make use of the secondary command buffer functionality here, but you
can imagine that it's helpful to reuse common operations from primary command
buffers.

## Starting command buffer recording

We begin recording a command buffer by calling `vkBeginCommandBuffer` with a
small `VkCommandBufferBeginInfo` structure as argument that specifies some
details about the usage of this specific command buffer.

```c++
for (size_t i = 0; i < commandBuffers.size(); i++) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr; // Optional

    vkBeginCommandBuffer(commandBuffers[i], &beginInfo);
}
```

The `flags` parameter specifies how we're going to use the command buffer. The
following values are available:

* `VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT`: The command buffer will be
rerecorded right after executing it once.
* `VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT`: This is a secondary
command buffer that will be entirely within a single render pass.
* `VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT`: The command buffer can be
resubmitted while it is also already pending execution.

We have used the last flag because we may already be scheduling the drawing
commands for the next frame while the last frame is not finished yet. The
`pInheritanceInfo` parameter is only relevant for secondary command buffers. It
specifies which state to inherit from the calling primary command buffers.

If the command buffer was already recorded once, then a call to
`vkBeginCommandBuffer` will implicitly reset it. It's not possible to append
commands to a buffer at a later time.

## Starting a render pass

Drawing starts by beginning the render pass with `vkCmdBeginRenderPass`. The
render pass is configured using some parameters in a `VkRenderPassBeginInfo`
struct.

```c++
VkRenderPassBeginInfo renderPassInfo = {};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
renderPassInfo.renderPass = renderPass;
renderPassInfo.framebuffer = swapChainFramebuffers[i];
```

The first parameters are the render pass itself and the attachments to bind. We
created a framebuffer for each swap chain image that specifies it as color
attachment.

```c++
renderPassInfo.renderArea.offset = {0, 0};
renderPassInfo.renderArea.extent = swapChainExtent;
```

The next two parameters define the size of the render area. The render area
defines where shader loads and stores will take place. The pixels outside this
region will have undefined values. It should match the size of the attachments
for best performance.

```c++
VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
renderPassInfo.clearValueCount = 1;
renderPassInfo.pClearValues = &clearColor;
```

The last two parameters define the clear values to use for
`VK_ATTACHMENT_LOAD_OP_CLEAR`, which we used as load operation for the color
attachment. I've defined the clear color to simply be black with 100% opacity.

```c++
vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
```

The render pass can now begin. All of the functions that record commands can be
recognized by their `vkCmd` prefix. They all return `void`, so there will be no
error handling until we've finished recording.

The first parameter for every command is always the command buffer to record the
command to. The second parameter specifies the details of the render pass we've
just provided. The final parameter controls how the drawing commands within the
render pass will be provided. It can have one of two values:

* `VK_SUBPASS_CONTENTS_INLINE`: The render pass commands will be embedded in
the primary command buffer itself and no secondary command buffers will be
executed.
* `VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS`: The render pass commands will
be executed from secondary command buffers.

We will not be using secondary command buffers, so we'll go with the first
option.

## Basic drawing commands

We can now bind the graphics pipeline:

```c++
vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
```

The second parameter specifies if the pipeline object is a graphics or compute
pipeline. We've now told Vulkan which operations to execute in the graphics
pipeline and which attachment to use in the fragment shader, so all that remains
is telling it to draw the triangle:

```c++
vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
```

The actual `vkCmdDraw` function is a bit anticlimactic, but it's so simple
because of all the information we specified in advance. It has the following
parameters, aside from the command buffer:

* `vertexCount`: Even though we don't have a vertex buffer, we technically still
have 3 vertices to draw.
* `instanceCount`: Used for instanced rendering, use `1` if you're not doing
that.
* `firstVertex`: Used as an offset into the vertex buffer, defines the lowest
value of `gl_VertexIndex`.
* `firstInstance`: Used as an offset for instanced rendering, defines the lowest
value of `gl_InstanceIndex`.

## Finishing up

The render pass can now be ended:

```c++
vkCmdEndRenderPass(commandBuffers[i]);
```

And we've finished recording the command buffer:

```c++
if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
}
```

In the next chapter we'll write the code for the main loop, which will acquire
an image from the swap chain, execute the right command buffer and return the
finished image to the swap chain.

[C++ code](/code/command_buffers.cpp) /
[Vertex shader](/code/shader_base.vert) /
[Fragment shader](/code/shader_base.frag)
