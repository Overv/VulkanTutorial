## Introduction

In this bonus chapter we'll take a look at compute shaders. Up until now all previous chapters dealt with the traditional graphics part of the Vulkan pipeline. But unlike older APIs like OpenGL, compute shader support in Vulkan is mandatory. This means that you can use compute shaders on every Vulkan implementation available, no matter if it's a high-end desktop GPU or a low-powered embedded device.

This opens up the world of general purpose computing on graphics processor units (GPGPU), no matter where your application is running. GPGPU means that you can do general computations on your GPU, something that has traditionally been a domain of CPUs. But with GPUs having become more and more powerful and more flexible, many workloads that would require the general purpose capabilities of a CPU can now be done on the GPU in realtime.

A few examples of where the compute capabilities of a GPU can be used are image manipulation and physics, e.g. for a particle system. And it's even possible to use only compute for doing computational only work that does not require any graphics output, e.g. number crunching or AI related things. This is called "headless compute".

## Advantages

Doing computational expensive calculations on the GPU has several advantages. The most obvious one is offloading work from the CPU. Another one is not requiring a round-trip to the CPU and it's main memory, so all data can stay on the GPU without having to resolve to slow main memory reads.

Aside from these, GPUs are heavily parallelized with some of them having tens of thousands of small compute units. This often makes them a better fit for highly parallel workflows than a CPU with a few large units.

## The Vulkan pipeline

For the understanding of this chapter, it's important to know that compute is separated from the graphics part of the pipeline. This is visible in this block diagram of the Vulkan pipeline from the official specification:

![](/images/vulkan_pipeline_block_diagram.png)

In this diagram we can see the traditional graphics part of the pipeline on the left, and several stages on the right that are not part of this graphics pipeline, including the compute shader (stage). With the compute shader stage being detached from the graphics pipeline we'll be able to use it anywhere where we see fit. This is very different from e.g. the fragment shader which is always applied to the transformed output of the vertex shader.

## An example

An easy to understand example would be a particle system. Such systems are used in many games and often consist of thousands of particles that need to be updated at interactive frame rates. And sometimes even with complex physics applied, e.g. when testing fpr collisions. So rendering such a system requires vertices (passed as vertex buffers) and a way to update them based on some equation.

A "classical" CPU based particle system would store particles in the system's main memory and then use the CPU to update them. And after the update, the vertices need to be transferred to the GPU's memory again, so it'll display the updated particles in the next frame. The most straight-forward way would be recreating the vertex buffer with the new dta each frame. This is obviously very costly. Depending on your implementation, there are other options like mapping GPU memory so it can be written by the CPU (called "resizable BAR" on desktop systems, or unified memory on integrated GPUs) or just using a host local buffer (which would be the slowest method due to PCI-E bandwidth). But no matter what buffer update you'd choose, you always require a "round-trip" to the CPU to update the particles.

With a GPU based particle system, this round-trip is no longer required. Vertices are only uploaded to the GPU once and all updates are done by the GPU using compute shaders inside GPU memory. This is faster than the CPU based method not only, but mostly due to the much higher bandwidth between the GPU and it's memory. And doing this on a GPU with a dedicated compute queue, you can update particles in parallel to the rendering part of the graphics pipeline.

## Data manipulation

An important concept introduced with compute shaders is the possibility to manipulate data passed to it. In this tutorial you already learned about different buffer types like vertex and index buffer for passing primitives and uniform buffers for passing data to a shader (which can also be used to pass data to compute shaders). And you also used images do to texture mapping. But all of these have only been for reading data by the CPU.

But with compute shaders we also want to write data to buffers and images. And for that, Vulkan introduces two dedicated storage types.

### Shader storage buffer objects (SSBO)

A shader storage buffer (SSBO) allows you to read from and write to a buffer. Using these is similar to using uniform buffer objects. The biggest difference is that you can alias other buffer types to SSBOs and that they can be arbitrarily large.

