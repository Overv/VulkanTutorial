## 서론

이제 각 정점에 대한 어트리뷰트를 정점 셰이더에 넘길 수 있게 되었습니다만, 전역 변수는 어떤가요? 이 챕터에서부터 3D 그래픽스로 넘어갈 것인데 그러려면 모델-뷰-투영(projection) 행렬이 있어야 합니다. 이를 정점 데이터로 포함할 수도 있지만 그건 메모리도 낭비될 뿐만 아니라 변환(transformation)이 바뀌게 되면 정점 버퍼가 업데이트되어야 하는 문제가 있습니다. 변환 정보는 매 프레임마다 변화합니다.

Vulkan에서 이를 처리하는 올바른 방법은 *리소스 기술자(resource descriptor)*를 사용하는 것입니다. 기술자는 셰이더가 버퍼나 이미지와 같은 리소스에 자유롭게 접근하게 해 주는 방법입니다. 우리는 변환 행렬을 가지고 있는 버퍼를 설정하고 정점 셰이더가 기술자를 통해 이에 접근할 수 있도록 할 것입니다. 기술자의 사용은 세 부분으로 이루어져 있습니다:

* 파이프라인 생성 시점에 기술자 레이아웃 명시
* 기술자 풀로부터 기술자 집합(set) 할당
* 렌더링 시점에 기술자 바인딩

*기술자 레이아웃*은 파이프라인에서 접근할 리소스의 타입을 명시하고, 이는 렌더 패스에서 접근할 어태치먼트의 타입을 명시하는 것과 비슷합니다. *기술자 집합*은 기술자에 바인딩될 버퍼나 이미지를 명시하는데, 이는 프레임버퍼가 렌더 패스 어태치먼트에 바인딩될 실제 이미지 뷰를 명시하는 것과 비슷합니다. 이후에 기술자 집합은 정점 버퍼나 프레임버퍼와 유사하게 그리기 명령에 바인딩됩니다.

기술자에는 다양한 종류가 있는데 이 챕터에서는 유니폼 버퍼 객체(uniform buffer object, UBO)를 사용할 것입니다. 다른 타입의 기술자에 대해서는 나중 챕터에서 알아볼 것이고, 사용 방법은 동일합니다. 정점 셰이더에서 사용하려고 하는 데이터가 아래와 같은 C 구조체 형식의 데이터라고 해 봅시다:

```c++
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
```

이 데이터를 `VkBuffer`를 통해 복사하고 UBO 기술자를 사용하여 정점 셰이더에서 아래와 같이 접근할 수 있습니다:

```glsl
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

매 프레임마다 모델, 뷰, 투영 행렬을 갱신하여 이전 챕터에서 만든 사각형이 3D 회전하도록 만들어 보겠습니다.

## 정점 셰이더

정점 셰이더가 UBO를 사용하도록 수정하는 것은 위에서 본 것과 같습니다. 또한 여러분들이 MVP 변환에는 익숙하다고 가정하고 있습니다. 잘 모르시면 첫 번째 챕터에서 언급한 [관련 자료](https://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/)를 살펴보십시오.

```glsl
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

`uniform`, `in`, `out` 선언의 순서는 상관 없다는 것에 유의하십시오. `binding` 지시자는 어트리뷰트의 `location` 지시자와 유사합니다. 이 바인딩을 기술자 레이아웃에서 참조할 것입니다. `gl_Potision`이 있는 라인은 클립 공간에서의 위치를 계산하기 위해 변환 행렬들을 사용하는 것으로 수정되었습니다. 2D 삼각형의 경우와는 다르게, 클립 공간 좌표의 마지막 요소는 `1`이 아닐 수 있습고, 이는 최종적으로 정규화된 장치 좌표계(normalized device coordinate)로 변환될 때 나눠지게 됩니다. 원근 투영을 위해 이러한 과정이 수행되고, 이를 *perspective division*이라고 합니다. 이로 인해 가까운 물체가 멀리 있는 물체보다 크게 표현됩니다.

## 기술자 집합 레이아웃

다음 단계는 C++ 쪽에서 UBO를 정의하고 Vulkan에 이 기술자를 알려주는 것입니다.

```c++
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
```

GLM을 사용하면 셰이더에서 정의한 데이터 타입과 정확히 일치됩니다. 행렬 내의 데이터는 셰이더에 전달되어야 하는 형식과 바이너리 수준에서 호환되기 때문에 단순히 `UniformBufferObject`를 `VkBuffer`로 `memcpy`하기만 하면 됩니다.

셰이더에서 사용하는 기술자 바인딩에 대한 세부사항을 파이프라인 생성시에 알려주어야 하며, 이는 정점 어트리뷰트와 `location` 인덱스에 대해 했던 작업과 비슷합니다. 이러한 정보들을 정의하기 위한 `createDescriptorSetLayout` 함수를 새로 만듭니다. 파이프라인 생성 시점에서 이러한 정보를 사용해야 하기 때문에, 파이프라인 생성 직전에 호출하도록 합니다.

