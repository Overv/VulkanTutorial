## Introduction

The geometry has been colored using per-vertex colors so far, which is a rather
limited approach. In this part of the tutorial we're going to implement texture
mapping to make the geometry look more interesting. This will also allow us to
load and draw basic 3D models in a future chapter.

Adding a texture to our application will involve the following steps:

* Create an image object backed by device memory
* Fill it with pixels from an image file
* Create an image sampler
* Add a combined image sampler descriptor to sample colors from the texture

We've already worked with image objects before, but those were automatically
created by the swap chain extension. This time we'll have to create one by
ourselves. Creating an image and filling it with data is very similar to vertex
buffer creation. You create a `VkImage`, query its memory requirements, allocate
device memory, bind the memory to the image, and finally map the memory to
upload the pixel data. We'll use a staging and final image again, to make sure
that the texture image itself ends up in fast device local memory. There is a
command to copy the contents of images similar to `vkCmdCopyBuffer`.

However, there is something extra that we'll have to take care of when working
with images. Images can have different *layouts* that affect how the pixels are
organized in memory. Due to the way graphics hardware works, simply storing the
pixels row by row may not lead to the best performance, for example. When
performing any operation on images, you must make sure that they have the layout
that is optimal for use in that operation. We've actually already seen some of
these layouts when we specified the render pass:

* `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`: Optimal for presentation
* `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`: Optimal as attachment for writing
colors from the fragment shader
* `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`: Optimal as source in a transfer
operation, like `vkCmdCopyImage`
* `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`: Optimal as destination in a transfer
operation, like `vkCmdCopyImage`
* `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`: Optimal for sampling from a shader

One of the most common ways to transition the layout of an image is a *pipeline
barrier*. Pipeline barriers are primarily used for synchronizing access to
resources, like making sure that an image was written to before it is read, but
they can also be used to transition layouts. In this chapter we'll see how
pipeline barriers are used for this purpose. Barriers can additionally be used
to transfer queue family ownership when using `VK_SHARING_MODE_EXCLUSIVE`.

## Image library

