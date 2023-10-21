지난 몇 개 챕터에서 프레임버퍼에 관련한 이야기를 많이 언급했고, 이제 설정을 마친 렌더 패스는 스왑 체인 이미지와 동일한 포맷을 가진 하나의 프레임버퍼를 예정하여 만들어졌습니다. 하지만 프레임버퍼를 실제로 만들지는 않았습니다.

렌더 패스 생성 중 명시한 어태치먼트는 `VkFramebuffer` 객체로 래핑하여 설정되었습니다. 프레임버퍼 객체는 어태치먼트를 표현하는 모든 `VkImageView` 객체들을 참조합니다. 우리의 경우 색상 어태치먼트 하나입니다. 하지만 우리가 어태치먼트로 사용해야 하는 이미지의 개수는 표현하고자 하는 이미지를 스왑 체인으로부터 반환 받을 때 몇 개의 이미지가 반환되느냐에 달려 있습니다. 즉, 우리는 스왑 체인에 있는 모든 이미지에 대해 프레임버퍼를 생성해야 하고 그리기 시점에 출력된 이미지와 관계된 이미지를 사용해야 합니다.

이를 위해 프레임버퍼를 저장할 `std::vector` 클래스 멤버를 하나 더 추가합니다.

```c++
std::vector<VkFramebuffer> swapChainFramebuffers;
```

이 배열을 위한 객체를 `initVulkan`의 그래픽스 파이프라인 생성 뒤에서 호출되는 `createFramebuffers`라는 새로운 함수를 통해 생성합니다.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
}

...

void createFramebuffers() {

}
```

모든 프레임버퍼가 저장될 수 있도록 컨테이너의 크기를 조정하는 것부터 시작합니다:

```c++
void createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());
}
```

그리고 이미지 뷰를 순회하며 그로부터 프레임버퍼를 생성합니다:

```c++
for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    VkImageView attachments[] = {
        swapChainImageViews[i]
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
    }
}
```

보시는 것처럼 프레임버퍼의 생성은 굉장히 직관적입니다. 먼저 어떤 `renderPass`에 프레임버퍼가 호환되어야 하는지부터 명시합니다. 렌더 패스에 호환되는 프레임버퍼만 사용할 수 있는데, 일단 일반적으로 어태치먼트의 타입과 개수가 같아야 합니다.

`attachmentCount`와 `pAttachments` 매개변수가 렌더 패스의 `pAttachment`배열에서 기술된 해당하는 어태치먼트와 바인딩되어야 하는 `VkImageView` 객체를 명시합니다.

`width` 와 `height` 매개변수는 이름 그대로고 `layers`는 이미지 배열의 레이어 개수를 의미합니다. 우리의 스왑 체인 이미지는 하나이므로 레이어 개수는 `1`입니다.

프레임버퍼는 이를 기반으로 하는 이미지 뷰와 렌더 패스가 해제되기 전에 해제해야 하고, 렌더링이 끝난 후에 해제해야 합니다:

```c++
void cleanup() {
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    ...
}
```

이제 렌더링에 필요한 모든 객체의 생성이라는 목표를 달성했습니다. 다음 장에서는 실제 그리기 명령을 처음으로 작성해 보겠습니다.

[C++ code](/code/13_framebuffers.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
