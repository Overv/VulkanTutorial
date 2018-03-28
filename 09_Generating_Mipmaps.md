## Introduction
Our program can now load and render 3D models. In this chapter, we will add one more feature, mipmap generation. Mipmaps are widely used in games and rendering software, and Vulkan gives us complete control over how they are created. 

Mipmaps are precalculated, downscaled versions of an image. Each new image is half the width and height of the previous one.  Mip maps are used as a form of *Level of Detail.* Objects that are far away from the camera will sample their textures from the smaller mip images. Using smaller images increases the rendering speed and avoids artifacts such as [Moiré patterns](https://en.wikipedia.org/wiki/Moir%C3%A9_pattern). An example of what mip maps look like:

![](/images/mipmaps_example.jpg)

## Image creation

In Vulkan, each of the mip images is stored in different *mip levels* of a `VkImage`. Mip level 0 is the original image, and each mip level after that is half the size of previous level. The number of mip levels is specified when the `VkImage` is created. Up until now, we have always set this value to one. In `createTextureImage`, we need to calculate the number of mip levels from the dimensions of the image. First, add a class member to store this number:

```c++
    ...
    uint32_t mipLevels;
    VkImage textureImage;
    ...
```
The value for `mipLevels` can be found with a simple loop in `createTextureImage`:

```c++
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    ...
    mipLevels = 1;
    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;
    while (mipWidth > 1 && mipHeight > 1) {
        mipLevels++;
        mipWidth /= 2;
        mipHeight /= 2;
    }
```

The loop counts the number of mip levels by halving the dimensions of the current mip level, until either dimension is 1. To use this value, we need to change the `createImage` and `createImageView` functions to allow us to specify the number of mip levels. Add a `mipLevels` parameter to the functions:

```c++
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        ...
        imageInfo.mipLevels = mipLevels;
        ...
    }
```
```c++
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
        ...
        viewInfo.subresourceRange.levelCount = mipLevels;
        ...
```

Update all calls to these functions to use the right values:

```c++
    createImage(swapChainExtent.width, swapChainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    ...
    createImage(texWidth, texHeight, mipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
```
```c++
    swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    ...
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    ...
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
```



## Generating Mipmaps

Our texture image now has multiple mip levels, but the staging buffer can only be used to fill mip level 0. The other levels are still undefined. To fill these levels we need to generate the data from the single level that we have. We will use `vkCmdBlitImage` command. This command performs copying, scaling, filtering operations. We will use this to *blit* data from each level of our texture image.

Like other image operations, `vkCmdBlitImage` depends on the layout of the image it operates on. For optimal performance, the source image should be in `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL` and the destination image should be in `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` while blitting data. Vulkan allows us to transition each mip level of an image independently. `transitionImageLayout` only performs layout transitions on the entire image, so first we need to remove the existing transition to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` in `createTextureImage`:

```c++
    ...
    createImage(texWidth, texHeight, mipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
        
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
        copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
    ...
```
This will leave each level of the texture image in `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`.

We're now going to write the function generates the mip maps:

```c++
    void generateMipmaps(int32_t texWidth, int32_t texHeight) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        ...
        endSingleTimeCommands(commandBuffer);
    }
```

Since we can't use `transitionImageLayout`, we need to record and submit another `VkCommandBuffer`.


```c++
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = textureImage;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
```

We're going to insert several pipeline barriers, so we'll reuse this `VkImageMemoryBarrier` for each transition.

```c++
    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
    
    }
```

This loop will record each of the `VkCmdBlitImage` commands. Note that the loop variable starts at 1, not 0.

```c++
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
```

First, we transition level `i - 1` to `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`.

```c++
    VkImageBlit blit = {};
    blit.srcOffsets[0] = { 0, 0, 0 };
    blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = { 0, 0, 0 };
    blit.dstOffsets[1] = { mipWidth / 2, mipHeight / 2, 1 };
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;
```

Next, we specify the regions that will be used in the blit operation. The source mip level is `i - 1` and the destination mip level is `i`. The two elements of the `srcOffsets` array determine the 3D region that data will be blitted from. `dstOffsets` determines the region that data will be blitted to. The X and Y dimensions of the `dstOffsets[1]` are divided by two since each mip level is half the size of the previous level. The Z dimension of `srcOffsets[1]` and `dstOffsets[1]` must be 1, since a 2D image has a depth of 1.

```c++
    vkCmdBlitImage(commandBuffer,
        textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &blit, VK_FILTER_LINEAR);
```

Now, we record the blit command. Note that `textureImage` is used for both the `srcImage` and `dstImage` parameter. The source mip level was just transitioned to `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL` and the destination level is still in `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` from `createTextureImage`. The last parameter says to use a linear filter when scaling the data.

```c++
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
```

This barrier transitions mip level `i - 1` to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`.

```c++
        ...
        mipWidth /= 2;
        mipHeight /= 2;
    }
```

At the end of the loop, we divide the current mip dimensions by two.

```c++

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        endSingleTimeCommands(commandBuffer);
    }
```

Before we end the command buffer, we insert one more pipeline barrier. This barrier transitions the last mip level from `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`, since that wasn't handled by the loop.

Finally, add the call to `generateMipmaps` in `createTextureImage`:

```c++
    generateMipmaps(texWidth, texHeight);
```

Our texture image's mip maps are now completely filled.

## Sampler

One last thing we need to do before we see the results is modify `textureSampler`.

```c++
    void createTextureSampler() {
        ...
        samplerInfo.maxLod = static_cast<float>(mipLevels);
        ...
    }
```

This configures the sampler to sample from all mip levels up to `mipLevels`.

Now run your program and you should see the following:

![](/images/mipmaps.png)

It's not a dramatic difference, since our scene is so simple. There are subtle differences if you look closely.


![](/images/mipmaps_comparison.png)

The left image is our new program. The right image is our program without mipmaps. The most noticeable difference is the writing on the signs. With mipmaps, the writing has been blurred. Without mipmaps, the writing has harsh edges and gaps from Moiré artifacts.

You can play around with the sampler settings to see how they affect mipmapping. For example, by changing `minLod`, you can force the sampler to not use the lowest mip levels:

```c++
    samplerInfo.minLod = static_cast<float>(mipLevels / 2);
```

These settings will produce this image:


![](/images/highmipmaps.png)

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

[C++ code](/code/28_mipmapping.cpp) /
[Vertex shader](/code/26_shader_depth.vert) /
[Fragment shader](/code/26_shader_depth.frag)
