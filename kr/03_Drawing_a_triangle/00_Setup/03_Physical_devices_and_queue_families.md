## 물리적 장치 선택

VkInstance를 통해 Vulkan 라이브러리를 초기화 한 이후에는 우리가 필요로 하는 기능을 지원하는 시스템의 그래픽 카드를 찾고 선택해야 합니다. 여러 대의 그래픽 카드를 선택하고 동시에 사용할 수도 있습니다. 하지만 이 튜토리얼에서는 우리의 요구에 맞는 첫 번째 그래픽 카드만을 사용하도록 할 것입니다.

`pickPhysicalDevice` 함수를 추가하고 `initVulkan` 함수에서 이 함수를 호출하도록 합시다.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    pickPhysicalDevice();
}

void pickPhysicalDevice() {

}
```

우리가 선택할 그래픽 카드는 새롭게 클래스 멤버로 추가된 VkPhysicalDevice 핸들에 저장됩니다. 이 객체는 VkInstance가 소멸될 때 암시적(implicitly)으로 소멸되므로, `cleanup`에 무언가를 추가할 필요는 없습니다.

```c++
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
```

그래픽 카드의 목록을 불러오는 것은 확장의 목록을 불러오는 것과 비슷하며 그 개수를 질의(query)하는 것으로 시작됩니다.

```c++
uint32_t deviceCount = 0;
vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
```

Vulkan을 지원하는 장치가 없으면 더 진행할 이유가 없겠죠.

```c++
if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
}
```

그렇지 않으면 모든 VkPhysicalDevice 핸들을 저장할 배열을 할당합니다.

```c++
std::vector<VkPhysicalDevice> devices(deviceCount);
vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
```

이제 각 장치를 순회하면서 우리가 하고자 하는 작업에 적합한지 확인합니다. 모든 그래픽 카드가 같지는 않기 때문입니다. 이를 위해 아래와 같은 새 함수를 만듭니다:

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    return true;
}
```

그리고 어떤 물리적 장치든 요구사항에 맞는 것이 있는지를 확인합니다.

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

다음 섹션에서는 우리가 `isDeviceSuitable`에서 확인할 첫 번째 요구사항을 소개할 것입니다. 이후 챕터에서 보다 많은 Vulkan 기능을 사용할 것이기 때문에 보다 많은 요구사항을 확인하도록 확장해 나갈 것입니다.

## 기본 장치 적합성(suitability) 확인

장치의 적합성을 확인하기 위해 몇 가지 세부사항을 질의할 것입니다. 장치의 기본적인 속성인 이름, 타입, 지원하는 Vulkan 버전 등은 vkGetPhysicalDeviceProperties를 사용해 질의할 수 있습니다.

```c++
VkPhysicalDeviceProperties deviceProperties;
vkGetPhysicalDeviceProperties(device, &deviceProperties);
```

텍스처 압축, 64비트 float, 다중 뷰포트 렌더링(VR에서 유용합니다) 등과 같은 추가적인 기능을 지원하는지 여부는 vkGetPhysicalDeviceFeatures를 사용해 질의할 수 있습니다.

```c++
VkPhysicalDeviceFeatures deviceFeatures;
vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
```

장치 메모리라던가, 큐 패밀리(queue family)와 같은 더 세부적인 사항에 대한 질의도 가능하며, 이에 대해서는 이후에 논의할 것입니다(다음 섹션 참고).

예를 들어, 우리 응용 프로그램이 지오메트리(geometry) 셰이더를 지원하는 장치에서만 사용할 수 있도록 하고 싶습니다. 그러면 `isDeviceSuitable` 함수는 아래와 같이 구현할 수 있습니다.

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

장치가 적합한지 아닌지만 체크해서 첫 번째 장치를 선택하는 대신, 각 장치에 점수를 부여하고 가장 높은 점수의 장치를 선택하게 할 수도 있습니다. 이렇게 하면 적합한 장치에 더 많은 점수를 부여할 수 있지만 그러한 경우 적합한 장치가 내장 그래픽(integrated GPU) 카드일 경우 그 장치가 선택될 수도 있습니다. 이러한 방식은 다음과 같이 구현할 수 있습니다.

```c++
#include <map>

...

void pickPhysicalDevice() {
    ...

    // Use an ordered map to automatically sort candidates by increasing score
    std::multimap<int, VkPhysicalDevice> candidates;

    for (const auto& device : devices) {
        int score = rateDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    // Check if the best candidate is suitable at all
    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

int rateDeviceSuitability(VkPhysicalDevice device) {
    ...

    int score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    // Application can't function without geometry shaders
    if (!deviceFeatures.geometryShader) {
        return 0;
    }

    return score;
}
```

이 튜토리얼에서 이런 모든 기능을 구현할 필요는 없지만 여러분의 장치 선택 과정을 어떻게 설계할 수 있을지에 대한 아이디어는 얻게 되셨을겁니다. 물론 후보 장치들의 이름을 보여주고 유저가 선택하도록 할 수도 있습니다.

