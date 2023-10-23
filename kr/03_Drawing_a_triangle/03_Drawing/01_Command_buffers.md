그리기 명령이나 메모리 전송과 같은 Vulkan의 명령(command)은 함수 호출을 통해 직접 수행되는 것이 아닙니다. 수행하고자 하는 연산들을 모두 명령 버퍼 객체에 먼저 기록해야 합니다. 이로 인해 Vulkan에게 우리가 하고자 하는 것들을 알려줄 준비가 완료되었다면, 모든 명령이 한꺼번에 Vulkan으로 제출(submit)되어 동시에 실행 가능한 상태가 된다는 것입니다. 또한 원한다면 여러 쓰레드에서 명령을 기록할 수 있다는 장점도 있습니다.

## 명령 풀(Command pools)

명령 버퍼를 만드려면 먼저 명령 풀부터 만들어야 합니다. 명령 풀은 명령 버퍼로 할당될 버퍼의 메모리를 관리합니다. `VkCommandPool`을 저장할 새 클래스 멤버를 추가합니다:

```c++
VkCommandPool commandPool;
```

그리고 `initVulkan`의 프레임버퍼 생성 이후에 호출할 `createCommandPool` 함수를 새로 만듭니다.

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
}

...

void createCommandPool() {

}
```

명렬 풀 생성에는 두 개의 매개변수만 필요합니다:

```c++
QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

VkCommandPoolCreateInfo poolInfo{};
poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
```

명령 풀에는 두가지 가능한 플래그가 존재합니다:

* `VK_COMMAND_POOL_CREATE_TRANSIENT_BIT`: 명령 버퍼가 새로운 명령을 자주 기록할 것을 알려주는 힌트 (이에 따라 메모리 할당 방식이 바뀔 수 있음)
* `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`: 명령 버퍼가 독립적으로 재기록(rerecord)될 수 있음. 이 플래그가 없으면 모두 함께 리셋(reset)되어야 함

우리는 명령 버퍼를 매 프레임 기록할 것이기 때문에 리셋하고 재기록 하게 하려고 합니다. 따라서 커맨드 풀은 `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT` 플래그 비트로 설정합니다.

명령 버퍼는 이를 장치 큐 중 하나에 제출함으로써 실행됩니다. 장치 큐는 예를들어 우리가 획득한 그래픽스 또는 표시 큐와 같은 것입니다. 각 명령 풀은 한 종류의 큐에 제출할 명령 버퍼만 할당 가능합니다. 우리는 그리기를 위한 명령을 기록할 것이라서 그래픽스 큐 패밀리를 선택한 것입니다.

```c++
if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
}
```

마지막으로 `vkCreateCommandPool` 함수를 호출해 명령 풀을 만듭니다. 특별한 매개변수는 없습니다. 명령은 화면에 무언가를 그리기 위해 프로그램 내내 사용할 것이니, 마지막에 가서야 해제하게 됩니다:

```c++
void cleanup() {
    vkDestroyCommandPool(device, commandPool, nullptr);

    ...
}
```

## 명령 버퍼 할당

이제 명령 버퍼 할당을 시작해 봅시다.

`VkCommandBuffer` 객체를 클래스 멤버로 추가합니다. 명령 버퍼는 명령 풀이 소멸되면 자동으로 해제되므로 따로 정리 과정은 필요 없습니다.

```c++
VkCommandBuffer commandBuffer;
```

이제 명령 풀에서 하나의 명령 버퍼를 만들기 위해 `createCommandBuffer` 함수를 만들어 봅시다.

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
}

...

void createCommandBuffer() {

}
```

명령 버퍼는 `vkAllocateCommandBuffers` 함수를 사용해 할당되는데, 명령 풀을 명시하는 `VkCommandBufferAllocateInfo` 매개변수와 할당할 버퍼 개수를 매개변수로 받습니다:

```c++
VkCommandBufferAllocateInfo allocInfo{};
allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
allocInfo.commandPool = commandPool;
allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
allocInfo.commandBufferCount = 1;

if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
}
```

`level` 매개변수는 할당된 명령 버퍼가 주(primary) 명령 버퍼인지, 보조(secondary) 명령 버퍼인지를 명시합니다.

