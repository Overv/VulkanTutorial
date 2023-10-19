## 설정

파이프라인을 마무리 하기 전에, Vulkan에 렌더링을 할 때 사용할 프레임버퍼에 대해 알려줄 필요가 있습니다. 얼마나 많은 색상과 깊이 버퍼가 있을 것인지, 각각에 대해 얼마나 많은 샘플을 사용할 것인지, 렌더링 연산 과정에서 각각의 내용들이 어떻게 처리될 것인지 등을 명시해야 합니다. 이런 모든 정보가 *렌더 패스(render pass)* 객체에 포함되는데, 이를 위해 새롭게 `createRenderPass` 함수를 만들 겁니다. 이 함수를 `initVulkan` 함수에서 `createGraphicsPipeline` 전에 호출합니다.

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
}

...

void createRenderPass() {

}
```

## 어태치먼트(attachment) 기술

우리의 경우 스왑 체인 안에 있는 이미지들 중 하나로 하나의 색상 버퍼 어태치먼트만 있을 것입니다.

```c++
void createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
}
```

색상 어태치먼트의 `format`과 스왑 체인의 이미지 포맷은 동일해야 하며, 지금은 멀티샘플링을 하지 않으니 1개의 샘플을 사용합니다.

```c++
colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
```

`loadOp` 과 `storeOp`은 렌더링 전과 후에 데이터로 어떤 작업을 할 것인지를 결정합니다. `loadOp`과 관련해서는 다음과 같은 선택지가 있습니다:

* `VK_ATTACHMENT_LOAD_OP_LOAD`: 어태치먼트에 존재하는 내용을 유지
* `VK_ATTACHMENT_LOAD_OP_CLEAR`: 시작 시 상수값으로 내용을 채움
* `VK_ATTACHMENT_LOAD_OP_DONT_CARE`: 어떤 내용이 존재하는지 정의할 수 없음. 신경쓰지 않음

우리의 경우 새로운 프레임을 그리기 전에 clear 연산을 통해 검은 색으로 프레임버퍼를 지울 것입니다. `storeOp`의 선택지는 두 가지입니다:

* `VK_ATTACHMENT_STORE_OP_STORE`: 렌더링된 내용이 메모리에 저장되고 나중에 읽을 수 있음
* `VK_ATTACHMENT_STORE_OP_DONT_CARE`: 프레임버퍼의 내용이 렌더링 이후에도 정의되지 않음으로 남아 있음

우리는 화면에 삼각형을 그리고 그걸 확인하는 데 관심이 있으니 store 연산을 사용할 것입니다.

```c++
colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
```

`loadOp`과 `storeOp`는 색상과 깊이 데이터에 대해 적용되고 `stencilLoadOp` /
`stencilStoreOp`은 스텐실 데이터에 적용됩니다. 우리 프로그램은 스텐실 버퍼에 대해 아무런 작업을 하지 않기 때문에 이에 대한 로드와 저장은 신경쓰지 않습니다.

```c++
colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
```

Vulkan의 텍스처와 프레임버퍼는 특정 픽셀 포맷의 `VkImage` 객체로 표현됩니다. 하지만 픽셀의 메모리 레이아웃은 이미지를 어디에 사용하느냐에 따라 달라질 수 있습니다.

흔히 사용되는 레이아웃은 아래와 같습니다:

* `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`: 이미지가 색상 어태치먼트로 사용됨
* `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`: 스왑 체인을 통해 표시될 이미지
* `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`: 메모리 복사의 대상으로 사용되는 이미지

텍스처링(texturing) 챕터에서 이 주제에 대해 보다 자세히 논의할 것입니다. 지금 알아두셔야 할 중요한 사항은 이미지는 그 이후에 사용될 작업과 관련한 연산에 어울리는 특정한 레이아웃으로 정의해야 한다는 것입니다.

`initLayout`은 렌더 패스가 시작되기 전 이미지가 어떤 레이아웃일지 명시합니다. `finalLayout`은 렌더 패스가 끝나면 자동적으로 어떤 레이아웃으로 사용될지를 명시합니다. `initLayout`에 `VK_IMAGE_LAYOUT_UNDEFINED`를 사용하면 우리는 그 이미지가 전에 어떤 레이아웃이었는지 신경쓰지 않겠다는 뜻입니다. 이 경우의 단점은 이미지의 내용이 보존된다는 보장이 없다는 것이지만 지금 우리는 어차피 내용을 지우고 시작할 것이니 관계 없습니다. 렌더링 이후에 스왑 체인을 사용해 이미지를 표시할 것이니 `finalLayout`으로는 `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`를 사용합니다.

## 서브패스(subpass)와 어태치먼트 참조

라나의 렌더 패스는 여러 서브패스로 구성될 수 있습니다. 서브패스는 이전 패스에서 저장된 프레임버퍼의 내용을 가지고 렌더링 연산을 수행하는 일련의 패스입니다. 예를 들어 여러 후처리(post-processing) 과정을 연속적으로 적용하는 경우를 들 수 있습니다. 이러한 렌더링 연산들의 집합을 하나의 렌더 패스에 넣으면, Vulkan이 그 순서를 조정해 메모리 대역폭을 아껴서 보다 나은 성능을 얻을 수 있습니다. 하지만 우리의 삼각형 같은 경우 하나의 서브패스만 있으면 됩니다.

모든 서브패스는 위와 같은 내용에 우리가 이전 장에서 만든 하나 이상의 어태치먼트 구조체를 참조합니다. 이 참조는 `VkAttachmentReference` 구조체로 아래와 같습니다.

```c++
VkAttachmentReference colorAttachmentRef{};
colorAttachmentRef.attachment = 0;
colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
```

`attachment` 매개변수는 인덱스를 사용해 어태치먼트 배열로부터 참조할 어태치먼트를 명시합니다. 우리 배열은 하나의 `VkAttachmentDescription`만 가지고 있으므로 인덱스는 `0`입니다. `layout`은 서브패스 실행 동안 이 참조를 사용하고자 하는 어태치먼트의 레이아웃을 명시합니다. Vulkan은 서브패스가 시작되면 자동적으로 어태치먼트를 이 레이아웃으로 전환합니다. 우리는 어태치먼트가 색상 버퍼의 기능을 하길 원하고 `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`가 이름 그대로 가장 좋은 성능을 내줄 겁니다.

서브패스는 `VkSubpassDescription` 구조체를 사용해 기술됩니다:

```c++
VkSubpassDescription subpass{};
subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
```

Vulkan이 나중에는 계산을 위한 서브패스도 지원할 지 모르므로, 그래픽스 서브패스를 위한 것이면 이를 명시해야 합니다. 다음으로 색상 어태치먼트의 참조를 명시합니다:

```c++
subpass.colorAttachmentCount = 1;
subpass.pColorAttachments = &colorAttachmentRef;
```

이 배열의 어태치먼트의 인덱스는 프래그먼트 셰이더에서 직접 `layout(location = 0) out vec4 outColor` 지시자를 통해 참조됩니다!

다른 어태치먼트 타입들이 서브패스로부터 참조될 수 있습니다:

* `pInputAttachments`: 셰이서에서 읽어온 어태치먼트
* `pResolveAttachments`: 색상 어태치먼트의 멀티샘플링을 위한 어태치먼트
* `pDepthStencilAttachment`: 깊이와 스텐실 데이터를 위한 어태치먼트
* `pPreserveAttachments`: 이 서브패스에는 사용되지 않지만 데이터가 보존되어야 하는 어태치먼트

## 렌더 패스

이제 어태치먼트와 기본적인 서브패스 참조가 명시되었으므로 렌더 패스를 만들 수 있습니다. `VkRenderPass` 객체를 저장하기 위한 새로운 클래스 멤버를 `pipelineLayout` 위에 정의합니다.

```c++
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
```

렌더 패스 객체는 `VkRenderPassCreateInfo`구조체에 어태치먼트 배열과 서브패스들을 채워서 생성합니다. `VkRenderPassCreateInfo`객체는 그 배열의 인덱스를 명시함으로써 어태치먼트를 참조합니다.

```c++
VkRenderPassCreateInfo renderPassInfo{};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
renderPassInfo.attachmentCount = 1;
renderPassInfo.pAttachments = &colorAttachment;
renderPassInfo.subpassCount = 1;
renderPassInfo.pSubpasses = &subpass;

if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
}
```

파이프라인 레이아웃처럼, 렌더 패스도 프로그램 실행 중 계속 참조되므로 프로그램 종료 시점에 해제되어야 합니다:

```c++
void cleanup() {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    ...
}
```

많은 작업을 했는데요, 이제 다음 챕터에서 모든 것들을 합쳐서 드디어 그래픽스 파이프라인 객체를 만들 것입니다!

[C++ code](/code/11_render_passes.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
