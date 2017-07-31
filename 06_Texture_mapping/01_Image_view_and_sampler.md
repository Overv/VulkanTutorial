In this chapter we're going to create two more resources that are needed for the
graphics pipeline to sample an image. The first resource is one that we've
already seen before while working with the swap chain images, but the second one
is new - it relates to how the shader will read texels from the image.

## Texture image view

We've seen before, with the swap chain images and the framebuffer, that images
are accessed through image views rather than directly. We will also need to
create such an image view for the texture image.

Add a class member to hold a `VkImageView` for the texture image and create a
new function `createTextureImageView` where we'll create it:

```c++
VkImageView textureImageView;

...

void initVulkan() {
    ...
    createTextureImage();
    createTextureImageView();
    createVertexBuffer();
    ...
}

...

void createTextureImageView() {

}
```

The code for this function can be based directly on `createImageViews`. The only
two changes you have to make are the `format` and the `image`:

```c++
VkImageViewCreateInfo viewInfo = {};
viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
viewInfo.image = textureImage;
viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
viewInfo.subresourceRange.baseMipLevel = 0;
viewInfo.subresourceRange.levelCount = 1;
viewInfo.subresourceRange.baseArrayLayer = 0;
viewInfo.subresourceRange.layerCount = 1;
```

I've left out the explicit `viewInfo.components` initialization, because
`VK_COMPONENT_SWIZZLE_IDENTITY` is defined as `0` anyway. Finish creating the
image view by calling `vkCreateImageView`:

```c++
if (vkCreateImageView(device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image view!");
}
```

Because so much of the logic is duplicated from `createImageViews`, you may wish
to abstract it into a new `createImageView` function:

```c++
VkImageView createImageView(VkImage image, VkFormat format) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}
```

The `createTextureImageView` function can now be simplified to:

```c++
void createTextureImageView() {
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM);
}
```

And `createImageViews` can be simplified to:

```c++
void createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
    }
}
```

Make sure to destroy the image view at the end of the program, right before
destroying the image itself:

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyImageView(device, textureImageView, nullptr);

    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);
```

## Samplers

It is possible for shaders to read texels directly from images, but that is not
very common when they are used as textures. Textures are usually accessed
through samplers, which will apply filtering and transformations to compute the
final color that is retrieved.

These filters are helpful to deal with problems like oversampling. Consider a
texture that is mapped to geometry with more fragments than texels. If you
simply took the closest texel for the texture coordinate in each fragment, then
you would get a result like the first image:

![](/images/texture_filtering.png)

If you combined the 4 closest texels through linear interpolation, then you
would get a smoother result like the one on the right. Of course your
application may have art style requirements that fit the left style more (think
Minecraft), but the right is preferred in conventional graphics applications. A
sampler object automatically applies this filtering for you when reading a color
from the texture.

Undersampling is the opposite problem, where you have more texels than
fragments. This will lead to artifacts when sampling high frequency patterns
like a checkerboard texture at a sharp angle:

![](/images/anisotropic_filtering.png)

As shown in the left image, the texture turns into a blurry mess in the
distance. The solution to this is [anisotropic filtering](https://en.wikipedia.org/wiki/Anisotropic_filtering),
which can also be applied automatically by a sampler.

Aside from these filters, a sampler can also take care of transformations. It
determines what happens when you try to read texels outside the image through
its *addressing mode*. The image below displays some of the possibilities:

![](/images/texture_addressing.png)

We will now create a function `createTextureSampler` to set up such a sampler
object. We'll be using that sampler to read colors from the texture in the
shader later on.

```c++
void initVulkan() {
    ...
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    ...
}

...

