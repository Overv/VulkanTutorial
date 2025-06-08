## 서론

지금까지 작업한 물체는 3차원으로 투영되긴 했지만 여전히 평평한 물체입니다. 이 챕터에서는 3D 메쉬를 위해 Z 좌표를 추가할 예정입니다. 새로 추가된 세 번째 좌표를 가지고 사각형을 지금 있는 사각형 위에 그려 봄으로써 물체들이 깊이 값에 따른 정렬(sort)이 되지 않으면 발생하는 문제를 살펴볼 것입니다.

## 3D 형상(geometry)

`Vertex` 구조체를 수정하여 위치값으로 3차원 벡터를 사용하도록 하고 이와 대를되는 `VkVertexInputAttributeDescription`의 `format`도 갱신합니다:

```c++
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    ...

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        ...
    }
};
```

다음으로 정점 셰이더가 3차원 좌표를 입력으로 받아서 변환을 수행하도록 바꿉니다. 수정 후에 다시 컴파일하는 것을 잊지 마세요!

```glsl
layout(location = 0) in vec3 inPosition;

...

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
```

마지막으로 `vertices` 컨테이너를 Z 좌표를 포함하도록 수정합니다:

```c++
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};
```

지금 프로그램을 실행하전 전과 동일한 결과가 나타납니다. 좀 더 재미있는 결과를 보기 위해 형상들을 추가해서 이 챕터에서 다루고자하는 문제를 보여드리겠습니다. 정점들을 복사해서 현재 사각형 아래에 새로운 사각형이 아래 그림과 같이 위치하도록 정의합니다:

![](/images/extra_square.svg)

Z 좌표로 `-0.5f`를 사용하고 추가된 사각형에 대한 인덱스들도 추가합니다:

```c++
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};
```

이제 프로그램을 실행하면 마치 에셔의 그림같은 결과를 볼 수 있습니다:

![](/images/depth_issues.png)

문제는 아래에 위치한 사각형이 위쪽 사각형을 구성하는 프래그먼트 위에 그려진다는 것인데, 이는 아래 위치한 사각형의 인덱스가 인덱스 배열의 뒤쪽에 있기 때문입니다. 이 문제를 해결하는 방법은 두 가지가 있습니다:

* 모든 드로우콜을 뒤쪽에서 앞쪽 깊이값 순서로 정렬
* 깊이 버퍼를 사용해 깊이 테스트를 수행
 
첫 번째 접근법은 투명한 물체를 그릴 때 일반적으로 사용되는 방법인데, 순서와 무관하게 투명한 물체를 그리는 것은 상당히 어려운 문제이기 때문입니다. 프래그먼트를 깊이 순서대로 정렬하는 문제는 *깊이 버퍼*를 사용해서 해결하는 것이 일반적입니다. 깊이 버퍼는 모든 위치값에 대해 깊이를 저장하기 위해 사용하는 추가적인 어태치먼트입니다. 색상 어태치먼트가 색상 값을 저장하는 것과 다를 것이 없습니다. 래스터라이저가 프래그먼트를 만들어 낼 때마가 깊이 테스트를 통해 새로운 프래그먼트가 이미 쓰여저 있는 프래그먼트보다 더 가까이 있는 것인지를 테스트합니다. 그렇지 않은 경우에는 프래그먼트가 버려집니다(discarded). 깊이 테스트를 통과하면 그 프래그먼트의 깊이 값이 깊이 버퍼에 쓰여집니다. 프래그먼트 셰이더에서 색상 출력값을 조정하는 것처럼 깊이 값을 조정하는 것도 가능합니다.


```c++
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
```

GLM에서 만든 원근 투영 행렬은 기본적으로 OpenGL에서 사용하는 깊이 범위인 `-1.0`에서 `1.0` 사이로 깊이값을 도출하게 되어 있습니다. `GLM_FORCE_DEPTH_ZERO_TO_ONE`를 정의하여 Vulkan에서 사용하는 `0.0`에서 `1.0` 범위가 되도록 설정해야 합니다.

