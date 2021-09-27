
This is the chapter where everything is going to come together. We're going to
write the `drawFrame` function that will be called from the main loop to put the
triangle on the screen. Lets start by creating the function and call it from
`mainLoop`:

```c++
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
}

...

void drawFrame() {

}
```

## Outline of a frame

At a high level, rendering a frame in Vulkan consists of a common set of steps:

* Wait for previous frame to finish
* Acquire an image from the swapchain
* Record command buffers which draw the scene onto that image
* Submit the recorded command buffers
* Present the swapchain image

While we will expand the drawing function in later chapters, for now this is the
core of our render loop.

## Synchronization

Inside the `drawFrame` function, we perform the following operations on the swapchain:

* Acquire an image from the swap chain
* Record drawing commands that draw onto the image, which is an attachment in a framebuffer
* Return the image to the swap chain for presentation

Each of these events is set in motion using a single function call, but they are
executed asynchronously. The function calls will return before the operations
are actually finished and the order of execution is also undefined. That is
unfortunate, because each of the operations depends on the previous one
finishing.

There are two ways of synchronizing swap chain events: fences and semaphores.
They're both objects that can be used for coordinating operations by having one
operation signal and another operation wait for a fence or semaphore to go from
the unsignaled to signaled state.

The difference is that the state of fences can be accessed from your program
using calls like `vkWaitForFences` and semaphores cannot be. Fences are used
to make the CPU wait for a command buffer to finish executing or allow the
CPU to check if a command buffer has finished executing. Semaphores are used to
synchronize operations within or across command queues, specifically so that
we can stop the GPU from executing one operation, such as a draw call, until
another operation has finished.

We want to synchronize the queue operations of draw commands and presentation,
which are both GPU only, so semaphores are the best fit. This is because while
the functions are executed asynchronous from the perspective of the CPU,
semaphores allow us to order the operations on the GPU.

let us control the order of execution of operations on the GPUm,
thus efficiently

Semaphores efficiently control the order of operations within a single frame,
but there

However, while semaphores make sure the GPU doesn't present the image until
its finished being drawn, there is no protection from the CPU getting ahead
of the GPU and trying to record another frame before the current frame has
finished. Thus we need to also use a fence which at the top of every frame
waits for the previous frame to have finished, letting us reuse the command
buffer and semaphores.

## Creating the synchronization objects

We'll need one semaphore to signal that an image has been acquired and is ready
for rendering, another one to signal that rendering has finished and
presentation can happen, and a fence to make sure only one frame is rendering
at a time. Create three class members to store these semaphore objects and fence object:

```c++
VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
VkFence inFlightFence;
```

To create the semaphores, we'll add the last `create` function for this part of
the tutorial: `createSyncObjects`:

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
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
}

...

void createSyncObjects() {

}
```

Creating semaphores requires filling in the `VkSemaphoreCreateInfo`, but in the
current version of the API it doesn't actually have any required fields besides
`sType`:

```c++
void createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
}
```

Future versions of the Vulkan API or extensions may add functionality for the
`flags` and `pNext` parameters like it does for the other structures.

Creating a fence requires filling in the `VkFenceCreateInfo`:

```c++
VkFenceCreateInfo fenceInfo{};
fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
```

Creating the semaphores and fence follows the familiar pattern with
`vkCreateSemaphore` & `vkCreateFence`:

```c++
if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
    vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) ||
    vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS){

    throw std::runtime_error("failed to create semaphores!");
}
```

The semaphores and fence should be cleaned up at the end of the program, when
all commands have finished and no more synchronization is necessary:

```c++
void cleanup() {
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);
```

Onto the main drawing function!

## Waiting for the previous frame

At the start of the frame, we want to wait until the previous frame has
finished, so that the command buffer and semaphores are available to use. To do
that, we call `vkWaitForFences`:

```c++
void drawFrame() {
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
}
```

The `vkWaitForFences` function takes an array of fences and waits on the CPU
for either any or all of them to be signaled before returning. The `VK_TRUE` we
pass here indicates that we want to wait for all fences, but in the case of a
single one it obviously doesn't matter. This function also has a timeout
parameter that we set to the maximum value of a 64 bit unsigned integer,
`UINT64_MAX`, thus disabling the timeout.

We need to manually reset the fence to the unsignaled state by resetting it
with the `vkResetFences` call:
```c++
    vkResetFences(device, 1, &inFlightFence);
