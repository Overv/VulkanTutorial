이 챕터에서는 그래픽스 파이프라인에서 이미지를 샘플링하기 위해 필요한 리소스 두 개를 더 만들어 보겠습니다. 첫 번째 리소스는 스왑 체인 이미지를 다루면서 이미 살펴본 것이지만 두 번째는 새로운 것입니다. 셰이더에서 이미지로브터 텍셀을 어떻게 읽는 방법에 관한 리소스입니다.

## 텍스처 이미지 뷰

전에 본 스왑 체인 이미지와 프레임버퍼에서, 이미지는 직접 접근되는 것이 아니라 이미지 뷰를 통해 접근하였습니다. 텍스처 이미지에 관해서도 이러한 이미지 뷰가 필요합니다.

텍스터 이미지의 `VkImageView`를 위한 클래스 멤버를 추가하고 이를 생성할 `createTextureImageView` 함수를 새로 추가합니다:

```c++
VkImageView textureImageView;

...

void initVulkan() {
    ...
    createTextureImage();
    createTextureImageView();
    createVertexBuffer();
    ...
}

...

void createTextureImageView() {

}
```

이 함수의 코드는 `createImageViews`에 기반합니다. 두 가지 변경해야 할 것은 `format`과 `image` 입니다:

```c++
VkImageViewCreateInfo viewInfo{};
viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
viewInfo.image = textureImage;
viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
viewInfo.subresourceRange.baseMipLevel = 0;
viewInfo.subresourceRange.levelCount = 1;
viewInfo.subresourceRange.baseArrayLayer = 0;
viewInfo.subresourceRange.layerCount = 1;
```

`viewInfo.components`에 관한 명시적 초기화는 제외하였는데 `VK_COMPONENT_SWIZZLE_IDENTITY`는 어차피 `0`으로 정의되어 있기 떄문입니다. `vkCreateImageView`를 호출함으로써 이미지 뷰 생성을 마칩니다:

```c++
if (vkCreateImageView(device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image view!");
}
```

`createImageViews`와 대부분의 로직이 동일하기 때문에 `createImageView` 함수를 새로 추상화 하는것이 좋겠습니다:

```c++
VkImageView createImageView(VkImage image, VkFormat format) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}
```

`createTextureImageView` 함수는 이제 아래와 같이 간단해집니다:

```c++
void createTextureImageView() {
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
}
```

그리고 `createImageViews`는 아래와 같이 간단해집니다:

```c++
void createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
    }
}
```

프로그램 종료 시점에 이미지 뷰를 소멸하는 것을 잊지 마십시오. 이미지 자체를 소멸하기 직전에 이러한 작업을 수행합니다.

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyImageView(device, textureImageView, nullptr);

    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);
```

## 샘플러(Samplers)

셰이더가 이미지로부터 텍셀을 직접 읽는 것도 가능하지만 텍스처에 대해서 이렇게 하는 것은 일반적이지 않습니다. 텍스처는 대개 샘플러를 통해 접근되는데, 이는 추출할 최종 색상을 계산하기 위해 필터링과 변환을 수행합니다.

이 필터들은 오버샘플링(oversampling) 같은 문제를 다루는 데 유용합니다. 텍셀보다 프래그먼트가 많은 물체게 대해 텍스처 맵핑이 수행된다고 생각해 보십시오. 각 프래그먼트의 텍스처 좌표에 대해 단순히 가장 가까운 텍셀을 사용하면 첫 번째 이미지와 같은 결과를 얻게 될겁니다:

![](/images/texture_filtering.png)

네 개의 가장 가까운 텍셀을 선형 보간(linear interpolation)하면 오른쪽과 같이 좀 더 부드러운 결과를 얻을 수 있습니다. 물론 여러분 응용 프로그램의 아트 스타일이 왼쪽의 경우와 더 잘 어울릴 수도 있습니다(마인크래프트 같은 경우). 하지만 일반적인 그래픽스 응용 프로그램에서는 오른쪽의 경우가 더 선호됩니다. 샘플러 객체는 텍스처로부터 색상을 읽을 때 이러한 필터링을 자동으로 수행해 줍니다.

언더샘플링(undersampling)은 반대의 경우로, 프래그먼트보다 텍셀이 더 많은 경우입니다. 이러한 경우 체커보드(checkerboard) 텍스처와 같은 고주파(high frequency) 패턴을 비스듬히 바라볼 때 문제가 생깁니다:

![](/images/anisotropic_filtering.png)

왼쪽 이미지에서 볼 수 있듯이 먼 곳의 텍스처는 흐릿하게 뭉개집니다. 이러한 문제의 해결 방안은 [비등방성(anisotropic) 필터링](https://en.wikipedia.org/wiki/Anisotropic_filtering)으로, 역시나 샘플러를 통해 자동적으로 적용될 수 있습니다.

이러한 필터 이외에도 샘플러는 변환도 처리해 줍니다. 여러분의 이미지 범위 밖의 텍셀을 읽을 떄 어떻게 처리할지도 *어드레싱 모드(addressing mode)*를 기반으로 결정합니다. 아래 이미지는 몇 가지 가능성을 보여줍니다:

![](/images/texture_addressing.png)

이제 `createTextureSampler` 함수를 만들어 이러한 샘플러 객체를 설정해 봅시다. 나중에 셰이더에서 이러한 샘플러를 활용해 텍스처로부터 색상을 읽어올 것입니다.

```c++
void initVulkan() {
    ...
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    ...
}

