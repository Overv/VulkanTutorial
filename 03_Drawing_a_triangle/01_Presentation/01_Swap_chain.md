In this chapter we will look at the infrastructure that gives you images to
render to that can be presented to the screen afterwards. This infrastructure is
known as the *swap chain* and must be created explicitly in Vulkan. The swap
chain is essentially a queue of images that are waiting to be presented to the
screen. Our application will acquire such an image to draw to it, and then
return it to the queue. How exactly the queue works and the conditions for
presenting an image from the queue depend on how the swap chain is set up, but
the general purpose of the swap chain is to synchronize the presentation of
images with the refresh rate of the screen.

## Checking for swap chain support

Not all graphics cards are capable of presenting images directly to a screen for
various reasons, for example because they are designed for servers and don't
have any display outputs. Secondly, since image presentation is heavily tied
into the window system and the surfaces associated with windows, it is not
actually part of the Vulkan core. You have to enable the `VK_KHR_swapchain`
device extension after querying for its support.

For that purpose we'll first extend the `isDeviceSuitable` function to check if
this extension is supported. We've previously seen how to list the extensions
that are supported by a `VkPhysicalDevice`, so doing that should be fairly
straightforward. Note that the Vulkan header file provides a nice macro
`VK_KHR_SWAPCHAIN_EXTENSION_NAME` that is defined as `VK_KHR_swapchain`. The
advantage of using this macro is that the compiler will catch misspellings.

First declare a list of required device extensions, similar to the list of
validation layers to enable.

```c++
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
```

