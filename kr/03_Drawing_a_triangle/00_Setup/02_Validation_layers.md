## 검증 레이어란?

Vulkan API는 최소한의 드라이버 오버헤드를 기반으로 설계되었고 그를 위해서 기본적으로 API에서는 최소한의 오류 체크 기능만을 포함하고 있습니다. 열거자를 잘못된 값으로 설정한다거나 필요한 매개변수에 널 포인터를 넘긴다거나 하는 간단한 오류도 일반적으로는 명시적으로 처리되지 않아서 크래시나 정의되지 않은 동작을 일으키게 됩니다. Vulkan에서 여러분이 하는 작업은 매우 명시적이어야 하기 때문에, 새로운 GPU의 기능을 사용한다거나, 논리적 장치 생성 때 필요한 것들을 깜빡한다거나 하는 작은 실수를 저지르기 쉽습니다.

하지만, 그렇다고 그러한 체크 기능이 API에 포함될 수 없는것은 아닙니다. Vulkan은 *검증 레이어*라고 알려진 우아한 해결책을 만들었습니다. 검증 레이어는 선택적인 구성요소로 Vulkan 함수 호출에 후킹(hook)할 수 있는 추가적인 연산입니다. 일반적으로 검증 레이어에서 수행하는 연산은:

- 명세를 기반으로 매개변수의 값을 체크하여 잘못된 사용을 탐지
- 리소스의 누수를 탐지하기 위해 객체의 생성과 소멸을 추적
- 호출 지점으로부터 쓰레드를 추적하여 쓰레드 세이프티(safety)를 확인
- 표준 출력에 모든 호출과 매개변수를 로깅(logging)
- 프로파일링(profiling)과 리플레이(replaying)를 위한 Vulkan 호출 추적

진단(diagnostics) 검증 레이어의 함수 구현 예시는 아래와 같습니다:

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

이러한 검증 레이어들은 여러분이 의도하는 모든 디버깅 기능들을 얼마든지 누적(stack)할 수 있도록 되어 있습니다. 디버깅 빌드에서 검증 레이어를 활성화 하고 릴리즈 빌드에서는 비활성화 하면 양 쪽 상황에서 모두 문제가 없을 것입니다.

