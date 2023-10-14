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

다음으로는 우리가 사용할 장치의 기능을 명시해야 합니다. 이는 이전 챕터의 지오메트리 셰이더를 `vkGetPhysicalDeviceFeatures`로 질의했던 것과 비슷합니다. 지금은 특별헌 기능이 필요 없으니 그냥 정의만 해 두고 모든 값을 `VK_FALSE`로 둡시다. 나중에 Vulkan을 사용해 좀 더 흥미로운 것들을 할 때 다시 이 구조체를 사용할 것입니다.

```c++
VkPhysicalDeviceFeatures deviceFeatures{};
```

## 논리적 장치 생성하기

이전 두 개의 구조체가 준비되었으니 `VkDeviceCreateInfo` 구조체를 채워 봅시다.

```c++
VkDeviceCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
```

먼저 큐 생성 정보와 장치 기능 구조체에 대한 포인터를 추가합니다.

```c++
createInfo.pQueueCreateInfos = &queueCreateInfo;
createInfo.queueCreateInfoCount = 1;

createInfo.pEnabledFeatures = &deviceFeatures;
```

나머지 정보는 `VkInstanceCreateInfo` 구조체와 비슷해서 확장이나 검증 레이어를 명시할 수 있습니다. 차이점은 이것들이 이번에는 장치에 종속적(device specific)이라는 것입니다.

장치에 종속적인 확장 중 하나의 예시로는 `VK_KHR_swapchain`가 있는데, 렌더링된 이미지를 장치로부터 윈도우로 전달하는 기능입니다. 시스템의 Vulkan 장치가 이 기능을 지원하지 않을 수 있습니다. 예를 들어 계산 명령만 수행하는 장치일 경우에 그렇습니다. 이 확장에 대한 설명은 나중에 스왑 체인 챕터에서 다시 살펴볼 것입니다.

Vulkan의 이전 구현에서는 인스턴스와 장치 종속적인 검증 레이어가 구분되어 있었으나, [지금은 아닙니다](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap40.html#extendingvulkan-layers-devicelayerdeprecation). 즉, `VkDeviceCreateInfo`의 `enabledLayerCount` 와 `ppEnabledLayerNames` 필드가 최신 구현에서는 무시됩니다. 하지만, 이전 버전과의 호환성을 위해 어쨌든 설정해 주는 것이 좋습니다.

```c++
createInfo.enabledExtensionCount = 0;

if (enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
} else {
    createInfo.enabledLayerCount = 0;
}
```

지금은 장치 종속적인 확장은 필요하지 않습니다.

이제 `vkCreateDevice` 함수를 사용해 논리적 장치를 생성할 준비가 되었습니다.

```c++
if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
}
```

매개변수들은 상호작용할 물리적 장치, 큐와 방금 명시한 사용 정보, 선택적으로 명시할 수 있는 콜백에 대한 포인터, 마지막으로 논리적 장치를 저장할 핸들에 대한 포인터입니다. 인스턴스 생성 함수와 유사하게 이 호출은 존재하지 않는 확장을 활성화 한다거나, 지원하지 않는 기능을 명시하는 경우 오류를 반환합니다.

장치는 `cleanup`에서 `vkDestroyDevice`함수를 통해 소멸되어야 합니다.

```c++
void cleanup() {
    vkDestroyDevice(device, nullptr);
    ...
}
```

논리적 장치는 인스턴스와 직접적으로 상호작용하지 않으므로 매개변수에 포함되지 않습니다.

## 큐 핸들 얻기(Retrieving)

큐는 논리적 장치와 함께 생성되지만 아직 이들과 상호작용하기 위한 핸들은 얻지 못했습니다. 먼저 그래픽스 큐에 대한 핸들을 클래스 멤버에 추가해 줍시다.

```c++
VkQueue graphicsQueue;
```

장치 큐는 장치가 소멸될 때 자동으로 정리되므로 `cleanup`에서 해 주어야 할 일은 따로 없습니다.

`vkGetDeviceQueue`함수를 사용해 각 큐 패밀리에 대한 핸들을 얻어올 수 있습니다. 매개변수는 논리적 장치, 큐 패밀리, 큐 인덱스, 큐 핸들을 저장할 변수의 포인터 입니다. 이 패밀리에서 하나의 큐만 생성하고 있으므로 인덱스는 간단히 `0`으로 설정하면 됩니다.

```c++
vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
```

논리적 장치와 큐의 핸들이 확보 되었으니 이제 실제로 그래픽 카드를 사용해 무언가를 할 수 있습니다! 다음 몇 개 챕터에서는 윈도우 시스템에 결과를 표시하기 위한 리소스들을 설정해 보겠습니다.

[C++ code](/code/04_logical_device.cpp)
