Vulkan은 플랫폼 독립적인 API이기 때문에, 윈도우 시스템과 직접적으로 소통할 수는 없습니다. Vulkan과 윈도우 시스템간의 연결을 만들어 결과물을 화면에 보이도록 하기 위해서 우리는 WSI (Window System Integration)을 사용해야만 합니다. 이 챕터에서는 먼저 `VK_KHR_surface`를 살펴볼 것입니다. `VK_KHR_surface` 객체는 렌더링된 이미지를 표현할 표면(surface)의 추상화된 객체입니다. 우리 프로그램에서 표면은 GLFW를 사용해 열어놓은 윈도우가 뒷받침할 것입니다.

`VK_KHR_surface` 확장은 인스턴스 수준의 확장이고, 우리는 이미 활성화 시켜 놓았습니다. 왜냐하면 `glfwGetRequiredInstanceExtensions`를 통해 반환된 리스트에 포함되어 있거든요. 이 리스트는 다음 몇 챕터에서 사용할 다른 WSI 확장도 포함하고 있습니다.

윈도우 표면은 인스턴스 생성 이후에 곧바로 생성해야 하는데 이는 윈도우 표면이 물리적 장치의 선택에 영향을 주기 때문입니다. 이 내용을 여기까지 미룬 이유는 윈도우 표면이 렌더 타겟과 표현과 관련된 큰 주제이고, 이러한 내용으로 인해 기본적인 세팅 설명을 복잡하게 만들고 싶지 않았기 때문입니다. 또한 윈도우 표면은 Vulkan에서 선택적인 구성요소로, 오프 스크린(off-screen) 렌더링을 할 경우에는 필요하지 않습니다. Vulkan은 보이지 않는 윈도우를 생성하는 등의 편법을 동원하지 않고서도 이런 기능을 사용 가능합니다 (OpenGL에서는 편법으로 구현해야만 합니다).

## 윈도우 표면 생성

`surface` 클래스 멤버를 디버그 콜백 바로 뒤에 추가하는 것 부터 시작합니다.

```c++
VkSurfaceKHR surface;
```

`VkSurfaceKHR`객체와 그 활용은 플랫폼 독립적이지만, 생성에 있어서는 윈도우 시스템의 세부 사항에 의존적입니다. 예를 들어, 윈도우에서는 `HWND` 와 `HMODULE` 핸들이 필요합니다. 따라서 플랫폼 의존적인 확장들이 존재하고 윈도우의 경우 이 확장은 `VK_KHR_win32_surface`입니다. 이 확장은 `glfwGetRequiredInstanceExtensions`에 자동적으로 포함되어 있습니다.

윈도우즈에서 표면을 생성하기 위해 이러한 플랫폼별 확장을 사용하는 예시를 보여드리겠습니다. 하지만 이 튜토리얼에서 이를 실제 사용하진 않을 것입니다. GLFW와 같은 라이브러리를 사용하면서도 플랫폼별 코드를 사용하는 것은 적절하지 않습니다. GLFW에서는 `glfwCreateWindowSurface`를 통해 플랫폼별 차이에 따른 코드를 처리해 줍니다. 그래도, 사용하기 전에 뒤쪽에서 어떤 일이 벌어지는지는 알아두는 것이 좋겠죠.

네이티브 플랫폼 기능에 접근하기 위해서는 위쪽에 include를 바꿔줘야 합니다.

```c++
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
```

윈도우 표면은 Vulkan 객체기 때문에 우리가 값을 채워야 하는 `VkWin32SurfaceCreateInfoKHR` 구조체를 사용해야 합니다. 여기에는 두 개의 중요한 매개변수가 있는데 `hwnd` 와 `hinstance`입니다. 이는 윈도우와 프로세스에 대한 핸들입니다.

```c++
VkWin32SurfaceCreateInfoKHR createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
createInfo.hwnd = glfwGetWin32Window(window);
createInfo.hinstance = GetModuleHandle(nullptr);
```

`glfwGetWin32Window`함수는 GLFW 윈도우 객체로부터 `HWND`를 얻기위해 사용됩니다. `GetModuleHandle` 호출은 현재 프로세스의 `HINSTANCE` 핸들을 반환해줍니다.

이후에는 `vkCreateWin32SurfaceKHR`를 통해 표면을 생성할 수 있는데 매개변수는 인스턴스, 표면 생성 세부사항, 사용자 정의 할당자와 표면 핸들 저장을 위한 변수입니다. 정확히 하자면 이는 WSI 확장 함수이지만, 자주 사용되는 관계로 표준 Vulkan 로더에 포함되어 있고, 그렇기 때문에 명시적으로 로드할 필요가 없습니다.

```c++
if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
}
```