* `VK_COMMAND_BUFFER_LEVEL_PRIMARY`: 실행을 위해 큐에 제출될 수 있지만, 다른 명령 버퍼에서 호출은 불가능.
* `VK_COMMAND_BUFFER_LEVEL_SECONDARY`: 직접 제출은 불가능하지만 주 명령 버퍼로부터 호출될 수 있음.

보조 명령 버퍼의 기능은 여기에서 사용하진 않을 것이지만, 주 명령 버퍼에서 자주 사용되는 연산을 재사용하기 위해 유용하게 사용될 수 있다는 것은 눈치 채실 수 있을 겁니다.

우리는 하나의 명령 버퍼만을 할당하므로 `commandBufferCount`는 1입니다.

## 명령 버퍼 기록

이제 실행하고자 하는 명령을 명령 버퍼에 채우는 `recordCommandBuffer` 함수를 만들어 봅시다. `VkCommandBuffer`와 값을 쓰고자 하는 현재 스왑체인 이미지의 인덱스를 매개변수로 넘겨줍니다.

```c++
void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {

}
```

명령 버퍼의 기록은 항상 `vkBeginCommandBuffer`에 간단한 `VkCommandBufferBeginInfo` 구조체를 넘겨주는 것으로 시작합니다. 이 구조체는 해당 명령 버퍼의 사용 방식에 대한 세부 사항을 명시합니다.

```c++
VkCommandBufferBeginInfo beginInfo{};
beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
beginInfo.flags = 0; // Optional
beginInfo.pInheritanceInfo = nullptr; // Optional

if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
}
```

`flags` 매개변수는 명령 버퍼를 어떻게 사용할 것인지를 명시합니다. 아래와 같은 값들이 될 수 있습니다:

* `VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT`: 명령 버퍼는 한 번 실행 후 즉시 재기록됨.
* `VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT`: 이 버퍼는 보조 명령 버퍼로써 하나의 렌더링 패스에 완전하게 속해 있음.
* `VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT`: 명령이 지연될 경우에 명령 버퍼가 재제출(resubmit) 될 수 있음

연재는 어떤 플래그도 해당하지 않습니다.

`pInheritanceInfo` 매개변수는 보조 명령 버퍼에만 해당됩니다. 주 명령 버퍼에서 호출될 떄 어떤 상태를 상속(inherit)하는지 명시합니다.

명령 버퍼가 이미 기록된 상태에서 `vkBeginCommandBuffer`를 호출하면 암시적으로 버퍼가 리셋됩니다. 명령을 버퍼에 추가(append)하는 것은 불가능합니다.

## 렌더 패스 시작하기

그리기는 `vkCmdBeginRenderPass`를 사용해 렌더 패스를 시작함으로써 시작됩니다. 렌더 패스는 `VkRenderPassBeginInfo` 구조체의 매개변수를 기반으로 설정됩니다.

```c++
VkRenderPassBeginInfo renderPassInfo{};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
renderPassInfo.renderPass = renderPass;
renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
```

첫 매개변수는 렌더패스 그 자체와 바인딩할 어태치먼트입니다. 각 스왑 체인 이미지에 대해 프레임버퍼를 만들었고, 색상 어태치먼트로 명시된 상태입니다. 따라서 그 프레임버퍼를 우리가 그리고자 하는 스왑체인 이미지로 바인딩해야 합니다. 넘어온 imageIndex를 사용해 현재 스왑체인 이미지의 적정한 프레임버퍼를 선택할 수 있습니다.

```c++
renderPassInfo.renderArea.offset = {0, 0};
renderPassInfo.renderArea.extent = swapChainExtent;
```

다음 두 매개변수는 렌더 영역(area)의 크기를 명시합니다. 렌더 영역은 셰이더가 값을 읽고 쓰는 영역을 정의합니다. 이 영역 밖의 픽셀은 정의되지 않은 값을 가지게 됩니다. 어태치먼트와 같은 크기여야 성능이 높아집니다.

```c++
VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
renderPassInfo.clearValueCount = 1;
renderPassInfo.pClearValues = &clearColor;
```

