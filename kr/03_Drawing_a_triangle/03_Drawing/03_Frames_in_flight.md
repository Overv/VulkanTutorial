## 여러 프레임 사용하기(frames in-flight)

지금 우리 렌더링 루프에 눈에 띄는 문제가 하나 있습니다.
다음 프레임을 렌더링 하기 전에 이전 프레임을 기다려야만 하고 이로 인해 호스트는 불필요한 대기(ideling) 시간을 갖게 됩니다.

<!-- insert diagram showing our current render loop and the 'multi frame in flight' render loop -->

이를 수정하는 방법은 여러 프레임을 동시에 사용하는 것입니다.
즉, 하나의 프레임에 렌더링을 수행하는 것과 다음 프레임의 기록 과정을 서로 간섭이 없도록 할 것입니다.
어떻게 해야 할까요? 일단 렌더링에 필요한 모든 접근과 수정이 필요한 자원들이 모두 복제되어야만 합니다. 즉, 여러 개의 명령 버퍼, 세마포어와 펜스들이 있어야 합니다.
나중 챕터에서는 다른 리소스들에 대한 다중 인스턴스를 추가할 것이고, 그 챕터에서 이러한 개념을 다시 보게될 것입니다.

프로그램 상단에 얼마나 많은 프레임을 동시에 처리할 것인지 정의하는 상수를 먼저 추가합니다:

```c++
const int MAX_FRAMES_IN_FLIGHT = 2;
```

우리는 CPU가 GPU보다 *너무* 앞서나가는 것을 원하지는 않으므로 2로 설정하였습니다.
두 개의 프레임을 동시에 사용하면 CPU와 GPU는 각자의 작업을 동시에 수행할 수 있습니다.
CPU가 먼저 끝나면, 작업을 더 제출할기 전에 GPU가 렌더링을 끝내길 기다릴 것입니다.
세 개 이상의 프레임을 동시에 사용하면 CPU가 GPU보다 앞서나가 지연되는 프레임이 발생할 수 있습니다.
일반적으로, 이러한 지연은 좋지 않습니다. 하지만 이렇게 사용되는 프레임의 개수를 조정하는 것 또한 Vulkan의 명시성의 한 예가 될 것입니다.

각 프레임은 각자의 명령 버퍼, 세마포어 집합과 펜스를 가져야 합니다.
이들을 `std::vector`들로 바꾸고 이름도 변경합니다.

```c++
std::vector<VkCommandBuffer> commandBuffers;

...

std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
std::vector<VkFence> inFlightFences;
```

이제 여러 개의 명령 버퍼를 생성해야 합니다.
`createCommandBuffer`를 `createCommandBuffers`로 이름을 변경하고 명령 버퍼 벡터의 크기를 `MAX_FRAMES_IN_FLIGHT`로 수정합니다.
`VkCommandBufferAllocateInfo`를 수정하여 명령 버퍼의 숫자를 받고록 하고 새로운 명령 버퍼 벡터의 위치를 넘겨줍니다:

```c++
void createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    ...
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}
```

`createSyncObjects` 함수는 모든 객체를 생성하도록 수정되어야 합니다:

```c++
void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}
```

모두 정리되어야 하는 것도 마찬가지입니다:

```c++
void cleanup() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    ...
}
```

명령 풀을 해제하면 명령 버퍼가 자동으로 해제되기 때문에 명령 버퍼 정리에는 별도의 작업이 필요 없다는 사실을 기억하세요.

각 프레임에서 올바른 객체를 사용하기 위해서는 현재 프레임이 무엇인지를 알고 있어야 합니다. 이를 위해 프레임 인덱스를 사용할 것입니다:


```c++
uint32_t currentFrame = 0;
```

`drawFrame` 함수에서는 적절한 객체를 사용하도록 수정합니다:

```c++
void drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    ...

    vkResetCommandBuffer(commandBuffers[currentFrame],  0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    ...

    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    ...

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};

    ...

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

    ...

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
}
```

당연히 다음 프레임에는 프레임을 증가시켜주어야 합니다:

```c++
void drawFrame() {
    ...

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
```

모듈로 연산(%)을 사용해 프레임 인덱스가 `MAX_FRAMES_IN_FLIGHT`개의 프레임 뒤에는 다시 0이 되도록 합니다.

<!-- Possibly use swapchain-image-count for renderFinished semaphores, as it can't
be known with a fence whether the semaphore is ready for re-use. -->

이제 동기화화 관련한 모든 구현을 마쳐서 `MAX_FRAMES_IN_FLIGHT`개 이상의 프레임 작업이 동시에 큐에 들어가지 않도록 하였으며 이러한 프레임들이 서로 겹치지도 않게 되었습니다.
정리 부분과 같은 나머지 부분은 `vkDeviceWaitIdle` 처럼 보다 기초적인 동기화 방법에 의존하고 있습니다.
어떠한 접근법을 사용할지는 성능 요구사항에 따라서 여러분이 선택하셔야 합니다.

동기화에 대해 예를 통해 더 알고 싶으시면 Khronos에서 제공하는 [이 개요 문서](https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#swapchain-image-acquire-and-present)를 살펴보세요.

다음 챕터에서는 잘 동작하는 Vulkan 프로그램에 필요한 또다른 요구사항을 살펴보겠습니다. 

[C++ code](/code/16_frames_in_flight.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
