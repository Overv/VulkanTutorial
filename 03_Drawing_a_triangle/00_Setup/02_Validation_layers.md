## What are validation layers?

The Vulkan API is designed around the idea of minimal driver overhead and one of
the manifestations of that goal is that there is very limited error checking in
the API by default. Even mistakes as simple as setting enumerations to incorrect
values or passing null pointers to required parameters are generally not
explicitly handled and will simply result in crashes or undefined behavior.
Because Vulkan requires you to be very explicit about everything you're doing,
it's easy to make many small mistakes like using a new GPU feature and
forgetting to request it at logical device creation time.

However, that doesn't mean that these checks can't be added to the API. Vulkan
introduces an elegant system for this known as *validation layers*. Validation
layers are optional components that hook into Vulkan function calls to apply
additional operations. Common operations in validation layers are:

* Checking the values of parameters against the specification to detect misuse
* Tracking creation and destruction of objects to find resource leaks
* Checking thread safety by tracking the threads that calls originate from
* Logging every call and its parameters to the standard output
* Tracing Vulkan calls for profiling and replaying

Here's an example of what the implementation of a function in a diagnostics
validation layer could look like:

```c++
VkResult vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* instance) {

    if (pCreateInfo == nullptr || instance == nullptr) {
        log("Null pointer passed to required parameter!");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    return real_vkCreateInstance(pCreateInfo, pAllocator, instance);
}
```

These validation layers can be freely stacked to include all the debugging
functionality that you're interested in. You can simply enable validation layers
for debug builds and completely disable them for release builds, which gives you
the best of both worlds!