Vulkan은 내장 검증 레이어를 제공하지는 않지만 LunarG Vulkan SDK에서는 흔히 발생하는 오류 검출을 위한 레이어들을 제공하고 있습니다. 완전히 [오픈 소스](https://github.com/KhronosGroup/Vulkan-ValidationLayers)이니, 어떤 종류의 실수를 탐지해 주는지 알 수 있고, 여러분이 기여도 할 수 있습니다. 검증 레이어를 사용하는 것이 여러분의 응용 프로그램이 다른 드라이버에서 정의되지 않은 동작으로 인해 올바로 동작하지 않는 것을 방지하는 가장 좋은 방법입니다.

검증 레이어는 시스템에 설치되어 있어야 사용 가능합니다. 예를 들어 LunarG 검증 레이어는 Vulkan SDK가 설치된 PC에서만 사용 가능합니다.

Vulkan에는 두 가지 종류의 검증 레이어가 존재하는데 인스턴스와 장치(device) 레이어입니다. 인스턴스 레이어는 인스턴스와 같은 전역 Vulkan 객체들만을 체크하고 장치 레이어는 특정 GPU에 관련된 호출만을 체크합니다. 현재 장치 레이어는 더 이상 사용되지 않으며(deprecated), 인스턴스 검증 레이어가 모든 Vulkan 호출에 적용됩니다. 명세 문서에는 여전히 호환성을 위해 장치 수준에서 검증 레이어를 활성화 할 것을 권장하고 있습니다. [나중에](!kr/Drawing_a_triangle/Setup/Logical_device_and_queues) 보게 될 것인데, 우리는 같은 레이어를 인스턴스와 논리 장치 수준에서 사용할 것입니다.

## 검증 레이어 사용하기

이 장에서 우리는 Vulkan SDK에서 제공하는 표준 진단 레이어를 활성화 하는 법을 알아볼 것입니다. 확장과 마찬가지로, 검증 레이어는 그 이름을 명시하여 활성화해야 합니다. 모든 유용한 표준 검증들은 SDK에 포함되어 있는 `VK_LAYER_KHRONOS_validation`이라는 레이어에 포함되어 있습니다.

먼저 프로그램에 두 개의 구성 변수를 추가하여 사용할 레이어를 명시하고 그들을 활성화 할것인지를 알려 줍니다. 저는 디버깅 모드인지 아닌지에 따라 값을 설정하도록 했습니다. `NDEBUG` 매크로는 C++ 표준에 포함된 매크로로 "디버그가 아님"을 의미합니다.

```c++
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
```

`checkValidationLayerSupport` 함수를 추가하여 요청한 레이어들이 모두 사용 가능한지를 체크합니다. 먼저 가용한 모든 레이어 목록을 `vkEnumerateInstanceLayerProperties`를 사용해 만듭니다. 사용법은 인스턴스 생성 챕터에서 봤던 `vkEnumerateInstanceExtensionProperties`와 동일합니다.

```c++
bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    return false;
}
```

다음으로, `validationLayers` 내의 모든 레이어가 `availableLayers` 내에 존재하는지를 체크합니다. `strcmp` 사용을 위해 `<cstring>`의 include가 필요합니다.

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

이제 이 함수를 `createInstance`에서 사용할 수 있습니다.

```c++
void createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    ...
}
```

이제 프로그램을 디버그 모드에서 실행해 오류가 발생하지 않는지 확인하세요. 오류가 발생하면, FAQ를 살펴보세요.

마지막으로, `VkInstanceCreateInfo` 구조체 초기화를 수정해서 사용이 가능한 경우 검증 레이어의 이름을 포함하도록 하세요.

```c++
if (enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
} else {
    createInfo.enabledLayerCount = 0;
}
```

체크 과정이 성공적이라면 `vkCreateInstance`가 `VK_ERROR_LAYER_NOT_PRESENT` 오류를 반환하지 않을 것이지만, 확인을 위해 실행해 보시기 바랍니다.

## 메시지 콜백

검증 레이어는 기본적으로 디버그 메시지를 표준 출력창에 표시하지만, 프로그램에서 명시적으로 콜밸을 제공하여 우리가 원하는 방식대로 처리할 수도 있습니다. 이렇게 하면 어떤 종류의 메시지를 보기 원하는지 선택할 수 있는데 모든 메시지들이 (치명적인) 오류에 관한 것은 아니기 때문입니다. 지금은 그냥 그대로 두고 싶다면 이 챕터의 마지막으로 넘어가셔도 됩니다.

메시지 처리를 위한 콜백을 설정하고 관련한 설정들을 조정하고 싶다면, `VK_EXT_debug_utils` 확장을 사용해 디버그 메신저(messenger) 콜백을 설정해야 합니다.

먼저 검증 레이어가 활성화 되었는지 여부에 따라 필요한 확장 목록을 반환하는 `getRequiredExtensions` 함수를 만들겠습니다.

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

GLFW가 명시한 확장은 항상 필요하지만, 디버그 메시지를 위한 확장은 조건에 따라 추가하였습니다. 여기서 `VK_EXT_DEBUG_UTILS_EXTENSION_NAME` 매크로를 사용하였는데 이는 문자열 리터럴 "VK_EXT_debug_utils"와 동일하는 것에 유의하세요. 이러한 매크로를 사용하면 오타로 인한 오류를 방지할 수 있습니다.

이제 이 함수를 `createInstance`에서 사용합니다:

```c++
auto extensions = getRequiredExtensions();
createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
createInfo.ppEnabledExtensionNames = extensions.data();
```

`VK_ERROR_EXTENSION_NOT_PRESENT` 오류가 발생하지 않는지 프로그램을 실행해 확인하세요. 확장들이 존재하는지는 확인할 필요 없습니다. 왜냐하면 검증 레이어가 가용하다면 당연히 해당 확장들도 사용 가능하기 떄문입니다.

이제 디버그 콜백의 생김새를 한 번 봅시다. `PFN_vkDebugUtilsMessengerCallbackEXT` 프로토타입을 갖는 `debugCallback`이라는 새 스태틱 멤버 함수를 추가합시다. `VKAPI_ATTR`과 `VKAPI_CALL`를 통해 Vulkan에서 호출하기 위해 적절한 시그니처를 갖고 있는지 확인합니다.

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

첫 번쨰 파라메터는 메시지의 심각도를 나타내고, 아래 플래그 중 하나입니다.

- `VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT`: 진단 메시지
- `VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT`: 리소스의 생성과 같은 정보 메시지
- `VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT`: 오류는 아니지만 응용 프로그램의 버그일 수 있는 메시지
- `VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT`: 프로그램이 중단될 수 있는 허용되지 않는 동작에 대한 메시지

이러한 열거자들의 값은 비교 연산자를 통해 메시지가 특정 심각도와 같거나 더 심각한지를 설정할 수 있습니다. 예를 들어 아래와 같습니다:

```c++
if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    // Message is important enough to show
}
```

`messageType` 매개변수는 아래와 같은 값을 가질 수 있습니다:

- `VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT`: 명세 또는 성능과 관계없는 이벤트가 발생함
- `VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT`: 명세를 위반했거나 실수일 수 있는 경우
- `VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT`: Vulkan에서 효율적이지 않을 수 있음

`pCallbackData` 매개변수는 메세지의 상세 사항을 포함하여 아래와 같은 아주 중요한 매개변수를 갖는 `VkDebugUtilsMessengerCallbackDataEXT` 구조체를 참조합니다:

- `pMessage`: 널문자로 끝나는 문자열(string) 디버그 메시지
- `pObjects`: 메시지와 관련 있는 Vulkan 객체 핸들의 배열
- `objectCount`: 배열 내 객체의 숫자

마지막으로, `pUserData` 매개변수는 콜백 설정 시 명시한 포인터를 포함하며 사용자가 원하는 데이터를 전달할 수 있도록 합니다.

콜백은 불리언(boolean)을 반환하는데 해당 검증 레이어를 촉발한(triggered) Vulkan 호출이 중단(aborted)되어야 하는지 여부를 의미합니다. true가 반환되었다면 `VK_ERROR_VALIDATION_FAILED_EXT` 오류와 함께 호출이 중단됩니다. 보통 true 반환은 검증레이어 자체를 테스트 할 때에만 사용되므로 항상 `VK_FALSE`를 반환하는 것이 맞습니다.

이제 남은 것은 Vulkan에 콜백 함수를 알려주는 것 뿐입니다. 놀랍게도 Vulkan에서는 디버그 콜백조차 명시적으로 생성되고 소멸되는 핸들을 가지고 관리해야 합니다. 이러한 콜백은 *debug messenger*의 일부분으로 원하는 개수만큼 가질 수 있습니다. `instance` 바로 아래에 이 핸들을 위한 멤버를 추가 합시다:

```c++
VkDebugUtilsMessengerEXT debugMessenger;
```

이제 `initVulkan`에서 호출할 `setupDebugMessenger` 함수를 `createInstance` 바로 다음에 추가합니다:

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
}

