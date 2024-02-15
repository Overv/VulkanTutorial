
이제 모든 것이 결합되는 챕터입니다. 메인 루프에서 실행되어 삼각형을 화면에 표시하는 `drawFrame`함수를 작성할 것입니다. 이 함수를 만들고 `mainLoop`에서 호출하도록 합시다:

```c++
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
}

...

void drawFrame() {

}
```

## 프레임 개요

큰 그림에서 보자면, Vulkan에서 프레임을 하나 렌더링하는 것은 다음 단계들로 이루어집니다:

* 이전 프레임이 끝나기까지 대기
* 스왑 체인에서 이미지 얻어오기
* 그 이미지에 장면을 드로우하기 위한 명령 버퍼 기록
* 기록된 명령 버퍼 제출
* 스왑 체인 이미지 표시

챕터를 진행하면서 드로우 함수를 확장할 것이지만, 지금은 이 정도가 렌더링 루프의 핵심이라 보시면 됩니다.

<!-- Add an image that shows an outline of the frame -->

## 동기화(Synchronization)

<!-- Maybe add images for showing synchronization -->

(*역주: 여기서 '동기화'는 동시에 실행한다는 의미가 아닌 올바른 실행 스케줄(순서)를 유지한다는 의미*) Vulkan의 핵심 설계 철학은 GPU에서의 실행 동기화가 명시적이라는 겁니다. 연산의 순서는 우리가 다양한 동기화 요소들을 가지고 정의하는 것이고, 이를 통해 드라이버는 원하는 실행 순서를 파악합니다. 이 말은 많은 Vulkan API 호출이 GPU에서 실제 동작하는 시기는 비동기적이고, 실제 연산이 끝나기 전에 함수가 종료된다는 뜻입니다.

이 챕터에서는 여러 이벤트들에 대해 순서를 명시적으로 지정해야 할 필요가 있습니다. 이러한 이벤트들이 GPU에서 일어나기 때문인데 그 예로:

* 스왑 체인에서 이미지 얻어오기
* 그 이미지를 드로우하기 위한 명령 실행
* 표시를 위해 이미지를 화면에 나타내고 스왑체임으로 다시 반환

이러한 이벤트들은 단일 함수 호출로 동작하지만 실제 실행은 비동기적으로 이루어집니다.
실제 연산이 끝나기 전에 함수가 반환되고 실행 순서도 정의되어 있지 않습니다. 각 연산이 이전 연산에 종속적이기 때문에 이렇게 되면 안됩니다. 따라서 원하는 순서대로 실행이 될 수 있도록 하게 해 주는 요소들을 살펴볼 것입니다.

### 세마포어(Semaphores)

큐 연산들 사이에 순서를 추가하기 위해 세마포어를 사용할 수 있습니다. 여기서 큐 연산은 우리가 큐에 제출한 작업들이며 명령 버퍼 내의 연산이거나 나중에 볼 함수의 연산들입니다.
큐의 예시로는 그래픽스 큐와 표시 큐가 있습니다. 세마포어는 동일 큐 내에서, 그리고 서로 다른 큐 사이에서 순서를 정하기 위해 사용됩니다.

Vulkan에는 바이너리와 타임라인(timeline) 세마포어가 있습니다. 이 튜토리얼에서는 바이너리 세마포어만 사용할 것이고 타임라인 세마포어에 대해서는 논의하지 않을 것입니다. 이후에 세마포어라고 한다면 바이너리 세마포어를 의미하는 것입니다.

