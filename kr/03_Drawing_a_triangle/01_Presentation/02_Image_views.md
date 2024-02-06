스왑 체인에 포함된 `VkImage`를 사용하기 위해서는 렌더링 파이프라인에서 `VkImageView` 객체를 생성해야 합니다. 이미지 뷰(image view)는 말 그대로 이미지에 대한 뷰 입니다. 이를 통해 이미지에 어떻게 접근하는지와 이미지의 어느 부분에 접근할 것인지를 명시하는데, 예를 들어 2D 텍스처로 취급될 것인지, 밉맵(mipmap) 수준이 없는 깊이 텍스처(depth texture)로 취급될 것인지와 같은 사항입니다.

이 장에서 우리는 `createImageViews` 함수를 작성하여 스왑 체인에 있는 모든 이미지에 대한 이미지 뷰를 생성하고 이는 나중에 컬러 타겟으로 사용될 것입니다.

먼저 이미지 뷰를 저장할 클래스 멤버를 추가합니다:

```c++
std::vector<VkImageView> swapChainImageViews;
```

`createImageViews` 함수를 만들고 스왑 체인 생성 후에 호출합니다.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
}

void createImageViews() {

}
```

우선적으로 해애 할 일은 리스트의 크기를 조정해 우리가 생성할 이미지 뷰가 모두 들어갈 수 있도록 하는 것입니다.

```c++
void createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

}
```

다음으로 모든 스왑 체인 이미지에 대한 반복문을 만듭니다.

```c++
for (size_t i = 0; i < swapChainImages.size(); i++) {

}
```

이미지 뷰 생성에 대한 매개변수는 `VkImageViewCreateInfo` 구조체에 명시됩니다. 처음 몇 개의 매개변수는 직관적입니다.

```c++
VkImageViewCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
createInfo.image = swapChainImages[i];
```

`viewType`과 `format` 필드는 이미지 데이터가 어떻게 해석되어야 할지를 명시합니다. `viewType` 매개변수는 이미지를 1차원, 2차원, 3차원 혹은 큐브 맵(cube map)으로 취급할 수 있도록 합니다.

```c++
createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
createInfo.format = swapChainImageFormat;
```

`components` 필드는 컬러 채널을 뒤섞을 수 있도록 합니다. 예를 들어 흑백(monochrome) 텍스처를 위해서는 모든 채널을 빨간색 채널로 맵핑할 수 있습니다. 또한 `0`이나 `1`과 같은 상수를 채널에 맵핑할 수도 있습니다. 우리의 경우 기본(default) 맵핑을 사용할 것입니다.

```c++
createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
```

`subresourceRange` 필드는 이미지의 목적이 무엇인지와 어떤 부분이 접근 가능할지를 기술합니다. 우리 이미지는 컬러 타겟이고 밉맵핑이나 다중 레이어는 사용하지 않습니다.

```c++
createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
createInfo.subresourceRange.baseMipLevel = 0;
createInfo.subresourceRange.levelCount = 1;
createInfo.subresourceRange.baseArrayLayer = 0;
createInfo.subresourceRange.layerCount = 1;
```

스테레오 3D 응용 프로그램을 만든다면, 스왑 체인을 다중 레이어로 만들 것입니다. 그런 경우 각 이미지에 대한 다중 이미지 뷰를 만들 수 있고, 이는 왼쪽과 오른쪽 눈에 대한 이미지 표현을 서로 다른 레이어를 통해 접근할 수 있도록 합니다.

이제 이미지 뷰를 만드는 것은 `vkCreateImageView`를 호출하면 됩니다.

```c++
if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image views!");
}
```

이미지와는 다르게 이미지 뷰는 우리가 명시적으로 만든 것이기 때문에 소멸을 위해서는 프로그램 종료 시점에 반복문을 추가해야 합니다.

```c++
void cleanup() {
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    ...
}
```

이미지를 텍스처로 사용하기 위한 목적으로는 이미지 뷰를 만드는 것으로 충분하지만 렌더 타겟으로 만들기 위해서는 아직 할 일이 남아 있습니다. 이를 위해서는 프레임버퍼(framebuffer)와 관련된 추가적인 작업이 필요합니다. 하지만 우선 그래픽스 파이프라인부터 설정하도록 하겠습니다.

[C++ code](/code/07_image_views.cpp)
