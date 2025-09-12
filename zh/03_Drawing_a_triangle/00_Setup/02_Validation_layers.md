---
title: 校验层
---

## 校验层是什么？

Vulkan API 基于最小化驱动负担的思想设计，这个目标的一个体现形式就是，在默认情况下，这套 API 中的错误检查十分有限。哪怕是一点小问题，比如枚举
值传错了，或者在必需参数上传了一个空指针，通常都不会显式暴露出来，而只是简单地崩溃或者产生未定义行为。Vulkan 要求你在使用时显式设置每样东西，这
很容易导致许多小毛病的发生：比如使用了新的 GPU 特性却没有在创建逻辑设备的时候请求它。

然而，这并不意味着不能给这套 API 加上错误检查。Vulkan 使用了一个非常优雅的系统来进行错误检查，这就是*校验层*（*validation layers*）。校验
层是可选的，它们能在你调用 Vulkan 函数时插入钩子来执行额外的操作。一般来说，校验层有以下用途：

* 根据规范检测参数值，以避免误用
* 追踪对象的创建和析构过程，以发现资源泄露
* 追踪调用被发起的源头线程，以检查线程安全性
* 把每个调用及其参数都记录在标准输出上
* 追踪 Vulkan 函数的调用，以进行性能分析和重放

以下是诊断校验层（diagnostics validation layer）中一个函数的例子，用来说明其实现大概是什么样子：

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

这些校验层可以自由地组合起来，以实现你感兴趣的所有调试功能。你可以简单地在调试时开启校验层，然后在发布时彻底关掉校验层，这样两全其美。

