## 개요

지금 만든 정점 버퍼는 잘 동작하지만 CPU에서 접근이 가능하도록 선택한 메모리 타입이 그래픽 카드에서 읽기에는 최적화된 메모리 타입은 아닐 수 있습니다. 
가장 적합한 메모리는 `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT` 플래그를 가지고 있고 대개는 대상 그래픽 카드에 대해 CPU에서는 접근이 불가능합니다. 
이 챕터에서는 두 정점 버퍼를 만들 것입니다.
하나는 *스테이징 버퍼(staging buffer)*로 CPU에서 접근 가능하여 정점 배열을 넣을 수 있으며 다른 하나는 장치의 로컬 메모리에 있는 정점 버퍼입니다. 
그러고 나서 버퍼 복사 명령을 사용해 스테이징 버퍼에서 실제 정점 버퍼로 데이터를 옮길 것입니다.

## 전송 큐(Transfer queue)

버퍼 복사 맹령은 전송 연산을 지원하는 큐 패밀리가 필요하고 이는 `VK_QUEUE_TRANSFER_BIT`로 표기됩니다. 
좋은 소식은 `VK_QUEUE_GRAPHICS_BIT`이나 `VK_QUEUE_COMPUTE_BIT` 기능이 있는 큐 패밀리는 암시적으로 `VK_QUEUE_TRANSFER_BIT` 연산을 지원한다는 것입니다. 
이러한 경우 `queueFlags`에 이를 명시적으로 표시하도록 구현되는 것이 강제되지는 않습니다.

도전을 원하신다면 전송 연산을 위해 또 다른 큐 패밀리를 사용하도록 해 보십시오. 
이렇게 하려면 다음과 같은 추가적인 수정이 필요합니다:

* `QueueFamilyIndices`와 `findQueueFamilies`를 수정하여 `VK_QUEUE_TRANSFER_BIT` 비트를 갖지만 `VK_QUEUE_GRAPHICS_BIT`는 갖지 않는 큐 패밀리를 명시적으로 탐색합니다.
* `createLogicalDevice`를 수정하여 전송 큐에 대한 핸들을 요청하도록 합니다.
* 명령 버퍼를 위한 두 번째 명령 풀을 만들어 전송 큐 패밀리에 제출할 것입니다.
* 리소스의 `sharingMode`를 `VK_SHARING_MODE_CONCURRENT`로 하고 그래픽스와 전송 큐 패밀리를 명시합니다.
* (이 챕터에서 사용할 예정인) `vkCmdCopyBuffer`와 같은 전송 명령을 그래픽스 큐 대신 전송 큐에 전송합니다.

작업이 좀 필요하지만 이를 통해 큐 패밀리간에 리소스가 어떻게 공유되는지를 배우실 수 있을겁니다.

## 버퍼 생성 추상화

이 장에서는 여러 버퍼를 생성할 것이므로, 버퍼 생성에 관한 헬퍼 함수를 만드는 것이 좋을 것 같습니다.
새로운 함수인 `createBuffer`를 만들고 `createVertexBuffer`의 (맵핑을 제외한) 코드를 옮겨옵니다.

```c++
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
```

버퍼의 크기와 메모리 속성, 사용 목적에 대한 매개변수를 만들어서 다른 종류의 버퍼를 만들 때도 사용 가능하도록 함수를 정의하는 것을 잊지 마세요. 
마지막 두 매개변수는 핸들을 저장할 출력 변수입니다.

이제 버퍼 생성과 메모리 할당 코드를 `createVertexBuffer`에서 제거하고 대신 `createBuffer`를 호출하면 됩니다:

```c++
void createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory);

    void* data;
    vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, vertexBufferMemory);
}
```

프로그램을 실행해 정점 버퍼가 여전히 제대로 동작하는지 확인해보세요.

## 스테이징 버퍼 사용

이제 `createVertexBuffer`를 수정해 호스트에서 보이는 버퍼는 임시 버퍼로, 장치 로컬을 실제 정점 버퍼로 사용하도록 할것입니다.

```c++
void createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
}
```

새로운 `stagingBuffer`와 `stagingBufferMemory`를 사용해 정점 데이터를 맵핑하고 복사할 것입니다. 이 챕터에서 두 개의 새로운 버퍼 사용법(usage) 플래그를 사용합니다:

* `VK_BUFFER_USAGE_TRANSFER_SRC_BIT`: 메모리 전송 연산에서 소스(source)로 사용되는 버퍼
* `VK_BUFFER_USAGE_TRANSFER_DST_BIT`: 메모리 전송 연산에서 목적지(destination)로 사용되는 버퍼

`vertexBuffer`는 이제 장치 로컬인 메모리 타입에서 할당되고, 그로 인해 일반적으로 `vkMapMemory`는 사용할 수 없게 됩니다. 
하지만 `stagingBuffer`에서 `vertexBuffer`로 데이터를 복사할 수는 있습니다. 
우리가 이렇게 하려고 한다는 것을 `stagingBuffer`에는 전송 소스 플래그를, `vertexBuffer`에는 전송 목적지 플래그와 정점 버퍼 플래그를 사용해 알려주어야 합니다.

