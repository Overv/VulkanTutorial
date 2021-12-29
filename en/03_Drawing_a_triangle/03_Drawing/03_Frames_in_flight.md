## Frames in flight

Right now our render loop has one glaring flaw. We are required to wait on the
previous frame to finish before we can start rendering the next which results
in unnecessary idling of the host. 

<!-- insert diagram showing our current render loop and the 'multi frame in flight' render loop -->

The way to fix this is to allow multiple frames to be *in-flight* at once, that 
is to say, allow the rendering of one frame to not interfere with the recording
of the next. How do we do this? Any resource that is accessed and modified
during rendering must be duplicated. Thus, we need multiple command buffers,
semaphores, and fences. In later chapters we will also add multiple instances
of other resources, so we will see this concept reappear.

Start by adding a constant at the top of the program that defines how many
frames should be processed concurrently:

```c++
const int MAX_FRAMES_IN_FLIGHT = 2;
```

We choose the number 2 because we don't want the CPU to get *too* far ahead of
the GPU. With 2 frames in flight, the CPU and the GPU can be working on their
own tasks at the same time. If the CPU finishes early, it will wait till the
GPU finishes rendering before submitting more work. With 3 or more frames in
flight, the CPU could get ahead of the GPU, adding frames of latency.
Generally, extra latency isn't desired. But giving the application control over
the number of frames in flight is another example of Vulkan being explicit.

Each frame should have its own command buffer, set of semaphores, and fence.
Rename and then change them to be `std::vector`s of the objects:

```c++
std::vector<VkCommandBuffer> commandBuffers;

...

std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
std::vector<VkFence> inFlightFences;
```

Then we need to create multiple command buffers. Rename `createCommandBuffer`
to `createCommandBuffers`. Next we need to resize the command buffers vector
to the size of `MAX_FRAMES_IN_FLIGHT`, alter the `VkCommandBufferAllocateInfo`
to contain that many command buffers, and then change the destination to our
vector of command buffers:

```c++
void createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    ...
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}
```

The `createSyncObjects` function should be changed to create all of the objects:

```c++
void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}
```

Similarly, they should also all be cleaned up:

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

Remember, because command buffers are freed for us when we free the command
pool, there is nothing extra to do for command buffer cleanup.

To use the right objects every frame, we need to keep track of the current
frame. We will use a frame index for that purpose:

```c++
uint32_t currentFrame = 0;
```

The `drawFrame` function can now be modified to use the right objects:

```c++
void drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    ...

    vkResetCommandBuffer(commandBuffers[currentFrame],  0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    ...

    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    ...

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};

    ...

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

    ...

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
}
```

Of course, we shouldn't forget to advance to the next frame every time:

```c++
void drawFrame() {
    ...

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
```

By using the modulo (%) operator, we ensure that the frame index loops around
after every `MAX_FRAMES_IN_FLIGHT` enqueued frames.

<!-- Possibly use swapchain-image-count for renderFinished semaphores, as it can't
be known with a fence whether the semaphore is ready for re-use. -->

We've now implemented all the needed synchronization to ensure that there are
no more than `MAX_FRAMES_IN_FLIGHT` frames of work enqueued and that these
frames are not stepping over eachother. Note that it is fine for other parts of
the code, like the final cleanup, to rely on more rough synchronization like
`vkDeviceWaitIdle`. You should decide on which approach to use based on
performance requirements.

To learn more about synchronization through examples, have a look at [this extensive overview](https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#swapchain-image-acquire-and-present) by Khronos.


In the next chapter we'll deal with one more small thing that is required for a
well-behaved Vulkan program.


[C++ code](/code/16_frames_in_flight.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
