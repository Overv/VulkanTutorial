## 서론

유니폼 버퍼 튜토리얼에서 처음으로 기술자에 대해 알아봤었습니다. 이 챕터에서는 새로운 종류의 기술자인 *결합된 이미지 샘플러(combined image sampler)* 에 대해 알아보겠습니다. 이 기술자를 사용해서 셰이더로부터 우리가 만든 샘플러 객체를 거쳐 이미지 리소스에 접글할 수 있게 됩니다.

먼저 기술자 집합 레이아웃, 기술자 풀, 기술자 집합이 결합된 이미지 샘플러와 같은 것을 포함할 수 있도록 수정할 것입니다. 그 이후에 `Vertex`에 텍스처 좌표를 추가하고 프래그먼트 셰이더를 수정하여 정점 색상을 보간하는 것이 아니라 텍스처로부터 색상값을 읽어오도록 할 것입니다.

## 기술자 수정

`createDescriptorSetLayout` 함수로 가서 결합된 이미지 샘플러 기술자를 위해 `VkDescriptorSetLayoutBinding`를 추가합니다. 유니폼 버퍼 이후에 바인딩에 추가 합니다:

```c++
VkDescriptorSetLayoutBinding samplerLayoutBinding{};
samplerLayoutBinding.binding = 1;
samplerLayoutBinding.descriptorCount = 1;
samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
samplerLayoutBinding.pImmutableSamplers = nullptr;
samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
VkDescriptorSetLayoutCreateInfo layoutInfo{};
layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
layoutInfo.pBindings = bindings.data();
```

