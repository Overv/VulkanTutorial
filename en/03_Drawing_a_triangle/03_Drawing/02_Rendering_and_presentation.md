## Setup

This is the chapter where everything is going to come together. We're going to
write the `drawFrame` function that will be called from the main loop to put the
triangle on the screen. Create the function and call it from `mainLoop`:

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

## Synchronization

The `drawFrame` function will perform the following operations:

* Acquire an image from the swap chain
* Execute the command buffer with that image as attachment in the framebuffer
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
using calls like `vkWaitForFences` and semaphores cannot be. Fences are mainly
designed to synchronize your application itself with rendering operation,
whereas semaphores are used to synchronize operations within or across command
queues. We want to synchronize the queue operations of draw commands and
presentation, which makes semaphores the best fit.

## Semaphores

We'll need one semaphore to signal that an image has been acquired and is ready
for rendering, and another one to signal that rendering has finished and
presentation can happen. Create two class members to store these semaphore
objects:

```c++
VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
```

To create the semaphores, we'll add the last `create` function for this part of
the tutorial: `createSemaphores`:

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
    createCommandBuffers();
    createSemaphores();
}

...

void createSemaphores() {

}
```

Creating semaphores requires filling in the `VkSemaphoreCreateInfo`, but in the
current version of the API it doesn't actually have any required fields besides
`sType`:

```c++
void createSemaphores() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
}
```

Future versions of the Vulkan API or extensions may add functionality for the
`flags` and `pNext` parameters like it does for the other structures. Creating
the semaphores follows the familiar pattern with `vkCreateSemaphore`:

```c++
if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
    vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

    throw std::runtime_error("failed to create semaphores!");
}
```

The semaphores should be cleaned up at the end of the program, when all commands
have finished and no more synchronization is necessary:

```c++
void cleanup() {
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
```

## Acquiring an image from the swap chain

As mentioned before, the first thing we need to do in the `drawFrame` function
is acquire an image from the swap chain. Recall that the swap chain is an
extension feature, so we must use a function with the `vk*KHR` naming
convention:

```c++
void drawFrame() {
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
}
```

The first two parameters of `vkAcquireNextImageKHR` are the logical device and
the swap chain from which we wish to acquire an image. The third parameter
specifies a timeout in nanoseconds for an image to become available. Using the
maximum value of a 64 bit unsigned integer disables the timeout.

The next two parameters specify synchronization objects that are to be signaled
when the presentation engine is finished using the image. That's the point in
time where we can start drawing to it. It is possible to specify a semaphore,
fence or both. We're going to use our `imageAvailableSemaphore` for that purpose
here.

The last parameter specifies a variable to output the index of the swap chain
image that has become available. The index refers to the `VkImage` in our
`swapChainImages` array. We're going to use that index to pick the right command
buffer.

## Submitting the command buffer

Queue submission and synchronization is configured through parameters in the
`VkSubmitInfo` structure.

```c++
VkSubmitInfo submitInfo = {};
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
submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
```

The next two parameters specify which command buffers to actually submit for
execution. As mentioned earlier, we should submit the command buffer that binds
the swap chain image we just acquired as color attachment.

```c++
VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
submitInfo.signalSemaphoreCount = 1;
submitInfo.pSignalSemaphores = signalSemaphores;
```

The `signalSemaphoreCount` and `pSignalSemaphores` parameters specify which
semaphores to signal once the command buffer(s) have finished execution. In our
case we're using the `renderFinishedSemaphore` for that purpose.

```c++
if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
}
```

We can now submit the command buffer to the graphics queue using
`vkQueueSubmit`. The function takes an array of `VkSubmitInfo` structures as
argument for efficiency when the workload is much larger. The last parameter
references an optional fence that will be signaled when the command buffers
finish execution. We're using semaphores for synchronization, so we'll just pass
a `VK_NULL_HANDLE`.

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
VkSubpassDependency dependency = {};
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
dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
```

The operations that should wait on this are in the color attachment stage and
involve the reading and writing of the color attachment. These settings will
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
VkPresentInfoKHR presentInfo = {};
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

## Frames in flight

If you run your application with validation layers enabled now you may either get errors or notice that the memory usage slowly grows. The reason for this is that the application is rapidly submitting work in the `drawFrame` function, but doesn't actually check if any of it finishes. If the CPU is submitting work faster than the GPU can keep up with then the queue will slowly fill up with work. Worse, even, is that we are reusing the `imageAvailableSemaphore` and `renderFinishedSemaphore` semaphores, along with the command buffers, for multiple frames at the same time!

The easy way to solve this is to wait for work to finish right after submitting it, for example by using `vkQueueWaitIdle`:

```c++
void drawFrame() {
    ...

    vkQueuePresentKHR(presentQueue, &presentInfo);

    vkQueueWaitIdle(presentQueue);
}
```

However, we are likely not optimally using the GPU in this way, because the whole graphics pipeline is only used for one frame at a time right now. The stages that the current frame has already progressed through are idle and could already be used for a next frame. We will now extend our application to allow for multiple frames to be *in-flight* while still bounding the amount of work that piles up.

