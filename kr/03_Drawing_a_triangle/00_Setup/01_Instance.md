## 인스턴스 생성

가장 먼저 할 일은 *인스턴스*를 생성하여 Vulkan 라이브러리를 초기화하는 것입니다. 인스턴스는 여러분의 응용 프로그램과 Vulkan 라이브러리를 연결해 주는 매개체이고 인스턴스를 생성하는 것은 여러분의 응용 프로그램에 대해 드라이버에게 상세한 사항들을 알려주는 것과 같습니다.

먼저 `createInstance` 함수를 `initVulkan` 함수 내에서 호출합시다.

```c++
void initVulkan() {
    createInstance();
}
```

또한 인스턴스의 핸들을 저장하기 위한 멤버를 추가합니다:

```c++
private:
VkInstance instance;
```

이제, 인스턴스를 만드려면 우리 응용 프로그램에 대한 몇 가지 정보를 구조체에 채워넣어야 합니다. 엄밀히 말하면 선택적인 과정이지만, 이를 통해 드라이버에게 유용한 정보를 전달하고 특정 응용프로그램을 최적화 할 수 있습니다 (e.g. because it uses a well-known graphics engine with certain special behavior). 이 구조체는 `VkApplicationInfo`입니다:

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

앞서 언급한 것처럼 Vulkan의 많은 구조체는 `sType` 멤버를 통해 명시적으로 타입을 명시하도록 합니다. 또한 이 구조체는 `pNext` 멤버를 가지는 많은 구조체 중 하나인데, 나중을 위한 확장을 가리킬 수 있도록 합니다. 여기서는 초기화를 통해 그냥 `nullptr`로 두었습니다.

Vulkan에서는 많은 정보가 함수의 매개변수 대신 구조체로 전달되고 인스턴스 생성을 위해서는 하나 이상의 구조체를 전달해야 하는 경우가 있습니다. 지금 보시는 두 번째 구조체는 반드시 전달되어야 하는데 Vulkan 드라이버에게 어떤 전역(global) 확장과 검증 레이어를 사용하려고 하는지 알려주는 것입니다. 여기서 전역의 의미는 특정 장치가 아닌 전체 프로그램에 적용된다는 의미인데, 다음 몇 챕터를 보게 되면 확실히 이해될 것입니다.

```c++
VkInstanceCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
createInfo.pApplicationInfo = &appInfo;
```

이 두 개는 직관적으로 이해가 될 겁니다. 뒤에 나올 두 개는 의도하는 전역 확장을 명시합니다. overview 챕터에서 이야기한 것처럼 Vulkan은 플랫폼 독립적인 API기 때문에 우리는 윈도우 시스템과 상호작용하기위한 확장이 필요합니다. GLFW에는 여기에 필요한 확장을 반환해주는 편리한 함수가 있어서 이 것을 구조체에 전달해 줍니다:

```c++
uint32_t glfwExtensionCount = 0;
const char** glfwExtensions;

glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

createInfo.enabledExtensionCount = glfwExtensionCount;
createInfo.ppEnabledExtensionNames = glfwExtensions;
```

구조체의 마지막 두 멤버가 활성화할 전역 검증 레이어를 결정합니다. 다음 챕터에서 자세히 설명할 것이니 지금은 그냥 비워 둡시다.

```c++
createInfo.enabledLayerCount = 0;
```

이제 Vulkan이 인스턴스를 생성하기 위한 모든 것들을 명시했으니 `vkCreateInstance`를 호출할 수 있습니다:

```c++
VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
```

앞으로도 보시겠지만 Vulkan의 객체 생성 함수의 파라메터들의 일반적인 패턴은 아래와 같습니다:

- 생성 정보에 관한 구조체를 가리키는 포인터
- 생성자에 대한 사용자 정의 콜백을 가리키는 포인터. 튜토리얼에서는 항상 `nullptr`
- 새로운 객체의 핸들을 저장하기 위한 변수의 포인터

문제 없이 잘 동작했다면 `VkInstance` 클래스 멤버에 인스턴스의 핸들이 저장될 것입니다. 거의 대부분 Vulkan의 함수는 `VkResult` 타입의 값을 반환하는데 그 값은 `VK_SUCCESS`이거나 오류 코드입니다. 인스턴스가 성공적으로 생성되었다면, 결과값을 저장할 필요는 없고 그냥 성공 여부만 체크하면 됩니다.

