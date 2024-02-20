## 서론

지금까지 물체는 정점별로 할당된 색깔로 표현이 되었지만 이러한 방법으로는 한계가 있습니다. 이 챕터에서는 좀 더 흥미로운 표현을 위해 텍스처 맵핑을 구현해 보도록 하겠습니다. 이를 통해 나중에는 3D 모델을 로딩하고 그리는 것도 가능하게 될겁니다.

텍스처를 프로그램에 추가기 위해서는 아래와 같은 과정이 필요합니다:

* 장치 메모리가 베이크(baked)한 이미지 객체 생성
* 이미지 객체를 이미지 파일의 픽셀로 채움
* 이미지 샘플러(sampler) 생성
* 텍스처로부터 색상을 샘플링할 결합된(combined) 이미지 샘플러 기술자 추가

전에 이미 이미지 객체를 다뤄본 적 있지만 그 경우는 스왑 체인 확장이 자동적으로 만들어준 경우였습니다. 이번에는 직접 만들어야 합니다. 이미지를 만들고 여기에 데이터를 채우는 것은 정점 버퍼 생성과 비슷합니다. 스테이징 리소스를 먼저 만들고 여기에 픽셀 데이터를 채운 뒤 이를 렌더링에 사용할 최종 이미지 객체에 복사할 것입니다. 이러한 목적으로 스테이징 이미지를 만드는 것도 가능하지만 Vulkan에서는 `VkBuffer`로부터 이미지로 픽셀을 복사하는 것이 가능하고 이러한 목적으로 제공되는 API가 실제로 [어떤 하드웨어에서는 더 빠릅니다](https://developer.nvidia.com/vulkan-memory-management).

먼저 이 버퍼를 만들고 픽셀 값으로 채운 뒤 그 픽셀값을 복사할 이미지를 만듭니다. 이미지를 만드는 것은 버퍼를 만드는 것과 크게 다르지 않습니다. 전과 같이 메모리 요구조건을 질의하고, 장치 메모리를 할당하고 바인딩하면 됩니다.

하지만 이미지를 다룰 때 추가적으로 해 주어야 하는 작업이 있습니다. 이미지마다 다른 *레이아웃*을 가질 수 있는데 이는 픽셀들이 메모리에 어떻게 존재하는지에 영향을 미칩니다. 그래픽 하드웨어가 동작하는 방식 때문에 예를들어 픽셀값을 그냥 행별로 나열하는 것은 성능에 좋지 않을 수 있습니다. 이미지에 대해 어떤 연산을 수행할 때 해당 연산에 최적화된 형태의 레이아웃을 가지고 있는지를 확인해야 합니다. 전에 렌더 패스를 명시할 때 이러한 레이아웃을 이미 살펴본 바 있습니다:

* `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`: 표시 목적으로 최적
* `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`: 프래그먼트 셰이더에서 색상값을 쓰기 위한 어태치먼트로써 최적
* `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`: `vkCmdCopyImageToBuffer`에서처럼 전송의 소스로써 최적
* `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`: `vkCmdCopyBufferToImage`에서처럼 전송의 목적지로써 최적
* `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`: 셰이더에서 샘플링하는 목적으로 최적

이미지 레이아웃을 전송하는 가장 흔한 방법은 *파이프라인 배리어(barrier)* 입니다. 파이프라인 배리어는 리소스로의 동기화된 접근을 위해 주로 사용되는데, 예를 들자면 이미지에 대한 쓰기 이후 읽기를 보장하기 위해서와 같은 목적입니다. 하지만 이 방법은 레이아웃을 전송하기 위해서도 사용될 수 있습니다. 이 챕터에서 파이프라인 배리어를 이러한 목적으로 사용하는 방법을 살펴볼 것입니다. 배리어는 `VK_SHARING_MODE_EXCLUSIVE`일 때 큐 패밀리의 소유권(ownership) 이전을 위해서도 사용됩니다.

## 이미지 라이브러리

이미지를 로드하기 위한 다양한 라이브러리가 있고, BMP나 PPM과 같은 간단한 포맷은 직접 코드를 작성해도 됩니다. 이 튜토리얼에서 우리는 [stb collection](https://github.com/nothings/stb)의 stb_image를 사용할 예정입니다. 이 라이브러리의 장점은 모든 코드가 파일 하나에 있어서 빌드 구성이 간단해진다는 것입니다. `stb_image.h`를 다운로드하여 편리한 위치, 예를들자면 GLFW와 GLM이 있는 위치에 두십시오. 그리고 그 위치를 include 경로에 추가하십시오.

**Visual Studio**

`stb_image.h`가 있는 디렉토리를 `추가 포함 디렉토리` 경로에 추가하십시오.

![](/images/include_dirs_stb.png)

**Makefile**

`stb_image.h`가 있는 디텍초리는 GCC의 include 디렉토리에 추가하십시오:

```text
VULKAN_SDK_PATH = /home/user/VulkanSDK/x.x.x.x/x86_64
STB_INCLUDE_PATH = /home/user/libraries/stb

...

CFLAGS = -std=c++17 -I$(VULKAN_SDK_PATH)/include -I$(STB_INCLUDE_PATH)
```

## 이미지 로딩

이미지 라이브러리를 아래와 같이 include합니다:

```c++
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
```

The header only defines the prototypes of the functions by default. One code
file needs to include the header with the `STB_IMAGE_IMPLEMENTATION` definition
to include the function bodies, otherwise we'll get linking errors.

```c++
void initVulkan() {
    ...
    createCommandPool();
    createTextureImage();
    createVertexBuffer();
    ...
}

...

void createTextureImage() {

}
```

이미지를 로드하고 Vulkan 이미지 객체에 업로드하기 위한 `createTextureImage` 함수를 새로 만듭니다. 명령 버퍼를 사용할 것이기 때문에 `createCommandPool` 뒤에 호출해야 합니다.

`shaders` 디렉토리 옆에 이미지를 저장할 `textures` 디렉토리를 새로 만듭니다. `texture.jpg`라는 이미지를 로딩할 예정입니다. 저는 [CC0 라이센스 이미지](https://pixabay.com/en/statue-sculpture-fig-historically-1275469/)를 512 x 512 픽셀로 리사이징하여 사용하기로 했는데 여러분은 원하는 아무 이미지나 사용하십시오. 라이브러리는 JPEG, PNG, BMP, GIF같은 일반적인 이미지 파일 포맷을 지원합니다.

![](/images/texture.jpg)

라이브러리를 사용해 이미지를 로딩하는 것은 아주 쉽습니다:

```c++
void createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
}
```

`stbi_load` 함수는 파일 경로와 로드할 채널 개수를 인자로 받습니다. `STBI_rgb_alpha`값은 이미지에 알파 채널이 없어도 알파 채널을 포함하여 로드하도록 되어 있으며 추후 다른 텍스처 포맷을 사용할 때도 일관적인 코드를 사용 가능하므로 좋습니다. 중간의 세 매개변수는 너비, 높이와 이미지의 실제 채널 수의 출력입니다. 반환되는 포인터는 픽셀값 배열의 첫 요소의 포인터입니다. 픽셀들은 각 픽셀별 4바이트로 각 행이 배치되어 있으며 `STBI_rgb_alpha`의 경우 총 `texWidth * texHeight * 4`개의 값이 존재합니다.

## 스테이징 버퍼

이제 호스트에서 관찰 가능한 버퍼를 만들어 `vkMapMemory`를 사용해 픽셀 데이터를 복사할 수 있도록 하겠습니다. 임시 버퍼에 대한 변수를 `createTextureImage` 함수에 만듭니다:

```c++
VkBuffer stagingBuffer;
VkDeviceMemory stagingBufferMemory;
```

버퍼는 맵핑이 가능하고 전송의 소스로 활용할 수 있도록 호스트에서 관찰 가능한 곳에 있어야 하며, 그래야 나중에 이미지로 복사할 수 있습니다:

```c++
createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
```

이제 이미지 로딩 라이브러리부터 얻은 픽셀값을 버퍼로 복사합니다:

```c++
void* data;
vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
vkUnmapMemory(device, stagingBufferMemory);
```

이 시점에서 원본 픽셀 배열을 정리하는 것을 잊지 마세요:

```c++
stbi_image_free(pixels);
```

## 텍스처 이미지

셰이더에서 버퍼의 픽셀 값을 접근하도록 설정할 수도 있지만 이러한 목적으로는 Vulkan의 이미지 객체를 사용하는 것이 더 좋습니다. 이미지 객체를 사용하는 장점 중 하나는 2D 좌표로 색상값을 얻을 수 있어서 더 빠르고 편리하다는 것입니다. 이미지 객체가 갖고있는 픽셀은 텍셀(texel)이라고 하며 여기서부터는 그렇게 지칭하겠습니다. 아래와 같은 클래스 멤버를 추가합니다:

```c++
VkImage textureImage;
VkDeviceMemory textureImageMemory;
```

이미지에 대한 매개변수는 `VkImageCreateInfo`에 명시됩니다:

```c++
VkImageCreateInfo imageInfo{};
imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
imageInfo.imageType = VK_IMAGE_TYPE_2D;
imageInfo.extent.width = static_cast<uint32_t>(texWidth);
imageInfo.extent.height = static_cast<uint32_t>(texHeight);
imageInfo.extent.depth = 1;
imageInfo.mipLevels = 1;
imageInfo.arrayLayers = 1;
```

`imageType` 필드에 명시된 이미지 타입은 Vulkan에게 이미지의 텍셀이 어떤 좌표계를 사용하는지를 알려줍니다. 1D, 2D, 3D 이미지가 있습니다. 1차원 이미지는 데이터의 배열이나 그라디언트를 저장하기 위해 사용되고, 2차원 이미지는 주로 텍스처 용도로, 3차원 이미지는 복셀(voxel) 볼륨을 저장하기 위해 사용됩니다. `extent` 필드는 이미지의 크기를 명시하고, 이는 곧 각 축에 몇 개의 텍셀을 가지고 있는지를 의미합니다. 따라서 `depth`는 `0`이 아닌 `1`이어야 합니다. 우리 텍스처는 배열이 아니며 현재는 밉맵핑도 하지 않을 겁니다.

```c++
imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
```

Vulkan은 다양한 이미지 포맷을 지원하지만 텍셀과 버퍼의 픽셀 포맷으로 같은 포맷을 사용해야 합니다. 그렇지 않으면 복사 연산이 실패하게 됩니다.

```c++
imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
```

`tiling` 필드는 다음 두 값중 하나를 가집니다:

* `VK_IMAGE_TILING_LINEAR`: 우리 `pixels` 배열의 경우처럼 텍셀이 행 우선 순서(row-major order)로 저장됨
* `VK_IMAGE_TILING_OPTIMAL`: 텍셀이 최적 접근을 위해 구현에서 정의한 순서대로 저장됨

이미지의 레이아웃과는 달리 타일링 모드는 나중에 바꿀 수 없습니다. 이미지 메모리의 텍셀에 직접 접근하고 싶다면 `VK_IMAGE_TILING_LINEAR`를 사용해야 합니다. 우리의 경우 스테이징 이미지가 아닌 스테이징 버퍼를 사용하고 있으므로 이렇게 할 필요는 없습니다. 셰이더에서 효율적인 접근이 가능하도록 `VK_IMAGE_TILING_OPTIMAL`를 사용할겁니다.

```c++
imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
```

이미지의 `initialLayout`는 두 가지 값이 가능합니다:

* `VK_IMAGE_LAYOUT_UNDEFINED`: GPU에서 사용이 불가능하고 첫 전환(transition) 이후 텍셀이 버려짐
* `VK_IMAGE_LAYOUT_PREINITIALIZED`: GPU에서 사용이 불가능하고 첫 전환 이후 텍셀이 유지됨

첫 전환 이후에 텍셀이 유지되어야 하는 경우는 별로 없습니다. 유지되어야 하는 한 예로 `VK_IMAGE_TILING_LINEAR`와 함께 이미지를 스테이징 이미지로 활용하는 경우가 있습니다. 이 경우 텍셀 데이터를 업로드한 이후에 이미지를 전송의 소스로 전환하며, 데이터를 버리지 않습니다. 하지만 우리의 경우 먼저 이미지를 전송의 목적지로 전환한 후 버퍼 객체로부터 텍셀 데이터를 복사하기 때문에 이러한 속성이 필요없고 따라서 `VK_IMAGE_LAYOUT_UNDEFINED`를 사용해도 됩니다.

```c++
imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
```

`usage` 필드는 버퍼 생성과 동일한 의미를 갖습니다. 이미지는 버퍼 복사의 목적지로 활용될 것입니다. 또한 이미지는 셰이더에서 메쉬의 색상을 결정하기 위해 활용될 예정이므로 사용법에는 `VK_IMAGE_USAGE_SAMPLED_BIT`가 포함되어야 합니다.

```c++
imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
```

이미지는 하나의 큐 패밀리에서만 활용될 예정입니다. 전송 연산이 가능한 그래픽스 큐 패밀리입니다.

```c++
imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
imageInfo.flags = 0; // Optional
```

`samples` 플래그는 멀티샘플링과 관련되어 있습니다. 이는 이미지가 어태치먼트로 활용될때만 의미가 있으므로 샘플은 1로 둡니다. 희박한(sparse) 이미지의 경우에 대한 선택적인 플래그들이 몇 가지 있습니다. 희박한 이미지란 특정 영역만이 베이킹되는 이미지입니다. 예를 들어 복셀 지형을 위해 3D 텍스처를 사용한다고 하면 아무 것도 없는 영역에 대한 메모리 할당을 피하기 위해서 이러한 이미지를 사용할 수 있습니다. 이 튜토리얼에서는 이러한 사용 사례가 없으므로 기본값인 `0`으로 두겠습니다.

```c++
if (vkCreateImage(device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
}
```

이미지는 `vkCreateImage`로 만들어지고 특별히 언급할만한 매개변수는 없습니다. `VK_FORMAT_R8G8B8A8_SRGB` 포맷을 하드웨어가 지원하지 않는 경우가 있을 수 있습니다. 가능한 대안들의 목록을 가지고 있다가 지원되는 가장 괜찮은 것을 사용해야 합니다. 하지만 이 포맷은 널리 지원되므로 이러한 처리 과정을 지금은 넘어가겠습니다. 다른 포맷을 사용하려면 좀 귀찮은 변환 과정을 수행해야 합니다. 깊이 버퍼 챕터에서 이러한 시스템을 구현하면서 다시 살펴볼 것입니다.

```c++
VkMemoryRequirements memRequirements;
vkGetImageMemoryRequirements(device, textureImage, &memRequirements);

VkMemoryAllocateInfo allocInfo{};
allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
allocInfo.allocationSize = memRequirements.size;
allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

if (vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
}

vkBindImageMemory(device, textureImage, textureImageMemory, 0);
```

이미지를 위한 메모리 할당은 버퍼 메모리 할당과 완전히 동일합니다. `vkGetBufferMemoryRequirements` 대신에 `vkGetImageMemoryRequirements`를 사용하고, `vkBindBufferMemory` 대신에 `vkBindImageMemory`를 사용합니다.

함수가 꽤 커졌고, 나중 챕터에서는 이미지를 더 만들어야 하기 때문에 이미지 생성은 버퍼에서처럼 `createImage` 함수로 추상화해야 합니다. 함수를 만들고 이미지 객체의 생성과 메모리 할당을 이 곳으로 옮깁니다:

```c++
void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}
```

너비, 높이, 포맷, 타일링 모드, 사용법, 메모리 속성을 매개변수로 만들었는데 이것들은 앞으로 튜토리얼에서 만들 이미지마다 다르기 때문입니다.

`createTextureImage` 함수는 이제 아래와 같이 간략화됩니다:

```c++
void createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
}
```

## 레이아웃 전환(transitions)

이제 작성할 함수는 또한번 명령 버퍼를 기록하고 실행하는 부분이며 이에 따라 이러한 로직은 한두개의 헬퍼 함수로 옮기는 것이 좋겠습니다:

```c++
VkCommandBuffer beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
```

이 코드는 기존의 `copyBuffer`에 있던 코드에 기반해 만들어졌습니다. 이제 해당 함수는 아래와 같아집니다:

```c++
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}
```

아직 버퍼를 사용 중이라면 `vkCmdCopyBufferToImage`를 기록하고 실행하는 함수를 만들어 작업을 완료할 수도 있습니다. 하지만 이 명령은 먼저 이미지가 올바른 레이아웃에 있는 것을 요구합니다. 레이아웃 전환을 위한 함수를 새로 만듭니다:

```c++
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    endSingleTimeCommands(commandBuffer);
}
```

레이아웃 전환을 위한 가장 일반적인 방법은 *이미지 메모리 배리어*를 사용하는 것입니다. 이와 같은 파이프라인 배리어는 리소스에 대한 접근을 동기화하기 위해 사용되는데 예를 들자면 버퍼에 값을 쓰는 것이 읽기 전에 끝나야 하는 것을 보장하기 위해서와 같은 것입니다. 하지만 또한 이미지 레이아웃을 전환하고 `VK_SHARING_MODE_EXCLUSIVE`가 사용될 때 큐 패밀리 소유권을 이전하는 데에도 사용됩니다. 버퍼에 대해서는 대응되는 *버퍼 메모리 배리어*라는 것이 존재합니다.

```c++
VkImageMemoryBarrier barrier{};
barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
barrier.oldLayout = oldLayout;
barrier.newLayout = newLayout;
```

첫 두 필드는 레이아웃 전환을 명시합니다. 이미지에 존재하는 내용을 상관하지 않는다면 `oldLayout`에는 `VK_IMAGE_LAYOUT_UNDEFINED`를 사용할 수도 있습니다.

```c++
barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
```

큐 패밀리 소유권 이전을 위해 배리어를 사용한다면, 이 두 필드는 큐 패밀리의 인덱스여야 합니다. 그렇지 않은 경우에는 `VK_QUEUE_FAMILY_IGNORED`로 설정해야 합니다 (이 값이 기본값이 아닙니다!).

```c++
barrier.image = image;
barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
barrier.subresourceRange.baseMipLevel = 0;
barrier.subresourceRange.levelCount = 1;
barrier.subresourceRange.baseArrayLayer = 0;
barrier.subresourceRange.layerCount = 1;
```

`image`와 `subresourceRange`는 영향을 받는 이미지와 이미지의 특정 영역을 명시합니다. 우리 이미지는 배열도 아니고 밉맵 레벨도 없으므로 1 레벨과 하나의 레이어로 명시합니다.

```c++
barrier.srcAccessMask = 0; // TODO
barrier.dstAccessMask = 0; // TODO
```

배리어의 주 목적은 동기화이므로 리소스와 관련한 어떤 종류의 연산이 배리어 앞에 오고 어떤 연산이 배리어를 대기해야 하는시를 명시해야 합니다. 이미 `vkQueueWaitIdle`를 사용해 매뉴얼하게 동기화 하고 있지만 그래도 해 주어야 합니다. 올바른 값은 old와 new 레이아웃에 달려 있으며 어떤 전환할 수행할 것인지를 알게 된 후에 다시 돌아오겠습니다.

```c++
vkCmdPipelineBarrier(
    commandBuffer,
    0 /* TODO */, 0 /* TODO */,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
);
```

모든 파이프라인 배리어는 같은 함수로 제출됩니다. 명령 버퍼 다음으로 오는 첫 매개변수는 배리어 앞에 수행되어야 할 연산의 파이프라인 스테이지를 명시합니다. 두 번째 매개변수는 배리어를 대기할 파이프라인 스테이지를 명시합니다. 배리어 앞과 뒤에 명시할 수 있는 파이프라인의 스테이지는 배리어 전후에 리소스를 어떻게 사용할 것인지에 달려 있습니다. 가능한 값의 목록은 명세의 [이 표](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap7.html#synchronization-access-types-supported)에 있습니다. For example, if you're going to read from a uniform after
the barrier, you would specify a usage of `VK_ACCESS_UNIFORM_READ_BIT` and the
earliest shader that will read from the uniform as pipeline stage, for example
`VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT`. 셰이더가 아닌 파이프라인 스테이지에 이러한 사용법을 명시한다면 검증 레이어가 용법에 맞지 않는 파이프라인 스테이지를 명시했다고 경고를 낼 것입니다.

세 번째 매개변수는 `0` 또는 `VK_DEPENDENCY_BY_REGION_BIT`입니다. 후자는 배리어를 영역별 조건으로 바꿉니다. 즉, 예를 들자면 구현이 현재까지 쓰기가 완료된 리소스의 일부분을 읽을 수 있게 됩니다.

마지막 세 개의 매개변수는 세 종류의 타입에 대한 파이프라인 배리어의 배열에 대한 참조입니다. 세 종류 타입은 메모리 배리어, 버퍼 메모리 배리어, 이미지 메모리 배리어이고 현재는 마지막 것을 사용입니다. `VkFormat` 매개변수는 아직 사용하지 않는 것에 유의하세요. 이는 깊이 버퍼 챕터에서 특수한 전환을 위해 사용할 예정입니다.

## Copying buffer to image

`createTextureImage`로 다시 돌아가기 전에 추가적인 헬퍼 함수 `copyBufferToImage`를 작성하겠습니다:

```c++
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    endSingleTimeCommands(commandBuffer);
}
```

버퍼의 복사와 마찬가지로 버퍼의 어떤 부분이 이미지의 어떤 부분으로 복사될 것인지를 명시해야 합니다. 이는 `VkBufferImageCopy` 구조체를 통해 명시됩니다:

```c++
VkBufferImageCopy region{};
region.bufferOffset = 0;
region.bufferRowLength = 0;
region.bufferImageHeight = 0;

region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
region.imageSubresource.mipLevel = 0;
region.imageSubresource.baseArrayLayer = 0;
region.imageSubresource.layerCount = 1;

region.imageOffset = {0, 0, 0};
region.imageExtent = {
    width,
    height,
    1
};
```

대부분의 필드는 직관적입니다. `bufferOffset`은 픽셀 값이 시작하는 버퍼의 바이트 단위 오프셋입니다. `bufferRowLength`와 `bufferImageHeight` 필드는 픽셀이 메모리에 어떻게 배치되어있는지를 명시합니다. 예를 들어 각 행에 패딩(padding) 바이트가 있을 수 있습니다. 둘 다 `0`으로 명시하였다는 의미는 패딩 없이 연속적으로 데이터가 존재한다는 뜻입니다. `imageSubresource`, `imageOffset`, `imageExtent`는 픽셀이 복사될 이미지의 영역을 명시합니다.

버퍼에서 메모리로의 복사 연산은 `vkCmdCopyBufferToImage` 함수를 통해 큐에 등록됩니다:

```c++
vkCmdCopyBufferToImage(
    commandBuffer,
    buffer,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
);
```

네 번째 매개변수는 현재 이미지가 사용하고 있는 레이아웃을 명시합니다. 여기서 저는 이미지가 이미 픽셀을 복사하기에 최적화된 레이아웃으로 전환되었다고 가정하고 있습니다. 지금은 픽셀 값 덩어리를 전체 이미지에 복사하고 있지만 `VkBufferImageCopy`의 배열을 명시해서 버퍼로부터의 서로 다른 복사 연산들을 한 번에 수행할 수도 있습니다.

## 텍스처 이미지 준비

이제 텍스처 이미지를 사용하기 위해 필요한 모든 도구가 준비되었으니 `createTextureImage` 함수로 다시 돌아갑시다. 여기서 마지막에 했던 것은 텍스처 이미지를 만든 것이었습니다. 그 다음 단계로 스테이징 버퍼를 텍스처 이미지로 복사해야 합니다. 여기에는 두 단계가 필요합니다:

* 텍스처 이미지를 `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`로 전환
* 버퍼에서 이미지로의 복사 연산 실행

방금 만든 함수들을 사용하면 쉽게 수행할 수 있습니다:

```c++
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
```

이미지는 `VK_IMAGE_LAYOUT_UNDEFINED` 레이아웃으로 생성되었으므로 `textureImage`로 전환될 떄 기존(old) 레이아웃으로 명시되어야 합니다. 이것이 가능한 이유는 복사 연산을 수행하기 전, 기존에 쓰여있던 내용을 신경쓰지 않기 떄문에 가능한 것이라는 것을 기억하십시오.

셰이더에서 텍스처 이미지를 샘플링하려면 마지막으로 셰이더에서 접근이 가능하도록 한번 더 전환이 필요합니다:

```c++
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
```

## 전환 배리어 마스크(mask)

지금 시점에 검증 레이어가 켜진 상태에서 프로그램을 실행하면 `transitionImageLayout`의 접근 마스크와 파이프라인 스테이지가 유효하지 않다는 오류를 보실 수 있습니다. 전환의 레이아웃에서 이들을 설정해 줘야 합니다.

처리해야 할 전환이 두 가지 있습니다:

* Undefined → transfer destination: 전송은 무언가를 기다릴 필요 없이 쓰기를 수행
* Transfer destination → shader reading: 셰이더 읽기는 전송의 쓰기를 기다려야 하며, 정확히는 프래그먼트 셰이더의 읽기 연산임. 왜냐하면 이 시점이 텍스터를 사용하는 시점이므로

이러한 규칙들은 다음와 같은 접근 마스크와 파이프라인 스테이지로 명시됩니다:

```c++
VkPipelineStageFlags sourceStage;
VkPipelineStageFlags destinationStage;

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
} else {
    throw std::invalid_argument("unsupported layout transition!");
}

vkCmdPipelineBarrier(
    commandBuffer,
    sourceStage, destinationStage,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
);
```

이전 표에서처럼 전송 쓰기는 파이프라인 전환 단계에서 수행되어야 합니다. 쓰기 연산이 무언가를 기다릴 필요는 없으므로 빈 접근 마스크를 명시하고 파이프라인의 가장 첫 단계인 `VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT`를 배리어 전 연산으로 지정해야 합니다. 중요한 것은 `VK_PIPELINE_STAGE_TRANSFER_BIT`은 *실제* 그래픽스나 컴퓨트 파이프라인의 스테이지가 아니라는 점입니다. 전송이 일어나는 의사(pseudo)-스테이지에 가깝습니다. 의사 스테이지의 예시들에 대해서는 [이 문서](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap7.html#VkPipelineStageFlagBits)를 살펴보세요.

이미지는 같은 파이프라인 스테이지에서 쓰여지고 프래그먼트 셰이더에서 읽게 되므로 프래그먼트 셰이더 파이프라인 스테이지에 셰이더 읽기 접근을 명시하였습니다.

나중에 추가적인 전환을 수행해야 한다면 그 때 함수를 확장할 것입니다. 이제 프로그램은 올바로 동작하고, 보이는 장면은 이전과 동일합니다.

하나 언급하고 싶은 것은 초반부의 암시적 `VK_ACCESS_HOST_WRITE_BIT` 동기화에서의 명령 버퍼 제출 입니다. `transitionImageLayout` 함수는 명령이 하나만 존재하는 명령 버터를 실행하므로, 암시적 동기화를 수행하고 `srcAccessMask`를 `0`으로 설정할 수 있습니다. 이는 레이아웃 전환에서 `VK_ACCESS_HOST_WRITE_BIT` 의존성이 필요한 경우에 입니다. 이를 명시적으로 할지 아닐지는 여러분들에게 달려 있지만 저는 개인적으로 이러한 OpenGL 스타일의 "숨겨진" 연산이 있는 것을 좋아하지는 않습니다.

사실 모든 연산을 지원하는 특별한 이미지 레이아웃인 `VK_IMAGE_LAYOUT_GENERAL`가 있습니다. 이것의 문제는 당연하지만 어떤 연산에 대해서 최선의 성능을 보장하지 않는다는 것입니다. 이것은 이미지를 입력과 출력에 동시에 사용하거나 사전 초기화가 끝난 이후에 이미지를 읽어온다거나 하는 등의 특정한 케이스에서는 필요할 수 있습니다.

지금까지 명령을 제출한 헬퍼 함수들은 큐가 아이들 상태가 될때까지 대기하여 명령을 동기적으로 수행하도록 설정되었습니다. 실제 응용 프로그램에서는 이러한 연산들을 하나의 명령 버퍼에 통합하여 비동기적으로 실행하여 높은 쓰루풋(throughput)을 달성하는 것이 권장됩니다. 특히 `createTextureImage` 함수의 전환과 복사 연산에 대해서는요. 헬퍼 함수가 명령을 입력할 `setupCommandBuffer`를 만들고 `flushSetupCommands`를 추가하여 지금까지 기록된 명령을 실행하게 해보세요. 텍스처 맵핑 이후에 이러한 작업을 시도하여 텍스처 리소스가 문제 없이 설정되는지 확인해 보시는 것이 가장 좋을 것 같습니다.

## 정리

스테이징 버퍼와 그 메모리를 마지막에 정리하는 것으로 `createTextureImage` 함수를 마무리 합시다:

```c++
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
```

메인 텍스터 이미지는 프로그램 종료 시까지 사용됩니다:

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);

    ...
}
```

이미지는 이제 텍스처를 포함하지만 그래픽스 파이프라인에서 접근이 가능하게 할 방법이 필요합니다. 다음 챕터에서 진행해 보겠습니다.

[C++ code](/code/24_texture_image.cpp) /
[Vertex shader](/code/22_shader_ubo.vert) /
[Fragment shader](/code/22_shader_ubo.frag)