이 과정은 리눅스 등 다른 플랫폼에서도 유사한데, 이 경우 `vkCreateXcbSurfaceKHR`는 XCB 커넥션과 윈도우, X11 등 세부사항을 생성 시에 명시해 주어야 합니다.

`glfwCreateWindowSurface` 함수는 이러한 과정을 각 플랫폼별로 구현해 두었습니다. 우리는 이를 우리의 프로그램에 통합해서 사용하기만 하면 됩니다. `initVulkan`에서 호출할 `createSurface` 함수를 인스턴스 생성과 `setupDebugMessenger` 뒤에 추가하겠습니다.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
}

void createSurface() {

}
```

GLFW 호출은 구조체 대신 간단한 매개변수들을 받기 때문에 구현이 직관적입니다.

```c++
void createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}
```

매개변수는 `VkInstance`, GLFW 윈도우에 대한 포인터, 사용자 정의 할당자와 `VkSurfaceKHR` 변수에 대한 포인터입니다. 내부적으로는 플랫폼 관련 호출을 한 뒤에 `VkResult` 값을 전달하여 반환해 줍니다. GLFW는 표면 소멸을 위한 특별한 함수를 제공하지는 않으므로, 기본(original) API를 통해 구현해야 합니다:

```c++
void cleanup() {
        ...
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        ...
    }
```

표면이 인스턴스보다 먼저 소멸되도록 해야 합니다.

## 표현 지원에 대한 질의(Querying for presentation support)

Vulkan 구현이 윈도우 시스템 통합을 지원하지만, 그렇다고 모든 시스템의 장치가 이를 지원한다는 의미는 아닙니다. 따라서 `isDeviceSuitable`를 확장하여 장치가 우리가 만든 표면에 이미지를 표현할 수 있는지를 확인해야 합니다. 표현(presentation)은 큐의 기능이므로 이는 우리가 만든 표면에 표현 기능을 지원하는 큐 패밀리를 찾는 문제로 귀결됩니다.

그리기 명령(drawing command)을 지원하는 큐 패밀리와 표현 기능을 지원하는 큐 패밀리가 동일하지 않을 수 있습니다. 따라서 `QueueFamilyIndices` 구조체를 수정해 별도의 표현 큐 상황을 고려하겠습니다.

```c++
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};
```

다음으로, `findQueueFamilies`함수를 수정해서 윈도우 표면에 표현 기능이 있는 큐 패밀리를 찾아 봅시다. 이를 체크하기 위한 함수는 `vkGetPhysicalDeviceSurfaceSupportKHR` 함수이고, 물리적 장치, 큐 패밀리 인덱스와 표면을 매개변수로 받습니다. `VK_QUEUE_GRAPHICS_BIT`와 동일한 루프에 이 함수의 호출을 추가합니다:

```c++
VkBool32 presentSupport = false;
vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
```

불리언 값을 확인하고 표현 패밀리의 큐 인덱스를 저장합니다:

```c++
if (presentSupport) {
    indices.presentFamily = i;
}
```

알아두셔야 할 것은 결국 이 두 큐 패밀리는 동일할 가능성이 아주 높다는 것입니다. 하지만 이 프로그램에서는 접근법의 일관성을 위해 이 둘을 별개의 큐인 것처럼 처리할 것입니다. 그리기와 표현을 동일한 큐에서 지원하는 물리적 장치를 선호하도록 로직을 구현하면 성능이 개선될 수 있습니다.

## 표현 큐 생성하기

이제 남은 것은 논리적 장치 생성 과정을 수정하여 표현 큐를 생성하고 `VkQueue` 핸들을 찾는 과정입니다. 핸들을 위한 멤버 변수를 추가합니다:

```c++
VkQueue presentQueue;
```

다음으로 여러 개의 `VkDeviceQueueCreateInfo` 구조체를 추가하여 두 개의 패밀리로부터 큐를 생성해야 합니다. 깔끔한 방법은 요구되는 모든 큐에 대한 고유한(unique) 큐 패밀리 집합을 생성하는 방법입니다:

```c++
#include <set>

...

QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

float queuePriority = 1.0f;
for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
}
```

그리고 `VkDeviceCreateInfo`에서 해당 벡터를 가리키도록 수정합니다:

```c++
createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
createInfo.pQueueCreateInfos = queueCreateInfos.data();
```

큐 패밀리가 같다면 인덱스를 한 번만 넘겨주면 됩니다. 마지막으로 큐 핸들을 얻기 위한 호출을 추가합니다.

```c++
vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
```

큐 패밀리가 같은 경우 두 개의 핸들은 이제 같은 값을 가질 것입니다. 다음 챕터에서는 스왑 체인을 살펴보고 어떻게 스왑 체인이 표면에 이미지를 표현하는 기능을 제공하는지 알아볼 것입니다.

[C++ code](/code/05_window_surface.cpp)
