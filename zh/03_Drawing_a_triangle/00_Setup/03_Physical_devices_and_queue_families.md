---
title: 物理设备与队列族
---

## 选择一个物理设备

通过 VkInstance 初始化了 Vulkan 的库之后，我们需要在系统中选择一个支持我们需要的功能的显卡。事实上，我们可以同时选择并使用
任意数量的显卡，但是在这个教程里，我们会专注于第一个满足我们需要的显卡。

我们会添加一个函数 `pickPhysicalDevice`，并且在 `initVulkan` 函数中使用它。

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    pickPhysicalDevice();
}

void pickPhysicalDevice() {

}
```

我们最终选择使用的显卡会被添加为一个 VkPhysicalDevice 句柄的成员变量。在 VkInstance 被销毁的时候，这个对象也会被销毁，所以
我们不需要在 cleanup 里面对它进行清理。

```c++
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
```

列出显卡的方式和列出插件的方式很相似。它也是先查询数量。

```c++
uint32_t deviceCount = 0;
vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
```

如果没有支持 Vulkan 的显卡，那么就不需要再搜寻下去了。

```c++
if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
}
```

除此之外我们可以分配一个数组来容纳所有的 VkPhysicalDevice 句柄。

```c++
std::vector<VkPhysicalDevice> devices(deviceCount);
vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
```

因为显卡之间存在不同之处，现在我们需要去对每一个显卡评估是否合适用于我们要进行的操作。为此，我们将引入一个新的函数：

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    return true;
}
```

然后调用这个函数去检查是否有显卡满足我们的要求。

```c++
for (const auto& device : devices) {
    if (isDeviceSuitable(device)) {
        physicalDevice = device;
        break;
    }
}

if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
}
```

下一部分会介绍我们在 `isDeviceSuitable` 要检查的第一个要求。在后面的章节中，我们开始使用更多 Vulkan 的功能的时候，我们也会
扩展这个函数来包含更多的检查。

## 检查设备的基础兼容性

为了评估一个设备的兼容性，我们可以从查询设备的一些细节开始。设备的基础信息比如说设备的名称，类型和支持的 Vulkan 版本可以通过 `vkGetPhysicalDeviceProperties` 查询到。

```c++
VkPhysicalDeviceProperties deviceProperties;
vkGetPhysicalDeviceProperties(device, &deviceProperties);
```

通过 `vkGetPhysicalDeviceFeatures`，可以查询是否支持一些可选的功能，比如说纹理压缩，64 位浮点型和多视口渲染（较多用于 VR）。

```c++
VkPhysicalDeviceFeatures deviceFeatures;
vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
```

更多能从设备中查到的细节，比如设备内存和队列族（参见下一部分），将在之后讨论。

例如说，如果我们的应用程序只有在支持几何着色器的显卡中可以使用，那么 `isDeviceSuitable` 函数就应该是这样：

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
           deviceFeatures.geometryShader;
}
```

除了只检查是否设备是否是第一个，你也可以给每个设备打分，然后选择最高分的那个。这样你就可以找到一个最合适的显卡。但是如果只有
集成 GPU 可以用，那就退回到集成 GPU。你可以写成类似于下面这个样子：

```c++
#include <map>

...

