## 개요

Vulkan에서 버퍼는 그래픽 카드가 읽을 수 있는, 임의의 데이터를 저장하는 메모리 영역을 의미합니다. 이 챕터에서의 예시처럼 정점 데이터를 저장하는 데 사용될 수도 있지만 나중 챕터에서 살펴볼 것인데 다른 용도로도 자주 사용됩니다. 지금까지 살펴본 Vulkan 객체와는 다르게 버퍼는 스스로 메모리를 할당하지 않습니다. 지금까지 살펴본 것처럼 Vulkan API는 프로그래머에게 거의 모든 제어권을 주는데, 메모리 관리 또한 이에 포함됩니다.

## 버퍼 생성

`createVertexBuffer` 함수를 새로 만들고 `initVulkan`의 `createCommandBuffers` 바로 직전에 호출하도록 합니다.

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
    createVertexBuffer();
    createCommandBuffers();
    createSyncObjects();
}

...

void createVertexBuffer() {

}
```

버퍼 생성을 위해서는 `VkBufferCreateInfo` 구조체를 채워야 합니다.

```c++
VkBufferCreateInfo bufferInfo{};
bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
bufferInfo.size = sizeof(vertices[0]) * vertices.size();
```

첫 번째 필드는 `size`이고, 버퍼의 바이트 단위 크기를 명시합니다. 정점 데이터의 바이트 단위 크기를 계산하는 것은 `sizeof`를 사용하면 됩니다.

```c++
bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
```

두 번째 필드는 `usage`인데 버퍼의 데이터가 어떤 목적으로 사용될지를 알려줍니다. bitwise OR를 사용해 목적을 여러개 명기하는것도 가능합니다. 우리의 사용 목적은 정점 버퍼이며 다른 타입에 대해서는 다른 챕터에서 알아보겠습니다.

```c++
bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
```

스왑 체인의 이미지처럼 버퍼는 특정 큐 패밀리에 의해 소유되거나 여러 큐 패밀리에서 공유될 수 있습니다. 버퍼는 그래픽스 큐에서만 활용될 예정이므로 독점(exclusive) 접근으로 두겠습니다.

`flag` 매개변수는 sparse한 버퍼 메모리를 설정하기 위해 사용되는데, 지금은 사용하지 않습니다. 기본값인 `0`으로 둘 것입니다.

이제 `vkCreateBuffer`로 버퍼를 만들 수 있습니다. 버퍼 핸들을 저장할 `vertexBuffer`를 클래스의 멤버로 정의합니다.

```c++
VkBuffer vertexBuffer;

...

void createVertexBuffer() {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(vertices[0]) * vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }
}
```

버퍼는 프로그램이 끝날 때까지 렌더링 명령에서 활용되기 위해 유효한 상태로 남아있어야 하고, 스왑 체인에는 종속적이지 않으므로 `cleanup` 함수에서 정리합니다:

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyBuffer(device, vertexBuffer, nullptr);

    ...
}
```

## 메모리 요구사항

버퍼가 생성되었지만 아직 실제로 메모리가 할당된 것은 아닙니다. 버퍼의 메모리 할당을 위한 첫 단계는 `vkGetBufferMemoryRequirements`라는 이름의 함수로 메모리 요구사항을 질의하는 것입니다.

```c++
VkMemoryRequirements memRequirements;
vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);
```

`VkMemoryRequirements` 구조체에는 세 개의 필드가 있습니다:

* `size`: 요구되는 메모리의 바이트 단위 크기로, `bufferInfo.size`와는 다를 수 있음
* `alignment`: `bufferInfo.usage`와 `bufferInfo.flags`에 의해 결정되는, 메모리 영역에서 버퍼가 시작되는 바이트 오프셋(offset)
* `memoryTypeBits`: 버퍼에 적합한 메모리 타입의 비트 필드

그래픽 카드는 할당할 수 있는 서로 다른 종류의 메모리가 있습니다. 각 메모리 타입은 허용 가능한 연산과 성능 특성이 다릅니다. 버퍼의 요구사항과 우리 응용 프로그램의 요구사항을 결합하여 적합한 메모리 타입을 결정해야 합니다. 이러한 목적을 위해서 `findMemoryType` 함수를 새로 만듭시다.

```c++
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {

}
```

먼저 사용 가능한 메모리 타입의 정보를 `vkGetPhysicalDeviceMemoryProperties`를 사용해 질의해야 합니다.

