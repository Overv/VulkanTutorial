## 일반적인 구조

이전 챕터에서 Vulkan 프로젝트를 만들고 필요한 설정들을 하고 샘플 코드를 통해 테스트 해 봤습니다. 이 챕터에서는 아래 코드로부터 처음부터 시작해 보겠습니다:

```c++
#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

class HelloTriangleApplication {
public:
    void run() {
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    void initVulkan() {

    }

    void mainLoop() {

    }

    void cleanup() {

    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

먼저 LunarG SDK의 Vulkan 헤더를 include합니다. 이 헤더는 함수, 구조체와 열거자들을 제공해 줍니다. `stdexcept`와 `iostream` 헤더는 오류를 보고하고 전파하기 위해 include하였습니다. `cstdlib`은 `EXIT_SUCCESS`와 `EXIT_FAILURE` 매크로를 제공합니다.

프로그램은 클래스로 래핑되어 있는데 Vulkan 객체들을 프라이빗 클래스 멤버로 저장할 것이며, 그 각각을 초기화하는 함수를 추가할 것입니다. 초기화 함수는 `initVulkan` 함수 안에서 호출할 것입니다. 모든 것들이 준비가 되고 나면 메인 루프로 들어가 프레임을 렌더링하기 시작합니다. `mainLoop` 함수 본문을 작성해서 루프를 추가하고 윈도우가 닫히기 전까지 반복하도록 할 것입니다. 윈도우가 닫히고 `mainLoop`가 반환되면, 사용한 리소스들을 `cleanup` 함수를 통해 해제(deallocate)할 것입니다.

실행 도중 치명적인 오류가 발생하면 `std::runtime_error` 예외를 메지시와 함께 throw할 것인데, 이는 `main`함수로 전파되어 명령 창에 출력될 것입니다. 여러 가지 표준 예외 타입들을 다루기 위해 좀 더 일반적인 `std::exception`을 catch하도록 했습니다. 곧 마주하게 될 오류의 한 예는 필요한 특정 확장이 지원되지 않는 경우가 있습니다.

다음 챕터부터는 대부분 `initVulkan` 함수 안에서 호출할 하나의 새로운 함수를 추가하고 하나 이상의 Vulkan 객체를 프라이빗 클래스 멤버로 추가할 것입니다. 해당 객체는 마지막에 `cleanup`을 통해 해제되어야 합니다.

## 리소스 관리

`malloc`을 통해 할당된 메모리는 `free`되어야 하듯이, 모든 우리가 만든 Vulkan 객체는 더 이상 필요하지 않은 시점에서는 명시적으로 소멸되어야 합니다. C++에서는 [RAII](https://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization)를 통해 자동으로 리소스를 관리하거나 `<memory>` 헤더를 통해 제공되는 스마트 포인터를 사용할 수 있습니다. 하지만 이 튜토리얼에서는 Vulkan 객체의 할당과 해제를 직접 하는 방식을 선택했습니다. 결국 Vulkan의 특별한 점은 모든 실수를 피하기 위해 모든 작업들이 명시되어야 한다는 점이고, API의 동작 방식을 배우기 위해 객체의 생애주기도 명시적으로 나타내는 것이 좋다고 생각했습니다.

이 튜토리얼을 따라한 뒤에, 여러분은 Vulkan 객체를 생성자에서 획득하고 소멸자에서 해제하는 C++ 클래스를 만들어 자동적으로 리소스 관리를 하도록 할 수 있습니다. 또는 `std::unique_ptr`나 `std::shared_ptr`에 사용자 정의 deleter를 명시하여 사용할 수도 있습니다. 큰 규모의 Vulkan 프로그램에는 RAII 모델의 사용을 추천하지만, 학습 목적으로는 어떤 일이 발생하는지 알고 있는것이 더 좋습니다.

Vulkan 객체는 `vkCreateXXX`같은 함수를 사용해 직접 만들어지거나 `vkAllocateXXX`와 같은 함수로 다른 객체를 통해 할당될 수 있습니다. 객체가 더 이상 사용되지 않는 게 확실하면, 이와 대응되는 `vkDestroyXXX`와 `vkFreeXXX`를 사용해 소멸시켜야 합니다. 이 함수들의 매개변수는 객체의 타입에 따라 다른데, 모든 함수들이 공유하는 매개변수가 하나 있습니다. `pAllocator`입니다. 이는 사용자 정의 메모리 할당자를 위한 콜백을 명시할 수 있도록 하는 선택적인 매개변수입니다. 튜토리얼에서 이 매개변수는 무시할 것이고, 항상 `nullptr`을 인자로 넘길 것입니다.

## GLFW 통합하기

오프스크린 렌더링을 하려는 목적이면 윈도우 없이도 Vulkan은 완벽하게 동작하지만, 실제로 무언가를 보여주는 것이 훨씬 재미있겠죠! 먼저 `#include <vulkan/vulkan.h>`를 아래 코드로 대체하십시오.