Vulkan 没有任何内置的校验层，但是 LunarG Vulkan SDK 提供了一套校验层来检查普遍会犯的错误。这些校验层是完全
[开源](https://github.com/KhronosGroup/Vulkan-ValidationLayers)的，所以你可以查看它们检查哪些错误类型，也可以向其贡献代码。使用校验
层是避免你的应用程序因不小心依赖未定义行为而在不同的驱动上出错的最佳方式。

校验层只有在安装在系统上之后才能使用。例如，LunarG 校验层只能在装了 Vulkan SDK 的电脑上使用。

Vulkan 中曾经有两种不同类型的校验层：实例校验层和基于特定设备的校验层。与之对应的想法是，实例层只检查与全局 Vulkan 对象，例如与 instance 有
关的调用；而基于特定设备的校验层则只检查与某种特定 GPU 有关的调用。基于特定设备的校验层现在已经被弃用，这意味着实例校验层可以作用于所有 Vulkan
调用。规范文档仍然推荐你同时在设备层面启用校验层以提高兼容性，这是某些实现所需要的。我们将简单地在逻辑设备层面启用一些和实例层面相同的校验层，
我们[之后](!zh/Drawing_a_triangle/Setup/Logical_device_and_queues)再讨论这个。

## 使用校验层

在这一节我们会看看如何启用一个 Vulkan SDK 提供的标准诊断层。和扩展一样，校验层也需要通过指定名字的方式启用。有用的校验策略都集成在了 SDK 提供
的 `VK_LAYER_KHRONOS_validation` 层中。

首先在程序里加两个配置变量来指定要启用的层，以及是否启用它们。我选择基于是否开启调试模式来设置这个值。`NDEBUG` 宏是 C++ 标准的一部分，代表着
“没有进行调试”（no debug）。

```c++
const int WIDTH = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
```

我们添加了一个新函数 `checkValidationLayerSupport` 来检查是否所有被请求的层都可用。首先使用 `vkEnumerateInstanceLayerProperties`
函数列出所有可用的层。这个函数的使用方式与之前讲解创建实例的时候使用的 `vkEnumerateInstanceExtensionProperties` 函数相同。

```c++
bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    return false;
}
```

接下来，检查 `validationLayers` 中的层是否都存在于 `availableLayers` 列表里。你可能需要引入 `<cstring>` 头文件来使用 `strcmp`。

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

现在我们可以在 `createInstance` 中使用这个函数了：

```c++
void createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    ...
}
```

现在以调试模式运行这个程序并且确保没有任何错误。如果出错了，请参阅“常见问题”页面。

最后，修改 `VkInstanceCreateInfo` 结构体实例，在启用校验层时包含相应的校验层名：

```c++
if (enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
} else {
    createInfo.enabledLayerCount = 0;
}
```

如果检查成功了，那么 `vkCreateInstance` 不应该返回 `VK_ERROR_LAYER_NOT_PRESENT` 错误，不过你应该运行一下程序来确保这点。

## 信息回调函数

校验层默认向标准输出打印调试信息，但我们亦可以在我们的程序中提供回调函数来自己处理这些调试信息。这还能使我们决定想看到哪些种类的哪些消息，毕竟不
是所有消息都代表必要的（致命的）错误。如果你不想现在就这么做，你可以跳过本章的剩余部分。

为了设置回调函数，处理信息及有关详情，我们需要使用 `VK_EXT_debug_utils` 扩展来设置一个调试信使（debug messenger）。

首先我们创建一个 `getRequiredExtensions` 函数，这个函数将根据启用的校验层返回我们需要的插件列表：

```c++
std::vector<const char*> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}
```

由 GLFW 指定的扩展总是需要启用，而调试信使的扩展是有条件地启用的。注意此处我使用了 `VK_EXT_DEBUG_UTILS_EXTENSION_NAME` 宏，它等同
于字符串字面量 "VK_EXT_debug_utils"。使用这个宏可以让你避免打错字。

现在我们可以在 `createInstance` 里面使用这个函数了：

```c++
auto extensions = getRequiredExtensions();
createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
createInfo.ppEnabledExtensionNames = extensions.data();
```

运行程序并且确保没有收到 `VK_ERROR_EXTENSION_NOT_PRESENT` 错误。我们实际上不用检查这个插件是否存在，因为校验层可用这件事本身暗示了它的存
在。

现在我们来看看回调函数应该长什么样。加入一个名为 `debugCallback` 的新的静态成员函数并且使用 `PFN_vkDebugUtilsMessengerCallbackEXT` 函
数原型。`VKAPI_ATTR` 和 `VKAPI_CALL` 确保了这个函数拥有正确的修饰符，以使 Vulkan 能够调用它。

```c++
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}
```

第一个参数指明了消息的严重性，其值是下列值之一：

* `VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT`：诊断消息
* `VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT`：信息性消息，例如一个资源被创建
* `VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT`：有关此消息的行为不一定是一个错误，但很有可能是应用程序中的一个 bug（警告）
* `VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT`：有关此消息的行为是非法的，并且可能导致程序崩溃（错误）

枚举值被设置为递增的，这样就可以用比较运算符来检查一条消息是否比某个严重程度更严重，例如：

```c++
if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    // 消息足够严重，需要被显示
}
```

`messageType` 参数可以是以下值：

- `VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT`：发生了一个与规范或性能无关的事件
- `VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT`：发生了违反规范的行为或者有可能发生的错误
- `VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT`：潜在的会使 Vulkan 性能劣化的使用方式

`pCallbackData` 参数指向 `VkDebugUtilsMessengerCallbackDataEXT` 类型的结构体，其中包含了这个信息的细节，其中最重要的成员有：

- `pMessage`：调试信息，是一个空字符结尾字符串
- `pObjects`：有关此消息的 Vulkan 对象句柄数组
- `objectCount`：数组中的对象数量

最后，`pUserData` 参数包含了一个在设置回调函数时指定的指针，允许你传入自己的数据。

回调函数返回一个布尔值指示当校验层消息被 Vulkan 函数调用触发时是否应该退出程序。如果回调函数返回了真值，这个调用就会中止，并返回
`VK_ERROR_VALIDATION_FAILED_EXT` 错误代码。这通常只用于测试校验层本身，因此你应该始终返回 `VK_FALSE`。

现在只剩下告知 Vulkan 有关这个回调函数的信息。说起来或许会有些令人惊讶，就连 Vulkan 中的调试回调函数也由一个需要显式创建和销毁的句柄来管理。
这种回调函数是*调试信使*的一部分，并且你可以根据需要想设置多少个就设置多少个。在 `instance` 下方添加一个类成员来保存这个句柄：

```c++
VkDebugUtilsMessengerEXT debugMessenger;
```

现在添加一个 `setupDebugMessenger` 函数，然后在 `initVulkan` 中的 `createInstance` 之后调用：

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
}