## 깊이 이미지와 뷰

색상 어태치먼트처럼 깊이 어태치먼트도 이미지를 기반으로 정의됩니다. 차이점은 스왑 체인이 깊이 이미지를 자동으로 만들어 주지 않는다는 점입니다. 한 번에 하나의 그리기 연산만 수행되는 상태이기 때문에 현재는 깊이 이미지는 하나만 있으면 됩니다. 깊이 이미지 또한 세 가지 리소스인 이미지, 메모리, 이미지 뷰 리소스를 필요로 합니다:

```c++
VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;
```

이러한 리소스들을 준비하기 위해 `createDepthResources` 함수를 새로 만듭니다:

```c++
void initVulkan() {
    ...
    createCommandPool();
    createDepthResources();
    createTextureImage();
    ...
}

...

void createDepthResources() {

}
```

깊이 이미지를 만드는 것은 꽤 직관적입니다. 스왑 체인 범위를 통해 정의된 색상 어태치먼트와 동일한 해상도를 가져야 하며, 이미지의 사용법이 깊이 어태치먼트에 적합해야 하고, 타일링과 장치 로컬 메모리에 최적화되어야 합니다. 문제는, 깊이 이미지에 적합한 포맷이 무엇이냐 입니다. 깊이 이미지를 위한 포맷은 깊이 컴포넌트를 반드시 가져야 하고 이는 `VK_FORMAT_`에 `_D??_`로 표시되어 있습니다.

텍스처 이미지와는 다르게 특정 포맷을 사용해야 하는 것은 없는데 프로그램에서 텍셀 값을 직접 접근하지는 않을 것이기 때문입니다. 그냥 적절한 정밀도를 가지면 되는데 실제 응용 프로그램에서는 최소 24비트 정도를 사용합니다. 이러한 요구조건에 맞는 포맷들이 몇 가지 있습니다:

* `VK_FORMAT_D32_SFLOAT`: 32비트 부동소수점 깊이값
* `VK_FORMAT_D32_SFLOAT_S8_UINT`: 부호 있는 32비트 부동소수점 깊이값과 8비트의 스텐실 요소
* `VK_FORMAT_D24_UNORM_S8_UINT`: 24비트 부동소수점 깊이값과 8비트 스텐실 요소