지금은 시작하는 단계이므로 Vulkan 지원 여부만 있으면 되고 그러니 그냥 아무 GPU나 선택하도록 하겠습니다.

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    return true;
}
```

다음 섹션에서는 첫 번째 실제 필요로 하는 기능을 체크해 보겠습니다.

## 큐 패밀리(Queue families)

그리기부터 텍스처 업로드까지 거의 대부분의 Vulkan 명령 실행 전에, 명령(command)들이 큐에 제출되어야만 합니다. 다양한 종류의 *큐 패밀리*로부터 도출된 다양한 종류의 큐가 있으며, 각 큐 패밀리는 처리할 수 있는 명령이 제한되어 있습니다. 예를 들어 계산(compute) 명령만을 처리할 수 있는 큐 패밀리가 있고, 메모리 전송 관련 명령만을 처리할 수 있는 큐 패밀리도 있습니다.

장치가 어떤 큐 패밀리를 지원하는지와 이들 중 어떤 것이 우리가 사용하고자 하는 명령을 지원하는지를 체크해야만 합니다. 이를 위해 `findQueueFamilies` 함수를 추가하고 우리가 필요로하는 큐 패밀리들을 찾도록 해 봅시다.

지금은 그래픽스 명령을 지원하는 큐만 확인할 것이므로 함수는 아래와 같습니다:

```c++
uint32_t findQueueFamilies(VkPhysicalDevice device) {
    // Logic to find graphics queue family
}
```

하지만, 이후 챕터부터 또다른 큐가 필요하기 때문에 이를 대비해 인덱스를 구조체로 만드는 것이 낫습니다:

```c++
struct QueueFamilyIndices {
    uint32_t graphicsFamily;
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    // Logic to find queue family indices to populate struct with
    return indices;
}
```

큐 패밀리를 지원하지 않으면 어떻게 될까요? `findQueueFamilies`에서 예외를 throw할 수도 있지만, 이 함수는 장치 적합성을 확인하기 위한 목적으로는 적합하지 않습니다. 예를 들어 전송(transfer) 큐 패밀리가 있는 장치를 *선호*하긴 하지만 필수 요구사항은 아닐수도 있습니다. 따라서 특정한 큐 패밀리가 있는지 알려주는 방법이 필요합니다.

큐 패밀리가 존재하지 않는것에 대한 마법같은 인덱스를 사용하는 방법은 없습니다. `0`을 포함해서 모든 `uint32_t` 값이 사실상 유효한 큐 패밀리의 인덱스일 수 있기 때문입니다. 다행히 C++17에서는 값이 존재하는지 아닌지를 구분할 수 있는 자료 구조를 지원합니다.

```c++
#include <optional>

...

std::optional<uint32_t> graphicsFamily;

std::cout << std::boolalpha << graphicsFamily.has_value() << std::endl; // false

graphicsFamily = 0;

std::cout << std::boolalpha << graphicsFamily.has_value() << std::endl; // true
```

`std::optional`은 무언가 값을 할당하기 전에는 값이 없는 상태를 나타낼 수 있는 래퍼(wrapper)입니다. 어느 시점에 여러분은 그 안에 값이 있는지 없는지를 `has_value()` 멤버 함수를 통해 확인할 수 있습니다. 따라서 로직을 아래와 같이 바꿀 수 있습니다:

```c++
#include <optional>

...

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    // Assign index to queue families that could be found
    return indices;
}
```

이제 실제로 `findQueueFamilies`를 구현할 수 있습니다:

```c++
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    ...

    return indices;
}
```

큐 패밀리의 목록을 가져오는 과정은 예상하실 수 있듯이 `vkGetPhysicalDeviceQueueFamilyProperties`를 사용하는 것입니다.

```c++
uint32_t queueFamilyCount = 0;
vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
```

VkQueueFamilyProperties 구조체는 지원하는 연산의 종류와 해당 패밀리로부터 생성될 수 있는 큐의 개수 등의 큐 패밀리 세부 사항을 포함하고 있습니다. 우리는 `VK_QUEUE_GRAPHICS_BIT`을 지원하는 최소한 하나의 큐 패밀리를 찾아야만 합니다.

```c++
int i = 0;
for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        indices.graphicsFamily = i;
    }

    i++;
}
```

이제 큐 패밀리 룩업(lookup) 함수가 있으니 `isDeviceSuitable` 함수에서 이를 사용해 장치가 우리가 사용하고자 하는 명령을 처리할 수 있는지 확인합니다:

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    return indices.graphicsFamily.has_value();
}
```

좀 더 편리하게 사용하기 위해, 구조체 안에도 확인 기능을 추가합니다:

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

`findQueueFamilies` 에서 빠른 종료를 위해서도 사용합니다:

```c++
for (const auto& queueFamily : queueFamilies) {
    ...

    if (indices.isComplete()) {
        break;
    }

    i++;
}
```

좋습니다. 우선은 적절한 물리적 장치를 찾는 것은 이것으로 끝입니다! 다음 단계는 [논리적 장치](!kr/Drawing_a_triangle/Setup/Logical_device_and_queues)와의 인터페이스를 만드는 것입니다.

[C++ code](/code/03_physical_device_selection.cpp)
