Vulkan은 "기본 프레임버퍼(default framebuffer)"의 개념이 없습니다. 따라서 렌더링을 수행할 버퍼를 소유한 하부 구조(infrastructure)를 만들어서 화면에 그려지게 해야 합니다. 이 하부 구조는 *스왑 체인(swap chain)*이라고 하며 Vulkan의 경우 명시적으로 생성되어야 합니다. 스왑 체인은 기본적으로 화면에 표시되길 기다리는 이미지들의 큐입니다. 우리 응용 프로그램에서는 이러한 이미지를 만들고 큐에 반환할 것입니다. 큐가 어떻게 동작하고 어떤 조건에서 큐의 이미지가 표시될 것인지와 같은 사항들은 스왑 체인의 설정에 따라 달라집니다. 하지만 일반적으로 스왑 체인의 역할은 화면의 주사율(refresh rate)과 이미지의 표시를 동기화(synchronize)하는 것입니다.

## 스왑 체인 지원 확인하기

모든 그래픽 카드가 이미지를 곧바로 화면에 표시하는 기능을 지원하는 것은 아닙니다. 예를 들어 서버를 위해 설계된 그래픽 카드는 디스플레이 출력이 없을 수 있습니다. 또한, 이미지의 표현은 윈도우 시스템, 그 윈도우와 연관된 표면(surface)와 밀접하게 관련되어 있기 때문에 Vulkan 코어(core)에는 포함되어 있지 않습니다. 지원하는지를 확인한 후에 `VK_KHR_swapchain` 장치 확장을 활성화시켜줘야만 합니다.

이러한 목적으로 우리는 먼저 `isDeviceSuitable` 함수를 수정해 이러한 확장을 지원하는지 확인할 것입니다. `VkPhysicalDevice`를 사용해 지원하는 확장의 목록을 얻는 법을 이미 봤기 때문에 어렵지 않을 겁니다. Vulkan 헤더 파일은 `VK_KHR_swapchain`로 정의된 `VK_KHR_SWAPCHAIN_EXTENSION_NAME` 매크로를 지원합니다. 매크로를 사용하면 컴파일러가 타이핑 오류를 탐지할 수 있습니다.

먼저 필요로 하는 장치 확장의 목록을 정의합니다. 이는 검증 레이어 목록을 얻은 것과 유사합니다.

```c++
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
```

다음으로 `checkDeviceExtensionSupport` 함수를 새로 만듭니다. 이는 `isDeviceSuitable`에서 추가적인 체크를 위해 호출됩니다:

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

함수의 본문을 수정해 확장들을 열거하고 요구되는 확장들이 있는지를 확인합니다.

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

요구사항에 있지만 확인되지 않은 확장들을 표현하기 위해 문자열(string) 집합(set)을 사용했습니다. 이렇게 하면 가용한 확장들을 열거하면서 쉽게 제외할 수 있습니다. 물론 `checkValidationLayerSupport`에서처럼 중첩된 루프(nested loop)를 사용해도 됩니다. 성능에 영향은 없습니다. 이제 코드를 실행해 여러분의 그래픽 카드가 스왑 체인을 생성할 수 있는지 확인하세요. 사실 이전 장에서 확인한 표현 큐가 사용 가능하다면 스왑 체인 확장도 반드시 지원해야만 합니다. 하지만 이러한 사항들을 명시적으로 확인하고, 확장 또한 명시적으로 활성화 하는 것이 더 좋습니다.

## 장치 확장 활성화

스왑 체인을 사용하려면 먼저 `VK_KHR_swapchain` 확장을 활성화 해야 합니다. 확장을 활성화하기 위해서는 논리적 장치 생성 구조체에 약간의 변경만 해 주면 됩니다:

```c++
createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
createInfo.ppEnabledExtensionNames = deviceExtensions.data();
```

기존의 `createInfo.enabledExtensionCount = 0;` 명령문을 대체하도록 해야 합니다.

## 스왑 체인 지원 세부사항 질의

스왑 체인이 사용 가능한지만 확인하는 것으로 끝이 아닙니다. 왜냐하면 우리의 윈도우 표면과 실제로 호환이 되지 않을 수도 있기 떄문입니다. 스왑 체인을 생성하는 것 또한 인스턴스나 장치 생성보다 복잡한 설정이 필요하므로 더 진행하기 전에 필요한 세부 사항들을 질의하는 것 부터 시작해야 합니다.