스텐실 요소는 [스텐실 테스트](https://en.wikipedia.org/wiki/Stencil_buffer)에 사용되는데 깊이 테스트와 함께 사용되는 추가적인 테스트 입니다. 이에 대해서는 나중 챕터에서 살펴보도록 하겠습니다.

지금은 간단하게 `VK_FORMAT_D32_SFLOAT` 포맷을 사용할 것인데 이 포맷은 거의 대부분 하드웨어에서 지원되기 때문입니다(하드웨어 데이터베이스를 살펴보세요). 그래도 약간의 유연성을 가지도록 하는 것도 좋을 것 같습니다. `findSupportedFormat` 함수를 통해 선호도 순으로 정렬된 포맷들의 후보를 받고, 그 중 지원되는 가장 첫 번째 포맷을 확인합니다:

```c++
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {

}
```

포맷의 지원 여부는 타일링 모드와 사용법에 따라 다르기 때문에 이러한 요소들도 매개변수로 받아야 합니다. 포맷의 지원 여부는 `vkGetPhysicalDeviceFormatProperties` 함수를 통해 질의할 수 있습니다:

```c++
for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
}
```

`VkFormatProperties` 구조체는 세 개의 필드를 갖습니다:

* `linearTilingFeatures`: 선형 타일링이 지원되는 사용법
* `optimalTilingFeatures`: 최적 타일링지 지원되는 사용법
* `bufferFeatures`: 버퍼 용으로 지원되는 사용법

여기서는 첫 두 경우만 신경쓰면 되고, 확인은 함수에서 `tiling` 매개변수로 받은 값으로 수행합니다:

```c++
if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
    return format;
} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
    return format;
}
```

후보의 어떤 포맷도 지원되지 않는 경우 특수한 값을 반환하거나 예외를 던지도록 처리할 수 있습니다:

```c++
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}
```

이제 이 함수를 사용하는 `findDepthFormat` 헬퍼 함수를 만들어 깊이 요소를 가지면서 깊이 어태치먼트 사용법을 지원하는 포맷을 선택하도록 합니다:

```c++
VkFormat findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}
```

여기에서는 `VK_IMAGE_USAGE_` 대신 `VK_FORMAT_FEATURE_` 플래그를 사용해야 합니다. 후보 포맷들은 모두 깊이 요소를 가지고 있지만 뒤의 두 경우는 스텐실 요소도 포함합니다. 아직은 사용하지 않을 것이지만 이러한 포맷들에 대해 레이아웃 전환을 수행할 때에는 스텐실 요소도 고려해야 합니다. 간단한 헬퍼 함수를 하나 더 추가해서 선택된 깊이 포맷이 스텐실 요소를 포함하는지 체크합니다:

```c++
bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
```

`createDepthResources`에서 함수를 호출해 깊이 포맷을 찾습니다:

```c++
VkFormat depthFormat = findDepthFormat();
```

이제 `createImage`와 `createImageView` 헬퍼 함수를 호출하기 위한 모든 필요한 정보가 준비되었습니다:

```c++
createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
depthImageView = createImageView(depthImage, depthFormat);
```

하지만 `createImageView` 함수는 현재 서브리소스가 `VK_IMAGE_ASPECT_COLOR_BIT`인 것으로 가정하고 있기 때문에 이를 매개변수로 바꿔야 합니다:

```c++
VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    ...
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    ...
}
```

이 함수를 호출하는 모든 부분에서 적합한 aspect를 사용하도록 수정합니다:

```c++
swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
...
depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
...
textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
```

여기까지가 깊이 이미지 생성 부분입니다. 맵핑이나 다른 이미지를 복사할 필요는 없습니다. 색상 어태치먼트처럼 렌더 패스의 시작 지점에서 지울 것이기 때문입니다.

### 깊이 이미지의 명시적 전환

이미지의 레이아웃을 깊이 어태치먼트로 명시적으로 전환할 필요는 없는데, 이를 렌더 패스에서 처리할 예정이기 때문입니다. 하지만 완전성을 위해 이 섹션에서 설명은 진행 하도록 하겠습니다. 필요하다면 그냥 넘어가셔도 됩니다.

`createDepthResources`의 마지막에 `transitionImageLayout`를 호출합니다:

```c++
transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
```

깊이 이미지에 이미 쓰여있는 정보는 상관이 없기 때문에 정의되지 않은 레이아웃을 초기 레이아웃으로 사용해도 됩니다. `transitionImageLayout`에서 적정한 서브리소스 aspect를 사용하도록 몇 가지 로직을 수정합니다:

```c++
if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (hasStencilComponent(format)) {
        barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
} else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
}
```

스텐실 요소는 사용하지 않을 것이지만 깊이 이미지의 레이아웃 전환에는 포함 시켜야 합니다.

마지막으로 적절한 접근 마스크와 파이프라인 스테이지를 추가합니다:

```c++
if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
} else {
    throw std::invalid_argument("unsupported layout transition!");
}
```

깊이 테스트 진행 과정에서 프래그먼트가 보이는지를 테스트 하기 위해 깊이 버퍼값을 읽어야 하고, 새로운 프래그먼트가 그려지면 값이 쓰여져야 합니다. 값을 읽는 것은 `VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT` 스테이지에서, 쓰는 것은 `VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT` 스테이지에서 이루어집니다. 명시된 연산과 일치하는 가장 빠른 단계의 파이프라인 스테이지를 선택해서 필요한 시점에 깊이 어태치먼트로 사용될 수 있게끔 해야 합니다.

## 렌더 패스

이제 `createRenderPass`를 수정해서 깊이 어태치먼트를 포함하도록 해야 합니다. 먼저 `VkAttachmentDescription`를 명시합니다:

```c++
VkAttachmentDescription depthAttachment{};
depthAttachment.format = findDepthFormat();
depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
```

`format`은 깊이 이미지와 동일해야 합니다. 그리기가 끝나면 깊이 값은 사용되지 않기 때문에 깊이갚의 저장(`storeOp`)은 신경쓰지 않습니다. 이렇게 하면 하드웨어가 추가적인 최적화를 진행할 수 있게 됩니다. 색상 버퍼처럼 이전 깊이 값은 신경쓰지 않으므로 `VK_IMAGE_LAYOUT_UNDEFINED`를 `initialLayout`로 사용합니다.

```c++
VkAttachmentReference depthAttachmentRef{};
depthAttachmentRef.attachment = 1;
depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
```

첫 서브패스의 어태치먼트에 참조를 추가합니다:

```c++
VkSubpassDescription subpass{};
subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
subpass.colorAttachmentCount = 1;
subpass.pColorAttachments = &colorAttachmentRef;
subpass.pDepthStencilAttachment = &depthAttachmentRef;
```

색상 어태치먼트와는 달리 서브패스는 하나의 깊이(+스텐실) 어태치먼트만 사용할 수 있습니다. 여러 버퍼에 대해 깊이 테스트를 수행하는 것은 말이 안됩니다.

```c++
std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
VkRenderPassCreateInfo renderPassInfo{};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
renderPassInfo.pAttachments = attachments.data();
renderPassInfo.subpassCount = 1;
renderPassInfo.pSubpasses = &subpass;
renderPassInfo.dependencyCount = 1;
renderPassInfo.pDependencies = &dependency;
```

다음으로 `VkSubpassDependency` 구조체를 갱신해서 두 어태치먼트를 참조하도록 합니다.

```c++
dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
```

마지막으로 서브패스의 의존성을 확장하여 로드(load) 연산의 한 과정으로 깊이 이미지의 전환과 지우기간에 충돌이 없도록 해 줍니다. 깊이 이미지는 먼저 초기 프래그먼트 테스트 파이프라인 스테이지에서 접근되고 *지우기* 로드 연산을 사용하기 때문에 접근 마스크를 쓰기로 명시해 주어야 합니다.

## 프레임버퍼

다음 단계는 프레임버퍼 생성 부분을 수정해 깊이 이미지를 깊이 어태치먼트에 바인딩하는 것입니다. `createFramebuffers`로 가서 깊이 이미지 뷰를 두 번째 어태치먼트로 명시합니다:

```c++
std::array<VkImageView, 2> attachments = {
    swapChainImageViews[i],
    depthImageView
};

VkFramebufferCreateInfo framebufferInfo{};
framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
framebufferInfo.renderPass = renderPass;
framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
framebufferInfo.pAttachments = attachments.data();
framebufferInfo.width = swapChainExtent.width;
framebufferInfo.height = swapChainExtent.height;
framebufferInfo.layers = 1;
```

각 스왑 체인 이미지마다 색상 어태치먼트가 다르지만 깊이 이미지는 하나로 모두에 대해 사용할 수 있는데 우리 세마포어로 인해 한 번에 하나의 서브패스만 실행되기 때문입니다.

`createFramebuffers`의 호출을 옮겨서 깊이 이미지 뷰가 실제로 생성된 다음에 호출되도록 합니다:

```c++
void initVulkan() {
    ...
    createDepthResources();
    createFramebuffers();
    ...
}
```

## 지우기 값

`VK_ATTACHMENT_LOAD_OP_CLEAR`에 여러 개의 어태치먼트가 존재하기 때문에 지우기 값도 여러 개를 명시해 주어야 합니다. `recordCommandBuffer`로 가서 `VkClearValue`의 배열을 만듭니다:

```c++
std::array<VkClearValue, 2> clearValues{};
clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
clearValues[1].depthStencil = {1.0f, 0};

renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
renderPassInfo.pClearValues = clearValues.data();
```

Vulkan에서 깊이 버퍼의 깊이값은 `0.0`과 `1.0` 사이이고, `1.0`이 원면, `0.0`이 근면입니다. 각 점의 초기값은 가장 먼 깊이여야 하므로 `1.0`으로 설정합니다.

`clearValues`의 순서가 어태치먼트의 순서와 대응된다는 것에 유의하세요.

## 깊이와 스텐실 상태

이제 깊이 어태치먼트를 사용할 준비가 되었고, 그래픽스 파이프라인에서 실제로 깊이 테스트를 수행하도록 설정해야 합니다. 이는 `VkPipelineDepthStencilStateCreateInfo` 구조체를 통해 설정됩니다:

```c++
VkPipelineDepthStencilStateCreateInfo depthStencil{};
depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
depthStencil.depthTestEnable = VK_TRUE;
depthStencil.depthWriteEnable = VK_TRUE;
```

`depthTestEnable` 필드는 새로운 프래그먼트의 깊이값에 대해 깊이 버퍼에 기존에 쓰여진 값과의 비교를 통해 버리기를 수행할지 여부를 명시합니다. `depthWriteEnable` 필드는 테스트를 통과한 새로운 프래그먼트의 깊이 값이 깊이 버퍼에 쓰여길 것인지를 명시합니다.

```c++
depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
```

`depthCompareOp` 필드는 프래그먼트의 버리기나 유지를 결정하기 위한 비교 연산을 명시합니다. 일반적으로 사용되는 적은 깊이값(=더 가까운 프래그먼트)을 사용할 것이고, 이는 즉 새로운 프래그먼트의 깊이값이 더 *작아야 한다*는 의미입니다.

```c++
depthStencil.depthBoundsTestEnable = VK_FALSE;
depthStencil.minDepthBounds = 0.0f; // Optional
depthStencil.maxDepthBounds = 1.0f; // Optional
```

`depthBoundsTestEnable`, `minDepthBounds`, `maxDepthBounds` 필드는 깊이 바운드(bound) 테스트에 대한 선택적인 값입니다. 이는 특정 범위 내의 프래그먼트만 유지할 수 있도록 해 줍니다. 우리는 사용하지 않을 것입니다.

```c++
depthStencil.stencilTestEnable = VK_FALSE;
depthStencil.front = {}; // Optional
depthStencil.back = {}; // Optional
```

마지막 세 필드는 스텐실 버퍼 연산의 설정이고, 이 튜토리얼에서는 사용하지 않습니다. 이 연산을 사용하려면 깊이/스텐실 이미지의 포맷이 스텐실 요소를 가지고 있어야만 합니다.

```c++
pipelineInfo.pDepthStencilState = &depthStencil;
```

`VkGraphicsPipelineCreateInfo` 구조체를 수정하여 방금 설정한 깇이 스텐실 상태를 참조하도록 합니다. 깊이 스텐실 상태는 렌더 패스가 깊이 스텐실 어태치먼트를 가지는 경우, 항상 명시 되어야만 합니다.

이 상태에서 프로그램을 실행하면 각 물체의 프래그먼트가 올바른 순서로 표시되는 것을 보실 수 있습니다:

![](/images/depth_correct.png)

## 윈도우 리사이징 처리

윈도우가 리사이징되면 새로운 색상 어태치먼트의 해상도에 맞게 깊이 버퍼 해상도 또한 변해야 합니다. `recreateSwapChain` 함수를 확장하여 이러한 경우 깊이 리소스를 재생성하도록 합니다:

```c++
void recreateSwapChain() {
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createDepthResources();
    createFramebuffers();
}
```

정리는 스왑 체인 정리 함수에서 수행되어야 합니다:

```c++
void cleanupSwapChain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    ...
}
```

축하합니다. 이제 프로그램이 어떤 3D 형상이라도 제대로 그릴 수 있는 준비가 되었습니다. 다음 챕터에서 텍스처가 입혀진 모델을 그려서 이를 시도해 보겠습니다!

[C++ code](/code/27_depth_buffering.cpp) /
[Vertex shader](/code/27_shader_depth.vert) /
[Fragment shader](/code/27_shader_depth.frag)