```c++
void initVulkan() {
    ...
    createDescriptorSetLayout();
    createGraphicsPipeline();
    ...
}

...

void createDescriptorSetLayout() {

}
```

바인딩은 `VkDescriptorSetLayoutBinding` 구조체를 통해 기술됩니다.

```c++
void createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
}
```

첫 두 필드는 셰이더에서 사용하는 `binding`과 기술자의 타입인 UBO를 명시합니다. 셰이더 변수가 UBO의 배열을 표현하는 것도 가능하며 `descriptorCount`는 그 배열의 개수를 명시합니다. 애니메이션을 위해 스켈레톤(skeleton) 관절들의 회전을 명시하는 등의 목적으로 사용이 가능합니다. MVP 변환은 하나의 UBO이므로 `descriptorCount`는 `1`을 사용합니다.

```c++
uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
```

또한 어떤 셰이더 단계를 기술자가 참조할지를 명시해야 합니다. `stageFlags` 필드는 `VkShaderStageFlagBits` 값의 조합, 또는 `VK_SHADER_STAGE_ALL_GRAPHICS` 일 수 있습니다. 우리의 경우 정점 셰이더에서만 기술자를 참조합니다.

```c++
uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
```

`pImmutableSamplers` 필드는 이미지 샘플링과 관련된 기술자에서만 사용되며 나중에 살펴볼 것입니다. 지금은 그냥 기본값으로 둡니다.

모든 기술자의 바인딩은 하나의 `VkDescriptorSetLayout` 객체로 표현됩니다. `pipelineLayout` 위에 새로운 클래스 멤버를 정의합니다:

```c++
VkDescriptorSetLayout descriptorSetLayout;
VkPipelineLayout pipelineLayout;
```

`vkCreateDescriptorSetLayout`를 사용해 레이아웃을 생성합니다. 이 함수는 `VkDescriptorSetLayoutCreateInfo`와 바인딩 배열을 받습니다:

```c++
VkDescriptorSetLayoutCreateInfo layoutInfo{};
layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
layoutInfo.bindingCount = 1;
layoutInfo.pBindings = &uboLayoutBinding;

if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
}
```

파이프라인 생성 시점에 기술자 집합 레이아웃을 명시해서 Vulkan에게 셰이더가 어떤 기술자를 사용할 것인지를 알려 주어야 합니다. 기술자 집합 레이아웃은 파이프라인 레이아웃 객체에 명시됩니다. `VkPipelineLayoutCreateInfo`를 수정하여 레이아웃 객체를 참조하도록 합니다:

```c++
VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 1;
pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
```

이 부분에 왜 여러개의 기술자 집합 레이아웃을 명시할 수 있게 되어있는지 궁금하실겁니다. 하나의 기술자 집합 레이아웃에 이미 모든 바인딩 정보가 들어있는데도 말이죠. 다음 챕터에서 기술자 풀과 기술자 집합을 살펴보면서 이에 대한 이야기를 해 보도록 하겠습니다.

기술자 레이아웃은 새로운 그래픽스 파이프라인이 생성될 동안, 즉 프로그램 종료 시까지 유지되어야 합니다:

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    ...
}
```

## 유니폼 버퍼(Uniform buffer)

다음 챕터에서 우리는 셰이더에서 사용할 UBO 데이터를 가진 버퍼를 명시할 예정입니다. 그러려면 먼저 버퍼를 만들어야겠죠. 매 프레임 새로운 데이터를 유니폼 버퍼에 복사할 예정이므로 스테이징 버퍼를 사용하는 것은 적절하지 않습니다. 단지 추가적인 작업으로 인해 성능이 낮아질 뿐입니다.

여러 프레임이 동시에 작업되기 때문에 버퍼가 여러 개 필요합니다. 버퍼에서 값을 읽는 동안 다름 프레임을 위한 데이터를 덮어 쓰면 안되기 때문이죠. 따라서 사용하고 있는 프레임의 개수만큼 유니폼 버퍼가 필요하며, GPU가 읽고 있는 버퍼가 아닌 다른 버퍼에 값을 기록해야 합니다.

`uniformBuffers`와 `uniformBuffersMemory`를 새로운 클래스 멤버로 추가합니다:

```c++
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

std::vector<VkBuffer> uniformBuffers;
std::vector<VkDeviceMemory> uniformBuffersMemory;
std::vector<void*> uniformBuffersMapped;
```

비슷하게, 버퍼를 할당하는 `createUniformBuffers` 함수를 새로 만들고 `createIndexBuffer` 뒤에 호출하도록 합니다:

```c++
void initVulkan() {
    ...
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    ...
}

...

void createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}
```

`vkMapMemory`를 사용해 버퍼를 생성한 뒤에 곧바로 맵핑하여 데이터를 쓰기 위한 포인터를 얻습니다. 프로그램의 실행 내내 버퍼는 이 포인터에 맵핑된 상태가 됩니다. 이러한 기술은 **"지속적 맵핑(persistent mapping)"**이라고 하며 모든 Vulkan 구현에서 동작합니다. 매번 데이터를 갱신할 때마다 버퍼를 맵핑하지 않아도 되기 때문에 성능이 증가합니다.

유니폼 데이터는 모든 드로우 콜(draw call)에서 활용되기 때문에 렌더링이 끝났을 때 해제되어야 합니다.

```c++
void cleanup() {
    ...

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    ...

}
```

## 유니폼 데이터 갱신

`updateUniformBuffer` 함수를 새로 만들고 `drawFrame` 함수에서 다음 프레임의 제출 전에 호출하도록 합니다:

```c++
void drawFrame() {
    ...

    updateUniformBuffer(currentFrame);

    ...

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    ...
}

...

void updateUniformBuffer(uint32_t currentImage) {

}
```

이 함수에서 매 프레임 새로운 변환을 생성하여 물체가 회전하도록 합니다. 이러한 기능을 구현하기 위해 두 개의 새로운 헤더를 include합니다:

```c++
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
```

`glm/gtc/matrix_transform.hpp` 헤더는 `glm::rotate`와 같은 모델 변환, `glm::lookAt`과 같은 뷰 변환, `glm::perspective`와 같은 투영 변환을 생성하기 위한 함수를 제공합니다. `glm::rotate`과 같은 함수가 라디안(radian)을 받도록 `GLM_FORCE_RADIANS`를 정의하여 혼동이 없도록 해야 합니다.

`chrono` 표준 라이브러리 헤더는 정확한 시간과 관련된 함수를 제공합니다. 이를 사용하여 프레임 레이트와 상관없이 물체가 1초에 90도 회전하도록 할 것입니다.

```c++
void updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
}
```

`updateUniformBuffer` 함수는 렌더링 시작부터 현재까지의 시간을 부동소수점 정밀도로 계산하기 위한 로직을 작성하는 것부터 시작합니다.

이제 UBO에 모델, 뷰, 투영 변환을 정의합니다. 모델 회전은 Z축에 대한 회전이며, `time` 변수를 사용합니다:

```c++
UniformBufferObject ubo{};
ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
```

`glm::rotate` 함수는 기존 변환과 회전 각도, 회전축을 매개변수로 받습니다. `glm::mat4(1.0f)` 생성자는 단위 행렬(identity matrix)를 반환합니다. `time * glm::radians(90.0f)`를 회전 각도로 사용함으로써 1초에 90도 회전을 하게 할 수 있습니다.

```c++
ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
```

뷰 변환에 대해서는 45도 위에서 물체를 바라보도록 했습니다. `glm::lookAt`은 눈의 위치, 바라보는 지점과 업(up) 벡터를 매개변수로 받습니다.

```c++
ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 10.0f);
```

45도의 수직 시야각(field-of-view)를 갖도록 원근 투영을 정의했습니다. 나머지 매개변수는 종횡비(aspect ratio), 근면(near plane)과 원면(far plane)입니다. 현재 스왑 체인의 범위(extent)를 기반으로 종횡비를 계산하여 윈도우 크기가 변해도 새로운 너비와 높이를 반영할 수 있도록 하는 것이 중요합니다.

```c++
ubo.proj[1][1] *= -1;
```

GLM은 원래 OpenGL을 기반으로 설계되었기 때문에 클립 좌표계의 Y축이 뒤집혀 있습니다. 이를 보정하기 위한 가장 간단한 방법은 투영행렬의 Y축 크기변환(scaling) 요소의 부호를 바꿔주는 것입니다. 이렇게 하지 않으면 위아래가 뒤집혀서 렌더링됩니다.

모든 변환이 정의되었으니 UBO의 데이터를 현재 유니폼 버퍼로 복사할 수 있습니다. 이 과정은 정점 버퍼에서와 완전히 동일하며, 스테이징 버퍼가 없다는 점만 다릅니다. 전에 이야기한 것처럼 유니폼 버퍼는 한 번만 맵핑하므로 다시 맵핑할 필요 없이 쓰기만 수행하면 됩니다:

```c++
memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
```

UBO를 이런 방식으로 사용하는 것은 자주 바뀌는 값을 셰이더에 전달하기 위한 효율적인 방법은 아닙니다. 작은 버퍼의 데이터를 셰이더에 전달하기 위한 더 효율적인 방법은 *Push 상수(push constant)*입니다. 이에 대해서는 나중 챕터에서 알아보도록 하겠습니다.

다음 챕터에서는 `VkBuffer`들을 유니폼 버퍼 기술자에 바인딩하는 기술자 집합에 대해 알아볼 것입니다. 이를 통해 셰이더가 이러한 변환 데이터에 접근할 수 있게 될 것입니다.

[C++ code](/code/22_descriptor_layout.cpp) /
[Vertex shader](/code/22_shader_ubo.vert) /
[Fragment shader](/code/22_shader_ubo.frag)
