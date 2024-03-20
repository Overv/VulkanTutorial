## 서론

우리 프로그램은 이제 텍스처의 다양한 디테일 레벨을 로딩할 수 있어서 관찰자로부터 멀리 떨어져 있는 물체를 렌더링 할 때 발생하는 문제를 해결할 수 있게 되었습니다. 이제 이미지가 훨씬 부드럽게 보이지만 자세히 살펴보면 물체의 모서리를 따라서 톱날과 같은 들쭉날쭉한 패턴을 볼 수 있을 겁니다. 우리가 초반에 만든 사각형을 렌더링하는 프로그램과 같은 경우에 더 두드러집니다:

![](/images/texcoord_visualization.png)

이러한 의도하지 않은 효과는 "앨리어싱(aliasing)" 이라고 하며 렌더링에 사용하는 픽셀 수의 한계로 인해 나타나는 현상입니다. 무한한 해상도를 가진 디스플레이는 존재하지 않으니, 어느 정도는 피할 수 없는 현상이긴 합니다. 이 문제를 해결하는 여러 가지 방법이 있는데 이 챕터에서는 유명한 방법 중 하나인 [멀티샘플 안티앨리어싱(anti-aliasing), MSAA](https://en.wikipedia.org/wiki/Multisample_anti-aliasing)에 집중해 보도록 하겠습니다.

일반적인 렌더링 과정에서 픽셀 색상은 하나의 샘플링 포인트로부터 결정되고, 일반적으로 이 포인트는 스크린의 각 픽셀의 중심점입니다. 어떤 선의 일부분이 픽셀을 지나긴 하지만 샘플링 포인트를 지나지는 않는다면, 해당 픽셀은 빈 픽셀이 되고 이로 인해 들쭉날쭉한 "계단" 현상이 발생합니다.

![](/images/aliasing.png)

MSAA가 하는 것은 이름 그대로 하나의 픽셀에 대해 여러 샘플링 포인트를 사용해서 최종 색상을 결정하는 것입니다. 예상할 수 있듯이 더 많은 샘플을 사용하면 더 좋은 결과를 얻을 수 있지만 연산량이 증가하게 됩니다.

![](/images/antialiasing.png)

우리의 구현에서는 사용 가능한 최대 샘플링 개수에 집중할 것입니다. 응용 프로그램에 따라 이러한 접근법보다는 품질이 만족되는 선에서 더 적은 샘플을 사용하는 것이 성능 면에서 더 나은 선택일 수 있습니다.

## 가용한 샘플 개수 획득

먼저 우리 하드웨어가 얼마나 많은 샘플을 사용할 수 있는지부터 결정해 보겠습니다. 대부분의 현대 GPU들은 최소 8개의 샘플을 지원하지만 이 숫자가 항상 보장되는 것은 아닙니다. 새 플래스 멤버를 추가해서 추적해 보도록 하겠습니다:

```c++
...
VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
...
```

기본적으로 픽셀당 하나의 샘플을 사용하는데 이는 멀티샘플링을 적용하지 않는 것과 마찬가지입니다. 정확한 최대 샘플 개수는 선택된 물리적 장치와 관련된 `VkPhysicalDeviceProperties`를 통해 얻을 수 있습니다. 깊이 버퍼를 사용하고 있기 때문에 색상과 깊이에 대한 샘플 개수를 모두 고려할 필요가 있습니다. 두 개가 모두 지원하는 최대 샘플 개수가 최종적으로 사용할 최대 샘플 개수입니다. 이러한 정보를 획득하기 위한 함수를 추가합니다:

```c++
VkSampleCountFlagBits getMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}
```

이제 이 함수를 사용해서 물리적 장치 선택 과정에서 `msaaSamples` 변수의 값을 설정할 것입니다. `pickPhysicalDevice` 함수를 조금만 변경하면 됩니다:

```c++
void pickPhysicalDevice() {
    ...
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            msaaSamples = getMaxUsableSampleCount();
            break;
        }
    }
    ...
}
```

## 렌더 타겟 설정

MSAA에서 각 픽셀은 오프스크린 버퍼에서 샘플링되고, 그 이후에 화면에 렌더링됩니다. 이 새로 등장한 버퍼는 지금까지 렌더링을 수행한 일반적인 이미지와는 약간 다릅니다. 각 픽셀에 하나 이상의 샘플을 저장합니다. 멀티샘플 버퍼가 생성되고 난 이후에 기본 프레임버퍼 (픽셀당 하나의 샘플을 저장하는)에 적용(resolve)되어야 합니다. 따라서 추가적인 렌더 타겟을 생성하고 그리기 과정을 수정해야만 합니다. 깊이 버퍼처럼 한 번에 하나의 그리기 연산만 활성화되기 때문에 렌더 타겟은 하나만 있으면 됩니다. 아래 클래스 멤버들을 추가합니다:

```c++
...
VkImage colorImage;
VkDeviceMemory colorImageMemory;
VkImageView colorImageView;
...
```

이 새로운 이미지가 픽셀당 의도한 숫자만큼의 샘플을 저장할 것이므로 그 숫자를 이미지 생성 과정에서 `VkImageCreateInfo`에 넘겨줘야 합니다. `createImage` 함수에 `numSamples` 매개변수를 추가합니다:

```c++
void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    ...
    imageInfo.samples = numSamples;
    ...
```

지금은 이 함수를 호출하는 모든 부분에서 `VK_SAMPLE_COUNT_1_BIT`를 사용하도록 수정합니다. 구현을 진행하면서 이 부분들을 적절한 값으로 대체할 것입니다:

```c++
createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
...
createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
```

이제 멀티샘플 색상 버퍼를 생성할 차례입니다. `createColorResources` 함수를 추가할 것인데 여기서는 `createImage`의 매개변수로 `msaaSamples`를 사용한 것에 주의하십시오. 또한 밉 레벨은 하나만 사용할 것인데 Vulkan 명세에서 픽셀당 샘플이 하나 이상인 경우에는 반드시 이렇게 하도록 요구하고 있습니다. 또한 어차피 텍스처로 활용할 것이 아니기 때문에 밉맵이 필요하지 않습니다:

```c++
void createColorResources() {
    VkFormat colorFormat = swapChainImageFormat;

    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
    colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}
```

일관성을 위해 `createDepthResources` 바로 앞에서 이 함수를 호출합니다:

```c++
void initVulkan() {
    ...
    createColorResources();
    createDepthResources();
    ...
}
```

이제 멀티샘플 색상 버퍼가 준비되었으니 깊이 값을 처리할 차례입니다. `createDepthResources`를 수정하여 깊이 버퍼에 사용할 샘플 개수를 적용합니다:

```c++
void createDepthResources() {
    ...
    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    ...
}
```

Vulkan 리소스를 추가적으로 만든 것이니 적절한 시점에 해제하는 것도 잊으면 안됩니다:

```c++
void cleanupSwapChain() {
    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);
    ...
}
```

`recreateSwapChain`를 수정하여 윈도우 크기가 변하면 적절한 해상도로 색상 이미지를 다시 생성하도록 합니다:

```c++
void recreateSwapChain() {
    ...
    createImageViews();
    createColorResources();
    createDepthResources();
    ...
}
```

이제 초기 MSAA 설정은 끝났고, 그래픽스 파이프라인, 프레임버퍼, 렌더패스에서 새로 만든 리소스를 사용하도록 하여 결과를 살펴보겠습니다!

## 새로운 어태치먼트 추가

먼저 렌더 패스부터 작업합니다. `createRenderPass`의 색상과 깊이 어태치먼트 생성 정보 구조체를 수정합니다:

```c++
void createRenderPass() {
    ...
    colorAttachment.samples = msaaSamples;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ...
    depthAttachment.samples = msaaSamples;
    ...
```

`finalLayout`을 `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`에서 `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`로 수정한 것을 눈치채셨을 겁니다. 그 이유는 멀티샘플링된 이미지가 바로 표시될 수 없기 때문입니다. 이 이미지를 일반적인 이미지에 먼저 적용(resolve)해야만 합니다. 이러한 요구사항이 깊이 버퍼에 대해서는 적용되지 않는데, 어쨌든 화면에 표시되지 않는 버퍼이기 때문입니다. 따라서 색상에 대한  어태치먼트만 추가하면 되고, 이를 적용 어태치먼트라고 하겠습니다:

```c++
    ...
    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    ...
```

이제 렌더 패스가 멀티샘플링된 색상 이미지를 일반적인 어태치먼트로 적용하도록 명시해야만 합니다. 적용의 대상이 되는 색상 버퍼를 참조인 어태치먼트를 새로 만듭니다:

```c++
    ...
    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ...
```

`pResolveAttachments` 서브패스 구조체 멤버가 새로 만든 어태치먼트 참조를 가리키도록 설정합니다. 이렇게 하면 렌더 패스가 멀티샘플 적용 연산을 정의하여 이미지를 화면에 렌더링 할 수 있습니다:

```
    ...
    subpass.pResolveAttachments = &colorAttachmentResolveRef;
    ...
```

멀티샘플링된 색상 이미지를 재사용하고 있으므로 `VkSubpassDependency`의 `srcAccessMask`를 수정해야 합니다. 이러한 수정을 통해 색상 어태치먼트로의 쓰기 연산이 이후의 연산이 시작되기 전에 완료될 수 있어서 쓰기 연산의 중복으로 인해 발생할 수 있는 불안정한 렌더링 문제를 해결할 수 있습니다:

```c++
    ...
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    ...
```

이제 렌더 패스 정보 구조체를 새로운 색상 어태치먼트로 갱신합니다:

```c++
    ...
    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
    ...
```

렌더 패스가 준비되면 `createFramebuffers`를 수정하여 새로운 이미지 뷰 들을 목록에 추가합니다:

```c++
void createFramebuffers() {
        ...
        std::array<VkImageView, 3> attachments = {
            colorImageView,
            depthImageView,
            swapChainImageViews[i]
        };
        ...
}
```

마지막으로 `createGraphicsPipeline`를 수정해 새로 만들어진 파이프라인에 샘플을 하나 이상 사용하도록 명시합니다: 

```c++
void createGraphicsPipeline() {
    ...
    multisampling.rasterizationSamples = msaaSamples;
    ...
}
```

이제 프로그램을 실행하면 아래와 같은 화면이 보입니다:

![](/images/multisampling.png)

밉맵핑처럼 변화가 확 눈에 들어오지는 않습니다. 자세히 살펴보면 모서리가 이제 더이상 들쭉날쭉하지 않고, 전체적으로 그 전의 이미지보다 부드러워 진 것을 확인할 수 있습니다.

![](/images/multisampling_comparison.png)

모서리 부분을 확대해 보면 차이점이 좀 더 눈에 띕니다:

![](/images/multisampling_comparison2.png)

## 품질 향상

현재의 MSAA 구현은 좀 더 복잡한 장면의 경우에 대해서는 품질에 문제가 발생할 수 있습니다. 에를 들어, 셰이더 앨리어싱으로 인해 발생될 수 있는 잠재적인 문제는 해결하고 있지 않습니다. 즉, MSAA는 모서리를 부드럽게만 할 뿐, 내부에 채워진 값에 대해서는 그렇지 못합니다. 이로 인해 예를 들어 폴리곤 자체는 부드럽게 표현되지만 적용된 색상에 대해서는 색상 대조가 큰 경우 앨리어싱이 발생하게 됩니다. 이 문제에 대한 접근법 중 하나로 [샘플 세이딩](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap27.html#primsrast-sampleshading)을 활성화하는 것이 있는데, 이렇게 하면 이미지 품질을 더 높일 수 있지만 더 높은 계산 비용이 발생합니다:

```c++

void createLogicalDevice() {
    ...
    deviceFeatures.sampleRateShading = VK_TRUE; // enable sample shading feature for the device
    ...
}

void createGraphicsPipeline() {
    ...
    multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
    multisampling.minSampleShading = .2f; // min fraction for sample shading; closer to one is smoother
    ...
}
```

이 예시에서는 샘플 세이딩을 비활성화 한 채로 놔둘 것이지만 경우에 따라 이러한 품질의 차이가 크게 눈에 뜨일수도 있습니다:

![](/images/sample_shading.png)

## 결론

여기에 오기까지 힘드셨을 것이지만, 이제 Vulkan 프로그램에 대한 기본 지식을 갖게 되셨을 겁니다. 여러분이 갖게 된 Vulkan의 기본 원리에 대한 지식은, 더 다양한 기능들을 살펴보기 위한 배경으로 충분할 것입니다:

* Push constants
* Instanced rendering
* Dynamic uniforms
* Separate images and sampler descriptors
* Pipeline cache
* Multi-threaded command buffer generation
* Multiple subpasses
* Compute shaders

현재 만들어진 프로그램은 다양한 방식으로 확장될 수 있는데, Blinn-Phong 라이팅을 추가한다거나, 후처리 효과를 더한다거나, 그림자 맵핑을 수행하는 등이 있을 겁니다. 다른 API의 튜토리얼을 통해 이러한 효과들이 작동하는 방식을 배우실 수 있을 겁니다. Vulkan은 명시성이라는 특징이 있긴 하지만 대부분의 컨셉은 비슷하기 때문입니다.

[C++ code](/code/30_multisampling.cpp) /
[Vertex shader](/code/27_shader_depth.vert) /
[Fragment shader](/code/27_shader_depth.frag)
