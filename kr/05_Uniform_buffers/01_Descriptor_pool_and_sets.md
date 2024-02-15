## 개요

이전 장에서의 기술자 레이아웃은 바인딩 될 수 있는 기술자의 타입을 명시합니다. 이 장에서는 각 `VkBuffer` 리소스를 위한 기술자 집합을 만들어서 유니폼 버퍼 기술자에 바인딩할 것입니다.

## 기술자 풀

기술자 집합은 직접 만들 수 없고 명령 버퍼처럼 풀로부터 할당되어야 합니다. 기술자 집합에서 이에 대응하는 것은 당연하게도 *기술자 풀*입니다. 이를 설정하기 위해 `createDescriptorPool` 함수를 새로 작성합니다.

```c++
void initVulkan() {
    ...
    createUniformBuffers();
    createDescriptorPool();
    ...
}

...

void createDescriptorPool() {

}
```

먼저 기술자 집합이 어떤 기술자 타입을 포함할 것인지, 몇 개를 포함할 것인지를 `VkDescriptorPoolSize` 구조체를 통해 명시합니다.

```c++
VkDescriptorPoolSize poolSize{};
poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
```

이 기술자 중 하나를 매 프레임 할당할 것입니다. 이 풀 크기에 대한 구조체는 `VkDescriptorPoolCreateInfo`에서 참조됩니다: 

```c++
VkDescriptorPoolCreateInfo poolInfo{};
poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
poolInfo.poolSizeCount = 1;
poolInfo.pPoolSizes = &poolSize;
```

가용한 개별 기술자의 최대 숫자와는 별개로 할당될 기술자 집합의 최대 숫자도 명시해야 합니다:

```c++
poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
```

명령 풀과 유사하게 이 구조체도 개별 기술자 집합이 해제가 될 수 있을지에 대한 선택적인 플래그로 `VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT`가 존재합니다. 기술자 세트는 생성한 이후에 건들지 않을 것이므로 이 플래그를 사용하지는 않을 것입니다. 따라서 `flags`는 기본값인 `0`으로 두면 됩니다.

```c++
VkDescriptorPool descriptorPool;

...

if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
}
```

기술자 풀의 핸들을 저장하기 위한 클래스 멤버를 추가하고 `vkCreateDescriptorPool`를 호출하여 생성합니다.

## 기술자 집합

이제 기술자 집합을 할당할 수 있습니다. 이를 위해 `createDescriptorSets` 함수를 추가합니다:

```c++
void initVulkan() {
    ...
    createDescriptorPool();
    createDescriptorSets();
    ...
}

...

void createDescriptorSets() {

}
```

기술자 집합의 할당은 `VkDescriptorSetAllocateInfo` 구조체를 사용합니다. 어떤 기술자 풀에서 할당할 것인지, 기술자 집합을 몇 개나 할당할 것인지, 기반이 되는 기술자 레이아웃이 무엇인지 등을 명시합니다:

```c++
std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
VkDescriptorSetAllocateInfo allocInfo{};
allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
allocInfo.descriptorPool = descriptorPool;
allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
allocInfo.pSetLayouts = layouts.data();
```

우리의 경우 사용 중인 각 프레임마다 하나의 기술자 집합을 생성할 것이고, 레이아웃은 모두 동일합니다. 안타깝게도 이 레이아웃들을 모두 복사해야만 하는데 이 다음 함수에서 집합의 개수와 배열의 개수가 일치되어야 하기 떄문입니다.

기술자 집합 핸들을 저장할 클래스 멤버를 추가하고 `vkAllocateDescriptorSets`를 사용해 할당합니다:

```c++
VkDescriptorPool descriptorPool;
std::vector<VkDescriptorSet> descriptorSets;

...

descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
}
```

기술자 집합은 기술자 풀이 소멸될 때 자동으로 해제되므로 명시적으로 정리해 줄 필요는 없습니다. `vkAllocateDescriptorSets` 호출은 기술자 집합을 할당하고 각각은 하나의 유니폼 버퍼 기술자를 갖고 있습니다.

```c++
void cleanup() {
    ...
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    ...
}
```

이제 기술자 집합은 할당되었으나 그 안의 기술자에 대한 구성이 남아 있습니다. 이제 각 기술자를 생성하기 위한 반복문을 추가합니다.

```c++
for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

}
```

우리 유니폼 버퍼 기술자와 같이, 버퍼를 참조하는 기술자는 `VkDescriptorBufferInfo` 구조체로 설정할 수 있습니다. 이 구조체는 버퍼와 데이터가 들어있는 버퍼의 영역을 명시합니다.

```c++
for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);
}
```

지금 우리가 하는 것처럼 전체 버퍼를 덮어쓰는 상황이라면 range에 `VK_WHOLE_SIZE`를 사용해도 됩니다. 기술자의 구성은 `vkUpdateDescriptorSets` 함수를 사용해 갱신되는데 `VkWriteDescriptorSet` 구조체의 배열을 매개변수로 받습니다. 

