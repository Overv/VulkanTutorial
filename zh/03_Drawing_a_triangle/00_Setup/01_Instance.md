---
title: 实例
---

## 创建一个实例

一切开始于创建一个*实例*（instance）来初始化 Vulkan 库。实例是连接 Vulkan 库和你的程序之间的桥梁，创建实例还涉及到向驱动指定你的应用程序的
一些细节。

添加一个 `createInstance` 函数，然后在 `initVulkan` 函数中调用它。

```c++
void initVulkan() {
    createInstance();
}
```

再在类中添加一个数据成员，用来保存实例的句柄：

```c++
private:
VkInstance instance;
```

现在，为了创建实例，我们首先需要用我们程序的一些信息去填充一个结构体。从技术上来说，这些信息是可有可无的，但是它们或许能够提供一些信息给驱动，以
使驱动针对我们的特定程序进行优化（例如，它使用了一个具有某些特殊行为的知名图形引擎）。这个结构体叫做 `VkApplicationInfo`：

```c++
void createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
}
```

就像之前提到过的那样，Vulkan 中的许多结构体需要你在 `sType` 成员中显式指定类型。这个结构体也是众多拥有 `pNext` 成员的结构体之一，这个成员在
将来可以指向扩展信息。我们现在执行默认初始化，所以此处置为 `nullptr`（空指针）。

Vulkan 中的许多信息都通过结构体来传递，而不是函数参数。我们还需要填充另一个结构体来为创建实例提供足够多的信息。接下来的这个结构体是必需的，它告
知 Vulkan 驱动我们要使用哪些全局的扩展以及验证层。“全局”意味着它们将在整个程序中生效，而不是某个特定的设备，接下来的几章里我们会说明这个问题。

```c++
VkInstanceCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
createInfo.pApplicationInfo = &appInfo;
```

前两个参数的意思非常明显。接下来的两个成员会指定我们想用的全局扩展。就像我们在概述那章里提到过的，Vulkan 是一套平台无关的 API，这意味着你需要
一个扩展与窗口系统（window system）来交互。GLFW 已经集成了一个好用的内置函数，它返回 GLFW 需要的 Vulkan 扩展，我们可以直接把它传给
Vulkan API：

```c++
uint32_t glfwExtensionCount = 0;
const char** glfwExtensions;

glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

createInfo.enabledExtensionCount = glfwExtensionCount;
createInfo.ppEnabledExtensionNames = glfwExtensions;
```

最后两个成员指定哪些全局验证层将会被启用。我们会在下一章深入讨论验证层，这里先暂时留空。

```c++
createInfo.enabledLayerCount = 0;
```

我们已经指定了初始化 Vulkan 实例需要的所有信息，现在终于可以调用 `vkCreateInstance` 函数了：

```c++
VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
```

如你所见，Vulkan 中创建对象的函数，其参数通常是这样的：

* 指向创建信息（creation info）的指针
* 指向自定义分配器回调函数的指针，此教程中永远被置为 `nullptr`
* 指向保存了要被创建的对象的句柄变量的指针

如果一切运行良好，那么创建好的实例的句柄就被保存在 `VkInstance` 类型的成员变量中了。几乎每一个 Vulkan 函数的返回值都是 `VkResult` 类型的，
它要么是 `VK_SUCCESS`，要么是一个错误代码。如果要检查实例是否被成功创建，我们不需要保存这个返回结果，只需要检查一下返回值就行了：

```c++
if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
}
```

现在运行这个程序以确定实例创建成功。

## 遭遇 VK_ERROR_INCOMPATIBLE_DRIVER

如果在 MacOS 上使用最新的 MoltenVK SDK，你可能从 `vkCreateInstance` 得到 `VK_ERROR_INCOMPATIBLE_DRIVER` 返回值。根据
[Vulkan SDK 的入门指南](https://vulkan.lunarg.com/doc/sdk/1.3.216.0/mac/getting_started.html)，从 1.3.216 版本开始，
`VK_KHR_PORTABILITY_subset` 扩展必须被启用。

为了解决这个错误，首先添加 `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR` 标志位到 `VkInstanceCreateInfo` 结构体的 `flags`
成员，然后添加 `VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME` 到实例的启用扩展列表。

典型的代码应该像这样：

```c++
...

std::vector<const char*> requiredExtensions;

for(uint32_t i = 0; i < glfwExtensionCount; i++) {
    requiredExtensions.emplace_back(glfwExtensions[i]);
}

requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

createInfo.enabledExtensionCount = (uint32_t) requiredExtensions.size();
createInfo.ppEnabledExtensionNames = requiredExtensions.data();

if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
}
```

## 检查插件是否受支持

如果你看过 `vkCreateInstance` 的文档，你就会看到有一个错误代码是 `VK_ERROR_EXTENSION_NOT_PRESENT`。我们可以简单地指定我们想用的扩展，
如果返回了这个错误码就直接终止程序。如果要检查那些必要的扩展，例如窗口系统接口（window system interface, WSI），这么做还有点道理，但如果我
们要检查那些可选的功能呢？

为了在创建实例之前得到所有受支持的扩展列表，可以用 `vkEnumerateInstanceExtensionProperties` 函数。它需要两个指针变量，一个指向受支持的扩
展数量，另一个指向一个 `VkExtensionProperties` 类型的、存储着扩展的细节的数组。它的第一个参数是可选的，允许我们使用一个特殊的验证层来选择扩
展，我们现在先忽略它。

为了分配那个存储着扩展的细节的数组，我们需要先知道扩展的数量。你可以通过把最后一个参数留空的方式来只请求扩展的数量：

```c++
uint32_t extensionCount = 0;
vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
```

现在来分配一个数组，保存扩展的细节（引入头文件 `#include <vector>`）：

```c++
std::vector<VkExtensionProperties> extensions(extensionCount);
```

最后我们就可以查询扩展的细节了：

```c++
vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
```

每个 `VkExtensionProperties` 结构体都包含着扩展的名称和版本。我们可以通过一个简单的循环来输出它们（`\t` 是一个制表符，用来缩进）：

```c++
std::cout << "available extensions:\n";

for (const auto& extension : extensions) {
    std::cout << '\t' << extension.extensionName << '\n';
}
```

如果你想输出 Vulkan 支持的详细信息，你可以把这段代码加到 `createInstance` 函数里。留一个课后练习，尝试创建一个函数，检查
`glfwGetRequiredInstanceExtensions` 函数返回的所有扩展是不是都在受支持的扩展列表里。

## 清理

`VkInstance` 只应该在程序退出之前被销毁。可以在 `cleanup` 函数中用 `vkDestroyInstance` 函数销毁它：

```c++
void cleanup() {
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}
```

`vkDestroyInstance` 函数的参数非常直接了当，就像在上一章里提过的那样，Vulkan 中的分配器和回收器都有一个可选的回调函数参数，这个参数被我们设
置为 `nullptr` 以忽略它。在随后的章节中，我们创建的所有其它 Vulkan 资源都会在实例被销毁之前回收。

在创建了实例之后、开始进行更复杂的步骤之前，是时候看看我们的[验证层](!zh/Drawing_a_triangle/Setup/Validation_layers)来评估调试选项了。

[C++ 代码](https://vulkan-tutorial.com/code/01_instance_creation.cpp)