```c++
VkPhysicalDeviceMemoryProperties memProperties;
vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
```

`VkPhysicalDeviceMemoryProperties` 구조체는 `memoryTypes`와 `memoryHeaps` 배열을 가지고 있습니다. 메모리 힙은 VRAM, 그리고 VRAM이 부족할 때 사용하는 RAM의 스왑 공간 같은 메모리 자원입니다. 이 힙 안에 여러 메모리 타입이 존재하게 됩니다. 지금은 메모리 타입만 사용하고 그 메모리가 어떤 힙에 존재하는 것인지 신경쓰지 않을 것이지만 그에 따라 성능에 영향이 있을 수 있다는 것은 예상하실 수 있을겁니다.

먼저 버퍼에 적합한 메모리 타입을 찾습니다:

```c++
for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if (typeFilter & (1 << i)) {
        return i;
    }
}

throw std::runtime_error("failed to find suitable memory type!");
```

`typeFilter` 매개변수는 적합한 메모리 타입을 명시하기 위한 비트 필드입니다. 즉 적합한 메모리 타입에 대한 인덱스는 그냥 반복문을 돌면서 해당 비트가 1인지를 확인하여 얻을 수 있습니다.

하지만 우리는 정점 버퍼를 위한 적합한 메모리 타입에만 관심이 있는 것이 아닙니다. 정점 데이터를 해당 메모리에 쓸 수 있어야 합니다. `memoryTypes` 배열은 힙과 각 메모리 타입의 속성을 명시하는 `VkMemoryType` 구조체의 배열입니다. 속성은 메모리의 특수 기능을 정의하는데 예를 들자면 CPU에서 값을 쓸 수 있도록 맵핑(map)할 수 있는지 여부와 같은 것입니다. 이 속성은 `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`로 명시되는데, 추가적으로 `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` 속성도 필요합니다. 그 이유에 대해서는 메모리 맵핑을 할 때 알게 될 겁니다.

이제 반복문을 수정해 이러한 속성에 대한 지원 여부를 확인합니다:

```c++
for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
    }
}
```

필요한 속성이 하나 이상일 수 있으므로 bitwise AND의 결과가 0이 아닌 것만을 확인하면 안되고 해당하는 비트 필드가 필요한 속성과 동일한지 확인해야 합니다. 버퍼에 적합한 메모리 타입이 있고 이러한 속성들을 가지고 있으면 해당 인덱스를 반환하고, 아니면 예외를 발생시키도록 합니다.

## 메모리 할당

이제 올바른 메모리 타입을 정하는 법이 마련되었으니 `VkMemoryAllocateInfo` 구조체를 채워 실제로 메모리를 할당해 보겠습니다.

```c++
VkMemoryAllocateInfo allocInfo{};
allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
allocInfo.allocationSize = memRequirements.size;
allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
```

이제 단지 크기와 타입을 명시하면 되는데, 모두 정점 버퍼의 메모리 요구사항과 원하는 속성으로부터 도출되는 값입니다. 메모리에 대한 핸들을 저장하기 위한 클래스 멤버를 만들고 `vkAllocateMemory`를 통해 메모리를 할당받습니다.

```c++
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

...

if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate vertex buffer memory!");
}
```

메모리 할당이 성공했으면 `vkBindBufferMemory`로 그 메모리를 버퍼와 연결(associate)시킵니다:

```c++
vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
```

첫 세 매개변수는 특별히 설명할 것이 없고, 네 번째 매개변수는 메모리 영역에서의 오프셋입니다. 지금 메모리는 정점 버퍼만을 위해 할당받은 것이므로 오프셋은 `0`입니다. 
오프셋이 0이 아니면, `memRequirements.alignment`를 사용해 분할 가능해야만 합니다.