세마포어는 시그널 상태(signaled)이거나 시그널이 아닌 상태(unsignaled)일 수 있습니다. 일단 시그널이 아닌 상태로 시작됩니다. 우리가 세마포어를 사용하는 방식은 우선 동일한 세마포어를 한 쪽 큐 연산에는 '시그널(signal)' 세마포어로, 다른 쪽에는 '대기(wait)' 세마포어로 사용하는 것입니다. 예를 들어 세마포어 S가 있고 큐 연산 A와 B가 있다고 합시다. Vulkan에게 우리가 알려주는 것은 실행이 끝나면 연산 A 세마포어 S를 '시그널'하도록 하고, 연산 B는 실행 전에 세마포어 S를 '대기'하도록 하는 것입니다. 연산 A가 끝나면 세마포어 S가 시그널 상태가 될 것인데, 연산 B는 S가 시그널 상태가 될때까지는 실행되지 않습니다. 연산 B가 실행이 시작되면 세마포어 S는 자동적으로 시그널이 아닌 상태로 돌아오고 다시 사용 가능한 상태가 됩니다.

방금 설명한 내용을 의사 코드로 표현하자면 다음과 같습니다:
```
VkCommandBuffer A, B = ... // 명령 버퍼 기록
VkSemaphore S = ... // 세마포어 생성

// A를 큐에 등록하고 끝나면 S를 '시그널'하도록 함 - 즉시 실행 시작
vkQueueSubmit(work: A, signal: S, wait: None)

// B를 큐에 등록하고 시작하기 위해 S를 기다림
vkQueueSubmit(work: B, signal: None, wait: S)
```

주의하셔야 할것은 위 코드에서 두 번의 `vkQueueSubmit()` 호출은 즉시 반환된다는 것입니다. 대기 과정은 GPU에서만 일어납니다. CPU는 블러킹(blocking)없이 계속 실행합니다. CPU가 대기하도록 하려면 다른 동기화 요소가 필요합니다.

### 펜스(Fences)

펜스도 비슷한 목적을 가지고 있습니다. 동일하게 동기화 실행을 위해 사용되며 CPU(호스트라고도 함)에서의 순차적 실행이 목적이라는 것만 다릅니다. 간단히 말해 호스트가 GPU가 어떤 작업을 끝냈다는 것을 알아야만 하는 상황에서 펜스를 사용합니다.

세마포어와 유사하게 펜스도 시그널 상태와 시그널이 아닌 상태를 가집니다. 작업 실행을 제출할 때 해당 작업에 펜스를 부착할 수 있습니다. 작업이 끝나면 펜스는 시그널 상태가 됩니다. 그리고 호스트는 펜스가 시그널 상태가 될때까지 기다리게 하면 호스트가 작업이 끝난 뒤에야만 진행되도록 보장할 수 있습니다.

구체적인 예시로 스크린샷을 찍는 예시가 있습니다. 예를들어 GPU에서 필요한 작업을 이미 수행했다고 합시다. 이제 이미지를 GPU에서부터 호스트로 전송하고 메모리를 파일로 저장해야 합니다. 전송을 위한 명령 버퍼 A와 펜스 F가 있다고 합시다. 명령 버퍼 A를 펜스 F와 함께 제출하고 호스트에게 F가 시그널 상태가 될때까지 기다리게 합니다. 이렇게 하면 호스트는 명령 버퍼가 실행을 끝낼 때까지 블러킹 상태가 됩니다. 따라서 안전하게 메모리 전송이 끝난 뒤 디스크에 파일을 저장할 수 있습니다.

방금 설명한 내용을 의사 코드로 표현하자면 다음과 같습니다:
```
VkCommandBuffer A = ... // 전송을 위한 명령 버퍼 기록
VkFence F = ... // 펜스 생성

// A를 큐에 등록하고 즉시 실행을 시작하며, 끝나면 F를 시그널 상태로 만듬
vkQueueSubmit(work: A, fence: F)

vkWaitForFence(F) // A의 실행이 끝날때 까지 실행 중단(블럭)

save_screenshot_to_disk() // 전송이 끝날 때까지 싱행이 불가능
```

세마포어 예시와는 달리 이 예시는 호스트의 실행을 *블럭*합니다. 즉 모든 연산이 끝날때까지 호스트는 아무것도 하지 않는다는 뜻입니다. 이 경우에는 스크린샷을 디스크에 저장하기 전까지 전송이 끝나야만 한다는 것을 보장해야 하기 때문입니다.