void createTextureSampler() {

}
```

Samplers are configured through a `VkSamplerCreateInfo` structure, which
specifies all filters and transformations that it should apply.

```c++
VkSamplerCreateInfo samplerInfo = {};
samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
samplerInfo.magFilter = VK_FILTER_LINEAR;
samplerInfo.minFilter = VK_FILTER_LINEAR;
```

The `magFilter` and `minFilter` fields specify how to interpolate texels that
are magnified or minified. Magnification concerns the oversampling problem
describes above, and minification concerns undersampling. The choices are
`VK_FILTER_NEAREST` and `VK_FILTER_LINEAR`, corresponding to the modes
demonstrated in the images above.

```c++
samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
```

The addressing mode can be specified per axis using the `addressMode` fields.
The available values are listed below. Most of these are demonstrated in the
image above. Note that the axes are called U, V and W instead of X, Y and Z.
This is a convention for texture space coordinates.

* `VK_SAMPLER_ADDRESS_MODE_REPEAT`: Repeat the texture when going beyond the
image dimensions.
* `VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT`: Like repeat, but inverts the
coordinates to mirror the image when going beyond the dimensions.
* `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE`: Take the color of the edge closest to
the coordinate beyond the image dimensions.
* `VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE`: Like clamp to edge, but
instead uses the edge opposite to the closest edge.
* `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER`: Return a solid color when sampling
beyond the dimensions of the image.

It doesn't really matter which addressing mode we use here, because we're not
going to sample outside of the image in this tutorial. However, the repeat mode
is probably the most common mode, because it can be used to tile textures like
floors and walls.

```c++
samplerInfo.anisotropyEnable = VK_TRUE;
samplerInfo.maxAnisotropy = 16;
```

These two fields specify if anisotropic filtering should be used. There is no
reason not to use this unless performance is a concern. The `maxAnisotropy`
field limits the amount of texel samples that can be used to calculate the final
color. A lower value results in better performance, but lower quality results.
There is no graphics hardware available today that will use more than 16
samples, because the difference is negligible beyond that point.

```c++
samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
```

The `borderColor` field specifies which color is returned when sampling beyond
the image with clamp to border addressing mode. It is possible to return black,
white or transparent in either float or int formats. You cannot specify an
arbitrary color.

```c++
samplerInfo.unnormalizedCoordinates = VK_FALSE;
```

The `unnormalizedCoordinates` field specifies which coordinate system you want
to use to address texels in an image. If this field is `VK_TRUE`, then you can
simply use coordinates within the `[0, texWidth)` and `[0, texHeight)` range. If
it is `VK_FALSE`, then the texels are addressed using the `[0, 1)` range on all
axes. Real-world applications almost always use normalized coordinates, because
then it's possible to use textures of varying resolutions with the exact same
coordinates.

```c++
samplerInfo.compareEnable = VK_FALSE;
samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
```

If a comparison function is enabled, then texels will first be compared to a
value, and the result of that comparison is used in filtering operations. This
is mainly used for [percentage-closer filtering](https://developer.nvidia.com/gpugems/GPUGems/gpugems_ch11.html)
on shadow maps. We'll look at this in a future chapter.

```c++
samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
samplerInfo.mipLodBias = 0.0f;
samplerInfo.minLod = 0.0f;
samplerInfo.maxLod = 0.0f;
```

All of these fields apply to mipmapping. We will look at mipmapping in a future
chapter, but basically it's another type of filter that can be applied.

The functioning of the sampler is now fully defined. Add a class member to
hold the handle of the sampler object and create the sampler with
`vkCreateSampler`:

```c++
VkImageView textureImageView;
VkSampler textureSampler;

...

void createTextureSampler() {
    ...

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}
```

Note the sampler does not reference a `VkImage` anywhere. The sampler is a
distinct object that provides an interface to extract colors from a texture. It
can be applied to any image you want, whether it is 1D, 2D or 3D. This is
different from many older APIs, which combined texture images and filtering into
a single state.

Destroy the sampler at the end of the program when we'll no longer be accessing
the image:

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);

    ...
}
```

## Anisotropy device feature

If you run your program right now, you'll see a validation layer message like
this:

![](/images/validation_layer_anisotropy.png)

That's because anisotropic filtering is actually an optional device feature. We
need to update the `createLogicalDevice` function to request it:

```c++
VkPhysicalDeviceFeatures deviceFeatures = {};
deviceFeatures.samplerAnisotropy = VK_TRUE;
```

And even though it is very unlikely that a modern graphics card will not support
it, we should update `isDeviceSuitable` to check if it is available:

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    ...

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && supportedFeatures.samplerAnisotropy;
}
```

The `vkGetPhysicalDeviceFeatures` repurposes the `VkPhysicalDeviceFeatures`
struct to indicate which features are supported rather than requested by setting
the boolean values.

Instead of enforcing the availability of anisotropic filtering, it's also
possible to simply not use it by conditionally setting:

```c++
samplerInfo.anisotropyEnable = VK_FALSE;
samplerInfo.maxAnisotropy = 1;
```

In the next chapter we will expose the image and sampler objects to the shaders
to draw the texture onto the square.

[C++ code](/code/sampler.cpp) /
[Vertex shader](/code/shader_ubo.vert) /
[Fragment shader](/code/shader_ubo.frag)