마지막 두 매개변수는 `VK_ATTACHMENT_LOAD_OP_CLEAR`에 사용될 지우기(clear) 값이고, 색상 어태치먼트의 로드 연산에 사용한 바 있습니다. 여기서는 간단히 검은색의 100% 불투명도로 지우도록 하겠습니다.

```c++
vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
```

이제 렌더 패스가 시작됩니다. 명령을 기록하는 함수는 `vkCmd` 접두어로 구분할 수 있습니다. 이들은 모두 `void` 반환이므로 기록을 끝낼 때 까지는 오류 처리가 불가능합니다.

모든 명령의 첫 매개변수는 명령을 기록할 명령 버퍼입니다. 두 번째 매개변수는 방금 만든, 렌더 패스 세부사항을 명시합니다. 마지막 매개변수는 렌더 패스 안의 그리기 명령이 어떻게 제공될지를 제어합니다. 두 개의 값 중 하나입니다:

* `VK_SUBPASS_CONTENTS_INLINE`: 렌더 패스 명령이 주 명령 버퍼에 포함되어 있고 보조 명령 버퍼는 실행되지 않음.
* `VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS`: 렌더 패스 명령이 보조 명령 버퍼에서 실행됨

보조 명령 버퍼는 사용하지 않을 것이므로, 첫 번째 값을 선택합니다.

## 기본 그리기 명령

이제 그래픽스 파이프라인을 바인딩합니다:

```c++
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
```

두 분째 매개변수는 파이프라인 객체가 그래픽스 파이프라인인지 계산(compute) 파이프라인인지를 명시합니다. 이제 Vulkan에게 그래픽스 파이프라인에서 어떤 명령을 실행하고 프래그먼트 셰이더에서 어떤 어태치먼트를 사용할 것인지를 알려 주었습니다.

[고정 함수 챕터](../02_Graphics_pipeline_basics/02_Fixed_functions.md#dynamic-state)에서 이야기 한 것처럼, 우리는 파이프라인에게 뷰포트와 시저 상태가 동적일 것이라고 명시해 둔 상태입니다. 따라서 이들을 명령 버퍼에서 그리기 명령을 수행하기 이전에 설정해 주어야 합니다:

```c++
VkViewport viewport{};
viewport.x = 0.0f;
viewport.y = 0.0f;
viewport.width = static_cast<float>(swapChainExtent.width);
viewport.height = static_cast<float>(swapChainExtent.height);
viewport.minDepth = 0.0f;
viewport.maxDepth = 1.0f;
vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

VkRect2D scissor{};
scissor.offset = {0, 0};
scissor.extent = swapChainExtent;
vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
```

이제 삼각형을 그리기 위한 그리기 명령을 추가합니다:

```c++
vkCmdDraw(commandBuffer, 3, 1, 0, 0);
```

실제 `vkCmdDraw` 명령은 아주 어렵지 않은데 미리 모든 정보를 설정해 두었기 때문입니다. 이 명령은 명령 버퍼 이외에 다음과 같은 매개변수를 갖습니다:

* `vertexCount`: 정점 버퍼는 없어도, 그리기 위해서는 3개의 정점이 필요합니다.
* `instanceCount`: 인스턴스(instanced) 렌더링을 위해 사용되는데, 그 기능을 사용하지 않는경우 `1`로 설정합니다.
* `firstVertex`: 정점 버퍼의 오프셋을 설정하는 데 사용되며, `gl_VertexIndex`의 가장 작은 값을 정의합니다.
* `firstInstance`: 인스턴스 렌더링의 오프셋을 설정하는 데 사용되며, `gl_InstanceIndex`의 가장 작은 값을 정의합니다.

## 마무리

이제 렌더 패스를 끝냅니다:

```c++
vkCmdEndRenderPass(commandBuffer);
```

그리고 명령 버퍼의 기록도 끝냅니다:

```c++
if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
}
```

다음 장에서는 메인 루프를 위한 코드를 작성할 것이고, 그 과정에서 스왑 체인 이미지를 얻고, 명령 버퍼를 기록하고 실행하며, 결과 이미지를 스왑 체인에 반환할 것입니다.

[C++ code](/code/14_command_buffers.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