```c++
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
```

이렇게 하면 GLFW는 자신에게 필요한 definition들과 Vulkan 헤더를 자동으로 include할 것입니다. `initWindow` 함수를 추가하고 `run`함수에 이 함수를 호출하는 라인을 다른 함수 호출에 앞서 삽입하세요. 이 함수를 사용해 GLFW를 초기화하고 윈도우를 생성할 것입니다.

```c++
void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

private:
    void initWindow() {

    }
```

`initWindow`에서 가장 먼저 호출하는 것은 `glfwInit()`이어야 합니다. 이 함수는 GLFW 라이브러리를 초기화합니다. GLFW는 원래 OpenGL 컨텍스트를 생성하게 되어있기 때문에, 이어지는 코드를 통해 OpenGL 컨텍스트를 생성하지 않도록 알려주어야 합니다:

```c++
glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
```

나중에 살펴볼 것이지만 윈도우 크기가 변하면 몇 가지 특수한 처리를 해 주어야 하기 때문에 지금은 윈도우 힌트를 추가적으로 호출하여 그 기능을 꺼 둡니다:

```c++
glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
```

이제 남은 것은 실제 윈도우를 만드는 것입니다. `GLFWwindow* window;` 프라이빗 클래스 멤버를 추가하여 윈도우의 참조를 저장하도록 하고 윈도우를 아래와 같이 초기화합니다:

```c++
window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
```

앞의 세 매개변수는 윈도우의 가로, 세로, 타이틀을 명시합니다. 네 번째 매개변수를 통해 윈도우를 열 모니터를 명시할 수 있고, 다섯 번째 매개변수는 OpenGL을 사용할 때만 필요합니다.

가로와 세로 크기를 하드코딩하는 대신 상수를 사용하는 것이 좋겠네요. 나중에 몇 번 해당 값을 참조하는 일이 있을 겁니다. 아래 코드를 `HelloTriangleApplication` 클래스 정의 앞쪽에 추가하였습니다:

```c++
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
```

그리고 윈도우 생성 호출을 아래와 같이 바꾸었습니다:

```c++
window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
```

이제 `initWindow` 함수는 아래와 같은 상태입니다:

```c++
void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}
```

오류가 발생하거나 윈도우가 닫힐 때까지 응용 프로그램이 계속 실행되게 하려면 `mainLoop` 함수에 이벤트 루프를 아래와 같이 추가해야 합니다:

```c++
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}
```

보기만 해도 이해가 되실 겁니다. 반복문을 돌면서 사용자가 윈도우를 닫기 위해 X 버튼을 눌렀는지와 같은 이벤트를 체크합니다. 이 루프가 나중에 프레임을 렌더링하기 위한 함수들을 호출하는 부분이 될겁니다.

윈도우가 닫히면, 리소스를 소멸시켜 정리하고 GLFW도 종료해야 합니다. 아래는 `cleanup` 함수의 첫 단계 코드입니다.

```c++
void cleanup() {
    glfwDestroyWindow(window);

    glfwTerminate();
}
```

프로그램을 실행하면 `Vulkan`이라는 이름의 윈도우가 보이고 윈도우를 닫기 전까지 응용 프로그램의 실행 상태가 유지될 것입니다. Vulkan 응용 프로그램의 뼈대를 만들었으니, 이제 [첫 번째 Vulkan 객체를 만들어 봅시다](!kr/Drawing_a_triangle/Setup/Instance)!

[C++ code](/code/00_base_code.cpp)