```c++
VkWriteDescriptorSet descriptorWrite{};
descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrite.dstSet = descriptorSets[i];
descriptorWrite.dstBinding = 0;
descriptorWrite.dstArrayElement = 0;
```

첫 두 필드는 갱신하고 바인딩할 기술자 집합을 명시합니다. 우리는 유니폼 버퍼 바인딩 인덱스로 `0`을 부여했습니다. 기술자는 배열일 수도 있으므로 갱신하고자 하는 첫 인덱스를 명시해 주어야 합니다. 지금은 배열이 아니므로 인덱스로는 `0`을 사용합니다.

```c++
descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
descriptorWrite.descriptorCount = 1;
```

기술자의 타입을 다시 명시해 주어야 합니다. `dstArrayElement` 인덱스부터 시작해서 배열의 여러 기술자를 한꺼번에 갱신하는 것이 가능합니다. `descriptorCount` 필드가 갱신할 배열의 요소 개수를 명시하게 됩니다.

```c++
descriptorWrite.pBufferInfo = &bufferInfo;
descriptorWrite.pImageInfo = nullptr; // Optional
descriptorWrite.pTexelBufferView = nullptr; // Optional
```

마지막 필드는 실제 기술자를 구성할 `descriptorCount`개의 구조체 배열을 참조합니다. 셋 중에 실제로 사용할 것이 무엇인지에 따라 달라집니다. `pBufferInfo` 필드는 버퍼 데이터를 참조하는 경우 사용되고, `pImageInfo`는 이미지 데이터를 참조하는 경우, `pTexelBufferView`는 버퍼 뷰를 참조하는 기술자에 대해 사용됩니다. 우리의 경우 버퍼를 참조하므로 `pBufferInfo`를 사용합니다.

```c++
vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
```

갱신은 `vkUpdateDescriptorSets`를 사용해 이루어집니다. 두 종류의 배열을 매개변수로 받는데 `VkWriteDescriptorSet`과 `VkCopyDescriptorSet` 입니다. 후자는 이름 그대로 기술자들끼리 복사할 때 사용됩니다.

## 기술자 집합 사용

이제 `recordCommandBuffer` 함수를 갱신해서 실제로 각 프레임에 대한 올바른 기술자 세트를 셰이더의 기술자와 `vkCmdBindDescriptorSets`를 통해 바인딩해야 합니다. 이는 `vkCmdDrawIndexed` 호출 전에 이루어져야 합니다:

```c++
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
```

정점 버퍼나 인덱스 버퍼와는 다르게, 기술자 집합은 그래픽스 파이프라인에서만 사용되는 것은 아닙니다. 따라서 기술자 집합을 그래픽스 또는 컴퓨트 파이프라인 중 어디에 사용할 것인지를 명시해야 합니다. 다음 매개변수는 기술자가 기반으로 하는 레이아웃입니다. 그 다음 세 개의 매개변수는 첫 기술자 집합의 인덱스와 바인딩할 집합의 개수, 그리고 바인딩할 집합의 배열입니다. 이에 대해선 잠시 뒤에 다시 실펴볼 것입니다. 마지막 두 개의 매개변수는 동적(dynamic) 기술자를 사용할 때를 위한 오프셋의 배열을 명시합니다. 이에 대해서는 나중 챕터에서 알아보겠습니다.

지금 시점에 프로그램을 실행하면 아무것도 보이지 않을 겁니다. 문제는 우리가 투영 행렬에 Y 뒤집기를 수행했기 때문에 정점이 시계방향 순서가 아닌 반시계 방향 순서로 그려진다는 것입니다. 이로 인해 후면 컬링(backface culling)이 동작하여 아무것도 그려지지 않게 됩니다. `createGraphicsPipeline` 함수로 가서  `VkPipelineRasterizationStateCreateInfo`의 `frontFace`를 바로잡아줍니다:

```c++
rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
```

이제 다시 실행해보면 아래와 같이 보일겁니다:

![](/images/spinning_quad.png)

이제 투영 행렬이 종횡비에 맞게 투영하므로 사각형이 정사각형으로 보입니다. `updateUniformBuffer`가 화면 크기 변경을 처리하므로 `recreateSwapChain`에서 기술자 집합을 다시 생성할 필요는 없습니다.

## 정렬 요구조건(Alignment requirements)

지금까지 대충 넘어갔던 것 중의 하나는 셰이더에서의 유니폼 정의와 C++ 구조체가 어떻게 일치해야 하는가에 관한 것입니다. 양 쪽에 동일한 타입을 사용하는 것이 당연해 보입니다:

```c++
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
```

하지만 그냥 이것으로 끝은 아닙니다. 예를 들어 구조체와 셰이더를 아래와 같이 수정해 봅시다:

```c++
struct UniformBufferObject {
    glm::vec2 foo;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

layout(binding = 0) uniform UniformBufferObject {
    vec2 foo;
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
```