확인해야 할 속성은 기본적으로 세 종류입니다:

* 기본 표면 기능 (스왑 체인의 최대/최소 이미지 개수, 이미지의 최대/최소 너비와 높이)
* 표면 포맷 (픽셀 포맷, 컬러 공간)
* 사용 가능한 표시 모드

`findQueueFamilies`와 유사하게, 구조체를 사용하여 이러한 세부 사항들을 질의 과정에서 사용할 것입니다. 위에 이야기한 세 종류의 속성들은 다음과 같은 구조체와 구조체 리스트로 만듭립니다.

```c++
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
```

다음으로 `querySwapChainSupport` 라는 이름의 새 함수를 만들고 이 구조체를 생성합니다.

```c++
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    return details;
}
```

이 장에서는 이러한 정보를 포함한 구조체를 질의하는 방법을 설명합니다. 이 구조체의 의미와 정확히 어떤 데이터들을 가지고 있는지는 다음 장에서 설명할 것입니다.

기본 표면 기능으로 시작해 봅시다. 이러한 속성들은 질의하기 쉽고 `VkSurfaceCapabilitiesKHR` 타입의 단일 구조체로 반환됩니다.

```c++
vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
```

이 함수는 지원되는 기능을 판단할 때, 명시된 `VkPhysicalDevice`와 `VkSurfaceKHR` 윈도우 표면을 고려하도록 구현되어 있습니다. 모든 질의 지원 함수들은 이러한 두 매개변수를 받도록 되어 있는데 이들이 스왑 체인의 핵심 구성요소이기 때문입니다.

다음 단계는 지원하는 표면 포맷을 질의하는 것입니다. 이는 구조체의 리스트로, 두 개의 함수 호출과 유사한 방식입니다.

```c++
uint32_t formatCount;
vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
}
```

모든 가능한 포맷을 저장할 수 있도록 벡터의 크기가 변하게 해야 합니다. 마지막으로, 지원하는 표현 모드를 질의하는 것도 `vkGetPhysicalDeviceSurfacePresentModesKHR`를 사용해 동일한 방식으로 이루어집니다:

```c++
uint32_t presentModeCount;
vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
}
```

이제 모든 세부 사항이 구조체 안에 있으니 `isDeviceSuitable`를 한번 더 확장하여 적절하게 스왑 체인이 지원되고 있는지 확인하도록 해 봅시다. 이 튜토리얼에서의 스왑 체인 지원은 우리가 가진 윈도우 표면에 대해 최소 하나의 이미지 포맷과 표면 모드를 지원하는 것이면 충분합니다.

```c++
bool swapChainAdequate = false;
if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
}
```

확장이 사용 가능한지 확인 후에 스왑 체인 지원 여부를 얻은 것일 뿐입니다. 함수의 마지막 라인은 아래와 같이 변경되어야 합니다.

```c++
return indices.isComplete() && extensionsSupported && swapChainAdequate;
```

## 스왑 체인에 대한 적절한 설정 선택하기

우리가 얻은 `swapChainAdequate` 조건이 만족되었다면 충분하지만, 서로 다른 최적화 요구사항에 대한 모드들이 여전히 존재합니다. 이제는 최적의 스왑 체인 설정을 찾기 위한 몇 가지 함수를 작성해 볼 것입니다. 결정해야 할 설정은 세 가지입니다:

* 표면 포맷 (색상 깊이(depth))
* 표시 모드 (이미지를 화면에 "스왑"하는 조건)
* 스왑 크기 (스왑 체인 이미지의 해상도)

이러한 각각의 설정에 대해 생각하고 있는 이상적인 값이 있을 것이고, 가능하면 이러한 값을 사용하도록 합니다. 그렇지 않다면 그 다음으로 괜찮은 값을 사용하도록 하는 로직을 만들 것입니다.

### 표면 포맷

이 설정에 대한 함수는 아래와 같이 시작합니다. 뒤에서 `SwapChainSupportDetails` 구조체의 `formats` 멤버를 인자로 넘겨줄 것입니다.

```c++
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

}
```

각 `VkSurfaceFormatKHR`는 `format` 과 `colorSpace` 멤버를 가지고 있습니다. `format`은 컬러 채널과 타입을 명시합니다. 예를 들어 `VK_FORMAT_B8G8R8A8_SRGB`는 B,G,R과 알파 채널을 그 순서대로 8비트 부호없는(unsigned) 정수로 저장하여 픽셀달 32비트를 사용합니다. `colorSpace` 멤버는 `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`를 사용해 SRGB 컬러 공간을 지원하는지 여부를 표시합니다. 참고로 이 플래그는 이전 버전 명세에서는 `VK_COLORSPACE_SRGB_NONLINEAR_KHR`였습니다.