일반적으로 꼭 필요한 경우가 아니라면 호스트를 블럭하지 않는것이 좋습니다. GPU에 전달하고 난 뒤 호스트는 다른 유용한 작업을 하는 것이 좋습니다. 펜스가 시그널 상태가 되는 것을 기다리는 것은 유용한 작업이 아니죠. 따라서 작업의 동기화를 위해서는 세마포어를 사용하거나, 아직 다루지 않은 다른 동기화 방법을 사용해야 합니다.

펜스의 경우 시그널이 아닌 상태로 되돌리는 작업은 매뉴얼하게 수행해 주어야 합니다. 펜스는 호스트의 실행을 제어하기 위해 사용되는 것이고 호스트가 펜스를 되돌리는 시점을 결정하게 됩니다. 이와는 달리 세마포어는 호스트와 상관없이 GPU 내의 작업 순서를 결정하기 위해 사용됩니다.

정리하자면, 세마포어는 GPU에서의 실행 순서를 명시하기 위해 사용하고 펜스는 CPU와 GPU간의 동기화를 위해 사용한다는 것입니다.

### 어떻게 결정해야 하나요?

사용할 수 있는 두 종류의 동기화 요소가 있고 마침 동기화가 필요한 두 과정이 존재합니다.
스왑체인 연산과 이전 프레임이 끝나기를 기다리는 과정입니다. 스왑체인 연산에 대해서는 세마포어를 사용할 것인데 이는 GPU에서 수행되는 작업이고, 호스트가 그 동안 기다리는 것을 원치 않기 때문입니다. 이전 프레임이 끝나기를 기다리는 것은 반대의 이유로 펜스를 사용할 것인데 호스트가 기다려야 하기 때문입니다. 그리고 기다려야 하는 이유는 한번에 하나 이상의 프레임이 그려지는 것을 원치 않기 때문입니다. 매 프레임마다 명령 버퍼를 다시 기록하기 때문에 다음 프레임을 위한 작업을 현재 프레임의 실행이 끝나기 전에 기록할 수 없습니다. 만일 그렇게 하게 되면 GPU가 명령을 실행하는 동안 명령 버퍼가 덮어쓰여지게 됩니다.

## 동기화 객체의 생성

스왑체인으로부터 이미지가 획득되어 렌더링할 준비가 되었는지에 대한 세마포어 하나와 렌더링이 끝나서 표시할 준비가 되었다는 세마포어 하나가 필요합니다. 또한 한 번에 하나의 프레임만 렌더링하기 위한 펜스 하나가 필요합니다.

이 세마포어와 펜스 객체를 저장하기 위한 클래스 멤버를 만듭니다:

```c++
VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
VkFence inFlightFence;
```

세마포어를 만들기 위해 이 장의 마지막 `create`함수인 `createSyncObjects`를 추가합니다:

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
}

...