void setupDebugMessenger() {
    if (!enableValidationLayers) return;

}
```

我们需要用这个信使极其回调函数的详细信息来填充一个结构体：

```c++
VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
createInfo.pfnUserCallback = debugCallback;
createInfo.pUserData = nullptr; // 可选的
```

`messageSeverity` 字段允许你指定你的回调函数在何种严重等级下被触发。我在此指定了除 `VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT`
以外的所有等级来接收所有可能的错误信息，并忽略更详细的一般调试信息。

类似地，`messageType` 字段允许你过滤回调函数的消息类型。我在这里简单地开启了所有类型，你可以关闭那些对你来说没什么用的。

最后，`pfnUserCallback` 指定了回调函数的指针。你可以给 `pUserData` 传递一个指针，这个指针会通过 `pUserData` 参数传递到回调函数中。比如
你可以用它来传递 `HelloTriangleApplication` 类的指针。

注意，配置校验层消息和调试回调函数还有很多不同的方法，不过这里给出的是一个很适合入门的方法。关于其它方法，参阅这份
[扩展规范](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VK_EXT_debug_utils)以获取更多信息。

这个结构体应该被传递到 `vkCreateDebugUtilsMessengerEXT` 函数中来创建 `VkDebugUtilsMessengerEXT` 对象。不幸的是，因为这个函数是一个扩
展函数，所以它不会被自动加载。我们必须自己用 `vkGetInstanceProcAddr` 函数来查找它的地址。我们要创建一个我们自己的钩子函数，帮助我们在幕后完
成这一切。我在 `HelloTriangleApplication` 类定义之前添加了这个函数：

```c++
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
```

如果这个函数没有被加载，`vkGetInstanceProcAddr` 函数则返回 `nullptr`。现在，如果该函数可用，我们就可以调用这个函数来创建这个扩展对象了：

```c++
if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
}
```

倒数第二个参数依然是那个被我们设置成 `nullptr` 的可选的分配器回调函数，其余的参数含义都很明了。由于调试信使与我们的特定的 Vulkan 实例极其验
证层相关联，实例需要被显式设置为第一个参数。一会你还会看到这种模式用在其它*子*对象上。

`VkDebugUtilsMessengerEXT` 对象还需要使用 `vkDestroyDebugUtilsMessengerEXT` 函数来清除。与 `vkCreateDebugUtilsMessengerEXT`
类似，这个函数需要被显式加载。

在 `CreateDebugUtilsMessengerEXT` 下面添加另外一个钩子函数：

```c++
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
```

确保这个函数是一个静态成员函数，或者是一个在类外面的函数。然后我们可以在 `cleanup` 函数中调用它：

```c++
void cleanup() {
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}
```

## 调试实例的创建和销毁

尽管我们已经用校验层增加了调试功能，但我们仍然没有覆盖所有内容。调用 `vkCreateDebugUtilsMessengerEXT` 的前提是成功创建一个合法的实例，而
`vkDestroyDebugUtilsMessengerEXT` 必须在实例被销毁前调用。这使我们目前无法调试调用 `vkCreateInstance` 和 `vkDestroyInstance` 过程
中的问题。

然而，若你仔细阅读了[扩展文档](https://github.com/KhronosGroup/Vulkan-Docs/blob/main/appendices/VK_EXT_debug_utils.adoc#examples)，
你将发现，有一种方法可以专门为这两个函数调用创建调试组件信使。只需要简单地将 `VkInstanceCreateInfo` 的 `pNext` 成员设置为一个指向
`VkDebugUtilsMessengerCreateInfoEXT` 结构体的指针即可。首先将填充信使的创建信息的过程提取到一个单独的函数中：

```c++
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

...

void setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}
```

现在我们可以在 `createInstance` 函数中重用这段代码：

```c++
void createInstance() {
    ...

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    ...

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}
```

变量 `debugCreateInfo` 放在了 if 语句外面来确保它不会在 `vkCreateInstance` 调用被销毁。用这种方法创建的额外的调试信使会自动在
`vkCreateInstance` 和 `vkDestroyInstance` 中被使用，并在之后被清理。

## 测试

现在，让我们故意犯一个错误，看看校验层是如何工作的。暂时移除 `cleanup` 函数中调用 `DestroyDebugUtilsMessengerEXT` 的代码并运行你的程序。
当程序退出后你应该看到类似下图的输出：

![](/images/validation_layer_test.png)

> 如果你没有看到任何消息，[检查你的安装配置](https://vulkan.lunarg.com/doc/view/1.2.131.1/windows/getting_started.html#user-content-verify-the-installation)。

如果你想看到哪个调用触发了消息，你可以在消息回调函数里打一个断点，然后看看堆栈跟踪。

## 配置

除了在 `VkDebugUtilsMessengerCreateInfoEXT` 结构体中指定标志之外，还有很多设置校验层行为的方法，浏览 Vulkan SDK 中的 `Config` 目录，
`vk_layer_settings.txt` 文件解释了如何设置这些校验层。

要为你的应用程序设置校验层，把这个文件复制到你工程的 `Debug` 和 `Release` 文件夹里然后照着上面的说明来设置你想要的行为。然而，在此教程的余下
部分，我假设你用的是默认设置。

在此教程中，我会故意犯几个错误来让你看看校验层对于捕获这些错误有多大的帮助，并且告诉你清楚地知道你在用 Vulkan 做什么有多重要。现在是时候看看
[系统中的 Vulkan 设备](!zh/Drawing_a_triangle/Setup/Physical_devices_and_queue_families)了。

[C++ 代码](/code/02_validation_layers.cpp)
