## Introduction

The descriptor set layout from the previous chapter describes the type of
descriptors that can be bound. In this chapter we're going to create
a descriptor set for each `VkBuffer` resource to bind it to the
uniform buffer descriptor.

## Descriptor pool

Descriptor sets can't be created directly, they must be allocated from a pool
like command buffers. The equivalent for descriptor sets is unsurprisingly
called a *descriptor pool*. We'll write a new function `createDescriptorPool`
to set it up.

```c++
void initVulkan() {
    ...
    createUniformBuffers();
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
VkDescriptorPoolSize poolSize{};
poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
```

We will allocate one of these descriptors for every frame. This
pool size structure is referenced by the main `VkDescriptorPoolCreateInfo`:

```c++
VkDescriptorPoolCreateInfo poolInfo{};
poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
poolInfo.poolSizeCount = 1;
poolInfo.pPoolSizes = &poolSize;
```

Aside from the maximum number of individual descriptors that are available, we
also need to specify the maximum number of descriptor sets that may be
allocated:

```c++
poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
```

The structure has an optional flag similar to command pools that determines if
individual descriptor sets can be freed or not:
`VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT`. We're not going to touch
the descriptor set after creating it, so we don't need this flag. You can leave
`flags` to its default value of `0`.

```c++
VkDescriptorPool descriptorPool;

...

if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
}
```

Add a new class member to store the handle of the descriptor pool and call
`vkCreateDescriptorPool` to create it.

## Descriptor set

We can now allocate the descriptor sets themselves. Add a `createDescriptorSets`
function for that purpose:

```c++
void initVulkan() {
    ...
    createDescriptorPool();
    createDescriptorSets();
    ...
}

...

void createDescriptorSets() {

}
```

A descriptor set allocation is described with a `VkDescriptorSetAllocateInfo`
struct. You need to specify the descriptor pool to allocate from, the number of
descriptor sets to allocate, and the descriptor set layout to base them on:

```c++
std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
VkDescriptorSetAllocateInfo allocInfo{};
allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
allocInfo.descriptorPool = descriptorPool;
allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
allocInfo.pSetLayouts = layouts.data();
```

In our case we will create one descriptor set for each frame in flight, all with the same layout.
Unfortunately we do need all the copies of the layout because the next function expects an array matching the number of sets.

Add a class member to hold the descriptor set handles and allocate them with
`vkAllocateDescriptorSets`:

```c++
VkDescriptorPool descriptorPool;
std::vector<VkDescriptorSet> descriptorSets;

...

descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
}
```

You don't need to explicitly clean up descriptor sets, because they will be
automatically freed when the descriptor pool is destroyed. The call to
`vkAllocateDescriptorSets` will allocate descriptor sets, each with one uniform
buffer descriptor.

```c++
void cleanup() {
    ...
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    ...
}
```

The descriptor sets have been allocated now, but the descriptors within still need
to be configured. We'll now add a loop to populate every descriptor:

```c++
for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

}
```

Descriptors that refer to buffers, like our uniform buffer
descriptor, are configured with a `VkDescriptorBufferInfo` struct. This
structure specifies the buffer and the region within it that contains the data
for the descriptor.

```c++
for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);
}
```

If you're overwriting the whole buffer, like we are in this case, then it is also possible to use the `VK_WHOLE_SIZE` value for the range. The configuration of descriptors is updated using the `vkUpdateDescriptorSets`
function, which takes an array of `VkWriteDescriptorSet` structs as parameter.

```c++
VkWriteDescriptorSet descriptorWrite{};
descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrite.dstSet = descriptorSets[i];
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
`VkCopyDescriptorSet`. The latter can be used to copy descriptors to each other,
as its name implies.

## Using descriptor sets

We now need to update the `recordCommandBuffer` function to actually bind the
right descriptor set for each frame to the descriptors in the shader with `vkCmdBindDescriptorSets`. This needs to be done before the `vkCmdDrawIndexed` call:

```c++
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
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
matrix, the vertices are now being drawn in counter-clockwise order instead of
clockwise order. This causes backface culling to kick in and prevents
any geometry from being drawn. Go to the `createGraphicsPipeline` function and
modify the `frontFace` in `VkPipelineRasterizationStateCreateInfo` to correct
this:

