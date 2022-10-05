## 创建实例-instance

您需要做的第一件事是通过创建 *instance* 来初始化 Vulkan 库。实例是您的应用程序和 Vulkan 库之间的连接，创建它涉及向驱动程序指定有关您的应用程序的一些详细信息。

首先添加一个`createInstance`函数并在`initVulkan`函数中调用。

```c++
void initVulkan() {
    createInstance();
}
```

然后，添加一个数据成员来保存实例的句柄：

```c++
private:
VkInstance instance;
```

现在，在创建实例之前，我们首先需要在一个结构体中填写一些关于我们的应用程序的信息。此数据在技术上是可选的，但它可能会为驱动程序提供一些有用的信息，以优化我们的特定应用程序（例如，因为它使用具有某些特殊行为的知名图形引擎）。 这个结构叫做 `VkApplicationInfo`：

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

如前所述，Vulkan中的许多结构都要求您在`sType`成员中显式指定类型。和很多其他结构体一样，该结构体也有成员变量`pNext`，该变量可以指向未来的扩展信息。这里我们使用该值默认值`nullptr`，未对其进行更改。

Vulkan中的许多信息是通过结构而不是函数参数传递的。这里我们还需要再填写一个结构体来为创建实例提供足够的信息。这一个结构是必须填写的，它告诉Vulkan 驱动程序我们要使用哪些全局扩展和验证层。这里的全局意味着这些属性适用于整个程序环境而不是指定的设备属性，这些概念在接下来的几章中将变得更加清晰。

```c++
VkInstanceCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
createInfo.pApplicationInfo = &appInfo;
```

前两个参数意思很明晰那。接下来的参数指定所需的全局扩展。正如概述章节中提到的，Vulkan 是一个平台无关的 API，这意味着您需要一个扩展来与平台相关的窗口系统交互。GLFW 有一个方便的内置函数，它能够直接返回所需的扩展配置参数，我们可以直接使用将其传递给结构体：

```c++
uint32_t glfwExtensionCount = 0;
const char** glfwExtensions;

glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

createInfo.enabledExtensionCount = glfwExtensionCount;
createInfo.ppEnabledExtensionNames = glfwExtensions;
```

结构的最后两个成员设置确定是否要启用的全局验证层。我们将在下一章更深入地讨论这些内容，现在暂时将它们留空。

```c++
createInfo.enabledLayerCount = 0;
```

现在我们已经指定了Vulkan实例创建所需的一切，我们终于可以调用`vkCreateInstance`了：

```c++
VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
```

正如您将看到的，创建对象的函数参数的一般模式如下：

* 一个指向创建信息结构体的指针
* 一个指向自定义分配器回调的指针，在本教程中始终为 `nullptr`
* 一个指向存储新对象变量的句柄指针

如果一切顺利，那么实例的句柄就存储在
类型为`VkInstance`的类成员变量。几乎所有 Vulkan 函数都返回一个类型的值
`VkResult` 是 `VK_SUCCESS` 或错误代码。可以使用该变量检查是否
实例创建成功，当实例创建失败时，我们不需要存储结果：

```c++
if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
}
```

现在运行程序可验证成功创建实例。

## 扩展支持检查

如果您查看`vkCreateInstance`文档，您会看到可能的错误代码之一是`VK_ERROR_EXTENSION_NOT_PRESENT`。该错误代码表示设备不支持我们
指定的扩展属性。这对于指定属性创建窗口系统界面是有意义的，但是我们如何才能检查设备是否支持扩展属性呢？

在创建Vulkan实例前，可以使用`vkEnumerateInstanceExtensionProperties`函数获取设备支持的扩展属性列表。该函数需要一个整形变量作为参数返回存储可支持扩展属性的数量，还需要一个队列指针存储可扩展属性列表数据。该函数的第一个参数是一个可选参数，设置该参数可以允许我们过滤指定验证层的扩展信息，这里我们不使用该参数。

我们需要知道设备支持的扩展属性的数量才能分配合适的内存大小存储属性列表信息。我们可以设置保存属性列表的指针为空来获取扩展属性的数量。

```c++
uint32_t extensionCount = 0;
vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
```

现在可以创建队列(`include <vector>`)，分配合适的内存大小属性列表保存数据了:

```c++
std::vector<VkExtensionProperties> extensions(extensionCount);
```

最后，再次调用函数，我们可以查询获取设备扩展属性列表：

```c++
vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
```

每个`VkExtensionProperties`结构包含扩展的名称和版本。我们可以用一个简单的for循环列出它们（`\t`是缩进的制表符）：

```c++
std::cout << "available extensions:\n";

for (const auto& extension : extensions) {
    std::cout << '\t' << extension.extensionName << '\n';
}
```

可以将此代码添加到`createInstance`函数中获取Vulkan支持属性的一些详细信息。作为一个挑战，可以创建一个函数获取支持的扩展属性，并检查
`glfwGetRequiredInstanceExtensions`要求的支持是否在扩展列表中。

## 内存回收

`VkInstance`应该在程序退出之前最后被销毁。可以使用`vkDestroyInstance`函数在`cleanup`封装函数中销毁它：

```c++
void cleanup() {
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}
```

`vkDestroyInstance`函数的参数很简单。如前一章所述，Vulkan中的分配和释放函数有一个可选的分配器回调，通过传递`nullptr`我们忽略该参数的使用。 我们将在接下来的章节中创建的所有其他 Vulkan 资源都应该在实例被销毁之前进行清理。

创建实例后，在继续执行更复杂的步骤之前，是时候通过[验证层](!ch/03_绘制三角形/00_设置/02_验证层)来评估我们的调试选项了.

[C++ code](/code/01_instance_creation.cpp)