...

void createTextureSampler() {

}
```

샘플러는 `VkSamplerCreateInfo` 구조체를 통해 설정되는데, 적용되어야 할 필터와 변환들을 명시합니다.

```c++
VkSamplerCreateInfo samplerInfo{};
samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
samplerInfo.magFilter = VK_FILTER_LINEAR;
samplerInfo.minFilter = VK_FILTER_LINEAR;
```

`magFilter`와 `minFilter` 필드는 확대되거나 축소되는 텍스처를 어떻게 보간할 것인지를 명시합니다. 확대(magnification)는 위에서 설명한 오버샘플링 문제를 처리하는 방법이고 축소(minification)는 언더샘플링에 대한 방법입니다. 우리의 선택은 `VK_FILTER_NEAREST`와 `VK_FILTER_NEAREST`인데, 위 이미지에서 예시로 보여드린 방법에 대응되는 옵션입니다. 

```c++
samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
```

어드레싱 모드는 축(axis)별로 `addressMode` 필터를 통해 명시됩니다. 가능한 값들은 아래와 같습니다. 위 이미지에서 거의 모든 경우의 예시를 보여드렸습니다. 축들은 X,Y,Z가 아닌 U,V,W로 명시된다는 점을 주의하십시오. 이것이 텍스처 공간 좌표를 표현하는 일반적인 표기법입니다.

* `VK_SAMPLER_ADDRESS_MODE_REPEAT`: 이미지 범위 밖을 벗어날 경우 반복
* `VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT`: 반복과 유사하지만 범위 밖을 벗어날 경우 좌표를 뒤집어 이미지가 거울상(mirror)이 되도록 함
* `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE`: 범위 밖을 벗어날 경우 가장 가까운 축의 모서리(edge) 색상을 사용함
* `VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE`: 위 경우와 같지만 가장 가까운 모서리의 반대쪽 모서리를 사용
* `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER`: 범위 밖을 샘플링할 경우 단색(solid color)값을 반환함

여기서는 어떤 어드레싱 모드를 사용하건 관계 없습니다. 이 튜토리얼에서는 이미지 범위 밖에서는 샘플링을 하는 경우가 없기 떄문입니다. 하지만 반복(repeat) 모드가 가장 일반적인데 이 모드가 벽이나 바닥 같은 타일 텍스처에 가장 적합하기 때문입니다.

```c++
samplerInfo.anisotropyEnable = VK_TRUE;
samplerInfo.maxAnisotropy = ???;
```

이 두 필드는 비등방성 필터링을 사용할 것인지를 명시합니다. 성능에 문제가 없다면 이 기능을 사용하지 않은 이유가 없습니다. `maxAnisotropy` 필드는 최종 색성을 계산할 때 사용되는 텍셀 샘플의 수에 대한 제한값입니다. 값이 작으면 성능이 높지만 품질이 떨어집니다. 어떤 값을 사용할지를 알아내기 위해 물리적 장치의 속성을 얻어와야 합니다:

```c++
VkPhysicalDeviceProperties properties{};
vkGetPhysicalDeviceProperties(physicalDevice, &properties);
```

`VkPhysicalDeviceProperties` 구조체의 문서를 보시면 `limit`라고 이름지어진 `VkPhysicalDeviceLimits` 멤버를 보실 수 있습니다. 이 구조체는 `maxSamplerAnisotropy` 멤버를 가지고 있고 이것이 우리가 `maxAnisotropy`에 사용할 수 있는 최대값입니다. 가장 좋은 품질을 원한다면 그 값을 바로 사용하면 됩니다:

```c++
samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
```

프로그램의 시작 시점에 이 속성을 질의하고 값이 필요한 곳에 넘겨줄 수도 있습니다. 아니면 `createTextureSampler` 함수 내에서 질의하는 방법도 있습니다.

```c++
samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
```

`borderColor` 필드는 clamp to border 어드레싱 모드일 때, 범위 밖을 샘플링하는 경우 반환할 색상 값을 명시합니다. 검은색, 흰색, 또는 투명색을 float이나 int 포맷으로 반환할 수 있습니다. 임의의 색상을 명시하는 것은 불가능합니다.

```c++
samplerInfo.unnormalizedCoordinates = VK_FALSE;
```

`unnormalizedCoordinates` 필드는 이미지의 텍셀에 접근할 떄 어떤 좌표계를 사용할 지 명시합니다. `VK_TRUE`인 경우 `[0, texWidth)`와 `[0, texHeight)` 범위의 좌표를 사용하면 됩니다. `VK_FALSE`인 경우엔 텍셀은 모든 축에 대해 `[0,1)`로 접근합니다. 실제 응용 프로그램에서는 거의 항상 정규화된(normalized) 좌표계를 사용하는데, 이렇게 하면 다양한 해상도의 텍스처에 대해서도 동일한 좌표를 사용할 수 있기 때문입니다.

```c++
samplerInfo.compareEnable = VK_FALSE;
samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
```

비교(comparison) 함수가 활성화되면 텍셀은 먼저 값과 비교된 뒤에 그 비교 결과가 필터링 연산에 사용됩니다. 이는 주로 그림자 맵핑에서 [percentage-closer filtering](https://developer.nvidia.com/gpugems/GPUGems/gpugems_ch11.html)에 사용됩니다. 이에 대해서는 나중 챕터에서 살펴보겠습니다.

```c++
samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
samplerInfo.mipLodBias = 0.0f;
samplerInfo.minLod = 0.0f;
samplerInfo.maxLod = 0.0f;
```

이 필드들은 모두 밉맵핑에 적용됩니다. 밉맵핑에 대해서는 [나중 챕터](/Generating_Mipmaps)에서 살펴볼 것이고, 적용될 수 있는 또 다른 종류의 필터입니다.

이제 샘플러를 위한 기능이 모두 정의되었습니다. 샘플러 객체의 핸들을 저장할 클래스 멤버를 추가하고 `vkCreateSampler`를 사용해 샘플러를 생성합니다:

```c++
VkImageView textureImageView;
VkSampler textureSampler;