void createSyncObjects() {

}
```

세마포어 생성을 위해서는 `VkSemaphoreCreateInfo`를 채워야 하는데 현재 API 버전에서는 `sType` 이외의 다른 필드는 요구하지 않습니다:

```c++
void createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
}
```

나중 버전의 Vulkan API나 확장에서는 다른 구조체처럼 `flags`와 `pNext` 기능이 추가될 수 있습니다.

펜스를 만들기 위해서는 `VkFenceCreateInfo`를 채워야 합니다:

```c++
VkFenceCreateInfo fenceInfo{};
fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
```

세마포어와 펜스를 만드는 것은 `vkCreateSemaphore` & `vkCreateFence`를 사용하는 기존과 비슷한 패턴입니다:

```c++
if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
    vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
    vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
    throw std::runtime_error("failed to create semaphores!");
}
```

프로그램의 종료 시점에 세마포어와 펜스는 정리되어야 하며 이는 모든 명령이 끝나고 더이상 동기화가 필요하지 않은 시점입니다:

```c++
void cleanup() {
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);
```

메인 그리기 함수로 가 봅시다!

## 이전 프레임 기다리기

프레임 시작 시점에 이전 프레임이 끝나기를 기다려야 하므로 명령 버퍼와 세마포어(*역주: 펜스일 듯*)가 사용 가능해야 합니다. 이를 위해 `vkWaitForFences`를 호출합니다.

```c++
void drawFrame() {
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
}
```

`vkWaitForFences` 함수는 펜스 배열을 받아서, 몇 개 또는 전체 펜스들이 시그널인 상태를 반환할 때까지 호스트를 대기하도록 합니다. 인자로 넘긴 `VK_TRUE`는 모든 펜스를 기다리겠다는 의미인데 지금은 하나의 펜스만 있으므로 크게 의미는 없습니다. 이 함수는 또한 타임아웃(timeout) 매개변수를 갖는데 `UINT64_MAX`를 통해 64비트 부호없는 정수의 최대값으로 설정했습니다. 즉 타임아웃을 사용하지 않겠다는 의미입니다.

대기 후에 `vkResetFences` 호출을 통해 펜스를 시그널이 아닌 상태로 리셋해 주어야 합니다:

```c++
    vkResetFences(device, 1, &inFlightFence);