```c++
rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
```

Run your program again and you should now see the following:

![](/images/spinning_quad.png)

The rectangle has changed into a square because the projection matrix now
corrects for aspect ratio. The `updateUniformBuffer` takes care of screen
resizing, so we don't need to recreate the descriptor set in
`recreateSwapChain`.

## Alignment requirements

One thing we've glossed over so far is how exactly the data in the C++ structure should match with the uniform definition in the shader. It seems obvious enough to simply use the same types in both:

```c++
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
```

However, that's not all there is to it. For example, try modifying the struct and shader to look like this:

```c++
struct UniformBufferObject {
    glm::vec2 foo;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

layout(binding = 0) uniform UniformBufferObject {
    vec2 foo;
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
```

Recompile your shader and your program and run it and you'll find that the colorful square you worked so far has disappeared! That's because we haven't taken into account the *alignment requirements*.

Vulkan expects the data in your structure to be aligned in memory in a specific way, for example:

* Scalars have to be aligned by N (= 4 bytes given 32 bit floats).
* A `vec2` must be aligned by 2N (= 8 bytes)
* A `vec3` or `vec4` must be aligned by 4N (= 16 bytes)
* A nested structure must be aligned by the base alignment of its members rounded up to a multiple of 16.
* A `mat4` matrix must have the same alignment as a `vec4`.

You can find the full list of alignment requirements in [the specification](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap15.html#interfaces-resources-layout).

Our original shader with just three `mat4` fields already met the alignment requirements. As each `mat4` is 4 x 4 x 4 = 64 bytes in size, `model` has an offset of `0`, `view` has an offset of 64 and `proj` has an offset of 128. All of these are multiples of 16 and that's why it worked fine.

The new structure starts with a `vec2` which is only 8 bytes in size and therefore throws off all of the offsets. Now `model` has an offset of `8`, `view` an offset of `72` and `proj` an offset of `136`, none of which are multiples of 16. To fix this problem we can use the [`alignas`](https://en.cppreference.com/w/cpp/language/alignas) specifier introduced in C++11:

```c++
struct UniformBufferObject {
    glm::vec2 foo;
    alignas(16) glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
```

If you now compile and run your program again you should see that the shader correctly receives its matrix values once again.

Luckily there is a way to not have to think about these alignment requirements *most* of the time. We can define `GLM_FORCE_DEFAULT_ALIGNED_GENTYPES` right before including GLM:

```c++
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
```

This will force GLM to use a version of `vec2` and `mat4` that has the alignment requirements already specified for us. If you add this definition then you can remove the `alignas` specifier and your program should still work.

Unfortunately this method can break down if you start using nested structures. Consider the following definition in the C++ code:

```c++
struct Foo {
    glm::vec2 v;
};

struct UniformBufferObject {
    Foo f1;
    Foo f2;
};
```

And the following shader definition:

```c++
struct Foo {
    vec2 v;
};

layout(binding = 0) uniform UniformBufferObject {
    Foo f1;
    Foo f2;
} ubo;
```

In this case `f2` will have an offset of `8` whereas it should have an offset of `16` since it is a nested structure. In this case you must specify the alignment yourself:

```c++
struct UniformBufferObject {
    Foo f1;
    alignas(16) Foo f2;
};
```

These gotchas are a good reason to always be explicit about alignment. That way you won't be caught offguard by the strange symptoms of alignment errors.

```c++
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
```

Don't forget to recompile your shader after removing the `foo` field.

## Multiple descriptor sets

As some of the structures and function calls hinted at, it is actually possible
to bind multiple descriptor sets simultaneously. You need to specify a descriptor set layout for
each descriptor set when creating the pipeline layout. Shaders can then
reference specific descriptor sets like this:

```c++
layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
```

You can use this feature to put descriptors that vary per-object and descriptors
that are shared into separate descriptor sets. In that case you avoid rebinding
most of the descriptors across draw calls which is potentially more efficient.

[C++ code](/code/23_descriptor_sets.cpp) /
[Vertex shader](/code/22_shader_ubo.vert) /
[Fragment shader](/code/22_shader_ubo.frag)