...

void createTextureSampler() {
    ...

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}
```

샘플러가 `VkImage`를 참조하지 않는다는 것을 주목하십시오. 샘플러는 텍스처에서 색상을 추출하는 인터페이스를 제공하는 별도의 객체입니다. 이는 1D, 2D, 3D 등 원하는 어떤 이미지에도 적용될 수 있습니다. 이것이 다른 오래된 API들과는 다른 점인데, 그것들의 경우 텍스처 이미지와 필터링을 하나의 상태로 결합합니다.

프로그램의 종료 시점, 더 이상 이미지에 접근할 필요가 없어지는 시점에 샘플러를 소멸시킵니다:

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);

    ...
}
```

## 비등방성 장치 기능

지금 시점에서 프로그램을 실행하면 아래와 같은 검증 레이어 메시지를 보게 됩니다:

![](/images/validation_layer_anisotropy.png)

사실 비등방성 필터링은 장치의 선택적인 기능입니다. 따라서 그 기능을 요청하기 위해서는 `createLogicalDevice` 함수를 수정해야 합니다:

```c++
VkPhysicalDeviceFeatures deviceFeatures{};
deviceFeatures.samplerAnisotropy = VK_TRUE;
```

최근의 그래픽 카드가 이를 지원하지 않은 가능성은 매우 낮지만, 그래도 `isDeviceSuitable`에서 이를 확인하도록 합니다:

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    ...

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}
```

불리언 값으로 기능을 요청하는 대신, `vkGetPhysicalDeviceFeatures`를 사용해 `VkPhysicalDeviceFeatures` 구조체를 변경함으로써 어떤 기능이 지원되는지를 표시하도록 할 수 있습니다.

비등방성 필터링의 가용성을 강제하는 대신, 아래와 같이 그 기능을 사용하지 않도록 할 수도 있습니다:

```c++
samplerInfo.anisotropyEnable = VK_FALSE;
samplerInfo.maxAnisotropy = 1.0f;
```

다음 챕터에서는 이미지와 샘플러 객체를 셰이더에 노출하여 사각형에 텍스처를 입혀 보도록 하겠습니다.

[C++ code](/code/25_sampler.cpp) /
[Vertex shader](/code/22_shader_ubo.vert) /
[Fragment shader](/code/22_shader_ubo.frag)
