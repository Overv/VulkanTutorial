지난 몇 개의 챕터에서 프레임버퍼와 관련한 많은 것들을 이야기 했고 스왑 체인 이미지와 같은 포맷인, 단일 프레임버퍼를 사용할 렌더 패스를 설정했습니다. 하지만 아직 실제 생성은 하지 않았습니다.

렌더 패스 생성 과정에서 명시한 어태치먼트는 `VkFramebuffer` 객체로 래핑하여 바인딩됩니다. 프레임버퍼 객체는 어태치먼트를 표현하는 모든 `VkImageView` 객체를 참조합니다. 우리의 경우 어태치먼트는 색상 어태치먼트 하나입니다. 하지만 어태치먼트로 사용해야 하는 이미지는 우리가 화면에 표시하기 위한 이미지를 요청했을 때 스왑 체인이 어떠한 이미지를 반환하냐에 달려 있습니다. 즉, 우리는 스왑 체인에 있는 모든 이미지에 대해 프레임버퍼를 만들어야 하고, 그리기 시점에는 그 중 하나를 선택해서 사용해야 합니다.

이를 위해 프레임버퍼를 저장할 `std::vector` 클래스 멤버를 하나 더 만듭니다:


```c++
std::vector<VkFramebuffer> swapChainFramebuffers;
```

이 배열을 위한 객체는 `initVulkan`에서 호출할 새로운 `createFramebuffers`함수에서 만들 것입니다. 그래픽스 파이프라인 생성 이후에 호출합니다:


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


모든 프레임버퍼를 저장할 수 있도록 컨테이너 크기부터 조정합니다:


```c++
void createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());
}
```


그리고 이미지 뷰를 순회하면서 이를 기반으로 프레임버퍼를 만듭니다:


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


보이시는 것처럼 프레임버퍼의 생성은 매우 직관적입니다. 먼저 어떤 `renderPass`에 프레임버퍼가 호환되어야 하는지 명시합니다. 호환되는 경우에만 렌더 패스에 프레임버퍼를 사용할 수 있는데, 숫자와 타입이 같아야 합니다.

`attachmentCount`와 `pAttachments` 매개변수는 렌더 패스의 `pAttachment` 배열에 해당하는 어태치먼트 기술자와 바인딩될 `VkImageView` 객체를 명시합니다.

`width` 와 `height` 매개변수는 설명하지 않아도 될 것 같고, `layers`는 이미지 배열의 레이어 수를 의미합니다. 우리 스왑 체인 이미지는 하나이므로, 레이어 수도 `1`입니다.

프레임버퍼는 렌더링이 모두 완료된 후에, 이를 사용하는 이미지 뷰와 렌더 패스보다 먼저 해제되어야 합니다:


```c++
void cleanup() {
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    ...
}
```


이제 렌더링을 위해 필요한 모든 객체를 만들었습니다. 이제 다음 챕터에서는 실제 그리기 명령을 작성해 보도록 하겠습니다.


[C++ code](/code/13_framebuffers.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