Start by adding a constant at the top of the program that defines how many frames should be processed concurrently:

```c++
const int MAX_FRAMES_IN_FLIGHT = 2;
```

Each frame should have its own set of semaphores:

```c++
std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
```

The `createSemaphores` function should be changed to create all of these:

```c++
void createSemaphores() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create semaphores for a frame!");
        }
}
```

Similarly, they should also all be cleaned up:

```c++
void cleanup() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    }

    ...
}
```

To use the right pair of semaphores every time, we need to keep track of the current frame. We will use a frame index for that purpose:

```c++
size_t currentFrame = 0;
```

The `drawFrame` function can now be modified to use the right objects:

```c++
void drawFrame() {
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    ...

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};

    ...

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

    ...
}
```

Of course, we shouldn't forget to advance to the next frame every time:

```c++
void drawFrame() {
    ...

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
```

By using the modulo (%) operator, we ensure that the frame index loops around after every `MAX_FRAMES_IN_FLIGHT` enqueued frames.

Although we've now set up the required objects to facilitate processing of multiple frames simultaneously, we still don't actually prevent more than `MAX_FRAMES_IN_FLIGHT` from being submitted. Right now there is only GPU-GPU synchronization and no CPU-GPU synchronization going on to keep track of how the work is going. We may be using the frame #0 objects while frame #0 is still in-flight!

To perform CPU-GPU synchronization, Vulkan offers a second type of synchronization primitive called *fences*. Fences are similar to semaphores in the sense that they can be signaled and waited for, but this time we actually wait for them in our own code. We'll first create a fence for each frame:

```c++
std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
std::vector<VkFence> inFlightFences;
size_t currentFrame = 0;
```

I've decided to create the fences together with the semaphores and renamed `createSemaphores` to `createSyncObjects`:

```c++
void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}
```

The creation of fences (`VkFence`) is very similar to the creation of semaphores. Also make sure to clean up the fences:

```c++
void cleanup() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    ...
}
```

We will now change `drawFrame` to use the fences for synchronization. The `vkQueueSubmit` call includes an optional parameter to pass a fence that should be signaled when the command buffer finishes executing. We can use this to signal that a frame has finished.

```c++
void drawFrame() {
    ...

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
    ...
}
```

Now the only thing remaining is to change the beginning of `drawFrame` to wait for the frame to be finished:

```c++
void drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    ...
}
```

The `vkWaitForFences` function takes an array of fences and waits for either any or all of them to be signaled before returning. The `VK_TRUE` we pass here indicates that we want to wait for all fences, but in the case of a single one it obviously doesn't matter. Just like `vkAcquireNextImageKHR` this function also takes a timeout. Unlike the semaphores, we manually need to restore the fence to the unsignaled state by resetting it with the `vkResetFences` call.

If you run the program now, you'll notice something something strange. The application no longer seems to be rendering anything. With validation layers enabled, you'll see the following message:

![](/images/unsubmitted_fence.png)

That means that we're waiting for a fence that has not been submitted. The problem here is that, by default, fences are created in the unsignaled state. That means that `vkWaitForFences` will wait forever if we haven't used the fence before. To solve that, we can change the fence creation to initialize it in the signaled state as if we had rendered an initial frame that finished:

```c++
void createSyncObjects() {
    ...

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    ...
}
```

The memory leak is gone now, but the program is not quite working correctly yet. If `MAX_FRAMES_IN_FLIGHT` is higher than the number of swap chain images or `vkAcquireNextImageKHR` returns images out-of-order then it's possible that we may start rendering to a swap chain image that is already *in flight*. To avoid this, we need to track for each swap chain image if a frame in flight is currently using it. This mapping will refer to frames in flight by their fences so we'll immediately have a synchronization object to wait on before a new frame can use that image.

First add a new list `imagesInFlight` to track this:

```c++
std::vector<VkFence> inFlightFences;
std::vector<VkFence> imagesInFlight;
size_t currentFrame = 0;
```

Prepare it in `createSyncObjects`:

```c++
void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

    ...
}
```

Initially not a single frame is using an image so we explicitly initialize it to *no fence*. Now we'll modify `drawFrame` to wait on any previous frame that is using the image that we've just been assigned for the new frame:

```c++
void drawFrame() {
    ...

    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    ...
}
```

Because we now have more calls to `vkWaitForFences`, the `vkResetFences` call should be **moved**. It's best to simply call it right before actually using the fence:

```c++
void drawFrame() {
    ...

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    ...
}
```

We've now implemented all the needed synchronization to ensure that there are no more than two frames of work enqueued and that these frames are not accidentally using the same image. Note that it is fine for other parts of the code, like the final cleanup, to rely on more rough synchronization like `vkDeviceWaitIdle`. You should decide on which approach to use based on performance requirements.

To learn more about synchronization through examples, have a look at [this extensive overview](https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#swapchain-image-acquire-and-present) by Khronos.

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