컬러 공간에 대해서 우리는 가능하면 SRGB를 사용할 것인데, 이것이 [보다 정확한 색상 인지가 가능하기 때문입니다](http://stackoverflow.com/questions/12524623/). 또한 이는 나중에 살펴볼 (예를들면 텍스처와 같은) 이미지에 대한 표준 컬러 공간입니다. 이러한 이유로 컬러 포맷도 SRGB 컬러 포맷을 사용하는 것이고 가장 흔히 사용되는 것이 `VK_FORMAT_B8G8R8A8_SRGB`입니다.

리스트를 순회하며 이러한 선호하는 조합이 사용 가능한지 확인합니다.

```c++
for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return availableFormat;
    }
}
```

실패한다면 사용 가능한 포맷들에 얼마나 "좋은지" 여부를 바탕으로 순위를 매길 수 있습니다. 하지만 대부분 명시된 첫 번째 포맷을 그냥 사용하는 것으로 충분합니다.

```c++
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}
```

### 표시 모드(Presentation mode)

표시 모드는 스왑 체인에서 가장 중요한 설정인데, 이미지를 화면에 표시하는 실제 조건을 나타내는 부분이기 때문입니다. Vulkan에서는 네 가지 모드가 가능합니다:

* `VK_PRESENT_MODE_IMMEDIATE_KHR`: 여러분 응용 프로그램에서 제출(submit)된 이미지가 곧바로 화면에 전송되어 테어링(tearing) 현상이 발생할 수 있습니다.
* `VK_PRESENT_MODE_FIFO_KHR`: 스왑 체인은 큐가 되어, 디스플레이는 화면이 갱신될 때 큐의 앞에서 이미지를 가져오고, 프로그램은 렌더링된 이미지를 큐의 뒤에 삽입합니다. 큐가 꽉 차면 프로그램은 대기해야 합니다. 현대 게임에서 볼 수 있는 수직 동기화(vertical sync)와 유사합니다. 화면이 갱신되는 순간은 "수직 공백(vertical blank)"라 불립니다.
* `VK_PRESENT_MODE_FIFO_RELAXED_KHR`: 이 모드는 이전 모드와 프로그램이 지연되어서 마지막 수직 공백때 큐가 비는 경우에만 다르게 동작합니다. 다음 수직 공백을 기다리는 대신, 그 다음 이미지가 도착하는 즉시 전송됩니다. 이러한 경우 눈에 띄는 테어링이 발생하게 됩니다.
* `VK_PRESENT_MODE_MAILBOX_KHR`: 이것도 두 번째 모드의 또다른 버전입니다. 큐가 꽉 찼을 때 응용 프로그램을 대기시키는 대신, 큐에 있는 이미지가 새로운 이미지로 대체됩니다. 이 모드는 가능한 빠르게 렌더링을 수행하면서 테어링을 방지할 수 있고, 표준적인 수직 동기화보다 지연시간(latency) 문제를 줄일 수 있습니다. This is commonly known as "triple buffering", although the existence of three buffers alone does not necessarily mean that the framerate is unlocked.

`VK_PRESENT_MODE_FIFO_KHR` 모드만 사용 가능한 것이 보장되기 때문에 사용 가능한 최선의 모드를 찾는 함수를 작성합니다:

```c++
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    return VK_PRESENT_MODE_FIFO_KHR;
}
```

에너지 사용량 문제가 없는 경우라면, 개인적으로 `VK_PRESENT_MODE_MAILBOX_KHR`가 좋은 대체 모드라고 생각합니다. 테어링을 방지하면서도 새로운 이미지를 가급적 최신 이미지로 수직 공백 전까지 유지하기 때문에 꽤 낮은 지연시간을 갖습니다. 모바일 장치와 같이 에너지 사용량 문제가 중요한 경우에는 대신 `VK_PRESENT_MODE_FIFO_KHR`를 사용하는 것이 좋을 것입니다. 이제 `VK_PRESENT_MODE_MAILBOX_KHR` 가 사용 가능한지 리스트 내에서 찾아 봅니다:

```c++
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}
```

### 스왑 크기(extent)

이제 주요 속성 하나만 남았고, 마지막 함수로 추가할 것입니다:

```c++
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {

}
```

스왑 크기는 스왑 체인 이미지의 해상도이고 거의 대부분의 경우에 *픽셀 단위에서* 우리가 이미지를 그리고자 하는 윈도의 해상도와 동일한 값을 가집니다(보다 상세한 내용은 곧 살펴볼 것입니다). 가능한 해상도의 범위는 `VkSurfaceCapabilitiesKHR` 구조체에 정의되어 있습니다. Vulkan은 `currentExtent` 멤버의 너비와 높이를 설정하여 윈도우의 해상도와 맞추도록 하고 있습니다. 하지만 어떤 윈도우 매니저의 경우 `currentExtent`의 너비와 높이 값을 특수한 값(`uint32_t`의 최대값)으로 설정하여 이 두 값을 다르게 할 수 있습니다. 이러한 경우 윈도우에 가장 적절한 해상도를 `minImageExtent`와 `maxImageExtent` 사이 범위에서 선택하게 됩니다. 하지만 올바른 단위(unit)으로 해상도를 명시해야 합니다.

GLFW는 크기를 측정하는 두 단위가 있고 이는 픽셀과 [스크린 좌표계](https://www.glfw.org/docs/latest/intro_guide.html#coordinate_systems) 입니다. 예를 들어 우리가 이전에 윈도우를 생성할 때 명시한 `{WIDTH, HEIGHT}` 해상도는 스크린 좌표계 기준으로 측정한 값입니다. 하지만 Vulkan은 픽셀 단위로 동작하기 때문에, 스왑 체인의 크기도 픽셀 단위로 명시해 주어야만 합니다. 안타깝게도 여러분이 (애플릐 레티나 디스플레이와 같은) 고DPI 디스플레이를 사용하는 경우, 스크린 좌표계가 픽셀 단위와 달라집니다. 높은 픽셀 밀도로 인해 픽셀 단위의 윈도우 해상도는 스크린 좌표계 단위의 윈도우 해상도보다 커집니다. Vulkan이 스왑 크기에 관한 것을 수정해 주지 않는 한, 그냥 `{WIDTH, HEIGHT}`를 사용할 수는 없습니다. 대신에 `glfwGetFramebufferSize`를 사용해서 윈도우의 해상도를 최대 및 최소 이미지 크기와 맞추기 전에 픽셀 단위로 받아와야만 합니다.

```c++
#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp

...

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}
```

여기서 `clamp` 함수는 `width`와 `height` 값을 구현에서 허용 가능한 최대와 최소 크기로 제한하기 위해 사용되었습니다.

## 스왑 체인 생성하기

이제 런타임 선택을 위해 필요한 모든 헬퍼 함수들이 준비되었으니 동작하는 스왑 체인을 만들기 위한 모든 정보를 얻을 수 있습니다.

`createSwapChain`함수는 이러한 함수 호출의 결과를 받는 함수이고 `initVulkan`에서  논리적 장치 생성 이후에 호출됩니다.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
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

이러한 속성들 이외에도 스왑 체인에 몇 개의 이미지를 사용할 것인지 결정해야 합니다. 구현을 통해 동작하기 위한 최소 개수를 명시할 수 있습니다:

```c++
uint32_t imageCount = swapChainSupport.capabilities.minImageCount;
```

하지만 이러한 최소 개수를 사용하면, 렌더링을 수행할 또다른 이미지를 얻기위해 드라이버의 내부 연산을 기다리는 결과를 낳을 수 있습니다. 따라서 최소로 요구되는 것보다 하나 더 많은 이미지를 요구하는 것이 권장됩니다.

```c++
uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
```

또한 이 과정에서 최대 이미지 개수를 넘지 않도록 해야 하며 여기서 `0`은 최대 개수의 제한이 없다는 것을 의미하는 특별한 값입니다.

```c++
if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
}
```

Vulkan 객체들이 그렇듯이, 스왑 체인 객체를 생성하는 것도 커다란 구조체에 값을 채우는 과정이 필요합니다. 익숙한 코드로 시작합니다:

```c++
VkSwapchainCreateInfoKHR createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
createInfo.surface = surface;
```

어떤 표면에 스왑 체인이 연결되어야 하는지를 명시한 뒤에, 스왑 체인 이미지의 세부 사항들을 명시합니다:

```c++
createInfo.minImageCount = imageCount;
createInfo.imageFormat = surfaceFormat.format;
createInfo.imageColorSpace = surfaceFormat.colorSpace;
createInfo.imageExtent = extent;
createInfo.imageArrayLayers = 1;
createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
```

`imageArrayLayers`는 각 이미지가 구성하는 레이어의 개수를 명시합니다. 여러분이 스테레오 3D(stereoscopic 3D) 응용 프로그램을 개발하는 것이 아니라면 이 값은 항상 `1`입니다. `imageUsage` 비트 필드는 스왑 체인의 이미지에 어떤 연산을 적용할 것인지를 명시합니다. 이 튜토리얼에서 우리는 여기에 직접 렌더링을 수행할 것이므로 color attachment로 사용될 것입니다. 먼저 별도의 이미지에 렌더링한 뒤 후처리(post-processing)을 적용하는 것도 가능합니다. 이러한 경우 `VK_IMAGE_USAGE_TRANSFER_DST_BIT`과 같은 값을 사용하고 렌더링된 이미지를 스왑 체인 이미지로 전송하기 위한 메모리 연산을 사용해야 합니다.

```c++
QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

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

다음으로 여러 큐 패밀리에 걸쳐 사용될 스왑 체인의 이미지들이 어떻게 처리될 것인지를 명시해 주어야 합니다. 우리 응용 프로그램에서는 그래픽스 큐 패밀리와 표시 큐가 다른 경우가 이에 해당됩니다. 그래픽스 큐로부터 스왑 체인 이미지에 그리기가 수행될 것이고, 이를 표시 큐에 제출할 것입니다. 여러 큐에서 접근 가능한 이미지를 다루는 두 가지 방법이 있습니다:

* `VK_SHARING_MODE_EXCLUSIVE`: 하나의 이미지가 한 번에 하나의 큐 패밀리에 의해 소유(own)되고 다른 큐에서 사용되기 전에 명시적으로 전송되어야 합니다. 이 옵션이 성능이 가장 좋습니다.
* `VK_SHARING_MODE_CONCURRENT`: 소유권의 명시적 이동 없이 이미지가 여러 큐에서 동시에 접근 가능합니다.

큐 패밀리가 다르다면 이 튜토리얼에서는 소유권 챕터로 넘어가기 전에 동시성 모드(concurrent mode)를 사용할 것인데 동시성에 대한 설명은 몇몇 개념 때문에 나중에 설명하는 것이 낫기 때문입니다. 동시성 모드는 어떤 큐 패밀리의 소유권이 공유될 것인지 `queueFamilyIndexCount`와 `pQueueFamilyIndices` 매개변수를 사용해 미리 명시하게 되어 있습니다. 그래픽스 큐 패밀리와 표시 큐 패밀리가 동일하다면 (대부분의 하드웨어에서는 동일함) 독점(exclusive) 모드를 사용할 것입니다. 동시성 모드에서는 최소한 두 개의 서로다른 큐 패밀리는 명시해야만 하기 떄문입니다.

```c++
createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
```

이제 스왑 체인의 이미지에 적용할 특정 변환(transform)을 명시할 수 있습니다. 이를 그 기능이 지원될 때(`capabilities`의 `supportedTransforms`)에 가능한데 예를 들면 시계방향으로 90도 회전이라던가, 수평 뒤집기(flip) 등이 있습니다. 이러한 변환을 적용하지 않을 것이면, 현재 변환(current transformation)으로 명시하면 됩니다.

```c++
createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
```

`compositeAlpha` 필드는 윈도우 시스템의 다른 윈도우와의 블렌딩(blending)을 위해 알파 채널이 사용될 것인지를 명시합니다. 거의 개부분의 경우 알파 채널은 무시하는 것이 좋으므로 `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR`를 사용합니다.

```c++
createInfo.presentMode = presentMode;
createInfo.clipped = VK_TRUE;
```

`presentMode`는 이름만 봐도 아실 수 있겠죠. `clipped`가 `VK_TRUE` 면 다려진 픽셀의 색상에 대해서는 신경쓰지 않겠다는 의미인데, 예를 들면 다른 윈도우가 그 픽셀 위에 있는 경우입니다. 뒤쪽의 픽셀 값을 읽어와 의도하는 결과를 얻을 것이 아니라면 그냥 클리핑(clipping)을 활성화 하는게 성능에 좋습니다.

```c++
createInfo.oldSwapchain = VK_NULL_HANDLE;
```

이제 마지막 필드인 `oldSwapChain` 입니다. Vulkan을 사용하면 응용 프로그램이 실행되는 동안 스왑 체인이 사용 불가능해지거나, 최적화되지 않을 수 있습니다. 예를 들어 윈도우가 리사이즈(resize)되는 경우에 그렇습니다. 이러한 경우 스왑 체인이 처음부터 다시 만들어져야만 하고 이전 스왑 체인에 대한 참조가 여기에 명시되어야만 합니다. 이 주제는 복잡하기 때문에 [이후 챕터](!kr/Drawing_a_triangle/Swap_chain_recreation)에서 보다 자세히 배워볼 것입니다. 지금은 그냥 하나의 스왑 체인만 만드는 것으로 가정합시다.

이제 `VkSwapchainKHR` 객체를 저장할 클래스 멤버를 추가합니다:

```c++
VkSwapchainKHR swapChain;
```

이제 스왑 체인을 만드는 것은 단순히 `vkCreateSwapchainKHR`를 호출하는 것으로 간단해졌습니다.

```c++
if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
}
```

매개변수는 논리적 장치, 스왑 체인 생성 정보, 선택적인 사용자 정의 할당자와 핸들을 저장할 변수에 대한 포인터입니다. 새로울 것 없죠. 소멸은 장치 소멸에 앞서 `vkDestroySwapchainKHR`를 사용해 이루어져야 합니다:

```c++
void cleanup() {
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    ...
}
```

이제 프로그램을 실행해 스왑 체인이 올바로 생성되었는지 확인하세요! `vkCreateSwapchainKHR`에서 접근 위반 오류가 발생하거나 `Failed to find 'vkGetInstanceProcAddress' in layer SteamOverlayVulkanLayer.dll`와 같은 메시지를 마주치게 되면, [FAQ](!en/FAQ)에서 Steam 오버레이 레이어 내용을 살펴보세요.

검증 레이어가 활성화 된 상태에서 `createInfo.imageExtent = extent;` 명령문을 지워 보세요. 검증 레이어가 바로 실수를 탐지하고 도움이 되는 메시지를 출력하는 것을 볼 수 있을 겁니다.

![](/images/swap_chain_validation_layer.png)

## 스왑 체인 이미지의 획득(Retrieving)

이제 스왑 체인이 생성되었으니, 그 안의 `VkImage`들에 대한 핸들을 획득하는 과정이 남았습니다. 나중 챕터에서 렌더링 연산을 위해 이를 사용하게 됩니다. 핸들을 저장하기 위한 클래스 멤버를 추가합니다:

```c++
std::vector<VkImage> swapChainImages;
```

스왑 체인 구현에 의해 이미지들이 만들어지고 스왑 체인이 소멸될 때 자동으로 정리되므로 `cleanup` 코드에 뭔가를 추가할 필요는 없습니다.

`createSwapChain` 함수의 마지막 부분 `vkCreateSwapchainKHR` 뒤에 획득을 위한 코드를 추가할 것입니다. 획득 과정은 Vulkan으로부터 객체의 배열을 획득하는 일반적인 과정입니다. 기억하셔야 할 것은 우리는 스왑 체인의 이미지 최소 개수만 명시하였으므로 구현에 따라 더 많은 이미지가 생성되었을 수 있습니다. 그래서 `vkGetSwapchainImagesKHR`로 최종 생성된 이미지 개수를 먼저 얻고 컨테이너(container) 크기를 조정한 뒤 핸들을 얻어오도록 구현하였습니다.

```c++
vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
swapChainImages.resize(imageCount);
vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
```

마지막으로 멤버 변수에 스왑 체인 이미지를 위해 우리가 명시한 포맷과 크기를 저장합니다. 나중 챕터에서 이 값들을 사용할 것입니다.

```c++
VkSwapchainKHR swapChain;
std::vector<VkImage> swapChainImages;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;

...

swapChainImageFormat = surfaceFormat.format;
swapChainExtent = extent;
```

이제 그림을 그리고 화면에 표시될 이미지가 준비되었습니다. 다음 챕터에서부터는 이미지를 렌더링 타겟(render target)으로 설정하는 법, 실제 그래픽스 파이프라인과 그리지 명령에 대해 살펴볼 것입니다!

[C++ code](/code/06_swap_chain_creation.cpp)