```

However there is a problem with our current waiting setup. Because fences are
created unsignaled by default, the first call to `vkWaitForFences` happens
before we have had a chance to do anything which might signal the fence,
meaning we would wait forever!

Thankfully the API has solution to this problem. When we create the fence, we
will create it starting in the signaled state. This way, the wait on the first
frame will immediately return because the fence is in a signalled state. To do
this, we add a `VK_FENCE_CREATE_SIGNALED_BIT` flag to the `VkFenceCreateInfo`:

```c++
void createSyncObjects() {
    ...

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    ...
}
```

## Acquiring an image from the swap chain

The next thing we need to do in the `drawFrame` function is acquire an image
from the swap chain. Recall that the swap chain is an extension feature, so we
must use a function with the `vk*KHR` naming convention:

```c++
void drawFrame() {
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
}
```

The first two parameters of `vkAcquireNextImageKHR` are the logical device and
the swap chain from which we wish to acquire an image. The third parameter
specifies a timeout in nanoseconds for an image to become available. Using the
maximum value of a 64 bit unsigned integer means we effectively disable the
timeout.

The next two parameters specify synchronization objects that are to be signaled
when the presentation engine is finished using the image. That's the point in
time where we can start drawing to it. It is possible to specify a semaphore,
fence or both. We're going to use our `imageAvailableSemaphore` for that purpose
here.

The last parameter specifies a variable to output the index of the swap chain
image that has become available. The index refers to the `VkImage` in our
`swapChainImages` array. We're going to use that index to pick the `VkFrameBuffer`.

## Recording the command buffer

With the imageIndex specifying the swapchain image to use in hand, we can now
record the command buffer. First, we call `vkResetCommandBuffer` on the command
buffer to make sure it is able to be recorded.

```c++
vkResetCommandBuffer(commandBuffer, 0);
```

The second parameter of `vkResetCommandBuffer` is a `VkCommandBufferResetFlagBits`
flag. Since we don't want to do anything special, we leave it as 0.

Now call the function `recordCommandBuffer` to record the commands we want.

```c++
recordCommandBuffer(commandBuffer, imageIndex);
```

With a fully recorded command buffer, we can now submit it.

## Submitting the command buffer

Queue submission and synchronization is configured through parameters in the
`VkSubmitInfo` structure.

```c++
VkSubmitInfo submitInfo{};
submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
submitInfo.waitSemaphoreCount = 1;
submitInfo.pWaitSemaphores = waitSemaphores;
submitInfo.pWaitDstStageMask = waitStages;
```

The first three parameters specify which semaphores to wait on before execution
begins and in which stage(s) of the pipeline to wait. We want to wait with
writing colors to the image until it's available, so we're specifying the stage
of the graphics pipeline that writes to the color attachment. That means that
theoretically the implementation can already start executing our vertex shader
and such while the image is not yet available. Each entry in the `waitStages`
array corresponds to the semaphore with the same index in `pWaitSemaphores`.

```c++
submitInfo.commandBufferCount = 1;
submitInfo.pCommandBuffers = commandBuffer;
```

The next two parameters specify which command buffers to actually submit for
execution. We simply submit the single command buffer we have.

```c++
VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
submitInfo.signalSemaphoreCount = 1;
submitInfo.pSignalSemaphores = signalSemaphores;
```

The `signalSemaphoreCount` and `pSignalSemaphores` parameters specify which
semaphores to signal once the command buffer(s) have finished execution. In our
case we're using the `renderFinishedSemaphore` for that purpose.

```c++
if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
}
```

We can now submit the command buffer to the graphics queue using
`vkQueueSubmit`. The function takes an array of `VkSubmitInfo` structures as
argument for efficiency when the workload is much larger. The last parameter
references an optional fence that will be signaled when the command buffers
finish execution. This allows us to know when it is safe for the command
buffer to be reused, thus we want to give it `inFlightFence`. Now on the next
frame, the CPU will wait for this command buffer to finish executing.

## Subpass dependencies

Remember that the subpasses in a render pass automatically take care of image
layout transitions. These transitions are controlled by *subpass dependencies*,
which specify memory and execution dependencies between subpasses. We have only
a single subpass right now, but the operations right before and right after this
subpass also count as implicit "subpasses".

There are two built-in dependencies that take care of the transition at the
start of the render pass and at the end of the render pass, but the former does
not occur at the right time. It assumes that the transition occurs at the start
of the pipeline, but we haven't acquired the image yet at that point! There are
two ways to deal with this problem. We could change the `waitStages` for the
`imageAvailableSemaphore` to `VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT` to ensure that
the render passes don't begin until the image is available, or we can make the
render pass wait for the `VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT` stage.
I've decided to go with the second option here, because it's a good excuse to
have a look at subpass dependencies and how they work.

Subpass dependencies are specified in `VkSubpassDependency` structs. Go to the
`createRenderPass` function and add one:

```c++
VkSubpassDependency dependency{};
dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
dependency.dstSubpass = 0;
```

The first two fields specify the indices of the dependency and the dependent
subpass. The special value `VK_SUBPASS_EXTERNAL` refers to the implicit subpass
before or after the render pass depending on whether it is specified in
`srcSubpass` or `dstSubpass`. The index `0` refers to our subpass, which is the
first and only one. The `dstSubpass` must always be higher than `srcSubpass` to
prevent cycles in the dependency graph (unless one of the subpasses is
`VK_SUBPASS_EXTERNAL`).

```c++
dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependency.srcAccessMask = 0;
```

The next two fields specify the operations to wait on and the stages in which
these operations occur. We need to wait for the swap chain to finish reading
from the image before we can access it. This can be accomplished by waiting on
the color attachment output stage itself.

```c++
dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
```

The operations that should wait on this are in the color attachment stage and
involve the writing of the color attachment. These settings will
prevent the transition from happening until it's actually necessary (and
allowed): when we want to start writing colors to it.

```c++
renderPassInfo.dependencyCount = 1;
renderPassInfo.pDependencies = &dependency;
```

The `VkRenderPassCreateInfo` struct has two fields to specify an array of
dependencies.

## Presentation

The last step of drawing a frame is submitting the result back to the swap chain
to have it eventually show up on the screen. Presentation is configured through
a `VkPresentInfoKHR` structure at the end of the `drawFrame` function.

```c++
VkPresentInfoKHR presentInfo{};
presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