셰이더를 다시 컴파일하고 프로그램을 실행하면 지금까지 보였던 사각형이 사라진 것을 볼 수 있습니다! 왜냐하면 *정렬 요구조건*을 고려하지 않았기 때문입니다.

Vulkan은 구조체의 데이터가 메모리에 특정한 방식으로 정렬되어 있을 것이라고 예상합니다. 예를 들어:

* 스칼라 값은 N으로 정렬 (= 32비트 float의 경우 4바이트)
* `vec2`는 2N으로 정렬 (= 8바이트)
* `vec3` 또는 `vec4`는 4N으로 정렬 (= 16바이트)
* 중접된 구조체는 멤버의 기본 정렬을 16의 배수로 반올림한 것으로 정렬
* `mat4` 행렬은 `vec4`와 동일한 정렬이어야 함

정렬 요구조건에 대한 전체 내용은 [해당하는 명세](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap15.html#interfaces-resources-layout)를 보시면 됩니다.

원래 우리의 셰이더는 세 개의 `mat4` 필드를 사용하였으므로 항상 정렬 요구조건을 만족하였습니다. 각 `mat4`는 4 x 4 x 4 = 64바이트이고, `model`은 오프셋 `0`, `view`는 오프셋 `64`, `proj`는 오프셋 `128`입니다. 각각이 16의 배수이므로 문제가 없었습니다.

8바이트 크기인 `vec2`가 추가된 새 구조체로 인해 모든 오프셋이 맞지 않게 됩니다. 이제 `model`은 오프셋 `8`, `view`는 오프셋 `72`, `proj`는 오프셋 `136`이므로 16의 배수가 아닙니다. 이 문제를 해결하기 위해서는 C++11에서 추가된 [`alignas`](https://en.cppreference.com/w/cpp/language/alignas) 지정자를 사용하게 됩니다.

The new structure starts with a `vec2` which is only 8 bytes in size and therefore throws off all of the offsets. Now `model` has an offset of `8`, `view` an offset of `72` and `proj` an offset of `136`, none of which are multiples of 16. To fix this problem we can use the [`alignas`](https://en.cppreference.com/w/cpp/language/alignas) specifier introduced in C++11:

```c++
struct UniformBufferObject {
    glm::vec2 foo;
    alignas(16) glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
```

이제 컴파일하고 다시 실행해 보면 셰이더가 올바를 행렬값을 얻어오는 것을 볼 수 있습니다. GLM을 include하기 직전에 `GLM_FORCE_DEFAULT_ALIGNED_GENTYPES`를 정의할 수 있습니다:

```c++
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
```

이렇게 하면 GLM이 정렬 요구사항이 이미 만족된 `vec2`와 `mat4`를 사용하게 됩니다. 이 정의를 추가하면 `alignas` 지정자를 없애도 제대로 동작합니다.

안타깝게도 이 방법은 중첩된 구조체를 사용하면 통하지 않게 됩니다. C++에서 아래와 같은 정의를 생각해 보세요:

```c++
struct Foo {
    glm::vec2 v;
};

struct UniformBufferObject {
    Foo f1;
    Foo f2;
};
```

그리고 셰이더에서는 다음과 같이 정의했습니다:

```c++
struct Foo {
    vec2 v;
};

layout(binding = 0) uniform UniformBufferObject {
    Foo f1;
    Foo f2;
} ubo;
```

이 경우 `f2`는 오프셋 `8`을 갖게 되는데 실제로는 중첩된 구조체이기 때문에 `16`을 가져야만 합니다. 이러한 경우엔 정렬을 직접 명시해 주어야 합니다:

```c++
struct UniformBufferObject {
    Foo f1;
    alignas(16) Foo f2;
};
```

교훈은, 정렬을 언제나 명시해 주는 것이 좋다는 겁니다. 그렇게 하면 정렬 오류로 인해 생기는 이상한 문제들을 방지할 수 있습니다.

```c++
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
```

`foo`를 삭제한 뒤 셰이더를 다시 컴파일하는 것을 잊지 마세요.

## 다중 기술자 집합

몇몇 구조체와 함수 호출에서 눈치 채실 수 있듯이, 다중 기술자 집합을 동시에 바인딩 하는 것이 가능합니다. 이 경우 각 기술자 집합에 대해 파이프라인 레이아웃 생성시에 기술자 레이아웃을 생성해야 합니다. 셰이더에서는 특정 기술자 집합을 아래와 같이 참조해야 합니다:

```c++
layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
```

객체별로 다른 기술자를 사용하거나 별도의 기술자 집합에서 공유하는 기술자를 사용할 때 이러안 기능을 활용할 수 있습니다. 이 경우 드로우 콜마다 대부분의 기술자를 다시 바인딩하지 않아도 되어서 더 효율적일 수 있습니다.

[C++ code](/code/23_descriptor_sets.cpp) /
[Vertex shader](/code/22_shader_ubo.vert) /
[Fragment shader](/code/22_shader_ubo.frag)
