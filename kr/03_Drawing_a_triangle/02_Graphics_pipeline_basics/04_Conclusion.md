이제 이전 장에서 만든 모든 구조체와 객체들을 사용해 그래픽스 파이프라인을 만들 것입니다! 복습으로 우리가 가진 객체들의 종류를 되돌아봅시다:

* 셰이더 단계: 그래픽스 파이프라인 내의 프로그램 가능한 단계들의 기능을 정의하는 셰이더 모듈
* 고정 함수 상태: 파이프라인의 고정함수 단계들을 정의하는 구조체들. 여기에는 입력 조립기, 래스터화, 뷰포트와 컬러 블렌딩이 포함됨
* 파이프라인 레이아웃: 셰이더가 참조하는, 그리기 시점에 갱신될 수 있는 유니폼과 push 값들
* 렌더 패스: 파이프라인에서 참조하는 어태치먼트들과 그 사용 용도

이 것들이 모여 그래픽스 파이프라인의 기능을 완전히 명시합니다. 이제 우리는 `createGraphicsPipeline` 함수의 마지막 부분에 `VkGraphicsPipelineCreateInfo` 구조체를 만들 수 있습니다. `vkDestroyShaderModule` 보다는 전이어야 하는데 이것들이 생성 과정에서 사용되기 때문입니다.

```c++
VkGraphicsPipelineCreateInfo pipelineInfo{};
pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
pipelineInfo.stageCount = 2;
pipelineInfo.pStages = shaderStages;
```

`VkPipelineShaderStageCreateInfo`구조체의 배열을 참조하는 것으로 시작합니다.

```c++
pipelineInfo.pVertexInputState = &vertexInputInfo;
pipelineInfo.pInputAssemblyState = &inputAssembly;
pipelineInfo.pViewportState = &viewportState;
pipelineInfo.pRasterizationState = &rasterizer;
pipelineInfo.pMultisampleState = &multisampling;
pipelineInfo.pDepthStencilState = nullptr; // Optional
pipelineInfo.pColorBlendState = &colorBlending;
pipelineInfo.pDynamicState = &dynamicState;
```

그리고 고정함수 단계를 기술하는 구조체들을 참조합니다.

```c++
pipelineInfo.layout = pipelineLayout;
```

다음으로 파이프라인 레이아웃이 오는데, 여기에는 구조체에 대한 포인터가 아닌 Vulkan 핸들을 사용합니다.

```c++
pipelineInfo.renderPass = renderPass;
pipelineInfo.subpass = 0;
```

마지막으로 그래픽스 파이프라인이 사용할 렌더 패스에 대한 참조와 서브패스 인덱스가 있습니다. 이 특정 인스턴스가 아닌 다른 렌더 패스를 사용할 수도 있지만 그러한 경우 그것들이 `renderPass`와 *호환되어야 합니다*. 호환성에 대해서는 [여기](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap8.html#renderpass-compatibility)에 설명되어 있지만 그러한 기능은 이 튜토리얼에서는 사용하지 않을 겁니다.

```c++
pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
pipelineInfo.basePipelineIndex = -1; // Optional
```

두 개의 매개변수가 사실 더 있습니다. `basePipelineHandle`와
`basePipelineIndex` 입니다. Vulkan에서는 기존 파이프라인으로부터 새로운 그래픽스 파이프라인을 만들 수도 있습니다. 이러한 파이프라인 유도(derivative)는 대부분의 기능이 비슷한 파이프라인을 설정하는 데 성능적인 이점이 있고, 같은 부모로부터 유도된 파이프라인으로 교체하는 것은 더 빠르게 수행될 수 있습니다. 기존 파이프라인의 핸들을 `basePipelineHandle`에 명시하거나 곧 생성할 파리프라인의 인덱스를 `basePipelineIndex`를 사용해 참조할 수 있습니다. 지금은 하나의 파이프라인만 있으므로 널 핸들과 유효하지 않은 인덱스로 설정해 둡니다. 이러한 기능은 `VkGraphicsPipelineCreateInfo`의 `flag` 필드에 `VK_PIPELINE_CREATE_DERIVATIVE_BIT`가 명시되어 있어야만 사용할 수 있습니다.

마지막으로 `VkPipeline` 객체를 저장할 클래스 멤버를 준비해 둡시다:

```c++
VkPipeline graphicsPipeline;
```

그리고 최종적으로 그래픽스 파이프라인을 만듭니다:

```c++
if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
}
```

`vkCreateGraphicsPipelines`함수는 Vulkan의 다른 객체들을 만들 떄보다 더 많은 매개변수를 받습니다. 한 번의 호출로 여러 개의 `VkGraphicsPipelineCreateInfo`를 받아 여러 개의 `VkPipeline`객체를 만들 수 있게 되어있습니다.

`VK_NULL_HANDLE`를 넘겨준 두 번째 매개변수는 선택적으로 `VkPipelineCache`객체에 대한 참조를 넘겨줄 수 있습니다. 파이프라인 캐시(cache)는 여러 `vkCreateGraphicsPipelines` 호출을 위해, 파이프라인 생성을 위한 데이터를 저장하고 재사용하는데 사용될 수 있습니다. 만일 캐시가 파일로 저장되어 있다면 다른 프로그램에서도 사용될 수 있습니다. 이렇게 하면 나중에 파이프라인 생성을 위해 소요되는 시간을 눈에 띄게 줄일 수 있습니다. 이에 대해서는 파이프라인 캐시 챕터에서 살펴보겠습니다.

모든 그리기 연산 과정에서는 그래픽스 파이프라인이 필요하므로 프로그램이 종료될 때에만 해제되어야 합니다.

```c++
void cleanup() {
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    ...
}
```

이제 프로그램을 실행하고 작업에 대한 보상으로 성공적으로 파이프라인이 만들어졌는지 확인하세요! 이제 무언가 화면에 나오기까지 얼마 남지 않았습니다. 다음 몇 개 챕터에서는 스왑 체인으로부터 실제 프레임버퍼를 설정하고 그리기 명령을 준비해 보겠습니다.

[C++ code](/code/12_graphics_pipeline_complete.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
