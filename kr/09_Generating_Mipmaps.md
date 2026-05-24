## 서론

이제 우리 프로그램에서 3D 모델을 로딩하고 렌더링할 수 있게 되었습니다. 이 챕터에서는 기능을 하나 더 추가할 것인데, 밉맵 생성입니다. 밉맵은 게임이나 렌더링 소프트웨어에서 널리 사용되는 방법이고, Vulkan에서는 밉맵이 어떻게 생성될 것인지에 대한 완전한 제어 권한을 제공해 줍니다.

밉맵은 미리 계산된, 축소된 이미지입니다. 각 이미지는 전 단계의 이미지보다 가로와 세로가 절반 크기로 줄어든 이미지 입니다. 밉맵은 *디테일 레벨(Level of Detail)*, 다시말해 *LOD*으로써 사용됩니다. 카메라에서 멀리 떨어진 물체는 텍스처를 샘플링 할 때 더 작은 밉 이미지로부터 샘플링합니다. 더 작은 이미지를 사용하면 렌더링 속도가 빨라지며 [Moiré 패턴](https://en.wikipedia.org/wiki/Moir%C3%A9_pattern)과 같은 문제점을 해결할 수 있습니다. 밉맵의 예시는 아래와 같습니다:

![](/images/mipmaps_example.jpg)

## 이미지 생성

Vulkan에서 각 밉 이미지는 `VkImage`의 서로 다른 *밉 레벨*에 저장됩니다. 밉 레벨 0은 원본 이미지이고 0 레벨 이후의 밉 레벨들은 일반적으로 *밉 체인(chain)*이라고 부릅니다.

밉 레벨의 개수는 `VkImage`이 생성될 때 명시됩니다. 지금까지는 항상 이 값을 1로 설정했었습니다. 이제는 이미지의 크기로부터 밉 레벨의 개수를 계산해야 합니다. 먼저 이 숫자를 저장하기 위한 클래스 멤버를 추가합니다:

```c++
...
uint32_t mipLevels;
VkImage textureImage;
...
```

`mipLevels` 값은 `createTextureImage`에서 텍스처를 로딩한 뒤 저장됩니다:

```c++
int texWidth, texHeight, texChannels;
stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
...
mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

```

위와 같이 밉 체인의 레벨 개수를 계산합니다. `max` 함수는 가로 세로 중 큰 값을 찾습니다. `log2` 함수를 통해 그 값이 2로 몇번 나눌 수 있는지를 계산합니다. `floor` 함수를 통해 값이 2의 배수가 아닐 경우를 처리합니다. 원본 이미지가 밉 레벨 하나를 차지하기 때문에 `1`을 더합니다.

이 값을 사용하기 위해서는 `createImage`, `createImageView`, `transitionImageLayout` 함수를 수정해서 밉 레벨을 명시해야 합니다. 해당 함수들에 `mipLevels` 매개변수를 추가합니다:

```c++
void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    ...
    imageInfo.mipLevels = mipLevels;
    ...
}
```
```c++
VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
    ...
    viewInfo.subresourceRange.levelCount = mipLevels;
    ...
```
```c++
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
    ...
    barrier.subresourceRange.levelCount = mipLevels;
    ...
```

이 함수들을 호출하는 곳에서는 올바른 값을 넘겨주도록 수정합니다:

```c++
createImage(swapChainExtent.width, swapChainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
...
createImage(texWidth, texHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
```
```c++
swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
...
depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
...
textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
```
```c++
transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
...
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
```



## 밉맵 생성

이제 텍스처 이미지가 여러 밉 레벨을 가지고 있지만 스테이징 버퍼는 밉 레벨 0을 채우는 데에만 사용되고 있습니다. 다른 레벨들에 대해서는 아직 정의되지 않았습니다. 이러한 레벨들을 채우기 위해서는 하나의 레벨으로부터 데이터들을 생성해야만 합니다. 이를 위해 `vkCmdBlitImage` 명령을 사용할 것입니다. 이 명령은 복사, 크기 변환과 필터링 연산을 수행합니다. 이를 여러 번 호출해서 데이터를 텍스처 이미지의 여러 레벨으로 *blit* 하도록 할 것입니다.

`vkCmdBlitImage`은 전송 연산으로 취급되기 때문에, Vulkan에게 텍스처 이미지가 전송의 소스와 목적지로 사용된 것임을 알려줘야 합니다. `createTextureImage`에서 텍스처 이미지의 사용법 플래그에 `VK_IMAGE_USAGE_TRANSFER_SRC_BIT`를 추가합니다:

```c++
...
createImage(texWidth, texHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
...
```

다른 이미지 연산들처럼 `vkCmdBlitImage`도 연산이 수행되는 이미지의 레이아웃에 의존적입니다. 전체 이미지를 `VK_IMAGE_LAYOUT_GENERAL`로 전환할 수도 있지만 이렇게 하면 느려질 겁니다. 최적 성능을 위해서는 소스 이미지는 `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`에, 목적 이미지는 `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`에 있어야 합니다. Vulkan에서는 이미지의 각 레벨을 독립적으로 전환할 수 있도록 되어 있습니다. 각 blit마다 두 개의 밉 레벨을 처리하므로 blit 명령간에 최적 레이아웃으로 전환해 사용하면 됩니다.

`transitionImageLayout`는 전체 이미지에 대한 레이아웃 전환만 수행하므로 몇 가지 파이프라인 배리어 연산을 작성해야 합니다. `createTextureImage`에 기존에 있던 `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`로의 전환을 삭제합니다:

```c++
...
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
...
```

이렇게 하면 텍스처 이미지의 각 레벨이 `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`가 됩니다. 각 레벨은 blit 명령이 끝난 뒤에 `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`로 전환될 겁니다.

이제 밉맵 생성을 위한 코드를 작성할 것입니다:

```c++
void generateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    endSingleTimeCommands(commandBuffer);
}
```

몇 번의 전환을 수행할 것이므로 `VkImageMemoryBarrier`를 재사용할 것입니다. 위에서 설정한 필드는 모든 배리어에 대해 동일하게 사용됩니다. `subresourceRange.miplevel`, `oldLayout`, `newLayout`, `srcAccessMask`, `dstAccessMask`은 각 전환마다 바뀔 예정입니다.

```c++
int32_t mipWidth = texWidth;
int32_t mipHeight = texHeight;

for (uint32_t i = 1; i < mipLevels; i++) {

}
```

위 반복문에서 각각의 `VkCmdBlitImage` 명령을 기록할 것입니다. 반복문의 변수가 0이 아닌 1부터 시작한다는 것에 유의하세요.

```c++
barrier.subresourceRange.baseMipLevel = i - 1;
barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

vkCmdPipelineBarrier(commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
    0, nullptr,
    0, nullptr,
    1, &barrier);
```

먼저 `i - 1` 레벨을 `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`로 전환합니다. 이 전환은 `i - 1` 레벨이 다 채워질 때까지 기다릴 것인데 이는 이전의 blit 명령 또는 `vkCmdCopyBufferToImage`에 의해 이루어집니다. 현재의 blit 명령은 이러한 전환을 대기하게 됩니다.

```c++
VkImageBlit blit{};
blit.srcOffsets[0] = { 0, 0, 0 };
blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
blit.srcSubresource.mipLevel = i - 1;
blit.srcSubresource.baseArrayLayer = 0;
blit.srcSubresource.layerCount = 1;
blit.dstOffsets[0] = { 0, 0, 0 };
blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
blit.dstSubresource.mipLevel = i;
blit.dstSubresource.baseArrayLayer = 0;
blit.dstSubresource.layerCount = 1;
```

다음으로 blit 연산에 사용될 영역을 명시합니다. 소스 밉 레벨은 `i - 1`이고 목적 밉 레벨은 `i` 입니다. `srcOffsets` 배열의 두 요소는 데이터가 blit될 소스의 3D 영역을 결정합니다. `dstOffsets`은 데이터가 blit될 목적 영역을 의미합니다. `dstOffsets[1]`의 X와 Y 크기는 2로 나누었는데 각 밉 레벨이 이전 레벨의 절반 크기이기 때문입니다. `srcOffsets[1]`와 `dstOffsets[1]`의 Z 크기는 1이어야 하는데, 2D 이미지는 깊이값이 1이기 때문입니다.

```c++
vkCmdBlitImage(commandBuffer,
    image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1, &blit,
    VK_FILTER_LINEAR);
```

이제 blit 명령을 기록합니다. `srcImage`와 `dstImage` 매개변수에 모두 `textureImage` 가 사용된 것을 주목하십시오. 같은 이미지의 다른 레벨로 blit을 하고 있기 때문입니다. 소스 밉 레벨은 `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`로 전환되었고 목적 레벨은 `createTextureImage`에서 정의한대로 `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`인 상태입니다.

[정점 버퍼](!kr/Vertex_buffers/Staging_buffer)에서 제시한 직접 만든 전송 큐를 사용하는 경우 주의하셔야 합니다. `vkCmdBlitImage`는 그래픽스 기능의 큐에 제출되어야만 합니다.

마지막 매개변수는 blit에 사용할 `VkFilter`를 명시합니다. 여기서는 `VkSampler`를 만들 때 사용한 것과 같은 필터링 옵션을 사용합니다. 보간을 수행하기 위해 `VK_FILTER_LINEAR`를 사용합니다.

```c++
barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

vkCmdPipelineBarrier(commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
    0, nullptr,
    0, nullptr,
    1, &barrier);
```

이 배리어가 밉 레벨 `i - 1`을 `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`로 전환합니다. 이 전환은 현재의 blit 명령이 끝날 때까지 대기합니다. 이 전환이 끝날때까지 모든 샘플링 연산이 대기상태가 될 겁니다.

```c++
    ...
    if (mipWidth > 1) mipWidth /= 2;
    if (mipHeight > 1) mipHeight /= 2;
}
```

반복문의 끝 부분에서 현재의 밉 크기를 2로 나눕니다. 나누기 전에 크기가 0이 되지 않도록 확입니다. 이를 통해 이미지가 정사각형 크기가 아닐 때를 처리할 수 있는데 이 경우 밉의 가로 크기는 1이 되었는데 세로 크기는 그렇지 않은 상태일 수도 있기 때문입니다. 이러한 상황이 생기면 나머지 레벨이 처리될 때까지 가로 크기는 1로 고정됩니다.

```c++
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    endSingleTimeCommands(commandBuffer);
}
```

명령 버퍼를 끝내기 전에 파이프라인 배리어를 하나 더 추가했습니다. 이 배리어는 마지막 밉 레벨을 `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`에서 `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`로 바꿉니다. 이 경우는 반복문에서 처리될 수 없는데, 마지막 밉 레벨은 blit이 수행되지 않기 때문입니다.

끝으로 `createTextureImage`에 `generateMipmaps` 호출을 추가합니다:

```c++
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
...
generateMipmaps(textureImage, texWidth, texHeight, mipLevels);
```

이제 텍스처 이미지의 밉맵들이 모두 채워집니다.

## 선형 필터링 지원

`vkCmdBlitImage`와 같은 내장 함수를 통해 모든 밉 수준을 생성하는 것이 편리하지만, 안타깝게도 모든 플랫폼에서 지원이 보장된 것은 아닙니다. 우리가 사용하는 텍스처 이미지 포맷이 선형 필터링을 지원해야만 하고, 이는 `vkGetPhysicalDeviceFormatProperties` 함수를 통해 확인할 수 있습니다. `generateMipmaps` 함수에 확인 과정을 추가할 것입니다.

먼저 이미지 포맷을 명시하는 매개변수를 추가합니다:

```c++
void createTextureImage() {
    ...

    generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
}

void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {

    ...
}
```

`generateMipmaps` 함수에서 `vkGetPhysicalDeviceFormatProperties`를 사용해 텍스처 이미지 포맷에 대한 속성을 요청합니다:

```c++
void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {

    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

    ...
```

`VkFormatProperties` 구조체는 `linearTilingFeatures`, `optimalTilingFeatures`, `bufferFeatures` 필드를 갖는데 이들은 사용 방식에 따라서 포맷이 어떻게 사용될 수 있는지를 기술합니다. 우리는 텍스처 이미지를 최적 타일링 포맷으로 만들었기 때문에 `optimalTilingFeatures`를 확인해야 합니다. 선형 필터링 기능에 대한 지원을 확인하는 것은 `VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT`로 할 수 있습니다:

```c++
if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    throw std::runtime_error("texture image format does not support linear blitting!");
}
```

처리 방안의 대안은 두 가지가 있습니다. 선형 blit을 *지원하는* 일반적인 이미지 포맷을 찾는 함수를 작성할 수도 있고, 아니면 [stb_image_resize](https://github.com/nothings/stb/blob/master/stb_image_resize.h)와 같은 라이브러리를 사용해 소프트웨어적으로 밉맵 생성을 구현할 수도 있습니다. 원본 이미지를 로딩한 것과 같은 방식으로 각 밉 레벨을 로딩할 수 있습니다.

중요한 것은 어쨌든 실제로는 밉맵 레벨을 런타임에 생성하는 것은 일반적이지 않은 경우라는 것입니다. 로딩 속도 향상을 위해 일반적으로 이들은 미리 생성되어서 텍스처 파일의 기본 레벨 옆에 저장됩니다. 소프트웨어적으로 크기를 변환한 뒤 여러 레벨을 한 파일로 로딩하는 것은 독자들을 위한 연습 문제로 남겨두겠습니다.

## 샘플러

`VkImage`가 밉맵 데이터를 가지고 있으므로 `VkSampler`는 렌더링시에 데이터를 어떻게 읽어올 것인지를 제어할 수 있습니다. Vulkan은 `minLod`, `maxLod`, `mipLodBias`, `mipmapMode`를 명시할 수 있게 되어 있습니다("Lod"가 "디테일 레벨"을 의미합니다). 텍스처가 샘플링될 때, 샘플러는 아래 의사코드와 같은 방식으로 밉 레벨을 선택합니다:

```c++
lod = getLodLevelFromScreenSize(); //smaller when the object is close, may be negative
lod = clamp(lod + mipLodBias, minLod, maxLod);

level = clamp(floor(lod), 0, texture.mipLevels - 1);  //clamped to the number of mip levels in the texture

if (mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST) {
    color = sample(level);
} else {
    color = blend(sample(level), sample(level + 1));
}
```

`samplerInfo.mipmapMode`가 `VK_SAMPLER_MIPMAP_MODE_NEAREST`면, `lod`가 샘플링할 밉 레벨을 결정합니다. 밉맵 모드가 `VK_SAMPLER_MIPMAP_MODE_LINEAR`면, `lod`는 샘플링할 두 개의 밉 레벨을 결정합니다. 두 레벨에서 모두 샘플링이 되고 이들을 선형적으로 혼합한 결과가 반환됩니다.

샘플링 연산 또한 `lod`에 영향을 미칩니다:

```c++
if (lod <= 0) {
    color = readTexture(uv, magFilter);
} else {
    color = readTexture(uv, minFilter);
}
```

물체가 카메라에 가까이 있으면 `magFilter`가 필터로 사용됩니다. 물체가 카메라에서 멀리 있으면 `minFilter`가 사용됩니다. 일반적으로 `lod`는 양수이고 카메라에 가까이 있을 경우에는 0입니다. `mipLodBias`를 통해 Vulkan에 일반적으로 적용되는 것보다 더 낮은 `lod`와 `level`을 사용하도록 하게 할 수 있습니다.

이 챕터의 결과를 보기 위해서 `textureSampler`를 위한 값을 선택해야 합니다. `minFilter`와 `magFilter`에 대해서는 `VK_FILTER_LINEAR`를 이미 설정해 두었습니다. `minLod`, `maxLod`, `mipLodBias`, `mipmapMode`만 선택하면 됩니다.

```c++
void createTextureSampler() {
    ...
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f; // Optional
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerInfo.mipLodBias = 0.0f; // Optional
    ...
}
```

전체 밉 레벨을 사용하기 위해서 `minLod`는 0.0f로, `maxLod`는 `VK_LOD_CLAMP_NONE`로 설정했습니다. 이 값은 `1000.0f`와 같은데 텍스처의 모든 가능한 밉맵 레벨이 샘플링 가능하다는 뜻입니다. `lod` 값을 바꿀 이유는 없으므로 `mipLodBias`는 0.0f로 설정합니다.

이제 프로그램을 실행하면 아래와 같은 결과를 볼 수 있습니다:

![](/images/mipmaps.png)

큰 차이는 없는데, 우리 장면이 아주 간단하기 때문입니다. 자세히 들여다보면 몇 가지 세세한 차이점은 있습니다.

![](/images/mipmaps_comparison.png)

가장 큰 차이점은 종이에 쓰여진 것들입니다. 밉맵을 사용하면 좀 더 부드럽게 표시됩니다. 맵밉이 없을 땐 모서리가 두드러지고 Moiré 패턴으로 인한 간격이 보입니다.

샘플러 설정을 바꾸어 밉맵핑에 어떤 영향을 주는지 살펴보세요. 예를 들어 `minLod`를 바꾸면 샘플러가 가장 낮은 밉 레벨을 사용하지 않도록 할 수 있습니다:

```c++
samplerInfo.minLod = static_cast<float>(mipLevels / 2);
```

위와 같은 설정으로 인해 아래와 같은 결과가 도출됩니다:

![](/images/highmipmaps.png)

이것이 물체가 카메라에서부터 멀어졌을 때 높은 레벨의 밉이 적용된 모습니다.


[C++ code](/code/29_mipmapping.cpp) /
[Vertex shader](/code/27_shader_depth.vert) /
[Fragment shader](/code/27_shader_depth.frag)
