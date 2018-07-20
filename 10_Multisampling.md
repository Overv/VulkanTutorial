## Introduction
Our program can now load and render 3D models. In this chapter, we will add one more feature, mipmap generation. Mipmaps are widely used in games and rendering software, and Vulkan gives us complete control over how they are created. 

Mipmaps are precalculated, downscaled versions of an image. Each new image is half the width and height of the previous one.  Mipmaps are used as a form of *Level of Detail* or *LOD.* Objects that are far away from the camera will sample their textures from the smaller mip images. Using smaller images increases the rendering speed and avoids artifacts such as [Moiré patterns](https://en.wikipedia.org/wiki/Moir%C3%A9_pattern). An example of what mipmaps look like:

![](/images/mipmaps_example.jpg)

## Checking for multisampling support

First, add a class member that will store the number of samples used by the renderer. This number will be used in various places in our code:

```c++
...
VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
...
```

Having done that, we now need to determine what is the maximum number of samples supported by the hardware. This information can be extracted from `VkPhysicalDeviceProperties` associated with our selected physical device. We're using a depth buffer, so we have to take into account the sample count for both color and depth - the lower number will be the maximum we can support:

```c++
VkSampleCountFlagBits getMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}
```

We will now use this function to set the `msaaSamples` variable during physical device selection process. For this, we have to slightly modify the `pickPhysicalDevice` function:

```c++
void pickPhysicalDevice() {
            ...
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                msaaSamples = getMaxUsableSampleCount();
                break;
            }
            ...
}
```

Next, update `createImage` functions to allow us to specify the number of samples by adding a `numSmaples` parameter - this will become important later:

```c++
void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    ...
    imageInfo.samples = numSamples;
    ...
```

For now, update all calls to these functions using `VK_SAMPLE_COUNT_1_BIT` - we will be replacing this with proper values as we progress with implementation:

```c++
createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
...
createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
```

## Setting up render targets

Multisampling requires additional render targets. Add following class members:

```c++
...
VkImage colorImage;
VkDeviceMemory colorImageMemory;
VkImageView colorImageView;

VkImage depthMsaaImage;
VkDeviceMemory depthMsaaImageMemory;
VkImageView depthMsaaImageView;
...
```

Add a `createColorResources` function (note that we're using `msaaSamples` here as a function parameter to `createImage`):

```c++
void createColorResources() {
    VkFormat colorFormat = swapChainImageFormat;

    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
    colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    transitionImageLayout(colorImage, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
}
```

Call the function right before `createDepthResources`:

```c++
void initVulkan() {
    ...
    createColorResources();
    createDepthResources();
    ...
}
```

Notice new transition setup, so we need to modify `transitionImageLayout`:

```c++
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
    ...
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }
    ...
}
```

Modify `createDepthResources`:

```c++
void createDepthResources() {
    ...
    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthMsaaImage, depthMsaaImageMemory);
    depthMsaaImageView = createImageView(depthMsaaImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    transitionImageLayout(depthMsaaImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}
```

Don't forget to release resources:
```c++
void cleanupSwapChain() {
    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);
    vkDestroyImageView(device, depthMsaaImageView, nullptr);
    vkDestroyImage(device, depthMsaaImage, nullptr);
    vkFreeMemory(device, depthMsaaImageMemory, nullptr);
    ...
```

## Using multisampling

Modify `createRenderPass`:

```c++
void createRenderPass() {
    ...
    colorAttachment.samples = msaaSamples;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ...
    depthAttachment.samples = msaaSamples;
    ...
```

Add new attachment resolves:

```c++
    ...
    VkAttachmentDescription colorAttachmentResolve = {};
    colorAttachmentResolve.format = swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachmentResolve = {};
    depthAttachmentResolve.format = findDepthFormat();
    depthAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    ...
```

Add atachment reference for color:

```c++
    ...
    VkAttachmentReference colorAttachmentResolveRef = {};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ...
    subpass.pResolveAttachments = &colorAttachmentResolveRef;
    ...
```

Update render pass info struct with new attachments:

```c++
    ...
    std::array<VkAttachmentDescription, 4> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve, depthAttachmentResolve };
    ...
```

Modify `createGraphicsPipeline`:

```c++
void createGraphicsPipeline() {
    ...
    multisampling.rasterizationSamples = msaaSamples;
    ...
}
```

Modify `createFrameBuffers`:
```c++
void createFrameBuffers() {
        ...
        std::array<VkImageView, 4> attachments = {
            colorImageView,
            depthMsaaImageView,
            swapChainImageViews[i],
            depthImageView
        };
        ...
}
```

## Sampler

Now run your program and you should see the following:

![](/images/multisampling.png)

It's not a dramatic difference, since our scene is so simple. There are subtle differences if you look closely.

![](/images/multisampling_comparison.png)

The most noticeable difference is the writing on the signs. With mipmaps, the writing has been smoothed. Without mipmaps, the writing has harsh edges and gaps from Moiré artifacts.

![](/images/multisampling_comparison2.png)

This is how higher mip levels will be used when objects are further away from the camera.


## Conclusion

It has taken a lot of work to get to this point, but now you finally have a good
base for a Vulkan program. The knowledge of the basic principles of Vulkan that
you now possess should be sufficient to start exploring more of the features,
like:

* Push constants
* Instanced rendering
* Dynamic uniforms
* Separate images and sampler descriptors
* Pipeline cache
* Multi-threaded command buffer generation
* Multiple subpasses
* Compute shaders

The current program can be extended in many ways, like adding Blinn-Phong
lighting, post-processing effects and shadow mapping. You should be able to
learn how these effects work from tutorials for other APIs, because despite
Vulkan's explicitness, many concepts still work the same.

[C++ code](/code/29_multisampling.cpp) /
[Vertex shader](/code/26_shader_depth.vert) /
[Fragment shader](/code/26_shader_depth.frag)