```

더 진행하기 전에 현재 설계에 약간의 문제가 있습니다. 첫 프레임에 `drawFrame()`를 호출하기 때문에 바로 `inFlightFence`가 시그널 상태가 되도록 기다립니다. `inFlightFence`는 프레임 렌더링이 끝나야 시그널 상태가 되는데 지금은 첫 번째 프레임이므로 펜스를 시그널 상태로 만들어줄 이전 프레임이 없습니다! 따라서 `vkWaitForFences()`가 프로세스를 무한정 블럭해서 아무 일도 일어나지 않을 것입니다.

이 문제를 해결하는 많은 방법 중 API에 들어있는 똑똑한 해결책이 하나 있습니다. 펜스를 시그널인 상태로 생성해서 `vkWaitForFences()`의 첫 호출이 바로 반환되도록 하는 것입니다.

이렇게 하기 위해서 `VK_FENCE_CREATE_SIGNALED_BIT` 플래그를 `VkFenceCreateInfo`에 추가합니다:

```c++
void createSyncObjects() {
    ...

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    ...
}
```

## 스왑 체인에서 이미지 얻어오기

다음으로 `drawFrame` 함수에서 할 일은 스왑 체인으로부터 이미지를 얻어오는 것입니다. 스왑 체인은 확장 기능이므로 `vk*KHR` 네이밍으로 되어 있는 함수를 사용해야 하는 것을 잊지 마세요.

```c++
void drawFrame() {
    ...

    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
}
```

`vkAcquireNextImageKHR`의 첫 두 매개변수는 논리적 장치와 이미지를 얻어오려고 하는 스왑 체인입니다. 세 번째 매개변수는 이미지가 가용할때까지의 나노초 단위 타임아웃 시간입니다. 64비트의 부호없는 정주의 최대값을 사용했고, 그 의미는 타임아웃을 적용하지 않겠다는 의미입니다.

다음 두 매개변수는 표시 엔진이 이미지 사용이 끝나면 시그널 상태로 변환될 동기화 객체들을 명시합니다. 그 시점이 바로 우리가 새로운 프레임을 드로우 할 시점입니다.
세마포어나 펜스, 또는 그 둘 다를 명시할 수 있습니다. 여기서는 `imageAvailableSemaphore`를 사용할 것입니다.

마지막 매개변수는 사용이 가능해진 스왑 체인 이미지의 인덱스를 출력할 변수입니다. 이 인덱스는 `swapChainImages` 배열의 `VkImage`의 인덱스입니다. 이 인덱스를 사용해 `VkFrameBuffer`를 선택할 것입니다.

## 명령 버퍼 기록

사용할 스왑 체인 이미지의 imageIndex를 얻었으면 이제 명령 버퍼를 기록할 수 있습니다. 먼저, `vkResetCommandBuffer`를 호출해 명령 버퍼가 기록이 가능한 상태가 되도록 합니다.

```c++
vkResetCommandBuffer(commandBuffer, 0);
```

`vkResetCommandBuffer`의 두 번째 매개변수는 `VkCommandBufferResetFlagBits` 플래그입니다. 특별히 무언가 작업을 하지는 않을 것이므로 0으로 두겠습니다.

이제 `recordCommandBuffer`를 호출하여 원하는 명령을 기록합니다.

```c++
recordCommandBuffer(commandBuffer, imageIndex);
```

기록이 완료된 명령 버퍼가 있으니 이제 제출할 수 있습니다.

## 명령 버퍼 제출

큐 제출과 동기화는 `VkSubmitInfo` 구조체의 매개변수들로 설정합니다.

```c++
VkSubmitInfo submitInfo{};
submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
submitInfo.waitSemaphoreCount = 1;
submitInfo.pWaitSemaphores = waitSemaphores;
submitInfo.pWaitDstStageMask = waitStages;
```

첫 세 개의 매개변수는 실행이 시작되기 전 대기할 세마포어, 그리고 파이프라인의 어떤 스테이지(stage)에서 대기할지를 명시합니다. 우리의 경우 색상 값을 이미지에 기록하는 동안 대기할 것이므로 색상 어태치먼트에 쓰기를 수행하는 스테이지를 명시하였습니다. 즉 이론적으로는 우리의 구현이 정점 셰이더 등등을 이미지가 가용하지 않은 상태에서 실행될 수 있다는 뜻입니다. `waitStages` 배열의 각 요소가 `pWaitSemaphores`의 동일한 인덱스와 대응됩니다.

```c++
submitInfo.commandBufferCount = 1;
submitInfo.pCommandBuffers = &commandBuffer;
```

다음 두 매개변수는 실제로 실행을 위해 어떤 명령 버퍼를 제출할 것인지를 명시합니다.
만들어둔 유일한 명령 버퍼를 제출합니다.

```c++
VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
submitInfo.signalSemaphoreCount = 1;
submitInfo.pSignalSemaphores = signalSemaphores;
```

`signalSemaphoreCount`와 `pSignalSemaphores` 매개변수는 명령 버퍼 실행이 끝나면 시그널 상태가 될 세마포어를 명시합니다. 우리의 경우 `renderFinishedSemaphore`를 사용합니다.

```c++
if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
}
```

이제 `vkQueueSubmit`를 사용해 명령 버퍼를 그래픽스 큐에 제출합니다. 이 함수는 `VkSubmitInfo` 구조체 배열을 받을 수 있는데 작업량이 많을 때는 이 방식이 효율적입니다. 마지막 매개변수는 펜스로 명령 버퍼 실행이 끝나면 시그널 상태가 됩니다. 이렇게 하면 언제 안전하게 명령 버퍼를 다시 사용할 수 있는 상태가 되는지를 알 수 있으므로 `inFlightFence`를 사용합니다. 이제 다음 프레임이 되면, CPU는 이번 명령 버퍼의 실행이 끝날때까지 대기하다가 새로룽 명령들을 기록하게 됩니다.

## 서브패스 종속성(dependencies)

렌더패스의 서브패스는 이미지 레이아웃의 전환을 자동적으로 처리해 준다는 사실을 기억하십시오. 이러한 전환은 *서브패스 종속성*에 의해 제어되는데, 서브패스간의 메모리와 실행 종속성을 명시합니다. 지금은 하나의 서브패스만 있는 상태지만 이 서브패스의 바로 이전과 이후의 연산 또한 암시적으로 "서브패스"로 간주됩니다.

렌더패스의 시작 시점과 끝 시점에 전환을 처리해주는 내장된 종속성이 있긴 합니다만, 시작 시점의 경우 올바른 시점에 제어되지 않습니다. 이는 전환이 파이프라인의 시작 시점에 일어난다고 가정하고 설계되었는데 우리의 경우 파이프라인 시작 시점에 이미지를 획득하지는 않은 상태입니다. 이러한 문제를 해결하기 위한 방법이 두 가지가 있습니다. `imageAvailableSemaphore`의 to `waitStages`를 `VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT`으로 바꾸어 이미지가 가용할 때까지 렌더패스를 시작하지 않는 방법이 있고, 렌더패스가 `VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT` 스테이지를 대기하도록 하는 방법이 있습니다. 저는 여기서 두 번째 방법을 선택했는데, 서브패스 종속성과 그 동작 방식을 살펴보는 데 좋기 때문입니다.

서브패스 종속성은 `VkSubpassDependency` 구조체에 명시됩니다. `createRenderPass` 함수에 라애 코드를 추가합니다:

```c++
VkSubpassDependency dependency{};
dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
dependency.dstSubpass = 0;
```

첫 두 필드는 의존(dependency)하는 서브패스와 종속(dependent)되는 서브패스를 명시합니다.(*역주: 의존=선행되어야 하는 서브패스, 종속=후행해야 하는 서브패스*) 특수한 값인 `VK_SUBPASS_EXTERNAL`은 `srcSubpass` 또는 `dstSubpass` 중 어디에 설정되었느냐에 따라 서브패스의 앞과 뒤에 오는 암시적인 서브패스를 명시하는 값입니다. `0` 인덱스는 우리의 첫 번째(그리고 유일한) 서브패스를 의미하는 인덱스입니다. 종속성 그래프의 사이클을 방지하기 위해 `dstSubpass`는 항상 `srcSubpass`보다 커야 합니다(둘 중 하나가 `VK_SUBPASS_EXTERNAL`가 아닌 경우에 해당).

```c++
dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependency.srcAccessMask = 0;
```

다음 두 개의 필드는 대기할 연산과 그 연산이 일어날 스테이지를 명시합니다. 접근하기 전에, 스왑 체인이 이미지를 읽기를 마칠 때까지 기다려야 합니다. 따라서 색상 어태치먼트 출력 자체를 기다리도록 하면 됩니다.

```c++
dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
```

이 서브패스를 기다려야 하는 연산은 컬러 어태치먼트 스테이지에 있고 컬러 어태치먼트에 값을 쓰는 연산과 관련되어 있습니다. 이렇게 설정하면 실제로 필요하고 허용되기 전까지는 이미지의 전환이 발생하지 않습니다.: when we want to start writing colors to it.

```c++
renderPassInfo.dependencyCount = 1;
renderPassInfo.pDependencies = &dependency;
```

`VkRenderPassCreateInfo` 구조체는 종속성의 배열을 명시하기 위한 두 개의 필드를 가지고 있습니다.

## 표시

프레임을 그리는 마지막 단계는 결과를 스왑체인에 다시 제출해서 화면에 표시되도록 하는 단계입니다. 표시는 `drawFrame` 함수 마지막에 `VkPresentInfoKHR` 구조체를 통해 설정됩니다.

```c++
VkPresentInfoKHR presentInfo{};
presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