```c++
if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
}
```

이제 프로그램을 실행해 인스턴스가 잘 생성되었는지 확인하십시오.

## VK_ERROR_INCOMPATIBLE_DRIVER 오류에 맞닥뜨린다면:

최신 MoltenVK SDK를 MacOS에서 사용중이하면 `vkCreateInstance`로부터 `VK_ERROR_INCOMPATIBLE_DRIVER`가 반환될 수 있습니다. [Getting Start Notes](https://vulkan.lunarg.com/doc/sdk/1.3.216.0/mac/getting_started.html)를 살펴보십시오. 1.3.216 Vulkan SDK부터는 `VK_KHR_PORTABILITY_subset` 확장이 필수적입니다.

오류를 해결하기 위해서는 먼저 `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR` 비트를 `VkInstanceCreateInfo` 구조체 플래그에 추가하고, `VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME`를 인스턴스의 확장 리스트에 추가하십시오.

코드는 보통 아래와 같이 될 겁니다:

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

## 확장 지원 체크하기

`vkCreateInstance`문서를 보면 `VK_ERROR_EXTENSION_NOT_PRESENT` 오류 코드가 반환될 수 있다는 것을 알 수 있습니다. We could simply
specify the extensions we require and terminate if that error code comes back.
That makes sense for essential extensions like the window system interface, but
what if we want to check for optional functionality?

인스턴스를 생성하기 전에 지원하는 확장들의 리스트를 얻고 싶으면 `vkEnumerateInstanceExtensionProperties`를 사용하면 됩니다. 확장의 개수를 저장할 변수의 포인터와 확장의 상세 사항을 저장할 `VkExtensionProperties` 배열을 매개변수로 받습니다. 선택적으로 첫 번째 파라메터로 특정한 검증 레이어로 필터링하도록 할 수 있는데, 지금은 무시해도 됩니다.

확장의 세부 사항을 저장할 배열을 할당하려면 먼저 몇 개나 있는지 알아야 합니다. 그냥 마지막 매개변수를 빈 채로 먼저 확장의 개수만 요청합니다:

```c++
uint32_t extensionCount = 0;
vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
```

(`include <vector>`를 추가하고) 확장의 세부 사항들을 저장할 배열을 생성합니다.

```c++
std::vector<VkExtensionProperties> extensions(extensionCount);
```

마지막으로 확장의 세부 사항들을 요청합니다:

```c++
vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
```

`VkExtensionProperties`구조체 각각은 확장의 이름과 버전을 담고 있습니다. 간단한 for문을 사용해 목록을 순회할 수 있습니다. (`\t`는 들여쓰기를 위한 탭 문자입니다)

```c++
std::cout << "available extensions:\n";

for (const auto& extension : extensions) {
    std::cout << '\t' << extension.extensionName << '\n';
}
```

이 코드를 `createInstance`에 추가하면 Vulkan 지원에 대한 세부 사항을 알 수 있습니다. 문제를 하나 내 드리면, `glfwGetRequiredInstanceExtensions`가 반환한 확장들이 모두 지원되는지를 확인해 보세요.

## 정리하기

`VkInstance`는 프로그램 종료 전에 제거되어야 합니다. `cleanup`에서 `vkDestroyInstance` 함수를 통해 제거할 수 있습니다.

```c++
void cleanup() {
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}
```

`vkDestroyInstance` 함수의 매개변수는 명확히 이해가 되실겁니다. 이전 장에서 이야기한 것처럼, Vulkan의 할당과 해제 함수에 추가적으로 콜백을 전달할 수 있는데 `nullptr`로 둔 상태입니다. 앞으로 모든 챕터에서 우리가 생성할 Vulkan 리소스들은 인스턴스가 해제되기 전에 정리되어야 합니다.

인스턴스 생성 이후 보다 복잡한 과정을 살펴보기 전에, [검증 레이어](!kr/Drawing_a_triangle/Setup/Validation_layers)를 통해 디버깅 옵션을 살펴보겠습니다.

[C++ code](/code/01_instance_creation.cpp)
