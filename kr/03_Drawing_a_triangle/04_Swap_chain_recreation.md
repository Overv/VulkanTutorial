## 개요

지금까지 만든 프로그램으로 성공적으로 삼각형을 그렸지만 아직 잘 처리하지 못하는 상황이 있습니다. 윈도우 표면이 변경되어 스왑 체인이 더이상 호환되지 않을 때 입니다. 이러한 상황이 발생하는 이유 중 하나로 윈도우의 크기가 변하는 경우가 있습니다. 이러한 이벤트를 탐지하여 스왑 체인을 새로 만들어야만 합니다.

## 스왑 체인 재생성

`recreateSwapChain` 함수를 새로 만드는데 이 함수는 `createSwapChain` 함수 및 스왑 체인, 그리고 윈도우 크기와 관련한 모든 객체를 만드는 함수를 호출하도록 할 것입니다. 

```c++
void recreateSwapChain() {
    vkDeviceWaitIdle(device);

    createSwapChain();
    createImageViews();
    createFramebuffers();
}
```

먼저 `vkDeviceWaitIdle`를 호출하는데 이전 장에서처럼 이미 사용 중인 자원을 건드리면 안되기 때문입니다. 그리고 당연히 스왑 체인은 새로 만들어야 하고요. 이미지 뷰는 스왑 체인의 이미지와 직접적으로 관련되어 있기 때문에 다시 만들어야 합니다. 하지막으로 프레임버퍼도 스왑 체인 이미지와 직접적으로 관련되어 있으니 역시나 마찬가지로 다시 만들어 주어야 합니다.

이러한 객체들의 이전 버전은 모두 재생성 되기 전에 정리되어야 하는데, 이를 확실히 하기 위해 정리 코드의 몇 부분을 변도의 함수로 만들어 `recreateSwapChain` 함수에서 호출 가능하도록 할 것입니다. 이 함수는 `cleanupSwapChain`로 명명합시다:

```c++
void cleanupSwapChain() {

}

void recreateSwapChain() {
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createFramebuffers();
}
```

여기서는 간략화 해서 렌더패스는 재생성하지 않았습니다. 이론적으로는 응용 프로그램의 실행 동안 스왑 체인 이미지의 포맷도 바뀔 수 있습니다. 예를 들어 윈도우를 일반적인 모니터에서 HDR 모니터로 이동한다거나 하는 등을 생각해 볼 수 있습니다. 이러한 경우 응용 프로램에서 HDR로의 변경이 적절히 적용되도록 렌더패스 재생성도 필요할 수 있습니다.

새로 만들어진 객체들의 정리 코드는 `cleanup`에서 `cleanupSwapChain`로 옮깁니다:

```c++
void cleanupSwapChain() {
    for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
    }

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void cleanup() {
    cleanupSwapChain();

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}
```

`chooseSwapExtent`에서 이미 새로운 윈도우의 해상도를 질의해서 스왑 체인 이미지가 (새로운) 윈도우에 적합한 크기가 되도록 했다는 것에 주목하십시오. 따라서 `chooseSwapExtent`를 수정할 필요는 없습니다(`glfwGetFramebufferSize`를 사용해서 스왑 체인 생성 시점에 픽셀 단위의 표면 해상도를 얻어왔다는 것을 기억하세요).

이로써 스왑 체인을 재생성하는 부분은 끝입니다! 하지만 이러한 접근법의 단점은 새로운 스왑 체인이 생성될 때까지 모든 렌더링이 중단된다는 것입니다. 이전 스왑 체인이 사용되는 동안에 그리기가 수행되는 동안에 대 스왑 체인을 만드는 것도 가능합니다. 그러려면 `VkSwapchainCreateInfoKHR` 구조체의 `oldSwapChain` 필드에 이전 스왑 체인을 전달하고 사용이 끝난 뒤 소멸시키면 됩니다.

## 최적화되지 않았거나 부적합한 스왑 체인

이제 언제 스왑 체인 재생성이 필요한지 알아내서 `recreateSwapChain` 함수를 호출하면 됩니다. 다행히 Vulkan은 대개 표시 단계에서 현재 스왑 체인이 적합하지 않게 된 시점에 이러한 것을 알려 줍니다. `vkAcquireNextImageKHR`와 `vkQueuePresentKHR` 함수는 아래와 같은 특정한 값으로 이러한 상황을 알려줍니다.

* `VK_ERROR_OUT_OF_DATE_KHR`: 스왑 체인이 표면과 호환이 불가능하여 렌더링이 불가능하게 되었음. 일반적으로 윈도우의 크기가 변했을 때 발생
* `VK_SUBOPTIMAL_KHR`: 스왑 체인이 표면을 표현하는 데 여전히 사용 가능하지만 표면 속성이 정확히 일치하지는 않음

```c++
VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
}
```

이미지를 획득하려 할 때 스왑체인이 부적합하다고 판단되면 그 이미지는 표현에 활용할 수 없습니다. 따라서 즉시 스왑 체인을 재생성하고 다음 `drawFrame`을 다시 호출해야 합니다.

스왑 체인이 최적화되지 않은 경우에도 이렇게 하도록 할 수도 있지만 저는 이 경우에는 어쨌든 이미지를 이미 획득했기 때문에 그냥 진행하기로 했습니다. `VK_SUCCESS`와 `VK_SUBOPTIMAL_KHR`는 모두 "성공" 반환 코드로 취급합니다.