There are many libraries available for loading images, and you can even write
your own code to load simple formats like BMP and PPM. In this tutorial we'll be
using the stb_image library from the [stb collection](https://github.com/nothings/stb).
The advantage of it is that all of the code is in a single file, so it doesn't
require any tricky build configuration. Download `stb_image.h` and store it in a
convenient location, like the directory where you saved GLFW and GLM. Add the
location to your include path.

**Visual Studio**

Add the directory with `stb_image.h` in it to the `Additional Include
Directories` paths.

![](/images/include_dirs_stb.png)

**Makefile**

Add the directory with `stb_image.h` to the include directories for GCC:

```text
VULKAN_SDK_PATH = /home/user/VulkanSDK/x.x.x.x/x86_64
STB_INCLUDE_PATH = /home/user/libraries/stb

...

CFLAGS = -std=c++11 -I$(VULKAN_SDK_PATH)/include -I$(STB_INCLUDE_PATH)
```

## Loading an image

Include the image library like this:

```c++
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
```

The header only defines the prototypes of the functions by default. One code
file needs to include the header with the `STB_IMAGE_IMPLEMENTATION` definition
to include the function bodies, otherwise we'll get linking errors.

```c++
void initVulkan() {
    ...
    createCommandPool();
    createTextureImage();
    createVertexBuffer();
    ...
}

...

void createTextureImage() {

}
```

Create a new function `createTextureImage` where we'll load an image and upload
it into a Vulkan image object. We're going to use command buffers, so it should
be called after `createCommandPool`.

Create a new directory `textures` next to the `shaders` directory to store
texture images in. We're going to load an image called `texture.jpg` from that
directory. I've chosen to use the following
[CC0 licensed image](https://pixabay.com/en/statue-sculpture-fig-historically-1275469/)
resized to 512 x 512 pixels, but feel free to pick any image you want. The
library supports most common image file formats, like JPEG, PNG, BMP and GIF.

![](/images/texture.jpg)

Loading an image with this library is really easy:

```c++
void createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
}
```

The `stbi_load` function takes the file path and number of channels to load as
arguments. The `STBI_rgb_alpha` value forces the image to be loaded with an
alpha channel, even if it doesn't have one, which is nice for consistency with
other textures in the future. The middle three parameters are outputs for the
width, height and actual number of channels in the image. The pointer that is
returned is the first element in an array of pixel values. The pixels are laid
out row by row with 4 bytes per pixel in the case of `STBI_rgba_alpha` for a
total of `texWidth * texHeight * 4` values.

## Staging image

We're now going to create an image in host visible memory so that we can use
`vkMapMemory` and copy the pixels to it. Pixels within an image object are known
as texels and we'll use that name from this point on. Add the following two
variables in the `createTextureImage` function:

```c++
VDeleter<VkImage> stagingImage{device, vkDestroyImage};
VDeleter<VkDeviceMemory> stagingImageMemory{device, vkFreeMemory};
```

The parameters for an image are specified in a `VkImageCreateInfo` struct:

```c++
VkImageCreateInfo imageInfo = {};
imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
imageInfo.imageType = VK_IMAGE_TYPE_2D;
imageInfo.extent.width = texWidth;
imageInfo.extent.height = texHeight;
imageInfo.extent.depth = 1;
imageInfo.mipLevels = 1;
imageInfo.arrayLayers = 1;
```

The image type, specified in the `imageType` field, tells Vulkan with that kind
of coordinate system the texels in the image are going to be addressed. It is
possible to create 1D, 2D and 3D images. One dimensional images can be used to
store an array of data or gradient, two dimensional images are mainly used for
textures, and three dimensional images can be used to store voxel volumes, for
example. The `extent` field specifies the dimensions of the image, basically how
many texels there are on each axis. That's why `depth` must be `1` instead of
`0`. Our texture will not be an array and we won't be using mipmapping for now.

```c++
imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
```

Vulkan supports many possible image formats, but it makes the most sense to use
exactly the same format for the texels as the pixels loaded with the library.

```c++
imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
```

The `tiling` field can have one of two values:

* `VK_IMAGE_TILING_LINEAR`: Texels are laid out in row-major order like our
`pixels` array
* `VK_IMAGE_TILING_OPTIMAL`: Texels are laid out in an implementation defined
order for optimal access

If you want to be able to directly access texels in the memory of the image,
then you must use `VK_IMAGE_TILING_LINEAR`. We want to be able to directly copy
the data in `pixels` to the staging image memory, so we should use it. Unlike
the layout of an image, the tiling mode cannot be changed at a later time. We're
going to use `VK_IMAGE_TILING_OPTIMAL` for the final image.

```c++
imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
```

There are only two possible values for the `initialLayout` of an image:

* `VK_IMAGE_LAYOUT_UNDEFINED`: Not usable by the GPU and the very first
transition will discard the texels.
* `VK_IMAGE_LAYOUT_PREINITIALIZED`: Not usable by the GPU, but the first
transition will preserve the texels.

An initially undefined layout is suitable for images that will be used as
attachments, like color and depth buffers. In that case we don't care about any
initial data, because it'll probably be cleared by a render pass before use. If
you want to fill it with data, like a texture, then you should use the
preinitialized layout.

```c++
imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
```

The `usage` field has the same semantics as the one during buffer creation. The
staging image is going to be copied to the final texture image, so it should be
set up as a transfer source.

```c++
imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
```

The staging image will only be used by one queue family: the one that supports
transfer operations.

```c++
imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
imageInfo.flags = 0; // Optional
```

The `samples` flag is related to multisampling. This is only relevant for images
that will be used as attachments, so stick to one sample. There are some
optional flags for images that are related to sparse images. Sparse images are
images where only certain regions are actually backed by memory. If you were
using a 3D texture for a voxel terrain, for example, then you could use this to
avoid allocating memory to store large volumes of "air" values. We won't be
using it in this tutorial, so leave it to its default value of `0`.

```c++
if (vkCreateImage(device, &imageInfo, nullptr, stagingImage.replace()) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
}
```

The image is created using `vkCreateImage`, which doesn't have any particularly
noteworthy parameters. It is possible that the `VK_FORMAT_R8G8B8A8_UNORM` format
is not supported by the graphics hardware. You should have a list of acceptable
alternatives and go with the best one that is supported. However, support for
this particular format is so widespread that we'll skip this step. Using
different formats would also require annoying conversions. We will get back to
this in the depth buffer chapter, where we'll implement such a system.

```c++
VkMemoryRequirements memRequirements;
vkGetImageMemoryRequirements(device, stagingImage, &memRequirements);

VkMemoryAllocateInfo allocInfo = {};
allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
allocInfo.allocationSize = memRequirements.size;
allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

if (vkAllocateMemory(device, &allocInfo, nullptr, stagingImageMemory.replace()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
}

vkBindImageMemory(device, stagingImage, stagingImageMemory, 0);
```

Allocating memory for an image works in exactly the same way as allocating
memory for a buffer. Use `vkGetImageMemoryRequirements` instead of
`vkGetBufferMemoryRequirements`, and use `vkBindImageMemory` instead of
`vkBindBufferMemory`. Remember that we need the memory to be host visible to be
able to use `vkMapMemory`, so you should specify that property when looking for
the right memory type.

We can now use the `vkMapMemory` function to (temporarily) access the memory of
the staging image directly from our application. It returns a pointer to the
first byte in the memory buffer:

```c++
void* data;
vkMapMemory(device, stagingImageMemory, 0, imageSize, 0, &data);
```

Unfortunately we can't just copy the pixel bytes directly into the image memory
with `memcpy` and assume that this works correctly. The problem is that there
may be padding bytes between rows of pixels. In other words, the graphics card
may assume that one row of pixels is not `texWidth * 4` bytes wide, but rather
`texWidth * 4 + paddingBytes`. To handle this correctly, we need to query how
bytes are arranged in our staging image using `vkGetImageSubresourceLayout`:

```c++
VkImageSubresource subresource = {};
subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
subresource.mipLevel = 0;
subresource.arrayLayer = 0;

VkSubresourceLayout stagingImageLayout;
vkGetImageSubresourceLayout(device, stagingImage, &subresource, &stagingImageLayout);
```

Images contain one or more *subresources*, which are specific images within an
image. For example, there is one subresource for every entry in an array image.
In this case we don't have an array image, so there is simply one subresource at
entry 0 and the base mipmapping level.

The `rowPitch` member of the `VkSubresourceLayout` struct specifies the total
number of bytes of each row of pixels in the image. If this value is equal to
`texWidth * 4`, then we're lucky and we *can* use `memcpy`, because there are no
padding bytes in that case.

```c++
if (stagingImageLayout.rowPitch == texWidth * 4) {
    memcpy(data, pixels, (size_t) imageSize);
} else {

}
```

This is usually the case when your images have a power-of-2 size (e.g. 512 or
1024). Otherwise, we'll have to copy the pixels row-by-row using the right
offset:

```c++
uint8_t* dataBytes = reinterpret_cast<uint8_t*>(data);

for (int y = 0; y < texHeight; y++) {
    memcpy(
        &dataBytes[y * stagingImageLayout.rowPitch],
        &pixels[y * texWidth * 4],
        texWidth * 4
    );
}
```

Each subsequent row in the image memory is offset by `rowPitch` and the original
pixels are offset by `texWidth * 4` without padding bytes.

If you're done accessing the memory buffer, then you should unmap it with
`vkUnmapMemory`. It is not necessary to call `vkUnmapMemory` now if you want to
access the staging image memory again later on. The writes to the buffer will
already be visible without calling this function.

```c++
void* data;
vkMapMemory(device, stagingImageMemory, 0, imageSize, 0, &data);

    if (stagingImageLayout.rowPitch == texWidth * 4) {
        memcpy(data, pixels, (size_t) imageSize);
    } else {
        uint8_t* dataBytes = reinterpret_cast<uint8_t*>(data);

        for (int y = 0; y < texHeight; y++) {
            memcpy(&dataBytes[y * stagingImageLayout.rowPitch], &pixels[y * texWidth * 4], texWidth * 4);
        }
    }

vkUnmapMemory(device, stagingImageMemory);
```

Don't forget to clean up the original pixel array now:

```c++
stbi_image_free(pixels);
```

## Texture image

We will now abstract image creation into a `createImage` function, like we did
for buffers. Create the function and move the image object creation and memory
allocation to it:

```c++
void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VDeleter<VkImage>& image, VDeleter<VkDeviceMemory>& imageMemory) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, image.replace()) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, imageMemory.replace()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}
```

I've made the width, height, format, tiling mode, usage, and memory properties
parameters, because these will all vary between the images we'll be creating
throughout this tutorial.

The `createTextureImage` function can now be simplified to:

```c++
void createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VDeleter<VkImage> stagingImage{device, vkDestroyImage};
    VDeleter<VkDeviceMemory> stagingImageMemory{device, vkFreeMemory};
    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingImage, stagingImageMemory);

    VkImageSubresource subresource = {};
    subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.mipLevel = 0;
    subresource.arrayLayer = 0;

    VkSubresourceLayout stagingImageLayout;
    vkGetImageSubresourceLayout(device, stagingImage, &subresource, &stagingImageLayout);

    void* data;
    vkMapMemory(device, stagingImageMemory, 0, imageSize, 0, &data);

    if (stagingImageLayout.rowPitch == texWidth * 4) {
        memcpy(data, pixels, (size_t) imageSize);
    } else {
        uint8_t* dataBytes = reinterpret_cast<uint8_t*>(data);

        for (int y = 0; y < texHeight; y++) {
            memcpy(&dataBytes[y * stagingImageLayout.rowPitch], &pixels[y * texWidth * 4], texWidth * 4);
        }
    }

    vkUnmapMemory(device, stagingImageMemory);

    stbi_image_free(pixels);
}
```

The next step is to create the actual texture image. Define two new class
members to hold the handle to the image and its memory:

```c++
VDeleter<VkCommandPool> commandPool{device, vkDestroyCommandPool};
VDeleter<VkImage> textureImage{device, vkDestroyImage};
VDeleter<VkDeviceMemory> textureImageMemory{device, vkFreeMemory};
VDeleter<VkBuffer> vertexBuffer{device, vkDestroyBuffer};
```

The final texture image can now be created using the same function:

```c++
createImage(
    texWidth, texHeight,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    textureImage,
    textureImageMemory
);
```

The dimensions of the image should be the same as the staging image. The formats
should also be *compatible*, because the command simply copies the raw image
data. Two color formats are compatible if they have the same number of bytes per
pixel. Depth/stencil formats, which we'll see in one of the next chapters, need
to be exactly equal. The tiling mode on the other hand does not need to be the
same. The texture image will be used as the destination in the transfer, and we
want to be able to sample texels from it in the shader. The
`VK_IMAGE_USAGE_SAMPLED_BIT` flag is necessary to allow that. The memory of the
image should be device local for best performance, just like the vertex buffer.

## Layout transitions

The function we're going to write now involves recording and executing a command
buffer again, so now's a good time to move that logic into a helper function or
two:

```c++
VkCommandBuffer beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
```

The code for these functions is based on the existing code in `copyBuffer`. You
can now simplify that function to:

```c++
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}
```

If we were still using buffers, then we could now write a function to record and
execute `vkCmdCopyImage` to finish the job, but this command requires the images
to be in the right layout first. Create a new function to handle layout
transitions:

```c++
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    endSingleTimeCommands(commandBuffer);
}
```

One of the most common ways to perform layout transitions is using an *image
memory barrier*. A pipeline barrier like that is generally used to synchronize
access to resources, like ensuring that a write to a buffer completes before
reading from it, but it can also be used to transition image layouts and
transfer queue family ownership when `VK_SHARING_MODE_EXCLUSIVE` is used. There
is an equivalent *buffer memory barrier* to do this for buffers.

```c++
VkImageMemoryBarrier barrier = {};
barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
barrier.oldLayout = oldLayout;
barrier.newLayout = newLayout;
```

The first two fields specify layout transition. It is possible to use
`VK_IMAGE_LAYOUT_UNDEFINED` as `oldLayout` if you don't care about the existing
contents of the image.

```c++
barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
```

If you are using the barrier to transfer queue family ownership, then these two
fields should be the indices of the queue families. They must be set to
`VK_QUEUE_FAMILY_IGNORED` if you don't want to do this (not the default value!).

```c++
barrier.image = image;
barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
barrier.subresourceRange.baseMipLevel = 0;
barrier.subresourceRange.levelCount = 1;
barrier.subresourceRange.baseArrayLayer = 0;
barrier.subresourceRange.layerCount = 1;
```

The `image` and `subresourceRange` specify the image that is affected and the
specific part of the image. Our image is not an array and does not mipmapping
levels, so only one level and layer are specified.

```c++
barrier.srcAccessMask = 0; // TODO
barrier.dstAccessMask = 0; // TODO
```

Barriers are primarily used for synchronization purposes, so you must specify
which types of operations that involve the resource must happen before the
barrier, and which operations that involve the resource must wait on the
barrier. We need to do that despite already using `vkQueueWaitIdle` to manually
synchronize. The right values depend on the old and new layout, so we'll get
back to this once we've figured out which transitions we're going to use.

```c++
vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
);
```

All types of pipeline barriers are submitted using the same function. The first
parameter specifies in which pipeline stage the operations occur that should
happen before the barrier. The second parameter specifies the pipeline stage in
which operations will wait on the barrier. We want it to happen immediately, so
we're going with the top of the pipeline.

The third parameter is either `0` or `VK_DEPENDENCY_BY_REGION_BIT`. The latter
turns the barrier into a per-region condition. That means that the
implementation is allowed to already begin reading from the parts of a resource
that were written so far, for example.

The last three pairs of parameters reference arrays of pipeline barriers of the
three available types: memory barriers, buffer memory barriers, and image memory
barriers like the one we're using here. Note that we're not using the `VkFormat`
parameter yet, but we'll be using that one for special transitions in the depth
buffer chapter.

## Copying images

Before we get back to `createTextureImage`, we're going to write one more helper
function: `copyImage`:

```c++
void copyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    endSingleTimeCommands(commandBuffer);
}
```

Just like with buffers, you need to specify which part of the image needs to be
copied to which part of the other image. This happens through `VkImageCopy`
structs:

```c++
VkImageSubresourceLayers subResource = {};
subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
subResource.baseArrayLayer = 0;
subResource.mipLevel = 0;
subResource.layerCount = 1;

VkImageCopy region = {};
region.srcSubresource = subResource;
region.dstSubresource = subResource;
region.srcOffset = {0, 0, 0};
region.dstOffset = {0, 0, 0};
region.extent.width = width;
region.extent.height = height;
region.extent.depth = 1;
```

All of these fields are fairly self-explanatory. Image copy operations are
enqueued using the `vkCmdCopyImage` function:

```c++
vkCmdCopyImage(
    commandBuffer,
    srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1, &region
);
```

The first two pairs of parameters specify the source image/layout and
destination image/layout. I'm assuming here that they've been previously
transitioned to the optimal transfer layouts.

## Preparing the texture image

We now have all of the tools we need to finish setting up the texture image, so
we're going back to the `createTextureImage` function. The last thing we did
there was creating the texture image. The next step is to copy the staging image
to the texture image. This involves three operations:

* Transition the staging image to `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`
* Transition the texture image to `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`
* Execute the image copy operation

This is easy to do with the functions we just created:

```c++
transitionImageLayout(stagingImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
copyImage(stagingImage, textureImage, texWidth, texHeight);
```

Both `VK_IMAGE_LAYOUT_PREINITIALIZED` and `VK_IMAGE_LAYOUT_UNDEFINED` are valid
values for old layout when transitioning `textureImage`, because we don't care
about its contents before the copy operation.

To be able to start sampling from the texture image in the shader, we need one
last transition:

```c++
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
```

## Transition barrier masks

If run your application with validation layers enabled now, then you'll see that
it complains about the access masks in `transitionImageLayout` being invalid.
We still need to set those based on the layouts in the transition.

There are three transitions we need to handle:

* Preinitialized → transfer source: transfer reads should wait on host writes
* Preinitialized → transfer destination: transfer writes should wait on host
writes
* Transfer destination → shader reading: shader reads should wait on transfer
writes

These rules are specified using the following access masks:

```c++
if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
} else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
} else {
    throw std::invalid_argument("unsupported layout transition!");
}
```

If we need to do more transitions in the future, then we'll extend the function.
The application should now run successfully, although there are of course no
visual changes yet. One thing to note is that command buffer submission results
in implicit `VK_ACCESS_HOST_WRITE_BIT` synchronization at the beginning. Since
the `transitionImageLayout` function executes a command buffer with only a
single command, we can use this implicit synchronization and set `srcAccessMask`
to `0` for the first two types of transitions. It's up to you if you want to be
explicit about it or not, but I'm personally not a fan of relying on these
OpenGL-like "hidden" operations.

There is actually a special type of image layout that supports all operations,
`VK_IMAGE_LAYOUT_GENERAL`. The problem with it, of course, is that it doesn't
necessarily offer the best performance for any operation. It is required for
some special cases, like using an image as both input and output, or for reading
an image after it has left the preinitialized layout.

All of the helper functions that submit commands so far have been set up to
execute synchronously by waiting for the queue to become idle. For practical
applications it is recommended to combine these operations in a single command
buffer and execute them asynchronously for higher throughput, especially the
transitions and copy in the `createTextureImage` function. Try to experiment
with this by creating a `setupCommandBuffer` that the helper functions record
commands into, and add a `flushSetupCommands` to execute the commands that have
been recorded so far. It's best to do this after the texture mapping works to
check if the texture resources are still set up correctly.

In this tutorial we used another image as staging resource for the texture, but
it's also possible to use a buffer and copy pixels from it using
`vkCmdCopyBufferToImage`. It is recommended to use this approach for improved
performance on [some hardware](https://developer.nvidia.com/vulkan-memory-management)
if you need to update the data in an image often.

The image now contains the texture, but we still need a way to access it from
the graphics pipeline. We'll work on that in the next chapter.

[C++ code](/code/texture_image.cpp) /
[Vertex shader](/code/shader_ubo.vert) /
[Fragment shader](/code/shader_ubo.frag)