presentInfo.waitSemaphoreCount = 1;
presentInfo.pWaitSemaphores = signalSemaphores;
```

첫 두 매개변수는 `VkSubmitInfo`처럼, 표시를 수행하기 전 어떤 세마포어를 기다릴지를 명시합니다. 우리는 명령 버퍼 실행이 끝나서 삼각형이 그려질 때까지 대기해야 하므로 그 때 시그널 상태가 되는 `signalSemaphores`를 사용합니다.

```c++
VkSwapchainKHR swapChains[] = {swapChain};
presentInfo.swapchainCount = 1;
presentInfo.pSwapchains = swapChains;
presentInfo.pImageIndices = &imageIndex;
```

다음 두 매개변수는 이미지를 표시할 스왑 체인과 각 스왑 체인의 이미지 인덱스를 명시합니다. 이는 거의 항상 한 개만 사용합니다.

```c++
presentInfo.pResults = nullptr; // Optional
```

마지막으로 추가적인 매개변수로 `pResults`가 있습니다. 여기에는 `VkResult`의 배열을 명시해서 각각의 스왑 체인에서 표시가 성공적으로 이루어졌는지를 체크합니다. 하나의 스왑 체인만 사용하는 경우 그냥 표시 함수의 반환값으로 확인하면 되기 때문에 현재는 사용하지 않습니다.

```c++
vkQueuePresentKHR(presentQueue, &presentInfo);
```

`vkQueuePresentKHR` 함수는 이미지를 표시하라는 요청을 스왑 체인에 제출합니다. 다음 챕터에서 `vkAcquireNextImageKHR`와 `vkQueuePresentKHR`에 대한 오류 처리를 추가할 것입니다. 왜냐하면 지금까지와는 다르게 이 떄의 오류는 프로그램을 종료할 정도의 오류는 아닐 수 있기 때문입니다.

여기까지 모든 단계를 제대로 수행했다면 이제 프로그램을 실행하면 아래와 비슷한 장면을 보시게 될겁니다:

![](/images/triangle.png)

>이 삼각형은 여러분들이 그래픽스 관련한 튜토리얼에서 보던 삼각형과 다를 수 있습니다. 왜냐하면 이 튜토리얼에서는 셰이더가 선형 색상 공간(linear color space)에서 보간을 수행한 뒤 sRGB 색상 공간으로 변환을 수행하기 때문입니다. 이러한 차이점에 대해서는 [이 블로그](https://medium.com/@heypete/hello-triangle-meet-swift-and-wide-color-6f9e246616d9)를 참고하세요.

짝짝짝! 하지만 안타깝게도 검증 레이어가 활성화 된 상태라면, 프로그램이 종료될 때 오류가 발생하는 것을 보실 수 있습니다. `debugCallback`에 의해 출력되는 메시지가 그 이유를 알려줍니다:

![](/images/semaphore_in_use.png)

`drawFrame`의 모든 연산이 비동기적이라는 것을 기억하십시오. 즉 `mainLoop`에서 루프를 종료했을 때도 그리기와 표시 연산이 계속 진행되고 있다는 뜻입니다. 연산이 진행되는 도중에 리소스를 정리하는 것은 좋지 않습니다.

이 문제를 해결하기 위해 `mainLoop`를 끝내고 윈도우를 소멸하기 이전에 논리적 장치가 연산을 끝내기를 기다려야 합니다.

```c++
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}
```

`vkQueueWaitIdle`를 통해 특정 명령 큐의 연산이 끝나기를 기다리도록 할 수도 있습니다.
이 함수는 동기화를 위한 아주 기초적인 방법으로 사용될 수도 있습니다. 이제 윈도우를 닫아도 문제 없이 프로그램이 종료되는 것을 보실 수 있습니다.

## 결론

900줄이 좀 넘는 코드로 드디어 화면에 뭔가를 표시할 수 있었습니다. Vulkan 프로그램을 부트스트래핑(bootstrapping)하는 것은 많은 노력이 필요하지만, 명시성으로 인해 우리에게 엄청난 양의 제어권을 제공한다는 것을 알 수 있었습니다. 제가 권장하는 것은 이제 시간을 갖고 코드를 다시 읽어보면서 모든 Vulkan 객체들의 목적과 그들이 각각 어떻게 관련되어 있는지에 대한 개념을 복습해 보시라는 것입니다. 이러한 지식을 가진 상태에서 이제 프로그램의 기능을 확장해 나가 볼 것입니다.

다음 챕터에서는 렌더링 루프를 확장하여 여러 프레임을 사용하도록 할 것입니다.

[C++ code](/code/15_hello_triangle.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