Vulkan does not come with any validation layers built-in, but the LunarG Vulkan
SDK provides a nice set of layers that check for common errors. They're also
completely [open source](https://github.com/LunarG/VulkanTools/tree/master/layers),
so you can check which kind of mistakes they check for and contribute. Using the
validation layers is the best way to avoid your application breaking on
different drivers by accidentally relying on undefined behavior.

Validation layers can only be used if they have been installed onto the system.
For example, the LunarG validation layers are only available on PCs with the
Vulkan SDK installed.

There were formerly two different types of validation layers in Vulkan. Instance
and device specific layers. The idea was that instance layers would only check
calls related to global Vulkan objects like instances and device specific layers
only calls related to a specific GPU. Device specific layers have now been
deprecated, which means that instance validation layers apply to all Vulkan
calls. The specification document still recommends that you enable validation
layers at device level as well for compatibility, which is required by some
implementations. We'll simply specify the same layers as the instance at logical
device level, which we'll see [later on](!Drawing_a_triangle/Setup/Logical_device_and_queues).

## Using validation layers

In this section we'll see how to enable the standard diagnostics layers provided
by the Vulkan SDK. Just like extensions, validation layers need to be enabled by
specifying their name. Instead of having to explicitly specify all of the useful
layers, the SDK allows you to request the `VK_LAYER_LUNARG_standard_validation`
layer that implicitly enables a whole range of useful diagnostics layers.

Let's first add two configuration variables to the program to specify the layers
to enable and whether to enable them or not. I've chosen to base that value on
whether the program is being compiled in debug mode or not. The `NDEBUG` macro
is part of the C++ standard and means "not debug".

```c++
const int WIDTH = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
```

We'll add a new function `checkValidationLayerSupport` that checks if all of
the requested layers are available. First list all of the available extensions
using the `vkEnumerateInstanceLayerProperties` function. Its usage is identical
to that of `vkEnumerateInstanceExtensionProperties` which was discussed in the
instance creation chapter.

```c++
bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    return false;
}
```

Next, check if all of the layers in `validationLayers` exist in the
`availableLayers` list. You may need to include `<cstring>` for `strcmp`.

```c++
for (const char* layerName : validationLayers) {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) {
        if (strcmp(layerName, layerProperties.layerName) == 0) {
            layerFound = true;
            break;
        }
    }

    if (!layerFound) {
        return false;
    }
}

return true;
```

We can now use this function in `createInstance`:

```c++
void createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    ...
}
```

Now run the program in debug mode and ensure that the error does not occur. If
it does, then make sure you have properly installed the Vulkan SDK. If none or
very few layers are being reported, then you may be dealing with
[this issue](https://vulkan.lunarg.com/app/issues/578e8c8d5698c020d71580fc)
(requires a LunarG account to view). See that page for help with fixing it.

Finally, modify the `VkInstanceCreateInfo` struct instantiation to include the
validation layer names if they are enabled:

```c++
if (enableValidationLayers) {
    createInfo.enabledLayerCount = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();
} else {
    createInfo.enabledLayerCount = 0;
}
```

If the check was successful then `vkCreateInstance` should not ever return a
`VK_ERROR_LAYER_NOT_PRESENT` error, but you should run the program to make sure.

## Message callback

Unfortunately just enabling the layers doesn't help much, because they currently
have no way to relay the debug messages back to our program. To receive those
messages we have to set up a callback, which requires the `VK_EXT_debug_report`
extension.

We'll first create a `getRequiredExtensions` function that will return the
required list of extensions based on whether validation layers are enabled or
not:

```c++
std::vector<const char*> getRequiredExtensions() {
    std::vector<const char*> extensions;

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (unsigned int i = 0; i < glfwExtensionCount; i++) {
        extensions.push_back(glfwExtensions[i]);
    }

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    return extensions;
}
```

The extensions specified by GLFW are always required, but the debug report
extension is conditionally added. Note that I've used the
`VK_EXT_DEBUG_REPORT_EXTENSION_NAME` macro here which is equal to the literal
string "VK_EXT_debug_report". Using this macro lets you avoid typos.

We can now use this function in `createInstance`:

```c++
auto extensions = getRequiredExtensions();
createInfo.enabledExtensionCount = extensions.size();
createInfo.ppEnabledExtensionNames = extensions.data();
```

Run the program to make sure you don't receive a
`VK_ERROR_EXTENSION_NOT_PRESENT` error. We don't really need to check for the
existence of this extension, because it should be implied by the availability of
the validation layers.

Now let's see what a callback function looks like. Add a new static member
function called `debugCallback` with the `PFN_vkDebugReportCallbackEXT`
prototype. The `VKAPI_ATTR` and `VKAPI_CALL` ensure that the function has the
right signature for Vulkan to call it.

```c++
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData) {

    std::cerr << "validation layer: " << msg << std::endl;

    return VK_FALSE;
}
```

The first parameter specifies the type of message, which can be a combination of
any of the following bit flags:

* `VK_DEBUG_REPORT_INFORMATION_BIT_EXT`
* `VK_DEBUG_REPORT_WARNING_BIT_EXT`
* `VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT`
* `VK_DEBUG_REPORT_ERROR_BIT_EXT`
* `VK_DEBUG_REPORT_DEBUG_BIT_EXT`

The `objType` parameter specifies the type of object that is the subject of the
message. For example if `obj` is a `VkPhysicalDevice` then `objType` would be
`VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT`. This works because internally all
Vulkan handles are typedef'd as `uint64_t`.

The `msg` parameter contains the pointer to the message itself. Finally, there's
a `userData` parameter to pass your own data to the callback.

All that remains now is telling Vulkan about the callback function. Perhaps
somewhat surprisingly, even the debug callback in Vulkan is managed with a
handle that needs to be explicitly created and destroyed. Add a class member for
this handle right under `instance`:

```c++
VkDebugReportCallbackEXT callback;
```

Now add a function `setupDebugCallback` to be called from `initVulkan` right
after `createInstance`:

```c++
void initVulkan() {
    createInstance();
    setupDebugCallback();
}

void setupDebugCallback() {
    if (!enableValidationLayers) return;

}
```

We'll need to fill in a structure with details about the callback:

```c++
VkDebugReportCallbackCreateInfoEXT createInfo = {};
createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
createInfo.pfnCallback = debugCallback;
```

The `flags` field allows you to filter which types of messages you would like to
receive. The `pfnCallback` field specifies the pointer to the callback function.
You can optionally pass a pointer to the `pUserData` field which will be passed
along to the callback function via the `userData` parameter. You could use this
to pass a pointer to the `HelloTriangleApplication` class, for example.

This struct should be passed to the `vkCreateDebugReportCallbackEXT` function to
create the `VkDebugReportCallbackEXT` object. Unfortunately, because this
function is an extension function, it is not automatically loaded. We have to
look up its address ourselves using `vkGetInstanceProcAddr`. We're going to
create our own proxy function that handles this in the background. I've added it
right above the `VDeleter` definition.

```c++
VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
    auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
```

The `vkGetInstanceProcAddr` function will return `nullptr` if the function
couldn't be loaded. We can now call this function to create the extension
object if it's available:

```c++
if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug callback!");
}
```

Let's see if it works... Run the program and close the window once you're fed up
with staring at the blank window. You'll see that the following message is
printed to the command prompt:

![](/images/validation_layer_test.png)

Oops, it has already spotted a bug in our program! The
`VkDebugReportCallbackEXT` object needs to be cleaned up with a call to
`vkDestroyDebugReportCallbackEXT`. Change the `callback` variable to use our
deleter wrapper. Similarly to `vkCreateDebugReportCallbackEXT` the function
needs to be explicitly loaded. Create another proxy function right below
`CreateDebugReportCallbackEXT`:

```c++
void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}
```

Make sure that this function is either a static class function or a function
outside the class. We can then specify it as cleanup function:

```c++
VDeleter<VkDebugReportCallbackEXT> callback{instance, DestroyDebugReportCallbackEXT};
```

Make sure to change the line that creates the debug report callback to use the
`replace()` method of the wrapper:

```c++
if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, callback.replace()) != VK_SUCCESS) {
```

When you run the program again you'll see that the error message has
disappeared. If you want to see which call triggered a message, you can add a
breakpoint to the message callback and look at the stack trace.

## Configuration

There are a lot more settings for the behavior of validation layers than just
the flags specified in the `VkDebugReportCallbackCreateInfoEXT` struct. Browse
to the Vulkan SDK and go to the `Config` directory. There you will find a
`vk_layer_settings.txt` file that explains how to configure the layers.

To configure the layer settings for your own application, copy the file to the
`Debug` and `Release` directories of your project and follow the instructions to
set the desired behavior. However, for the remainder of this tutorial I'll
assume that you're using the default settings.

Throughout this tutorial I'll be making a couple of intentional mistakes to show
you how helpful the validation layers are with catching them and to teach you
how important it is to know exactly what you're doing with Vulkan. Now it's time
to look at [Vulkan devices in the system](!Drawing_a_triangle/Setup/Physical_devices_and_queue_families).

[C++ code](/code/validation_layers.cpp)