void setupDebugMessenger() {
    if (!enableValidationLayers) return;

}
```

메신저에 대한 세부 사항과 콜백에 대한 내용을 구조체에 채웁니다:

```c++
VkDebugUtilsMessengerCreateInfoEXT createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
createInfo.pfnUserCallback = debugCallback;
createInfo.pUserData = nullptr; // Optional
```

`messageSeverity` 필드는 여러분의 콜백이 호출될 심각도 타입을 모두 명시할 수 있도록 되어 있습니다. 저는 `VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT`를 제외하고 모두 명시해서, 잠재적 문제는 모두 받고, 일반적인 디버깅 정보는 포함하지 않도록 했습니다.

`messageType` 필드는 콜백이 알림을 받을 메시지 타입을 필터링 할 수 있도록 합니다. 여기서는 모든 타입을 활성화 하였습니다. 필요하지 않으면 몇 개를 비활성화 해도 됩니다.

마지막으로 `pfnUserCallback`필드는 콜백 함수에 대한 포인터를 명시합니다. 선택적으로 `pUserData` 필드에 포인터를 전달하여 콜백 함수와 함께 전달될 파라메터를 명시할 수 있습니다. 예를 들어 `HelloTriangleApplication` 클래스의 포인터를 전달하는 데 사용할 수 있습니다.

검증 레이어 메시지와 디버그 콜백을 구성하는 다양한 방법이 있지만, 이 튜토리얼을 위한 시작으로는 이 정도가 좋습니다. [확장 명세](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap50.html#VK_EXT_debug_utils)에서 보다 다양한 정보를 볼 수 있습니다.

`VkDebugUtilsMessengerEXT` 객체를 생성하기 위해 `vkCreateDebugUtilsMessengerEXT` 함수에 이 구조체를 전달해야 합니다. 안타깝게도 이 함수는 확장 함수이기 때문에 자동적으로 로드되지 않습니다. `vkGetInstanceProcAddr`를 사용해 주소값을 얻어야 합니다. 이를 처리하기 위한 방안으로 프록시 함수를 만들 것입니다. 저는 `HelloTriangleApplication` 클래스 정의 바로 다음에 추가 하였습니다.

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

함수가 로드될 수 없으면 `vkGetInstanceProcAddr`함수는 `nullptr`을 반환합니다. 이제 이 함수를 호출하여 가능한 경우에 확장 객체를 생성할 수 있습니다.

```c++
if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
}
```

두 번째부터 마지막까지의 매개변수는 선택적인 할당 콜백으로 `nullptr`로 설정할 것이고, 이를 제외하면 나머지는 직관적입니다. 디버그 메신저는 우리의 Vulkan 인스턴스와 레이어에 한정되어 있으므로 첫 인자에서 명시해야만 합니다. 이러한 패턴과 다른 _child_ 객체들은 나중에 다시 보게 될 것입니다.

`VkDebugUtilsMessengerEXT` 객체는 `vkDestroyDebugUtilsMessengerEXT`호출로 정리되어야 합니다. `vkCreateDebugUtilsMessengerEXT`와 마찬가지로 이 함수는 명시적으로 로드되어야 합니다.

`CreateDebugUtilsMessengerEXT` 바로 밑에 또 다른 프록시 함수를 만듭시다.

```c++
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
```

이 함수는 정적(static) 멤버 함수 또는 클래스 밖의 함수로 만드는 것을 잊지 마십시오. 이 함수를 `cleanup` 함수에서 호출합니다.

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

## 인스턴스의 생성과 소멸 디버깅

검증 레이어와 디버깅을 프로그램에 추가하였지만 아직 모든 것이 완료된 것은 아닙니다. `vkCreateDebugUtilsMessengerEXT` 호출을 위해서는 그 전에 유효한 인스턴스가 생성되어야 하고, `vkDestroyDebugUtilsMessengerEXT`은 인스턴스가 소멸되기 전에 호출되어야 합니다. 따라서 `vkCreateInstance`와 `vkDestroyInstance` 호출에 있어서 발생하는 문제는 디버깅이 불가능합니다.

[확장 문서](https://github.com/KhronosGroup/Vulkan-Docs/blob/main/appendices/VK_EXT_debug_utils.adoc#examples)를 자세히 읽어보면 위 두 함수 호출에 대해 별도의 디버깅 메신서를 생성하는 방법이 있다는 것을 알 수 있으실 겁니다. 이를 위해서는 `VkDebugUtilsMessengerCreateInfoEXT` 구조체에 대한 포인터를 `VkInstanceCreateInfo`의 `pNext` 확장 필드에 넘겨주기만 하면 됩니다. 먼저 메신저 생성 정보만 추출해 벌도의 함수로 만듭니다.

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

이제 이를 `createInstance` 함수에서 재사용 할 수 있습니다:

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

`debugCreateInfo` 변수를 if문 밖에 작성하여 `vkCreateInstance` 호출 전에 소멸되지 않도록 하였습니다. 추가적인 디버깅 메신저를 이런 식으로 만들면 `vkCreateInstance` 호출 동안에, 그리고 `vkDestroyInstance` 호출과 이후 정리 과정에 자동적으로 사용됩니다.

## 테스팅

이제 일부러 실수를 만들어서 검증 레이어가 동작하는지 봅시다. `DestroyDebugUtilsMessengerEXT`를 `cleanup`에서 잠시 제거하고 프로그램을 실행해 봅시다. 종료되면 이런 화면을 보시게 될겁니다.

![](/images/validation_layer_test.png)

> 아무런 메시지가 보이지 않으면 [설치를 확인해 보세요](https://vulkan.lunarg.com/doc/view/1.2.131.1/windows/getting_started.html#user-content-verify-the-installation).

어떤 호출이 메시지를 트리거하였는지 보려면 메시지 콜백에 중단점을 걸고 호출 스택을 확인하면 됩니다.

## 구성(Configuration)

`VkDebugUtilsMessengerCreateInfoEXT` 구조체에 명시한 플래그 이외에도 더 많은 검증 레이어의 동작에 관한 세팅을 할 수 있습니다. Vulkan SDK로 가서 `Config` 디렉터리를 보세요. `vk_layer_settings.txt` 파일을 찾을 수 있을텐데 레이어를 설정하기 위한 설명이 적혀 있습니다.

여러분의 응용 프로그램을 위한 레이어 설정을 하기 위해서는 그 파일을 프로젝트의 `Debug` 와 `Release` 디렉터리에 복사하고 원하는 동작을 하도록 안내를 따라 하면 됩니다. 하지만, 이 튜토리얼에서는 기본 세팅을 사용하는 것으로 가정할 것입니다.

앞으로 튜토리얼에서 검증 레이어의 유용함을 보여드리기 위해서, 그리고 Vulkan이 정확히 어떤 일을 하는지를 아는 것이 얼마나 중요한지 알려드리기 위해 일부러 실수를 하는 경우들이 있을 겁니다. 이제 [시스템의 Vulkan 장치](!kr/Drawing_a_triangle/Setup/Physical_devices_and_queue_families)에 대해 알아봅시다.

[C++ code](/code/02_validation_layers.cpp)