Going back to the GPU based particle system you might now wonder how to deal with vertices being updated (written) by the compute shader and read (drawn) by the vertex shader, as both usages would seemingly require different buffer types.

But that's not the case. In Vulkan you can specify multiple usages for buffers and images. So for the particle vertex buffer to be used as a vertex buffer (in the graphics pass) and as a storage buffer (in the compute pass) you simply create the buffer with two usage flags:

```c++
VkBufferCreateInfo bufferInfo{};
...
bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
...

if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create vertex buffer!");
}
```

The GLSL shader declaration for a SSBO looks like this:

```glsl
struct Particle {
	vec4 position;
	vec4 velocity;
};

layout(std140, binding = 0) buffer ParticleSSBO {
   Particle particles[ ];
};
```

In this example we have a typed SSBO with each particle having a position and velocity value (see the `Particle` struct). The SSBO then contains an unbound number of particles as marked by the `[]`. Not having to specify the number of elements in an SSBO is one of the advantages over e.g. uniform buffers.

The two flags `VK_BUFFER_USAGE_VERTEX_BUFFER_BIT` and `VK_BUFFER_USAGE_STORAGE_BUFFER_BIT` set with `bufferInfo.usage` tell the implementation that we want to use this buffer for two different scenarios: as a vertex buffer in the vertex shader and as a store buffer.

Writing to such a storage buffer object in the compute shader is straight-forward and similar to how you'd write to the buffer on the C++ side:

```glsl
particles[index].position += deltaTime * particles[index].velocity;
```

### Storage images

A storage image allows you read from and write to an image. Typical use cases are applying image effects to textures or doing post processing (which in turn is very similar).

This is similar for images:

```c++
VkImageCreateInfo imageInfo {};
...
imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
...

if (vkCreateImage(device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
}
```

The two flags `VK_IMAGE_USAGE_SAMPLED_BIT` and `VK_IMAGE_USAGE_STORAGE_BIT` set with `bufferInfo.usage` tell the implementation that we want to use this image for two different scenarios: as an image sampled in the fragment shader and as a storage image in the computer shader;

The GLSL shader declaration for storage image looks similar to sampled images used e.g. in the fragment shader:

```glsl
layout (binding = 0, rgba8) uniform readonly image2D inputImage;
layout (binding = 1, rgba8) uniform writeonly image2D outputImage;
```

A few differences here are additional attributes like `rgba8` for the format of the image, the `readonly` and `writeonly` qualifiers, telling the implementation that we'll only read from the input and write to the output image. And last but not least we need to use the `image2D` type do declare a storage image.

Reading from and writing to storage images in the compute shader is then done using `imageLoad` and `imageStore`: 

```glsl
vec3 pixel = imageLoad(inputImage, ivec2(gl_GlobalInvocationID.xy)).rgb;
imageStore(outputImage, ivec2(gl_GlobalInvocationID.xy), pixel);
```

## The compute shader stage

In the graphics samples we have used different pipeline stages to load shaders and access descriptors. Compute shaders are accessed in a similar way by using the `VK_SHADER_STAGE_COMPUTE_BIT` pipeline. So loading a compute shader is just the same as loading a vertex shader, but with a different shader stage. We'll talk about this in 
detail in the next paragraphs.

## Loading compute shaders

Loading compute shaders in our application is the same as loading any other other shader. The only real difference is that we'll need to use the `VK_SHADER_STAGE_COMPUTE_BIT` mentioned above.

```c++
auto computeShaderCode = readFile("shaders/compute.spv");

VkShaderModule computeShaderModule = createShaderModule(computeShaderCode);

VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
computeShaderStageInfo.module = computeShaderModule;
computeShaderStageInfo.pName = "main";
...
```

## Descriptors

Descriptors that we want to access in a compute shader also need to have this stage flag set:

```c++
VkDescriptorSetLayoutBinding uboLayoutBinding{};
uboLayoutBinding.binding = 0;
uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
uboLayoutBinding.descriptorCount = 1;
uboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
...
```