presentInfo.waitSemaphoreCount = 1;
presentInfo.pWaitSemaphores = signalSemaphores;
```

The first two parameters specify which semaphores to wait on before presentation
can happen, just like `VkSubmitInfo`.

```c++
VkSwapchainKHR swapChains[] = {swapChain};
presentInfo.swapchainCount = 1;
presentInfo.pSwapchains = swapChains;
presentInfo.pImageIndices = &imageIndex;
```

The next two parameters specify the swap chains to present images to and the
index of the image for each swap chain. This will almost always be a single one.

```c++
presentInfo.pResults = nullptr; // Optional
```

There is one last optional parameter called `pResults`. It allows you to specify
an array of `VkResult` values to check for every individual swap chain if
presentation was successful. It's not necessary if you're only using a single
swap chain, because you can simply use the return value of the present function.

```c++
vkQueuePresentKHR(presentQueue, &presentInfo);
```

The `vkQueuePresentKHR` function submits the request to present an image to the
swap chain. We'll add error handling for both `vkAcquireNextImageKHR` and
`vkQueuePresentKHR` in the next chapter, because their failure does not
necessarily mean that the program should terminate, unlike the functions we've
seen so far.

If you did everything correctly up to this point, then you should now see
something resembling the following when you run your program:

![](/images/triangle.png)

>This colored triangle may look a bit different from the one you're used to seeing in graphics tutorials. That's because this tutorial lets the shader interpolate in linear color space and converts to sRGB color space afterwards. See [this blog post](https://medium.com/@heypete/hello-triangle-meet-swift-and-wide-color-6f9e246616d9) for a discussion of the difference.

Yay! Unfortunately, you'll see that when validation layers are enabled, the
program crashes as soon as you close it. The messages printed to the terminal
from `debugCallback` tell us why:

![](/images/semaphore_in_use.png)

Remember that all of the operations in `drawFrame` are asynchronous. That means
that when we exit the loop in `mainLoop`, drawing and presentation operations
may still be going on. Cleaning up resources while that is happening is a bad
idea.

To fix that problem, we should wait for the logical device to finish operations
before exiting `mainLoop` and destroying the window:

```c++
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}
```

You can also wait for operations in a specific command queue to be finished with
`vkQueueWaitIdle`. These functions can be used as a very rudimentary way to
perform synchronization. You'll see that the program now exits without problems
when closing the window.

## Conclusion

A little over 900 lines of code later, we've finally gotten to the stage of seeing
something pop up on the screen! Bootstrapping a Vulkan program is definitely a
lot of work, but the take-away message is that Vulkan gives you an immense
amount of control through its explicitness. I recommend you to take some time
now to reread the code and build a mental model of the purpose of all of the
Vulkan objects in the program and how they relate to each other. We'll be
building on top of that knowledge to extend the functionality of the program
from this point on.

In the next chapter we'll deal with one more small thing that is required for a
well-behaved Vulkan program.

[C++ code](/code/15_hello_triangle.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
