## Introduction

The application we have now successfully draws a triangle, but there are some
circumstances that it isn't handling properly yet. It is possible for the window
surface to change such that the swap chain is no longer compatible with it. One
of the reasons that could cause this to happen is the size of the window
changing. We have to catch these events and recreate the swap chain.

## Recreating the swap chain

Create a new `recreateSwapChain` function that calls `createSwapChain` and all
of the creation functions for the objects that depend on the swap chain or the
window size.

```c++
void recreateSwapChain() {
    vkDeviceWaitIdle(device);

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers();
}
```

We first call `vkDeviceWaitIdle`, because just like in the last chapter, we
shouldn't touch resources that may still be in use. Obviously, the first thing
we'll have to do is recreate the swap chain itself. The image views need to be
recreated because they are based directly on the swap chain images. The render
pass needs to be recreated because it depends on the format of the swap chain
images. Viewport and scissor rectangle size is specified during graphics
pipeline creation, so the pipeline also needs to be rebuilt. It is possible to
avoid this by using dynamic state for the viewports and scissor rectangles.
Finally, the framebuffers and command buffers also directly depend on the swap
chain images.

Because of our handy `VDeleter` construct, most of the functions will work fine
for recreation and will automatically clean up the old objects. However, the
`createSwapChain` and `createCommandBuffers` functions still need some
adjustments.

```c++
VkSwapchainKHR oldSwapChain = swapChain;
createInfo.oldSwapchain = oldSwapChain;

VkSwapchainKHR newSwapChain;
if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &newSwapChain) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
}

swapChain = newSwapChain;
```

We need to pass the previous swap chain object in the `oldSwapchain` parameter
of `VkSwapchainCreateInfoKHR` to indicate that we intend to replace it. The old
swap chain needs to stick around until after the new swap chain has been
created, which means that we can't directly write the new handle to `swapChain`.
The `VDeleter` would clear the old object before `vkCreateSwapchainKHR` has a
chance to execute. That's why we use the temporary `newSwapChain` variable.

```c++
swapChain = newSwapChain;
```

This line will actually destroy the old swap chain and replace the handle with
the handle of the new swap chain.

The problem with `createCommandBuffers` is that it doesn't free the old command
buffers. There are two ways to solve this:

* Call `createCommandPool` as well, which will automatically free the old
command buffers
* Extend `createCommandBuffers` to free any previous command buffers

As there isn't really a need to recreate the command pool itself, I've chosen to
go for the second solution in this tutorial.

```c++
if (commandBuffers.size() > 0) {
    vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());
}

commandBuffers.resize(swapChainFramebuffers.size());
```

The `createCommandBuffers` function now first checks if the `commandBuffers`
vector already contains previous command buffers, and if so, frees them. That's
all it takes to recreate the swap chain!

## Window resizing

Now we just need to figure out when swap chain recreation is necessary and call
our new `recreateSwapChain` function. One of the most common conditions is
resizing of the window. Let's make the window resizable and catch that event.
Change the `initWindow` function to no longer include the `GLFW_RESIZABLE` line
or change its argument from `GLFW_FALSE` to `GLFW_TRUE`.

```c++
void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetWindowSizeCallback(window, HelloTriangleApplication::onWindowResized);
}

...

static void onWindowResized(GLFWwindow* window, int width, int height) {
    if (width == 0 || height == 0) return;

    HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->recreateSwapChain();
}
```

The `glfwSetWindowSizeCallback` function can be used to specify a callback for
the window resize event. Unfortunately it only accepts a function pointer as
argument, so we can't directly use a member function. Luckily GLFW allows us to
store an arbitrary pointer in the window object with `glfwSetWindowUserPointer`,
so we can specify a static class member and get the original class instance back
with `glfwGetWindowUserPointer`. We can then proceed to call
`recreateSwapChain`, but only if the size of the window is non-zero. This case
occurs when the window is minimized and it will cause swap chain creation to
fail.

The `chooseSwapExtent` function should also be updated to take the current width
and height of the window into account instead of the initial `WIDTH` and
`HEIGHT`:

```c++
int width, height;
glfwGetWindowSize(window, &width, &height);

VkExtent2D actualExtent = {width, height};
```

## Suboptimal or out-of-date swap chain

It is also possible for Vulkan to tell us that the swap chain is no longer
compatible during presentation. The `vkAcquireNextImageKHR` and
`vkQueuePresentKHR` functions can return the following special values to
indicate this.

* `VK_ERROR_OUT_OF_DATE_KHR`: The swap chain has become incompatible with the
surface and can no longer be used for rendering.
* `VK_SUBOPTIMAL_KHR`: The swap chain can still be used to successfully present
to the surface, but the surface properties are no longer matched exactly. For
example, the platform may be simply resizing the image to fit the window now.

```c++
VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
}
```

If the swap chain turns out to be out of date when attempting to acquire an
image, then it is no longer possible to present to it. Therefore we should
immediately recreate the swap chain and try again in the next `drawFrame` call.

You could also decide to do that if the swap chain is suboptimal, but I've
chosen to proceed anyway in that case because we've already acquired an image.
Both `VK_SUCCESS` and `VK_SUBOPTIMAL_KHR` are considered "success" return codes.

```c++
result = vkQueuePresentKHR(presentQueue, &presentInfo);

if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    recreateSwapChain();
} else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
}
```

The `vkQueuePresentKHR` function returns the same values with the same meaning.
In this case we will also recreate the swap chain if it is suboptimal, because
we want the best possible result. Try to run it and resize the window to see if
the framebuffer is indeed resized properly with the window.

Congratulations, you've now finished your very first well-behaved Vulkan
program! In the next chapter we're going to get rid of the hardcoded vertices in
the vertex shader and actually use a vertex buffer.

[C++ code](/code/swap_chain_recreation.cpp) /
[Vertex shader](/code/shader_base.vert) /
[Fragment shader](/code/shader_base.frag)