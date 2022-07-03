We can now combine all of the structures and objects from the previous chapters
to create the graphics pipeline! Here's the types of objects we have now, as a
quick recap:

* Shader stages: the shader modules that define the functionality of the
programmable stages of the graphics pipeline
* Fixed-function state: all of the structures that define the fixed-function
stages of the pipeline, like input assembly, rasterizer, viewport and color
blending
* Pipeline layout: the uniform and push values referenced by the shader that can
be updated at draw time
* Render pass: the attachments referenced by the pipeline stages and their usage

All of these combined fully define the functionality of the graphics pipeline,
so we can now begin filling in the `VkGraphicsPipelineCreateInfo` structure at
the end of the `createGraphicsPipeline` function. But before the calls to 
`vkDestroyShaderModule` because these are still to be used during the creation.

```c++
VkGraphicsPipelineCreateInfo pipelineInfo{};
pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
pipelineInfo.stageCount = 2;
pipelineInfo.pStages = shaderStages;
```

We start by referencing the array of `VkPipelineShaderStageCreateInfo` structs.

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

Then we reference all of the structures describing the fixed-function stage.

```c++
pipelineInfo.layout = pipelineLayout;
```

After that comes the pipeline layout, which is a Vulkan handle rather than a
struct pointer.

```c++
pipelineInfo.renderPass = renderPass;
pipelineInfo.subpass = 0;
```

And finally we have the reference to the render pass and the index of the sub
pass where this graphics pipeline will be used. It is also possible to use other
render passes with this pipeline instead of this specific instance, but they
have to be *compatible* with `renderPass`. The requirements for compatibility
are described [here](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap8.html#renderpass-compatibility),
but we won't be using that feature in this tutorial.

```c++
pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
pipelineInfo.basePipelineIndex = -1; // Optional
```

There are actually two more parameters: `basePipelineHandle` and
`basePipelineIndex`. Vulkan allows you to create a new graphics pipeline by
deriving from an existing pipeline. The idea of pipeline derivatives is that it
is less expensive to set up pipelines when they have much functionality in
common with an existing pipeline and switching between pipelines from the same
parent can also be done quicker. You can either specify the handle of an
existing pipeline with `basePipelineHandle` or reference another pipeline that
is about to be created by index with `basePipelineIndex`. Right now there is
only a single pipeline, so we'll simply specify a null handle and an invalid
index. These values are only used if the `VK_PIPELINE_CREATE_DERIVATIVE_BIT`
flag is also specified in the `flags` field of `VkGraphicsPipelineCreateInfo`.

Now prepare for the final step by creating a class member to hold the
`VkPipeline` object:

```c++
VkPipeline graphicsPipeline;
```

And finally create the graphics pipeline:

```c++
if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
}
```

The `vkCreateGraphicsPipelines` function actually has more parameters than the
usual object creation functions in Vulkan. It is designed to take multiple
`VkGraphicsPipelineCreateInfo` objects and create multiple `VkPipeline` objects
in a single call.

The second parameter, for which we've passed the `VK_NULL_HANDLE` argument,
references an optional `VkPipelineCache` object. A pipeline cache can be used to
store and reuse data relevant to pipeline creation across multiple calls to
`vkCreateGraphicsPipelines` and even across program executions if the cache is
stored to a file. This makes it possible to significantly speed up pipeline
creation at a later time. We'll get into this in the pipeline cache chapter.

The graphics pipeline is required for all common drawing operations, so it
should also only be destroyed at the end of the program:

```c++
void cleanup() {
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    ...
}
```

Now run your program to confirm that all this hard work has resulted in a
successful pipeline creation! We are already getting quite close to seeing
something pop up on the screen. In the next couple of chapters we'll set up the
actual framebuffers from the swap chain images and prepare the drawing commands.

[C++ code](/code/12_graphics_pipeline_complete.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