void pickPhysicalDevice() {
    ...

    // 使用一个有序映射来自动将候选的设备按照分数从小到大排序
    std::multimap<int, VkPhysicalDevice> candidates;

    for (const auto& device : devices) {
        int score = rateDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    // 检查最佳的候选显卡是否可用
    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

int rateDeviceSuitability(VkPhysicalDevice device) {
    ...

    int score = 0;

    // 独立显卡有非常大的性能优势
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // 不影响渲染质量时能存储的最多的纹理的数量
    score += deviceProperties.limits.maxImageDimension2D;

    // 应用程序必须要求有几何着色器
    if (!deviceFeatures.geometryShader) {
        return 0;
    }

    return score;
}
```

对于这个教程来说，你不需要去完全照着这样去实现。这只是给你一个思路去设计你选择显卡的过程。当然你也可以直接显示所有显卡的名字
然后让用户去做选择。

因为我们只是入门，支持 Vulkan 就是所有我们需要的了。所以我们可以选择任意一个 GPU：

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    return true;
}
```

在下一部分中，我们会讨论我们第一个需要去检查的特性。

## 队列族 （Queue Family）

在之前我们简单提到过，在 Vulkan 中，几乎所有的操作，所有从绘制到上传纹理的部分，都需要将命令上传给一个队列。从不同的
*队列族*（queue families）中会有不同类型的队列，并且每个队列族只允许一个命令子集。比如说，可能会有一个队列族只允许处理计算
命令，或者一个队列族只允许有关于内存转储的指令。

我们需要去检查哪一个队列族是设备支持的，并且其中哪一个支持我们想要使用的指令。我们可以添加一个新函数 `findQueueFamilies` 来
查找所有我们需要的队列族。

现在我们要去查找一个支持图形指令的队列族。这个函数可能是这个样子：

```c++
uint32_t findQueueFamilies(VkPhysicalDevice device) {
    // 查找图形队列族的逻辑
}
```

但是，在后面的一个章节中我们需要去查找另外一个队列。所以为将来做准备，更好的做法是把索引存储在一个结构体中：

```c++
struct QueueFamilyIndices {
    uint32_t graphicsFamily;
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    // 查找队列族索引并填充结构体的逻辑
    return indices;
}
```

但是如果队列族不能使用怎么办？我们可以在 `findQueueFamilies` 中抛出一个异常，但是这个函数不是一个处理设备兼容性的地方。比如
说，我们可能更*倾向于*一个有专用的传输队列族的设备，而不是需要它。所以我们需要判断一个特定的队列族是否存在。

因为任何一个 `uint32_t` 都有可能是一个可用的队列族（包括 `0`），所以似乎并不能使用一个特定的数字来代表一个队列族不存在。幸运的
是，C++17 引进了一个数据结构来区分值存在或不存在的情况：

```c++
#include <optional>

...

std::optional<uint32_t> graphicsFamily;

std::cout << std::boolalpha << graphicsFamily.has_value() << std::endl; // false

graphicsFamily = 0;

std::cout << std::boolalpha << graphicsFamily.has_value() << std::endl; // true
```

`std::optional` 是一个直到你给它赋值之前不去存储任何值的封装。你可以通过调用它的成员函数 `has_value()` 来查询它是否储存着一个
值。这样我们就可以将函数改为：

```c++
#include <optional>

...

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    // 将 index 赋值为找到的队列族
    return indices;
}
```

现在我们就可以去实际地去实现 `findQueueFamilies`：

```c++
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    ...

    return indices;
}
```

和往常一样，列出队列族的过程需要使用到 `vkGetPhysicalDeviceQueueFamilyProperties`：

```c++
uint32_t queueFamilyCount = 0;
vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
```

VkQueueFamilyProperties 结构体储存了一些关于队列族的细节信息，包括支持的操作的类型，和这个队列族可以创建的队列数量。我们需要
找到至少一个支持 `VK_QUEUE_GRAPHICS_BIT` 的队列族。

```c++
int i = 0;
for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        indices.graphicsFamily = i;
    }

    i++;
}
```

现在我们有了这个查找队列族的函数，我们可以在 `isDeviceSuitable` 中使用它来确保设备可以处理我们想要使用的指令：

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    return indices.graphicsFamily.has_value();
}
```

为了便捷性，我们可以在结构体里面添加一个用来检查的函数：

```c++
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() {
        return graphicsFamily.has_value();
    }
};

...

bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    return indices.isComplete();
}
```

现在我们就可以在 `findQueueFamilies` 中使用它来提前退出循环：

```c++
for (const auto& queueFamily : queueFamilies) {
    ...

    if (indices.isComplete()) {
        break;
    }

    i++;
}
```

很好！这些就是我们目前需要的所有的查找合适物理设备的东西了！下一步就是
[创建一个逻辑设备](!zh/Drawing_a_triangle/Setup/Logical_device_and_queues)与它交互。

[C++ code](/code/03_physical_device_selection.cpp)