이제 한 버퍼에서 다른 버퍼로 복사를 하는 `copyBuffer` 함수를 만듭니다.

```c++
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

}
```

메모리 전송 연산은 그리기와 마찬가지로 명령 버퍼를 통해 실행됩니다. 
그러므로 먼저 임시 명령 버퍼를 할당해야 합니다.
이렇게 임시 사용되는 버퍼에 대한 별도의 명령 풀을 만들면 메모리 할당 최적화가 수행될 수 있습니다. 
지금의 경우 명령 풀 생성시에 `VK_COMMAND_POOL_CREATE_TRANSIENT_BIT` 플래그를 사용해야 합니다.

```c++
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
}
```

그리고 바로 명령 버퍼에 기록을 시작합니다:

```c++
VkCommandBufferBeginInfo beginInfo{};
beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

vkBeginCommandBuffer(commandBuffer, &beginInfo);
```

명령 버퍼를 한 번만 사용하고 복사 연산 실행이 끝나서 함수가 반환될 때까지 대기하도록 할 것입니다. 
이러한 의도를 `VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT`를 사용해 드라이버에게 알려주는 것이 좋습니다.

```c++
VkBufferCopy copyRegion{};
copyRegion.srcOffset = 0; // Optional
copyRegion.dstOffset = 0; // Optional
copyRegion.size = size;
vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
```

버퍼 내용은 `vkCmdCopyBuffer` 명령을 통해 전송됩니다. 
소스와 목적지 버퍼, 그리고 복사할 영역에 대한 배열을 인자로 받습니다. 
영역은 `VkBufferCopy`로 정의되며 소스 버퍼의 오프셋, 목적지 버퍼의 오프셋, 크기로 구성됩니다. 
`vkMapMemory` 명령과는 달리 여기서는 `VK_WHOLE_SIZE`로 명시하는 것은 불가능합니다.

```c++
vkEndCommandBuffer(commandBuffer);
```

이 명령 버퍼는 복사 명령만을 포함하므로 바로 기록을 중단하면 됩니다. 
이제 명령 버퍼를 실행하여 전송을 완료합니다:

```c++
VkSubmitInfo submitInfo{};
submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
submitInfo.commandBufferCount = 1;
submitInfo.pCommandBuffers = &commandBuffer;

vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
vkQueueWaitIdle(graphicsQueue);
```

그리기 명령과는 다르게 여기서를 기다려야 할 이벤트가 없습니다. 
그냥 버퍼에 대한 전송 명령을 바로 실행하기를 원합니다. 
여기서도 마찬가지로 이러한 전송이 완료되는 것을 기다리는 두 가지 방법이 있습니다. 
`vkWaitForFences`로 펜스를 사용해 대기하거나, 전송 큐가 아이들(idle) 상태가 될때까지 대기하도록 `vkQueueWaitIdle`를 사용하는 것입니다. 
하나씩 실행하는 것이 아니라 여러 개의 전송 명령을 동시에 계획하고 전체가 끝날때까지 대기하는 경우에 펜스를 사용하면 됩니다. 
이렇게 하면 드라이버가 최적화 하기 더 좋습니다.

```c++
vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
```

전송 연산을 위해 사용한 명령 버퍼를 정리하는 것을 잊지 마세요.

이제 `createVertexBuffer` 함수에서 `copyBuffer`를 호출하여 정점 데이터를 장치의 로컬 버퍼로 옮깁니다: 

```c++
createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
```

스테이징 버퍼에서 장치 버퍼로 데이터를 복사한 뒤에는 정리해 주어야 합니다:

```c++
    ...

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
```

프로그램을 실행하여 삼각형이 잘 보이는지 확인하세요. 
개선점이 바로 눈에 보이지는 않지만 이제 정점 데이터는 고성능 메모리로부터 로드(load)됩니다. 
보다 복잡한 형상을 렌더링 할 때에는 이러한 사실이 중요해집니다.

## 결론

실제 응용 프로그램에서는 개별 버퍼마다 `vkAllocateMemory`를 호출하지 않는것이 좋다는 점을 주의하십시오. 
동시에 수행 가능한 메모리 할당은 물리적 장치의 `maxMemoryAllocationCount`에 의해 제한되며 NVIDIA GTX 1080와 같은 고성능 장치에서도 `4096`정도밖에 안됩니다. 
많은 객체를 위한 메모리 할당을 한꺼번에 수행하는 적정한 방법은 여러 객체들에 대해 `offset` 매개변수를 사용해 한 번에 할당을 수행하는 별도의 할당자(allocator)를 만드는 것입니다.

이러한 할당자를 직접 구현해도 되고, GPUOpen 이니셔티브에서 제공하는 [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) 라이브러리를 사용해도 됩니다. 
하지만 이 튜토리얼에서는 각 리소스에 대해 별도의 할당을 수행해도 상관없는데 지금은 위와 같은 제한에 걸릴 만큼 복잡한 작업은 하지 않기 떄문입니다.

[C++ code](/code/20_staging_buffer.cpp) /
[Vertex shader](/code/18_shader_vertexbuffer.vert) /
[Fragment shader](/code/18_shader_vertexbuffer.frag)