```c++
result = vkQueuePresentKHR(presentQueue, &presentInfo);

if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    recreateSwapChain();
} else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
}

currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
```

`vkQueuePresentKHR` 함수는 위와 같은 의미를 가진 같은 값들을 반환합니다. 이 경우에는 최적화되지 않은 경우에도 스왑 체인을 재생성하는데 가능한 좋은 결과를 얻고 싶기 때문입니다.

## 데드락(deadlock) 해소

지금 시점에서 코드를 실행하면 데드락이 발생할 수 있습니다. 코드를 디버깅해보면 `vkWaitForFences`에는 도달하지만 여기에서 더 이상 진행하지 못하는 것을 볼 수 있습니다. 이는 `vkAcquireNextImageKHR`이 `VK_ERROR_OUT_OF_DATE_KHR`을 반환하면 스왑체인을 재생성하고 `drawFrame`로 돌아가게 했기 때문입니다. 하지만 그러한 처리는 현재 프레임의 펜스가 기다리는 상태에서 일어날 수 있습니다. 바로 반환되는 바람에 아무런 작업도 제출되지 않았고 펜스는 시그널 상태가 될 수 없어서 `vkWaitForFences`에서 멈춘 상태가 됩니다.

다행히 손쉬운 해결법이 있습니다. 작업을 다시 제출할 것이 확실한 시점까지 펜스를 리셋하는 것을 미루는 것입니다. 이렇게 되면 빠른 반환이 일어났을 때 펜스는 여전히 시그널 상태이고 `vkWaitForFences`는 다음 프레임에서 데드락이 발생하지 않을 것입니다.

`drawFrame`의 시작 부분으 다음과 같이 되어야 합니다
:
```c++
vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

uint32_t imageIndex;
VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
}

// 작업을 제출하는 경우에만 펜스를 리셋
vkResetFences(device, 1, &inFlightFences[currentFrame]);
```

## 크기 변환의 명시적 처리

윈도우 크기 변환에 대해 많은 드라이버와 플랫폼이 `VK_ERROR_OUT_OF_DATE_KHR`를 자동으로 반환해주지만, 이러한 동작이 보장된 것은 아닙니다. 따라서 추가적인 코드를 통해 크기 변환을 명시적으로 처리해 주도록 하겠습니다. 먼저 크기 변환이 일어났을 때를 위한 플래그를 멤버 변수로 추가합니다:

```c++
std::vector<VkFence> inFlightFences;

bool framebufferResized = false;
```

`drawFrame`함수에서도 이 플래그를 체크하도록 수정합니다:

```c++
if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
    framebufferResized = false;
    recreateSwapChain();
} else if (result != VK_SUCCESS) {
    ...
}
```

이러한 작업을 `vkQueuePresentKHR` 뒤에 진행해서 세마포어가 적합상 상태에 있도록 하는 것이 중요합니다. 그렇지 않으면 시그널 상태인 세마포어가 제대로 대기를 하지 못할 수 있습니다. 이제 실제 크기 변경을 탐지하기 위해 GLFW 프레임워크의 `glfwSetFramebufferSizeCallback` 함수를 사용하여 콜백을 설정합니다:

```c++
void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {

}
```

콜백을 `static` 함수로 만드는 이유는 GLFW가 `HelloTriangleApplication` 인스턴스를 가리키는 `this` 포인터로부터 올바른 멤버 함수를 호출하는 법을 알 수 없기 때문입니다.

하지만 콜백 내에서 `GLFWwindow`에 대한 참조에 접근할 수 있고, 임의의 포인터를 그 안에 저장할 수 있는 `glfwSetWindowUserPointer`라는 GLFW 함수가 있습니다:

```c++
window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
glfwSetWindowUserPointer(window, this);
glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
```

이 값은 이제 콜백 내에서 `glfwGetWindowUserPointer`를 사용해 적절히 변환된 뒤 올바른 플래그 설정을 위해 사용됩니다:

```c++
static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}
```

이제 프로그램을 실행하고 윈도우 크기를 조정하여 프레임버퍼가 윈도우 크기에 맞게 조정되는지 살펴 보세요.

## 최소화 처리

스왑 체인이 부적합하게 되는 다른 또다른 경우로 특수한 윈도우 크기 변경 사례가 있습니다. 바로 윈도우 최소화 입니다. 이 경우가 특수한 이유는 프레임버퍼 크기가 `0`이 되기 떄문입니다. 이 튜토리얼에서는 이러한 경우에 대해 윈도우가 다시 활성화가 될때까지 정지하는 방식으로 처리할 것입니다. `recreateSwapChain` 함수를 사용합니다:

```c++
void recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    ...
}
```

처음의 `glfwGetFramebufferSize` 호출은 올바른 크기일 경우에 대한 것으로 이 경우 `glfwWaitEvents`는 기다릴 것이 없습니다.

축하합니다! 이제 올바로 동작하는 것 Vulkan 프로그램을 완성했습니다! 다음 챕터에서는 정점 셰이더에 하드코딩된 정점을 제거하고 정점 버퍼(vertex buffer)를 사용해 볼 것입니다.

[C++ code](/code/17_swap_chain_recreation.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