Next, create a new function `checkDeviceExtensionSupport` that is called from
`isDeviceSuitable` as an additional check:

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    return indices.isComplete() && extensionsSupported;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    return true;
}
```

Modify the body of the function to enumerate the extensions and check if all of
the required extensions are amongst them.

```c++
bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}
```

I've chosen to use a set of strings here to represent the unconfirmed required
extensions. That way we can easily tick them off while enumerating the sequence
of available extensions. Of course you can also use a nested loop like in
`checkValidationLayerSupport`. The performance difference is irrelevant. Now run
the code and verify that your graphics card is indeed capable of creating a
swap chain. It should be noted that the availability of a presentation queue,
as we checked in the previous chapter, implies that the swap chain extension
must be supported. However, it's still good to be explicit about things, and
the extension does have to be explicitly enabled.

Enabling the extension just requires a small change to the logical device
creation structure:

```c++
createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
createInfo.ppEnabledExtensionNames = deviceExtensions.data();
```

## Querying details of swap chain support

Just checking if a swap chain is available is not sufficient, because it may not
actually be compatible with our window surface. Creating a swap chain also
involves a lot more settings than instance and device creation, so we need to
query for some more details before we're able to proceed.

There are basically three kinds of properties we need to check:

* Basic surface capabilities (min/max number of images in swap chain, min/max
width and height of images)
* Surface formats (pixel format, color space)
* Available presentation modes

Similar to `findQueueFamilies`, we'll use a struct to pass these details around
once they've been queried. The three aforementioned types of properties come in
the form of the following structs and lists of structs:

```c++
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
```

We'll now create a new function `querySwapChainSupport` that will populate this
struct.

```c++
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    return details;
}
```

This section covers how to query the structs that include this information. The
meaning of these structs and exactly which data they contain is discussed in the
next section.

Let's start with the basic surface capabilities. These properties are simple to
query and are returned into a single `VkSurfaceCapabilitiesKHR` struct.

```c++
vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
```

This function takes the specified `VkPhysicalDevice` and `VkSurfaceKHR` window
surface into account when determining the supported capabilities. All of the
support querying functions have these two as first parameters because they are
the core components of the swap chain.

The next step is about querying the supported surface formats. Because this is a
list of structs, it follows the familiar ritual of 2 function calls:

```c++
uint32_t formatCount;
vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
}
```

Make sure that the vector is resized to hold all the available formats. And
finally, querying the supported presentation modes works exactly the same way
with `vkGetPhysicalDeviceSurfacePresentModesKHR`:

```c++
uint32_t presentModeCount;
vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
}
```

All of the details are in the struct now, so let's extend `isDeviceSuitable`
once more to utilize this function to verify that swap chain support is
adequate. Swap chain support is sufficient for this tutorial if there is at
least one supported image format and one supported presentation mode given the
window surface we have.

```c++
bool swapChainAdequate = false;
if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
}
```

It is important that we only try to query for swap chain support after verifying
that the extension is available. The last line of the function changes to:

```c++
return indices.isComplete() && extensionsSupported && swapChainAdequate;
```

## Choosing the right settings for the swap chain

If the `swapChainAdequate` conditions were met then the support is definitely
sufficient, but there may still be many different modes of varying optimality.
We'll now write a couple of functions to find the right settings for the best
possible swap chain. There are three types of settings to determine:

* Surface format (color depth)
* Presentation mode (conditions for "swapping" images to the screen)
* Swap extent (resolution of images in swap chain)

For each of these settings we'll have an ideal value in mind that we'll go with
if it's available and otherwise we'll create some logic to find the next best
thing.

### Surface format

The function for this setting starts out like this. We'll later pass the
`formats` member of the `SwapChainSupportDetails` struct as argument.

```c++
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

}
```

Each `VkSurfaceFormatKHR` entry contains a `format` and a `colorSpace` member. The
`format` member specifies the color channels and types. For example,
`VK_FORMAT_B8G8R8A8_UNORM` means that we store the B, G, R and alpha channels in
that order with an 8 bit unsigned integer for a total of 32 bits per pixel. The
`colorSpace` member indicates if the SRGB color space is supported or not using
the `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR` flag. Note that this flag used to be
called `VK_COLORSPACE_SRGB_NONLINEAR_KHR` in old versions of the specification.

For the color space we'll use SRGB if it is available, because it [results in more accurate perceived colors](http://stackoverflow.com/questions/12524623/).
Working directly with SRGB colors is a little bit challenging, so we'll use
standard RGB for the color format, of which one of the most common ones is
`VK_FORMAT_B8G8R8A8_UNORM`.

The best case scenario is that the surface has no preferred format, which Vulkan
indicates by only returning one `VkSurfaceFormatKHR` entry which has its
`format` member set to `VK_FORMAT_UNDEFINED`.

```c++
if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
    return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
}
```

If we're not free to choose any format, then we'll go through the list and see
if the preferred combination is available:

```c++
for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return availableFormat;
    }
}
```

If that also fails then we could start ranking the available formats based on
how "good" they are, but in most cases it's okay to just settle with the first
format that is specified.

```c++
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}
```

### Presentation mode

The presentation mode is arguably the most important setting for the swap chain,
because it represents the actual conditions for showing images to the screen.
There are four possible modes available in Vulkan:

* `VK_PRESENT_MODE_IMMEDIATE_KHR`: Images submitted by your application are
transferred to the screen right away, which may result in tearing.
* `VK_PRESENT_MODE_FIFO_KHR`: The swap chain is a queue where the display takes
an image from the front of the queue when the display is refreshed and the
program inserts rendered images at the back of the queue. If the queue is full
then the program has to wait. This is most similar to vertical sync as found in
modern games. The moment that the display is refreshed is known as "vertical
blank".
* `VK_PRESENT_MODE_FIFO_RELAXED_KHR`: This mode only differs from the previous
one if the application is late and the queue was empty at the last vertical
blank. Instead of waiting for the next vertical blank, the image is transferred
right away when it finally arrives. This may result in visible tearing.
* `VK_PRESENT_MODE_MAILBOX_KHR`: This is another variation of the second mode.
Instead of blocking the application when the queue is full, the images that are
already queued are simply replaced with the newer ones. This mode can be used to
implement triple buffering, which allows you to avoid tearing with significantly
less latency issues than standard vertical sync that uses double buffering.

Only the `VK_PRESENT_MODE_FIFO_KHR` mode is guaranteed to be available, so we'll
again have to write a function that looks for the best mode that is available:

```c++
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
    return VK_PRESENT_MODE_FIFO_KHR;
}
```

I personally think that triple buffering is a very nice trade-off. It allows us
to avoid tearing while still maintaining a fairly low latency by rendering new
images that are as up-to-date as possible right until the vertical blank. So,
let's look through the list to see if it's available:

```c++
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}
```

Unfortunately some drivers currently don't properly support
`VK_PRESENT_MODE_FIFO_KHR`, so we should prefer `VK_PRESENT_MODE_IMMEDIATE_KHR`
if `VK_PRESENT_MODE_MAILBOX_KHR` is not available:

```c++
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestMode = availablePresentMode;
        }
    }

    return bestMode;
}
```

### Swap extent

That leaves only one major property, for which we'll add one last function:

```c++
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {

}
```

The swap extent is the resolution of the swap chain images and it's almost
always exactly equal to the resolution of the window that we're drawing to. The
range of the possible resolutions is defined in the `VkSurfaceCapabilitiesKHR`
structure. Vulkan tells us to match the resolution of the window by setting the
width and height in the `currentExtent` member. However, some window managers do
allow us to differ here and this is indicated by setting the width and height in
`currentExtent` to a special value: the maximum value of `uint32_t`. In that
case we'll pick the resolution that best matches the window within the
`minImageExtent` and `maxImageExtent` bounds.

```c++
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {WIDTH, HEIGHT};

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}
```

The `max` and `min` functions are used here to clamp the value of `WIDTH` and
`HEIGHT` between the allowed minimum and maximum extents that are supported by
the implementation. Make sure to include the `<algorithm>` header to use them.

## Creating the swap chain

Now that we have all of these helper functions assisting us with the choices we
have to make at runtime, we finally have all the information that is needed to
create a working swap chain.

Create a `createSwapChain` function that starts out with the results of these
calls and make sure to call it from `initVulkan` after logical device creation.

```c++
void initVulkan() {
    createInstance();
    setupDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
}

void createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
}
```

There is actually one more small things that need to be decided upon, but it's
so simple that it's not really worth creating separate functions for them. The
first one is the number of images in the swap chain, essentially the queue
length. The implementation specifies the minimum amount of images to function
properly and we'll try to have one more than that to properly implement triple
buffering.

```c++
uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
}
```

A value of `0` for `maxImageCount` means that there is no limit besides memory
requirements, which is why we need to check for that.

As is tradition with Vulkan objects, creating the swap chain object requires
filling in a large structure. It starts out very familiarly:

```c++
VkSwapchainCreateInfoKHR createInfo = {};
createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
createInfo.surface = surface;
```

After specifying which surface the swap chain should be tied to, the details of
the swap chain images are specified:

```c++
createInfo.minImageCount = imageCount;
createInfo.imageFormat = surfaceFormat.format;
createInfo.imageColorSpace = surfaceFormat.colorSpace;
createInfo.imageExtent = extent;
createInfo.imageArrayLayers = 1;
createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
```

The `imageArrayLayers` specifies the amount of layers each image consists of.
This is always `1` unless you are developing a stereoscopic 3D application. The
`imageUsage` bit field specifies what kind of operations we'll use the images in
the swap chain for. In this tutorial we're going to render directly to them,
which means that they're used as color attachment. It is also possible that
you'll render images to a separate image first to perform operations like
post-processing. In that case you may use a value like
`VK_IMAGE_USAGE_TRANSFER_DST_BIT` instead and use a memory operation to transfer
the rendered image to a swap chain image.

```c++
QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
uint32_t queueFamilyIndices[] = {(uint32_t) indices.graphicsFamily, (uint32_t) indices.presentFamily};