Note that you can combine shader stages here, so if you want the descriptor to be accessible from the vertex and compute stage, e.g. for a uniform buffer with parameters shared across them, you simply set the bits for both stages:

```c++
uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
```

## Compute pipelines

As compute is not a part of the graphics pipeline, we can't use `vkCreateGraphicsPipelines` to attach the compute shader to it. Instead we need to create a dedicated compute pipeline `vkCreateComputePipelines` for running our compute commands. Since a compute pipeline does not touch any of the rasterization state, it has a lot less state than a graphics pipeline:

```c++
VkGraphicsPipelineCreateInfo pipelineInfo{};
pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
pipelineInfo.layout = computePipelineLayout;
pipelineInfo.stage = computeShaderStage;

if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline!");
}
```

The setup is a lot simpler, as we only require one shader stage and a pipeline layout. The pipeline layout works the same as with the graphics pipeline but may use storage types where required:

```c++
std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[0].dstSet = computeDescriptorSets[i];
descriptorWrites[0].dstBinding = 0;
descriptorWrites[0].dstArrayElement = 0;
descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
descriptorWrites[0].descriptorCount = 1;
descriptorWrites[0].pBufferInfo = &storageBufferInfo;

descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[1].dstSet = computeDescriptorSets[i];
descriptorWrites[1].dstBinding = 1;
descriptorWrites[1].dstArrayElement = 0;
descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
descriptorWrites[1].descriptorCount = 1;
descriptorWrites[1].pBufferInfo = &uniformBufferInfo;

vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
```

## Compute work groups

Before we get into how a compute shader works and how we submit it to the GPU, we need to talk about two important compute concepts: **work groups** and **invocations**. These two define how compute workloads are distributed across the GPU's compute units. 

## Compute shaders

Now that we learned about all the parts required to setup a compute shader pipeline, it's time to take a look at compute shaders. All of the things we learned about using GLSL shaders e.g. for vertex and fragment shaders also applies to compute shaders. The syntax is the same, and many concepts like passing data between the application and the shader is the same. But there are some important differences.

A very basic compute shader for updating a linear array of particles may look like this:

```glsl
#version 450

struct Particle {
	vec2 position;
	vec2 velocity;
};

layout(std140, binding = 0) buffer ParticleSSBO {
   Particle particles[ ];
};

layout (binding = 1) uniform ParameterUBO {
	float deltaTime;
} ubo;

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main() 
{
    uint index = gl_GlobalInvocationID.x;  
    vec2 vVel = particles[index].velocity.xy;
    vec2 vPos = particles[index].position.xy;
    particles[index].position.xy += vVel * ubo.deltaTime;
}
```

The top part references the shader storage buffer object (SSBO) we learned about earlier and also has a uniform buffer object that contains a delta time as a parameter passed to this shader. The `main` function is also pretty simplistic, but shows how to read and write to the SSBO containing the particles.

The interesting part here, that needs some explanation is this declaration:

```glsl
layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;
```
This defines the size of local invocations of this compute shader in at max. three dimensions. We'll get into the details of what this is and how this actually works later on. For now the most interesting takeaway here is that we work on a linear array of particles which means 1 dimension. So we only need to specify a number for `local_size_x`. In this case we could even omit the declarations for `local_size_y` and `local_size_z`.

## Dispatching work

## Submitting work

## Synchronization

Synchronization is an important part of Vulkan, even more so When doing compute in conjunction with graphics. For our sample, a GPU based particle system, not doing proper synchronization may result in the vertex starting to read (and draw) particles while the compute shader hasn't finished updating them, or the compute shader could start updating particles that are still in use by the vertex part of the pipeline.

So we must make sure that those cases don't happen by synchronizing the graphics and the compute load. There are different ways of doing so, which also depends on how you submit your compute workload. But as this tutorial is aimed at beginners, we'll go for the easiest way of syncing both: putting graphics and compute into the same command buffer.

## Shared compute shader memory

## @todo: compute queue family