물론 C++에서의 동적 메모리 할당처럼 이 메모리는 어떤 시점에 해제되어야만 합니다. 
버퍼 객체와 바인딩된 메모리는 버퍼가 더이상 사용되지 않을 때 해제되면 되기 때문에 버퍼의 소멸 이후에 해제하도록 합시다:

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
```

## 정점 버퍼 채우기

이제 정점 데이터를 버퍼에 복사할 시간입니다. 이는 CPU가 접근 가능한 메모리에 `vkMapMemory`로 [버퍼 메모리 맵핑](https://en.wikipedia.org/wiki/Memory-mapped_I/O)을 함으로써 수행합니다.

```c++
void* data;
vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
```

이 함수는 오프셋과 크기로 명시된 특정 메모리 리소스 영역에 접근이 가능하도록 해 줍니다. 여기서 오프셋과 크기는 각각 `0`과 `bufferInfo.size`입니다. `VK_WHOLE_SIZE`와 같은 특수한 값으로 전체 메모리를 맵핑하는 것도 가능합니다. 끝에서 두 번째 매개변수는 플래그를 명시하기 위해 사용될 수도 있지만 현재 API에는 아직 사용 가능한 것이 없습니다. 따라서 값은 `0`이어야만 합니다. 마지막 매개변수는 맵핑된 메모리에 대한 포인터 출력입니다.

```c++
void* data;
vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferInfo.size);
vkUnmapMemory(device, vertexBufferMemory);
```

이제 `memcpy`로 정점 데이터를 맵핑된 메모리에 복사하고 `vkUnmapMemory`를 사용해 다시 언맵핑합니다. 안타깝게도 드라이버가 즉시 버퍼 메모리에 복사를 수행하지 못할 수도 있습니다. 예를 들어 캐싱(chching) 때문에요. 또한 버퍼에의 쓰기 작업이 아직 맵핑된 메모리에 보이지 않을 수도 있습니다. 이러한 문제를 처리하기 위한 두 가지 방법이 있습니다:

* `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`로 명시된 호스트에 일관성(coherent) 메모리 힙을 사용함
* 맵핑된 메모리에 쓰기를 수행한 후 `vkFlushMappedMemoryRanges`를 호출하고, 맵핑된 메모리를 읽기 전에 `vkInvalidateMappedMemoryRanges` 를 호출함

우리는 첫 번째 방법을 사용했는데 이렇게 하면 맵핑된 메모리의 내용이 할당된 메모리와 항상 동일한 것이 보장됩니다. 이러한 방식은 명시적인 플러싱(flushing)에 비해 약간의 성능 손해가 있다는 것을 아셔야 하지만, 크게 상관 없습니다. 왜 그런지는 다음 챕터에서 살펴보도록 하겠습니다.

메모리 영역을 플러싱하거나 일관성 메모리 힙을 사용한다는 이야기는 드라이버가 버퍼에 대한 쓰기 의도를 파악하게 된다는 것이지만 아직 실제로 GPU가 그 메모리 영역을 볼 수 있다는 이야기는 아닙니다. 실제로 데이터가 GPU로 전송되는 것은 백그라운드에서 진행되며 [명세](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap7.html#synchronization-submission-host-writes)에서는 단순히 다음 `vkQueueSubmit` 호출 이전에 완료가 보장된다는 것만 정의하고 있습니다.

## 정점 버퍼 바인딩

이제 남은 것은 렌더링 연산에서 정점 버퍼를 바인딩 하는 것입니다. `recordCommandBuffer` 함수를 확장하여 이러한 작업을 수행하도록 합니다.

```c++
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

VkBuffer vertexBuffers[] = {vertexBuffer};
VkDeviceSize offsets[] = {0};
vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
```

`vkCmdBindVertexBuffers` 함수는 정점 버퍼를 이전 챕터에서 설정한 바인딩에 바인딩합니다. 명령버퍼를 제외한 첫 두 매개변수는 오프셋과 정점 버퍼의 바인딩 숫자를 명시합니다. 마지막 두 매개변수는 바인딩할 정점 버퍼의 배열과 정점 데이터를 읽기 시작할 바이트 오프셋을 명시합니다. 또한 `vkCmdDraw` 호출에서도 하드코딩된 숫자 `3`을 사용하는 대신 버퍼의 정점 개수를 넘겨주도록 수정합니다.

이제 프로그램을 실행하면 익숙한 삼각형을 다시 볼 수 있습니다:

![](/images/triangle.png)

`vertices` 배열을 수정하여 위쪽 정점의 색상을 바꿔봅시다:

```c++
const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};
```

이제 프로그램을 다시 실행하면 아래와 같은 화면이 보입니다:

![](/images/triangle_white.png)

다음 장에서는 정점 데이터를 정점 버퍼에 복사하는 데 있어 더 좋은 성능을 가지지만, 작업이 좀 더 필요한 방법들을 살펴볼 것입니다.

[C++ code](/code/19_vertex_buffer.cpp) /
[Vertex shader](/code/18_shader_vertexbuffer.vert) /
[Fragment shader](/code/18_shader_vertexbuffer.frag)