if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
} else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // Optional
    createInfo.pQueueFamilyIndices = nullptr; // Optional
}
```

Next, we need to specify how to handle swap chain images that will be used
across multiple queue families. That will be the case in our application if the
graphics queue family is different from the presentation queue. We'll be drawing
on the images in the swap chain from the graphics queue and then submitting them
on the presentation queue. There are two ways to handle images that are
accessed from multiple queues:

* `VK_SHARING_MODE_EXCLUSIVE`: An image is owned by one queue family at a time
and ownership must be explicitly transfered before using it in another queue
family. This option offers the best performance.
* `VK_SHARING_MODE_CONCURRENT`: Images can be used across multiple queue
families without explicit ownership transfers.

If the queue families differ, then we'll be using the concurrent mode in this
tutorial to avoid having to do the ownership chapters, because these involve
some concepts that are better explained at a later time. Concurrent mode
requires you to specify in advance between which queue families ownership will
be shared using the `queueFamilyIndexCount` and `pQueueFamilyIndices`
parameters. If the graphics queue family and presentation queue family are the
same, which will be the case on most hardware, then we should stick to exclusive
mode, because concurrent mode requires you to specify at least two distinct
queue families.

```c++
createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
```

We can specify that a certain transform should be applied to images in the swap
chain if it is supported (`supportedTransforms` in `capabilities`), like a 90
degree clockwise rotation or horizontal flip. To specify that you do not want
any transformation, simply specify the current transformation.

```c++
createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
```

The `compositeAlpha` field specifies if the alpha channel should be used for
blending with other windows in the window system. You'll almost always want to
simply ignore the alpha channel, hence `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR`.

```c++
createInfo.presentMode = presentMode;
createInfo.clipped = VK_TRUE;
```

The `presentMode` member speaks for itself. If the `clipped` member is set to
`VK_TRUE` then that means that we don't care about the color of pixels that are
obscured, for example because another window is in front of them. Unless you
really need to be able to read these pixels back and get predictable results,
you'll get the best performance by enabling clipping.

```c++
createInfo.oldSwapchain = VK_NULL_HANDLE;
```

That leaves one last field, `oldSwapChain`. With Vulkan it's possible that in
your swap chain becomes invalid or unoptimized while your application is
running, for example because the window was resized. In that case the swap chain
actually needs to be recreated from scratch and a reference to the old one must
be specified in this field. This is a complex topic that we'll learn more about
in [a future chapter](!Drawing_a_triangle/Swap_chain_recreation). For now we'll
assume that we'll only ever create one swap chain.

Now add a class member to store the `VkSwapchainKHR` object:

```c++
VkSwapchainKHR swapChain;
```

Creating the swap chain is now as simple as calling `vkCreateSwapchainKHR`:

```c++
if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
}
```

The parameters are the logical device, swap chain creation info, optional custom
allocators and a pointer to the variable to store the handle in. No surprises
there. It should be cleaned up using `vkDestroySwapchainKHR` before the device:

```c++
void cleanup() {
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    ...
}
```

Now run the application to ensure that the swap chain is created successfully!

Try removing the `createInfo.imageExtent = extent;` line with validation layers
enabled. You'll see that one of the validation layers immediately catches the
mistake and a helpful message is printed:

![](/images/swap_chain_validation_layer.png)

## Retrieving the swap chain images

The swap chain has been created now, so all that remains is retrieving the
handles of the `VkImage`s in it. We'll reference these during rendering
operations in later chapters. Add a class member to store the handles:

```c++
std::vector<VkImage> swapChainImages;
```

The images were created by the implementation for the swap chain and they will
be automatically cleaned up once the swap chain has been destroyed, therefore we
don't need to add any cleanup code.

I'm adding the code to retrieve the handles to the end of the `createSwapChain`
function, right after the `vkCreateSwapchainKHR` call. Retrieving them is very
similar to the other times where we retrieved an array of objects from Vulkan.
First query the number of images in the swap chain with a call to
`vkGetSwapchainImagesKHR`, then resize the container and finally call it again
to retrieve the handles.

```c++
vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
swapChainImages.resize(imageCount);
vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
```

Note that when we created the swap chain, we passed the number of desired images
to a field called `minImageCount`. The implementation is allowed to create more
images, which is why we need to explicitly query the amount again.

One last thing, store the format and extent we've chosen for the swap chain
images in member variables. We'll need them in future chapters.

```c++
VkSwapchainKHR swapChain;
std::vector<VkImage> swapChainImages;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;

...

swapChainImageFormat = surfaceFormat.format;
swapChainExtent = extent;
```

We now have a set of images that can be drawn onto and can be presented to the
window. The next chapter will begin to cover how we can set up the images as
render targets and then we start looking into the actual graphics pipeline and
drawing commands!

[C++ code](/code/swap_chain_creation.cpp)
