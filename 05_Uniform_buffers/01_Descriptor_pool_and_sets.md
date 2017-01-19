## Introduction

The descriptor layout from the previous chapter describes the type of
descriptors that can be bound. In this chapter we're going to create a
descriptor set, which will actually specify a `VkBuffer` resource to bind to the
uniform buffer descriptor.

## Descriptor pool

Descriptor sets can't be created directly, they must be allocated from a pool
like command buffers. The equivalent for descriptor sets is unsurprisingly
called a *descriptor pool*. We'll write a new function `createDescriptorPool`
to set it up.

```c++
void initVulkan() {
    ...
    createUniformBuffer();
    createDescriptorPool();
    ...
}

...

void createDescriptorPool() {

}
```

We first need to describe which descriptor types our descriptor sets are going
to contain and how many of them, using `VkDescriptorPoolSize` structures.

```c++
VkDescriptorPoolSize poolSize = {};
poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
poolSize.descriptorCount = 1;
```

We only have a single descriptor right now with the uniform buffer type. This
pool size structure is referenced by the main `VkDescriptorPoolCreateInfo`:

```c++
VkDescriptorPoolCreateInfo poolInfo = {};
poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
poolInfo.poolSizeCount = 1;
poolInfo.pPoolSizes = &poolSize;
```

We also need to specify the maximum number of descriptor sets that will be
allocated:

```c++
poolInfo.maxSets = 1;
```

The structure has an optional flag similar to command pools that determines if
individual descriptor sets can be freed or not:
`VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT`. We're not going to touch
the descriptor set after creating it, so we don't need this flag. You can leave
`flags` to its default value of `0`.

```c++
VDeleter<VkDescriptorPool> descriptorPool{device, vkDestroyDescriptorPool};

...

if (vkCreateDescriptorPool(device, &poolInfo, nullptr, descriptorPool.replace()) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
}
```

Add a new class member to store the handle of the descriptor pool and call
`vkCreateDescriptorPool` to create it.

## Descriptor set

We can now allocate the descriptor set itself. Add a `createDescriptorSet`
function for that purpose:

```c++
void initVulkan() {
    ...
    createDescriptorPool();
    createDescriptorSet();
    ...
}

...

void createDescriptorSet() {

}
```

A descriptor set allocation is described with a `VkDescriptorSetAllocateInfo`
struct. You need to specify the descriptor pool to allocate from, the number of
descriptor sets to allocate, and the descriptor layout to base them on:

```c++
VkDescriptorSetLayout layouts[] = {descriptorSetLayout};
VkDescriptorSetAllocateInfo allocInfo = {};
allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
allocInfo.descriptorPool = descriptorPool;
allocInfo.descriptorSetCount = 1;
allocInfo.pSetLayouts = layouts;
```

Add a class member to hold the descriptor set handle and allocate it with
`vkAllocateDescriptorSets`:

```c++
VDeleter<VkDescriptorPool> descriptorPool{device, vkDestroyDescriptorPool};
VkDescriptorSet descriptorSet;

...

if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor set!");
}
```

You don't need to use a deleter for descriptor sets, because they will be
automatically freed when the descriptor pool is destroyed. The call to
`vkAllocateDescriptorSets` will allocate one descriptor set with one uniform
buffer descriptor.

The descriptor set has been allocated now, but the descriptors within still need
to be configured. Descriptors that refer to buffers, like our uniform buffer
descriptor, are configured with a `VkDescriptorBufferInfo` struct. This
structure specifies the buffer and the region within it that contains the data
for the descriptor:

```c++
VkDescriptorBufferInfo bufferInfo = {};
bufferInfo.buffer = uniformBuffer;
bufferInfo.offset = 0;
bufferInfo.range = sizeof(UniformBufferObject);
```

The configuration of descriptors is updated using the `vkUpdateDescriptorSets`
function, which takes an array of `VkWriteDescriptorSet` structs as parameter.

```c++
VkWriteDescriptorSet descriptorWrite = {};
descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrite.dstSet = descriptorSet;
descriptorWrite.dstBinding = 0;
descriptorWrite.dstArrayElement = 0;
```

The first two fields specify the descriptor set to update and the binding. We
gave our uniform buffer binding index `0`. Remember that descriptors can be
arrays, so we also need to specify the first index in the array that we want to
update. We're not using an array, so the index is simply `0`.

```c++
descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
descriptorWrite.descriptorCount = 1;
```

We need to specify the type of descriptor again. It's possible to update
multiple descriptors at once in an array, starting at index `dstArrayElement`.
The `descriptorCount` field specifies how many array elements you want to
update.

```c++
descriptorWrite.pBufferInfo = &bufferInfo;
descriptorWrite.pImageInfo = nullptr; // Optional
descriptorWrite.pTexelBufferView = nullptr; // Optional
```

The last field references an array with `descriptorCount` structs that actually
configure the descriptors. It depends on the type of descriptor which one of the
three you actually need to use. The `pBufferInfo` field is used for descriptors
that refer to buffer data, `pImageInfo` is used for descriptors that refer to
image data, and `pTexelBufferView` is used for descriptors that refer to buffer
views. Our descriptor is based on buffers, so we're using `pBufferInfo`.

```c++
vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
```

The updates are applied using `vkUpdateDescriptorSets`. It accepts two kinds of
arrays as parameters: an array of `VkWriteDescriptorSet` and an array of
`VkCopyDescriptorSet`. The latter can be used to copy the configuration of
descriptors, as its name implies.

## Using a descriptor set

We now need to update the `createCommandBuffers` function to actually bind the
descriptor set to the descriptors in the shader with `cmdBindDescriptorSets`:

```c++
vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
```

Unlike vertex and index buffers, descriptor sets are not unique to graphics
pipelines. Therefore we need to specify if we want to bind descriptor sets to
the graphics or compute pipeline. The next parameter is the layout that the
descriptors are based on. The next three parameters specify the index of the
first descriptor set, the number of sets to bind, and the array of sets to bind.
We'll get back to this in a moment. The last two parameters specify an array of
offsets that are used for dynamic descriptors. We'll look at these in a future
chapter.

If you run your program now, then you'll notice that unfortunately nothing is
visible. The problem is that because of the Y-flip we did in the projection
matrix, the vertices are now being drawn in clockwise order instead of
counter-clockwise order. This causes backface culling to kick in and prevents
any geometry from being drawn. Go to the `createGraphicsPipeline` function and
modify the `cullFace` in `VkPipelineRasterizationStateCreateInfo` to correct
this:

```c++
rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
```

Run your program again and you should now see the following:

![](/images/spinning_quad.png)

The rectangle has changed into a square because the projection matrix now
corrects for aspect ratio. The `updateUniformData` takes care of screen
resizing, so we don't need to recreate the descriptor set in
`recreateSwapChain`.

## Multiple descriptor sets

As some of the structures and function calls hinted at, it is actually possible
to bind multiple descriptor sets. You need to specify a descriptor layout for
each descriptor set when creating the pipeline layout. Shaders can then
reference specific descriptor sets like this:

```c++
layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
```

You can use this feature to put descriptors that vary per-object and descriptors
that are shared into separate descriptor sets. In that case you avoid rebinding
most of the descriptors across draw calls which is potentially more efficient.

[C++ code](/code/descriptor_set.cpp) / /
[Vertex shader](/code/shader_ubo.vert) / /
[Fragment shader](/code/shader_ubo.frag)
