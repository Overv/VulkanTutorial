## 개요

사용할 물리적 장치를 선택한 뒤에는 이와 상호작용할 *논리적 장치*를 설정해야 합니다. 논리적 장치의 생성 과정은 인스턴스 생성 과정과 비슷하고 우리가 사용할 기능들을 기술해야 합니다. 또한 이제는 어떤 큐 패밀리가 가용한지를 알아냈기 때문에 어떤 큐를 생성할지도 명시해야 합니다. 만일 요구사항이 다양하다면, 하나의 물리적 장치로부터 여러 개의 논리적 장치를 만들 수도 있습니다.

논리적 장치에 대한 핸들을 저장한 멤버를 클래스에 생성하는 것부터 시작합니다.

```c++
VkDevice device;
```

다음으로 `initVulkan`에서 호출할 `createLogicalDevice` 함수를 추가합니다.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    pickPhysicalDevice();
    createLogicalDevice();
}

void createLogicalDevice() {

}
```

## 생성할 큐 명시하기

논리적 장치를 생성하는 것은 이전처럼 여러 세부사항을 구조체에 명시하는 과정을 포함하며, 그 첫번째가 `VkDeviceQueueCreateInfo`입니다. 이 구조체는 하나의 큐 패밀리에 대한 큐의 개수를 명시합니다. 현재 우리는 그래픽스 기능 관련 큐에만 관심이 있습니다.

```c++
QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

VkDeviceQueueCreateInfo queueCreateInfo{};
queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
queueCreateInfo.queueCount = 1;
```

현재의 드라이버들은 큐 패밀리 하나당 적은 수의 큐만을 생성할 수 있도록 제한되어 있고, 여러분도 하나 이상 필요하지는 않을겁니다. 왜냐하면 여러 쓰레드(thread)에 필요한 커맨드 버퍼들을 모두 생성해 두고 메인 쓰레드에서 적은 오버헤드의 호출로 이들을 한꺼번에 제출(submit)할 수 있기 떄문입니다.

Vulkan에서는 커맨드 버퍼의 실행 스케줄에 영향을 주는 큐의 우선순위를 `0.0`과 `1.0` 사이의 부동소수점 수로 명시할 수 있게 되어 있습니다. 큐가 하나밖에 없더라도 이를 명시해 주어야만 합니다:

```c++
float queuePriority = 1.0f;
queueCreateInfo.pQueuePriorities = &queuePriority;
```

## 사용할 장치 기능 명시하기


The next information to specify is the set of device features that we'll be
using. These are the features that we queried support for with
`vkGetPhysicalDeviceFeatures` in the previous chapter, like geometry shaders.
Right now we don't need anything special, so we can simply define it and leave
everything to `VK_FALSE`. We'll come back to this structure once we're about to
start doing more interesting things with Vulkan.

```c++
VkPhysicalDeviceFeatures deviceFeatures{};
```

## Creating the logical device

With the previous two structures in place, we can start filling in the main
`VkDeviceCreateInfo` structure.

```c++
VkDeviceCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
```

First add pointers to the queue creation info and device features structs:

```c++
createInfo.pQueueCreateInfos = &queueCreateInfo;
createInfo.queueCreateInfoCount = 1;

createInfo.pEnabledFeatures = &deviceFeatures;
```

The remainder of the information bears a resemblance to the
`VkInstanceCreateInfo` struct and requires you to specify extensions and
validation layers. The difference is that these are device specific this time.

An example of a device specific extension is `VK_KHR_swapchain`, which allows
you to present rendered images from that device to windows. It is possible that
there are Vulkan devices in the system that lack this ability, for example
because they only support compute operations. We will come back to this
extension in the swap chain chapter.

Previous implementations of Vulkan made a distinction between instance and device specific validation layers, but this is [no longer the case](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap40.html#extendingvulkan-layers-devicelayerdeprecation). That means that the `enabledLayerCount` and `ppEnabledLayerNames` fields of `VkDeviceCreateInfo` are ignored by up-to-date implementations. However, it is still a good idea to set them anyway to be compatible with older implementations:

```c++
createInfo.enabledExtensionCount = 0;

if (enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
} else {
    createInfo.enabledLayerCount = 0;
}
```

We won't need any device specific extensions for now.

That's it, we're now ready to instantiate the logical device with a call to the
appropriately named `vkCreateDevice` function.

```c++
if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
}
```

The parameters are the physical device to interface with, the queue and usage
info we just specified, the optional allocation callbacks pointer and a pointer
to a variable to store the logical device handle in. Similarly to the instance
creation function, this call can return errors based on enabling non-existent
extensions or specifying the desired usage of unsupported features.

The device should be destroyed in `cleanup` with the `vkDestroyDevice` function:

```c++
void cleanup() {
    vkDestroyDevice(device, nullptr);
    ...
}
```

Logical devices don't interact directly with instances, which is why it's not
included as a parameter.

## Retrieving queue handles

The queues are automatically created along with the logical device, but we don't
have a handle to interface with them yet. First add a class member to store a
handle to the graphics queue:

```c++
VkQueue graphicsQueue;
```

Device queues are implicitly cleaned up when the device is destroyed, so we
don't need to do anything in `cleanup`.

We can use the `vkGetDeviceQueue` function to retrieve queue handles for each
queue family. The parameters are the logical device, queue family, queue index
and a pointer to the variable to store the queue handle in. Because we're only
creating a single queue from this family, we'll simply use index `0`.

```c++
vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
```

With the logical device and queue handles we can now actually start using the
graphics card to do things! In the next few chapters we'll set up the resources
to present results to the window system.

[C++ code](/code/04_logical_device.cpp)
