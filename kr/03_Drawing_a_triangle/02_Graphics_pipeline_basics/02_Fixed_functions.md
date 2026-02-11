예전 그래픽스 API는 그래픽스 파이프라인 대부분의 단계에서 기본 상태(default state)를 제공했습니다. Vulkan에서는 파이프라인 상태 대부분을 명시적으로 설정해야 하고 이는 불변하는 파이프라인 상태 객체로 만들어집니다(baked). 이 챕터에서는 이러한 고정 함수 연산들에 대한 모든 구조체를 만들 것입니다.

## 동적 상태(Dynamic state)

*대부분*의 파이프라인 상태가 파이프라인 상태 객체로 만들어져야만 하지만 몇몇 상태들은 그리기 시점에 파이프라인을 *재생성하지 않고도 변경될 수 있습니다*. 예시로는 뷰포트의 크기라던지, 선의 두께라던지 블렌딩 상수 등이 있습니다. 동적 상태를 사용하고 싶고, 이런 상태들을 계속 제외된 상태로 두고 싶다면, `VkPipelineDynamicStateCreateInfo` 구조체를 아래와 같이 만들어야 합니다.

```c++
std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
};

VkPipelineDynamicStateCreateInfo dynamicState{};
dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
dynamicState.pDynamicStates = dynamicStates.data();
```

이렇게 하면 해당하는 값들의 설정은 무시되고 그리기 시점에 이들을 변경 가능(그리고 변경해야만) 합니다. 이렇게 하면 보다 유연한 설정이 가능하고 뷰포트나 시저(scissor) 상태에 대해서는 그렇게 하는 것이 일반적이지만, 파이프라인 상태 객체를 만드는 것이 보다 복잡해집니다.

## 정점 입력

`VkPipelineVertexInputStateCreateInfo` 구조체는 정점 데이터의 포맷을 기술하고, 이는 정점 셰이더로 넘겨집니다. 크게 두 가지 방법으로 기술됩니다:

* 바인딩: 데이터 사이의 간격과 데이터가 정점별 데이터인지 인스턴스별(per-instance) 데이터인지 여부 ([인스턴싱](https://en.wikipedia.org/wiki/Geometry_instancing) 참고)
* 어트리뷰트 기술: 정점 셰이더에 전달된 어트리뷰트의 타입, 어떤 바인딩에 이들을 로드할 것인지와 오프셋이 얼마인지

우리는 정점 셰이더에 정점 데이터를 하드 코딩하고 있기 때문에 지금은 이 구조체에 로드할 정점 데이터가 없다고 명시할 것입니다. 정점 버퍼 챕터에서 다시 살펴볼 것입니다.

```c++
VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
vertexInputInfo.vertexBindingDescriptionCount = 0;
vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
vertexInputInfo.vertexAttributeDescriptionCount = 0;
vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional
```

`pVertexBindingDescriptions`와 `pVertexAttributeDescriptions` 멤버는 앞서 언급한 정점 데이터를 로드하기 위한 세부 사항들을 기술하는 구조체의 배열에 대한 포인터입니다. 이 구조체를 `createGraphicsPipeline`함수의 `shaderStages` 배열 뒤에 추가합니다.

## 입력 조립

`VkPipelineInputAssemblyStateCreateInfo`구조체는 두 가지를 기술합니다: 정점으로부터 어떤 기하 형상이 그려질지와 프리미티브 재시작(restart)을 활성화할지 여부입니다. 앞의 내용은 `topology` 멤버에 명시되고 가능한 값들은 아래와 같습니다:

* `VK_PRIMITIVE_TOPOLOGY_POINT_LIST`: 정점으로부터 점을 그림
* `VK_PRIMITIVE_TOPOLOGY_LINE_LIST`: 재사용 없이 두 정점마다 선을 그림
* `VK_PRIMITIVE_TOPOLOGY_LINE_STRIP`: 선의 마지막 정점이 다음 선의 시작점으로 사용됨
* `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST`: 재사용 없이 3개의 정점마다 삼각형을 그림
* `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP `: 삼각형의 두 번쨰와 세 번째 정점이 다음 삼각형의 첫 두 개의 정점으로 사용됨

일반적으로 정점은 정점 버퍼로부터 인덱스 순서대로 로드되지만, *요소 버퍼(element buffer)*를 사용해 인덱스를 직접 명시할 수 있습니다. 이렇게 하면 정점의 재사용을 통해 성능을 최적화 할 수 있습니다. `primitiveRestartEnable` 멤버를 `VK_TRUE`로 설정했다면, `_STRIP` 토폴로지 모드의 선과 삼각형을 `0xFFFF` 또는 `0xFFFFFFFF`와 같은 특별한 인덱스를 사용해 분할할 수 있습니다.

이 예제에서는 삼각형들을 그릴 것이므로 구조체는 아래와 같은 값들로 설정할 것입니다:

```c++
VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
inputAssembly.primitiveRestartEnable = VK_FALSE;
```

## 뷰포트와 시저

뷰포트는 출력으로 그려질 프레임버퍼의 영역을 설정합니다. 튜토리얼에서는 거의 항상 `(0, 0)`에서 `(width, height)`까지고, 지금도 그렇게 설정합니다.

```c++
VkViewport viewport{};
viewport.x = 0.0f;
viewport.y = 0.0f;
viewport.width = (float) swapChainExtent.width;
viewport.height = (float) swapChainExtent.height;
viewport.minDepth = 0.0f;
viewport.maxDepth = 1.0f;
```

스왑 체인의 크기와 그 이미지가 윈도우의 `WIDTH`와 `HEIGHT`와는 다르다는 것을 기억하십시오. 스왑 체인 이미지는 나중에 프레임버퍼로 사용될 것이므로 그 크기를 사용해야 합니다.

`minDepth`와 `maxDepth` 값은 프레임버퍼에서 사용할 깊이 값의 범위입니다. 이 값들은 `[0.0f, 1.0f]` 범위여야 하지만 `minDepth`가 `maxDepth`보다 클 수 있습니다. 특수한 작업을 하는 것이 아니라면 `0.0f`와 `1.0f`의 일반적인 값을 사용하면 됩니다.

뷰포트가 이미지로부터 프레임버퍼로의 변환을 정의하는 반면, 시저 사각형은 픽셀이 저장될 실제 영역을 정의합니다. 시저 사각형 밖의 픽셀은 래스터화 단계에서 버려집니다. 이는 변환이 아닌 필터라고 보면 됩니다. 차이점이 아래에 나타나 있습니다. 왼쪽 시저 사각형은 위와 같은 결과가 도출되는 수 많은 가능성 중 하나일 뿐임에 주의하십시오. 뷰포트보다 큰 시저 사각형이면 모두 결과가 왼쪽 위와 같이 나타나게 됩니다.

![](/images/viewports_scissors.png)

따라서 전체 프레임버퍼에 그리고 싶다면 시저 사각형은 전체 범위를 커버하도록 명시하면 됩니다:

```c++
VkRect2D scissor{};
scissor.offset = {0, 0};
scissor.extent = swapChainExtent;
```

뷰포트와 시저 사각형은 파이프라인의 정적인 부분으로 명시할 수도 있고 [동적 상태](#dynamic-state)로 명시할 수도 있습니다. 정적으로 하는 경우 다른 상태들과 비슷하게 유지되지만 이들은 동적 상태로 명시하는 것이 유연성을 위해 더 편리한 방법입니다. 이런 방식이 더 일반적이고 동적 상태는 성능 저하 없도록 구현되어 있습니다.

동적 뷰포트와 시저 사각형을 위해서는 해당하는 동적 상태를 파이프라인에서 활성화해야 합니다:

```c++
std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
};

VkPipelineDynamicStateCreateInfo dynamicState{};
dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
dynamicState.pDynamicStates = dynamicStates.data();
```

그리고 그 개수를 파이프라인 생성 시에 명시해주면 됩니다.

```c++
VkPipelineViewportStateCreateInfo viewportState{};
viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
viewportState.viewportCount = 1;
viewportState.scissorCount = 1;
```

실제 뷰포트와 시저 사각형은 그리기 시점에 설정하면 됩니다.

동적 상태를 사용하면 하나의 명령 버퍼로부터 여러 뷰포트와 시저 사각형을 명시하는 것도 가능합니다.

동적 상태를 사용하지 않으면, 뷰포트와 시저 사각형은 `VkPipelineViewportStateCreateInfo` 구조체를 사용해 파이프라인에 설정되어야 합니다. 이렇게 생성된 뷰포트와 시저 사각형은 해당 파이프라인에서 불변성을 가집니다. 이 값들을 변경하고자 하면 새로운 값으로 새로운 파이프라인을 생성해야 합니다.

```c++
VkPipelineViewportStateCreateInfo viewportState{};
viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
viewportState.viewportCount = 1;
viewportState.pViewports = &viewport;
viewportState.scissorCount = 1;
viewportState.pScissors = &scissor;
```

어떻게 설정하셨건, 어떤 그래픽 카드에서는 다중 뷰포트와 시저 사각형이 사용 가능하므로 구조체의 멤버는 이들의 배열을 참조하도록 되어 있습니다. 다중 뷰포트 또는 시저 사각형을 사용하려면 GPU 기능을 활성화 하는 것도 필요합니다 (논리적 장치 생성 부분을 참고하세요).

## 래스터화

래스터화는 정점 셰이더로부터 만들어진 정점을 받아서 프래그먼트 셰이더에서 색상을 결정할 프래그먼트로 변환합니다. 또한 [깊이 테스트](https://en.wikipedia.org/wiki/Z-buffering),
[face culling](https://en.wikipedia.org/wiki/Back-face_culling)과 시저 테스트를 수행하고 출력 프래그먼트가 다각형 내부를 채우는지, 모서리만 그리는지(와이어프레임 렌더링)도 설정할 수 있습니다. 이 모든 것들은 `VkPipelineRasterizationStateCreateInfo` 구조체를 통해 설정합니다.

```c++
VkPipelineRasterizationStateCreateInfo rasterizer{};
rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
rasterizer.depthClampEnable = VK_FALSE;
```

`depthClampEnable`가 `VK_TRUE`면, near plane과 far plane 밖의 프래그먼트는 값이 버려지는 대신 clamp됩니다. 이는 그림자 맵과 같은 특수한 경우에 유용한 기능입니다. 이는 GPU 기능을 활성화함으로써 사용 가능합니다. 

```c++
rasterizer.rasterizerDiscardEnable = VK_FALSE;
```

`rasterizerDiscardEnable`가 `VK_TRUE`면 기하 요소는 래스터화 다음 단계로 넘어가지 않습니다. 이렇게 되면 기본적으로 프레임버퍼에 아무것도 출력되지 않습니다.

```c++
rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
```

`polygonMode`는 프래그먼트가 기하 요소를 생성하는 방식을 결정합니다. 아래와 같은 모드들이 사용 가능합니다:

* `VK_POLYGON_MODE_FILL`: 폴리곤 영역을 프래그먼트로 채움
* `VK_POLYGON_MODE_LINE`: 폴리곤의 모서리가 선으로 그려짐
* `VK_POLYGON_MODE_POINT`: 폴리곤의 정점이 점으로 그려짐

채우기 모드 이외에는 GPU 기능을 활성화해야 사용 가능합니다.

```c++
rasterizer.lineWidth = 1.0f;
```

`lineWidth` 멤버는 이해하기 쉽습니다. 프래그먼트 개수 단위로 선의 두께를 명시합니다. 지원하는 선의 최대 두께는 하드웨어에 따라 다르며, `1.0f` 이상의 값은 `wideLines` GPU 기능을 활성화해야만 사용 가능합니다.

```c++
rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
```

`cullMode` 변수는 사용할 face culling 타입을 결정합니다. culling을 하지 않거나, 앞면(front face)를 culling하거나, 뒷면(back fack)를 culling하거나, 양면 모두를 culling할 수 있습니다. `frontFace` 변수는 앞면으로 간주할 면의 정점 순서를 명시하며 시계방향 또는 반시계방향일 수 있습니다.

```c++
rasterizer.depthBiasEnable = VK_FALSE;
rasterizer.depthBiasConstantFactor = 0.0f; // Optional
rasterizer.depthBiasClamp = 0.0f; // Optional
rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
```

래스터화 단계에서 상수를 더하거나 프래그먼트의 기울기를 기반으로 깊이값을 편향(bias)시킬 수 있습니다. 이 기능은 그림자 맵에서 종종 사용되지만, 우리는 사용하지 않을 것입니다. `depthBiasEnable`를 `VK_FALSE`로 설정합니다.

## 멀티샘플링(Multisampling)

`VkPipelineMultisampleStateCreateInfo`구조체는 멀티샘플링을 설정하는데, 이는 [안티앨리어싱(anti-aliasing)](https://en.wikipedia.org/wiki/Multisample_anti-aliasing)을 하는 방법 중 하나입니다. 이는 동일한 픽셀로 래스터화되는 여러 다각형의 프래그먼트 셰이더 결과를 결합하여 수행됩니다. 주로 모서리에서 수행되며, 모서리가 앨리어싱에 따른 문제가 가장 눈에 띄게 발생하는 부분입니다. 단일 다각형이 픽셀에 맵핑되는 경우에는 프래그먼트 셰이더를 여러 번 실행할 필요가 없기 때문에, 단순히 고해상도로 렌더링한 후에 다운샘플링(downsampling)하는 것 보다 훨씬 계산 비용이 낮습니다. 이를 사용하려면 GPU 기능을 활성화해야 합니다.

```c++
VkPipelineMultisampleStateCreateInfo multisampling{};
multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
multisampling.sampleShadingEnable = VK_FALSE;
multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
multisampling.minSampleShading = 1.0f; // Optional
multisampling.pSampleMask = nullptr; // Optional
multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
multisampling.alphaToOneEnable = VK_FALSE; // Optional
```

멀티샘플링은 나중 챕터에서 다시 살펴볼 것이고, 지금은 활성화 하지 않은 상태로 두겠습니다.

## 깊이와 스텐실(stencil) 테스트

깊이 또는 스텐실 버퍼를 사용하는 경우 `VkPipelineDepthStencilStateCreateInfo`를 사용해 깊이와 스텐실 테스트를 설정해야 합니다. 지금은 그렇지 않으니 구조체에 대한 포인터 대신 `nullptr`를 전달합니다. 깊이 버퍼링 챕터에서 다시 사용할 것입니다.

## 컬러 블렌딩

프래그먼트 셰이더가 값을 반환한 후에는 이미 프레임버퍼에 쓰여진 색상값과 결합되어야 합니다. 이러한 변환 과정은 컬러 블렌딩이라 하며 두 가지 방법이 있습니다:

* 쓰여진 값과 새 값을 섞어 새로운 색상을 만듬
* 쓰여진 값과 새 값을 비트 연산(bitwise operation)하여 결합

컬러 블렌딩을 구성하는 두 종류의 구조체가 있습니다. 먼저 `VkPipelineColorBlendAttachmentState`는 어태치(attach)된 프레임버퍼별 설정을 담은 구조체이며 `VkPipelineColorBlendStateCreateInfo`는 *전역(global)* 컬러 블렌딩 설정을 담고 있습니다. 우리는 하나의 프레임버퍼만 사용합니다:

```c++
VkPipelineColorBlendAttachmentState colorBlendAttachment{};
colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
colorBlendAttachment.blendEnable = VK_FALSE;
colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
```

이 프레임버퍼별 구조체는 컬러 블렌딩의 첫 번째 방법을 설정할 수 있도록 합니다. 수행되는 연산은 다음과 같은 의사 코드(pseudocode)로 잘 설명됩니다:

```c++
if (blendEnable) {
    finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
    finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
} else {
    finalColor = newColor;
}

finalColor = finalColor & colorWriteMask;
```

`blendEnable`가 `VK_FALSE`면, 프래그먼트 셰이더에서 계산한 새로운 색상이 수정되지 않고 전달됩니다. 그렇지 않으면 새로운 색상을 위해 두 개의 결합(mix) 연산이 수행됩니다. 결과 색상은 `colorWriteMask`를 통해 명시된 채널들과 AND연산이 수행되어 전달될 채널이 결정됩니다.

컬러 블렌딩을 하는 가장 보편적인 방법은 알파 블렌딩(alpha blending)입니다. 이는 새로운 색상이 이미 쓰여진 색상과 불투명도(opacity)를 기반으로 섞이는 것입니다. `finalColor`는 다음과 같이 계산됩니다:

```c++
finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
finalColor.a = newAlpha.a;
```

이는 아래와 같은 매개변수를 사용하면 수행됩니다:

```c++
colorBlendAttachment.blendEnable = VK_TRUE;
colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
```

명세에 있는 `VkBlendFactor`와 `VkBlendOp` 열거형을 통해 모든 가능한 연산을 찾아볼 수 있습니다.

두 번째 구조체는 모든 프레임버퍼를 위한 구조체 배열의 참조이고, 앞서 언급한 계산들에 사용할 블렌드 팩터(blend factor)들로 사용될 블렌드 상수들을 설정할 수 있습니다.

```c++
VkPipelineColorBlendStateCreateInfo colorBlending{};
colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
colorBlending.logicOpEnable = VK_FALSE;
colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
colorBlending.attachmentCount = 1;
colorBlending.pAttachments = &colorBlendAttachment;
colorBlending.blendConstants[0] = 0.0f; // Optional
colorBlending.blendConstants[1] = 0.0f; // Optional
colorBlending.blendConstants[2] = 0.0f; // Optional
colorBlending.blendConstants[3] = 0.0f; // Optional
```

블렌딩의 두 번째 방법(비트 연산)을 하려면 `logicOpEnable`를 `VK_TRUE`로 설정해야 합니다. 그러면 비트 연산은 `logicOp` 필드에 명시됩니다. 주의하실 점은 이러한 경우 첫 번째 방법은 자동으로 비활성화 됩니다. 마치 여러분이 모든 프레임버퍼에 대해 `blendEnable`를 `VK_FALSE`로 설정한 것과 같이 말이죠! `colorWriteMask` 또한 이 모드에서 프레임버퍼의 어떤 채널이 영향을 받을지를 결정하기 위해 사용됩니다. 지금 우리가 한 것처럼 두 모드를 모두 비활성화 하는 것도 가능합니다. 이러한 경우 프래그먼트 색상은 변경되지 않고 그대로 프레임버퍼에 쓰여집니다.

## 파이프라인 레이아웃(layout)

셰이더에서 사용하는 `uniform`은 동적 상태 변수처럼 전역적인 값으로 셰이더를 재생성하지 않고 그리기 시점에 값을 변경하여 다른 동작을 하도록 할 수 있습니다. 이는 주로 변환 행렬을 정점 셰이더에 전달하거나, 프래그먼트 셰이더에 텍스처 샘플러(sampler)를 생성하기 위해 사용됩니다.

이러한 uniform 값은 `VkPipelineLayout` 객체를 생성하여 파이프라인 생성 단계에서 명시되어야 합니다. 나중 챕터로 넘어가기 전까지는 사용하지 않을 것이지만 빈 파이프라인 레이아웃이라도 생성해 두어야만 합니다.

이 객체를 저장할 클래스 멤버를 만들 것인데, 나중에 다른 함수에서 참조할 것이기 때문입니다.

```c++
VkPipelineLayout pipelineLayout;
```

그리고 `createGraphicsPipeline` 함수에서 객체를 만듭니다.

```c++
VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 0; // Optional
pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
}
```

구조체는 또한 *push 상수*를 명시하는데, 나중에 알아보겠지만 동적인 값을 셰이더에 전달하는 또 다른 방법입니다. 파이프라인 레이아웃은 프로그램의 실행 주기동안 참조되므로 마지막에는 소멸되어야 합니다:

```c++
void cleanup() {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    ...
}
```

## 결론

이로써 고정 함수 상태는 끝입니다! 이 모든 것들을 처음부터 만들어가는 과정은 힘들었지만, 그로 인해 그래픽스 파이프라인에서 일어나는 거의 모든 일들을 알게 되었습니다! 이러한 과정으로 인해 뜻밖의 오류가 발생할 가능성이 줄어들 것인데 특성 구성요소의 기본 상태를 제공한다면 그렇지 않았을 것입니다.

그래픽스 파이프라인 생성을 위해서는 아직도 하나 더 객체를 만들어야 하고, 이는 [렌더 패스(render pass)](!kr/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes)입니다.

[C++ code](/code/10_fixed_functions.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
