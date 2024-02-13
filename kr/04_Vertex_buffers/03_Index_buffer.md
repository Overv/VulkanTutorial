## 개요

실제 응용 프로그램에서 렌더링할 3D 메쉬는 여러 삼각형에서 정점을 공유하는 경우가 많습니다. 
이는 아래와 같은 간단한 사각형을 렌더링 할 때도 적용됩니다:

![](/images/vertex_vs_index.svg)

사각형을 렌더링하려면 삼각형 두 개가 필요하기 때문에 정점 6개를 갖는 정점 버퍼가 필요합니다. 
문제는 두 개의 정점은 동일한 데이터이기 때문에 50%의 중복이 발생한다는 것입니다. 
메쉬가 복잡해지면 평균적으로 정점 한개당 3개의 삼각형에서 재사용되어 상황은 더 나빠집니다. 
이 문제를 해결하는 방법은 *인덱스 버퍼(index buffer)*를 사용하는 것입니다.

인덱스 버퍼는 정점 버퍼에 대한 포인터 배열과 같습니다. 
정점 데이터의 순서를 바꾸고 이미 존재하는 데이터는 여러 정점으로 활용할 수 있게 해줍니다. 
위 그림에서는 정점 버퍼가 네 개의 정점을 가지고 있을 때 인덱스 버퍼를 사용해 사각형을 표현하는 예시를 보여줍니다. 
첫 세 개의 인덱스가 위 오른쪽 삼각형을 정의하며 마지막 세 개의 인덱스가 왼쪽 아래 삼각형을 정의합니다.

## 인덱스 버퍼 생성

이 챕터에서 우리는 정점 데이터를 수정하고 인덱스 데이터를 추가하여 위 그림과 같은 사각형을 그려 볼 것입니다. 
네 개의 꼭지점을 표현하도록 정점 데이터를 수정합니다:

```c++
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};
```

왼쪽 위 꼭지점은 빨간색, 오른쪽 위는 초록색, 오른쪽 아래는 파란색, 왼쪽 아래는 흰색입니다. 
인덱스 버퍼의 내용은 새로운 `indices`를 추가하여 정의합니다. 
오른쪽 위 삼각형과 왼쪽 아래 삼각형을 위해 그림에서와 같이 인덱스를 정의합니다.

```c++
const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};
```

`vertices`요소 개수에 따라 인덱스 버퍼로 `uint16_t`나 `uint32_t`를 사용하는 것이 모두 가능합니다. 
지금은 65535개보다는 정점이 적으므로 `uint16_t`를 사용합니다.

정점 데이터와 마찬가지로 인덱스도 `VkBuffer`를 통해 GPU로 전달되어 접근 가능하게 만들어야만 합니다.
인덱스 버퍼 리소스를 저장할 두 개의 새로운 클래스 멤버를 정의합니다:

```c++
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;
```

새로 추가하는 `createIndexBuffer` 함수는 `createVertexBuffer`와 거의 동일합니다:

```c++
void initVulkan() {
    ...
    createVertexBuffer();
    createIndexBuffer();
    ...
}

void createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
```

눈에 띄는 차이점은 두 가지입니다. 
`bufferSize`는 인덱스 자료형인 `uint16_t` 또는 `uint32_t`의 크기 곱하기 인덱스의 개수입니다. 
`indexBuffer`의 사용법은 당연히 `VK_BUFFER_USAGE_VERTEX_BUFFER_BIT`가 아닌 `VK_BUFFER_USAGE_INDEX_BUFFER_BIT` 입니다.
이 외에는 모든 것이 같습니다. 
`indices`의 내용을 장치의 로컬 인덱스 버퍼에 복사하기 위해 스테이징 버퍼를 만들어 사용합니다.

인덱스 버퍼 정점 버퍼처럼 프로그램 종료 시점에 정리되어야 합니다:

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    ...
}
```

## 인덱스 버퍼 사용

그리기를 위해 인덱스 버퍼를 사용하기 위해서는 `recordCommandBuffer`에 두 가지 변화가 필요합니다. 
우선 정범 버퍼터럼 인덱스 버퍼도 바인딩 해야 합니다. 
차이점은 하나의 인덱스 버퍼만 가질 수 있다는 것입니다. 
각 정점 어트리뷰트에 대해 안타깝게도 여러 개의 인덱스를 사용할 수는 없으니 하나의 어트리뷰트만 바뀌어도 전체 정점 데이터를 복사해야 합니다.

```c++
vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
```

인덱스 버퍼의 바인딩은 `vkCmdBindIndexBuffer`로 수행되며 이 함수는 인덱스 버퍼, 바이트 오프셋, 인덱스 데이터의 타입을 매개변수로 받습니다. 
앞서 이야기한 것처럼 타입은 `VK_INDEX_TYPE_UINT16` 또는 `VK_INDEX_TYPE_UINT32` 입니다.

인덱스 버퍼를 바인딩한 것만으로 아직 바뀐 것은 없습니다. 
Vulkan에 인덱스 버퍼를 사용하도록 그리기 명령 또한 수정해야 합니다. 
`vkCmdDraw` 라인을 `vkCmdDrawIndexed`로 바꿉니다:

```c++
vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
```

`vkCmdDraw` 호출과 매우 유사합니다. 
첫 두 매개변수는 인덱스의 개수와 인스턴스 개수를 명시합니다. 
인스턴싱을 하지 않으므로 `1`로 두었습니다. 
인덱스의 개수는 정점 셰이더에 넘겨질 정점의 개수를 의미합니다. 
다음 매개변수는 인덱스 버터의 오프셋이고 `1`을 사용하면 두 번째 인덱스부터 렌더링을 시작합니다. 
마지막에서 두 번째 매개변수는 인덱스 버퍼에 추가할 인덱스의 오프셋을 의미합니다. 
마지막 매개변수는 인스턴싱을 위한 오프셋이고, 여기서는 사용하지 않습니다.

이제 프로그램을 실행하면 아래와 같은 화면이 보입니다:

![](/images/indexed_rectangle.png)

이제 인덱스 버퍼를 사용해 정점을 재사용하여 메모리를 아끼는 법을 배웠습니다. 
나중에 복잡한 3D 모델을 로딩할 챕터에서는 이러한 기능이 특히 중요합니다.

이전 챕터에서 버퍼와 같은 다중 리소스들을 한 번의 메모리 할당으로 진행해야 한다고 언급했지만 사실 그 이상으로 해야 할 일들이 있습니다. 
[드라이버 개발자가 추천하길](https://developer.nvidia.com/vulkan-memory-management) 정점과 인덱스 버퍼와 같은 다중 버퍼를 하나의 `VkBuffer`에 저장하고 `vkCmdBindVertexBuffers`와 같은 명령에서 오프셋을 사용하라고 합니다. 
이렇게 하면 데이터가 함께 존재하기 때문에 더 캐시 친화적입니다. 
또한 같은 렌더링 연산에 사용되는 것이 아니라면, 메모리 덩어리(chunk)를 여러 리소스에서 재사용 하는 것도 가능합니다.
이는 *앨리어싱(aliasing)*이라고 불리며 몇몇 Vulkan 함수는 이러한 동작을 수행하려 한다는 것을 알려주기 위한 명시적 플래그도 존재합니다.
이렇게 하면 

[C++ code](/code/21_index_buffer.cpp) /
[Vertex shader](/code/18_shader_vertexbuffer.vert) /
[Fragment shader](/code/18_shader_vertexbuffer.frag)