`stageFlags`를 사용해 결합된 이미지 샘플러 기술자가 프래그먼트 셰이더에서 사용될 것이라는 것을 꼭 명시하십시오. 프래그먼트의 색상이 결정되는 것은 그 시점이기 때문입니다. 정점 셰이더에서 텍스처 샘플링을 하는 것도 가능한데, 예를 들어 정점들을 [하이트맵(heightmap)](https://en.wikipedia.org/wiki/Heightmap)을 기반으로 동적으로 변경하고 하려고 하는 경우에 사용할 수 있습니다.

또한 결합된 이미지 샘플러를 위해 기술자 풀을 넉넉하게 만들어야 합니다. `VkDescriptorPoolCreateInfo`에 `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER` 타입의 `VkPoolSize`를 추가할 것입니다. `createDescriptorPool` 함수로 가서 이 기술자를 위한 `VkDescriptorPoolSize`를 포함하도록 수정합니다:

```c++
std::array<VkDescriptorPoolSize, 2> poolSizes{};
poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

VkDescriptorPoolCreateInfo poolInfo{};
poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
poolInfo.pPoolSizes = poolSizes.data();
poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
```

부적합한 기술자 풀은 검증 레이어가 문제를 탐지하지 못하는 대표적인 예입니다 (Vulkan 1.1 기준). 풀이 충분히 크지 않다면 `vkAllocateDescriptorSets`은 `VK_ERROR_POOL_OUT_OF_MEMORY` 오류 코드와 함께 실패하지만 드라이버 내부적으로 문제를 해결하려 시도합니다. 즉 어떤 경우에는 (하드웨어 및 풀 크기와 할당 크기에 따라) 기술자 풀의 크기 제한을 넘는 경우에도 드라이버가 우리의 할당 문제를 회피할 수 있게 해줄수도 있습니다. 그렇지 못한 경우에는 `vkAllocateDescriptorSets`가 실패하고 `VK_ERROR_POOL_OUT_OF_MEMORY`를 반환합니다. 어떤 환경에서는 할당에 성공하고 어떤 환경에서는 실패하기 때문에 까다로운 문제입니다.

Vulkan은 할당과 관련한 역할을 드라이버에 맡기기 때문에, 특정한 타입 (`VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER` 등)을의 기술자를 `descriptorCount` 멤버에 명시된 숫자대로 생성하는 것은 더 이상 엄격한 요구사항이 되지 못합니다. 하지만 그것을 지키는 것이 좋은 구현 방법이고 추후에는 [검증 모범사례](https://vulkan.lunarg.com/doc/view/1.2.189.0/linux/best_practices.html)를 활성화하는 경우 `VK_LAYER_KHRONOS_validation`가 이러한 종류의 문제에 대한 경고를 하게 될 것입니다.

마지막 단계는 실제 이미지와 샘플러 리소스를 기술자 집합의 기술자들에 바인딩하는 것입니다. `createDescriptorSets` 함수로 가 봅시다.

```c++
for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureImageView;
    imageInfo.sampler = textureSampler;

    ...
}
```

결합된 이미지 샘플러 구조체의 리소스는 `VkDescriptorImageInfo` 구조체에 명시되어야 하고, 이는 유니폼 버퍼 기술자의 버퍼 리소스가 `VkDescriptorBufferInfo` 구조체에 명시되었던 것과 같습니다. 이제 이전 챕터에서의 구조체들이 함께 활용됩니다.

```c++
std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[0].dstSet = descriptorSets[i];
descriptorWrites[0].dstBinding = 0;
descriptorWrites[0].dstArrayElement = 0;
descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
descriptorWrites[0].descriptorCount = 1;
descriptorWrites[0].pBufferInfo = &bufferInfo;

descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[1].dstSet = descriptorSets[i];
descriptorWrites[1].dstBinding = 1;
descriptorWrites[1].dstArrayElement = 0;
descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
descriptorWrites[1].descriptorCount = 1;
descriptorWrites[1].pImageInfo = &imageInfo;

vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
```

기술자는 버퍼와 동일하게 이미지 정보와 함께 갱신되어야 합니다. 이번에는 `pBufferInfo`가 아닌 `pImageInfo`가 사용됩니다. 이제 셰이더에서 기술자를 활용항 준비가 되었습니다!

## 텍스처 좌표

텍스처 맵핑을 위한 중요한 요소가 아직 빠져 있고, 이는 각 정점의 텍스처 좌표입니다. 텍스처 좌표는 이미지가 어떻게 대상에 맵핑될 것인지를 결정합니다.

```c++
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};
```

`Vertex` 구조체를 텍스처 좌표인 `vec2`를 포함하도록 수정합니다. `VkVertexInputAttributeDescription`도 추가해서 정점 셰이더의 입력으로 텍스처 좌표를 사용하도록 합니다. 이렇게 해야 이 값을 프래그먼트 셰이더로 넘길때 사각형의 표면에 걸쳐 보간이 이루어집니다.

```c++
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};
```

이 튜토리얼에서 저는 왼쪽 위 모서리에 `0, 0`을, 오른쪽 아래 모서리에 `1, 1`을 사용해서 텍스처가 사각형을 채우도록 하였습니다. 다른 좌표값으로 테스트 해 보세요. `0` 이하의 값이나 `1` 이상의 값으로 어드레싱 모드가 어떻게 동작하는지 살펴보세요!

## 셰이더

마지막 단계는 셰이더를 수정해 텍스처로부터 색상을 샘플링하도록 하는 것입니다. 먼저 정점 셰이더를 수정해서 프래그먼트 셰이더로 텍스처 좌표를 넘기도록 합니다:

```glsl
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
```

정점별 색상과 동일하게 `fragTexCoord`값도 래스터라이저에 의해 사각형 전체에 걸쳐 부드럽게 보간됩니다. 텍스처 좌표를 프래그먼트 세이더의 출력 색상으로 하여 이를 눈으로 확인할 수 있습니다:

```glsl
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragTexCoord, 0.0, 1.0);
}
```

아래와 같은 이미지가 보일 겁니다. 셰이더를 다시 컴파일하는 것을 잊지 마세요!

![](/images/texcoord_visualization.png)

초록색 채널이 수평축 좌표, 빨간색 채널이 수직축 좌표입니다. 검은색과 노란색 모서리를 통해 텍스처 좌표가 `0, 0`에서 `1, 1` 사이로 보간되었다는 것을 확인 가능합니다. 색상값으로 데이터를 가시화 하는 것이 셰이더 프로그래밍에서는 `printf`와 같은 겁니다. 더 나은 대안이 없기 때문이죠!

결합된 이미지 샘플러 기술자는 GLSL에서 샘플러 유니폼으로 표현됩니다. 프래그먼트 셰이더에서 이에 대한 참조를 추가합니다:

```glsl
layout(binding = 1) uniform sampler2D texSampler;
```

다른 타입의 이미지를 위한 `sampler1D`와 `sampler3D` 타입도 있습니다. 올바른 바인딩을 해야 하는 것에 주의 하세요.

```glsl
void main() {
    outColor = texture(texSampler, fragTexCoord);
}
```

텍스처는 `texture` 내장함수에 의해 샘플링됩니다. 이 함수는 `sampler`와 텍스처 좌표를 인자로 받습니다. 샘플러는 필터링과 변환을 자동적으로 수행해줍니다. 이제 프로그램을 실행하면 사각형 위에 텍스처가 보일 겁니다:

![](/images/texture_on_square.png)

텍스처 좌표를 `1`보다 큰 값으로 해서 어드레싱 모드를 살펴 보세요. 예를들어 다음 프래그먼트 셰이더는 `VK_SAMPLER_ADDRESS_MODE_REPEAT`인 경우 아래와 같은 이미지를 나타냅니다:

```glsl
void main() {
    outColor = texture(texSampler, fragTexCoord * 2.0);
}
```

![](/images/texture_on_square_repeated.png)

텍스처 색상을 정점 색상을 활용해 변경하는 것도 가능합니다:

```glsl
void main() {
    outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
}
```

여기서는 RGB와 알파 채널을 분리해서 알파 채널값은 영향을 받지 않도록 하였습니다.

![](/images/texture_on_square_colorized.png)

이제 셰이더에서 이미지에 접근하는 법을 알았습니다! 프레임버퍼에 쓰여진 이미지와 결합하게 되면 아주 강력한 기술이 됩니다. 그러한 이미지를 입력으로 사용해 멋진 후처리(post-processing) 효과나 3D 공간상에서 카메라를 표현하는 등의 작업을 할 수 있습니다.

[C++ code](/code/26_texture_mapping.cpp) /
[Vertex shader](/code/26_shader_textures.vert) /
[Fragment shader](/code/26_shader_textures.frag)
