To use any `VkImage`, including those in the swap chain, in the render pipeline
we have to create a `VkImageView` object. An image view is quite literally a
view into an image. It describes how to access the image and which part of the
image to access, for example if it should be treated as a 2D texture depth
texture without any mipmapping levels.

In this chapter we'll write a `createImageViews` function that creates a basic
image view for every image in the swap chain so that we can use them as color
targets later on.

First add a class member to store the image views in. Unlike the `VkImage`s, the
`VkImageView` objects are created by us so we need to clean them up ourselves
later.

```c++
std::vector<VDeleter<VkImageView>> swapChainImageViews;
```

Create the `createImageViews` function and call it right after swap chain
creation.

```c++
void initVulkan() {
    createInstance();
    setupDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
}

void createImageViews() {

}
```

The first thing we need to do is resize the list to fit all of the image views
we'll be creating. This is also the place where we'll actually define the
deleter function.

```c++
void createImageViews() {
    swapChainImageViews.resize(swapChainImages.size(), VDeleter<VkImageView>{device, vkDestroyImageView});

}
```

The `resize` function initializes all of the list items with the right deleter.
Next, set up the loop that iterates over all of the swap chain images.

```c++
for (uint32_t i = 0; i < swapChainImages.size(); i++) {

}
```

The parameters for image view creation are specified in a
`VkImageViewCreateInfo` structure. The first few parameters are straightforward.

```c++
VkImageViewCreateInfo createInfo = {};
createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
createInfo.image = swapChainImages[i];
```

The `viewType` and `format` fields specify how the image data should be
interpreted. The `viewType` parameter allows you to treat images as 1D textures,
2D textures, 3D textures and cube maps.

```c++
createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
createInfo.format = swapChainImageFormat;
```

The `components` field allows you to swizzle the color channels around. For
example, you can map all of the channels to the red channel for a monochrome
texture. You can also map constant values of `0` and `1` to a channel. In our
case we'll stick to the default mapping.

```c++
createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
```

The `subresourceRange` field describes what the image's purpose is and which
part of the image should be accessed. Our images will be used as color targets
without any mipmapping levels or multiple layers.

```c++
createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
createInfo.subresourceRange.baseMipLevel = 0;
createInfo.subresourceRange.levelCount = 1;
createInfo.subresourceRange.baseArrayLayer = 0;
createInfo.subresourceRange.layerCount = 1;
```

If you were working on a stereographic 3D application, then you would create a
swap chain with multiple layers. You could then create multiple image views for
each image representing the views for the left and right eyes by accessing
different layers.

Creating the image view is now a matter of calling `vkCreateImageView`:

```c++
if (vkCreateImageView(device, &createInfo, nullptr, swapChainImageViews[i].replace()) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image views!");
}
```

That's it, now run the program to verify that the image views are created
properly and destroyed properly. Checking the latter requires enabling the
validation layers, or putting a print statement in the deleter function.

An image view is sufficient to start using an image as a texture, but it's not
quite ready to be used as a render target just yet. That requires one more step
of indirection, known as a framebuffer. But first we'll have to set up the
graphics pipeline.

[C++ code](/code/image_views.cpp)
