## Introduction

In this bonus chapter we'll take a look at compute shaders. Up until now all previous chapters dealt with the traditional graphics part of the Vulkan pipeline. But unlike older APIs like OpenGL, compute shader support in Vulkan is mandatory. This means that you can use compute shaders on every Vulkan implementation available, no matter if it's a high-end desktop GPU or a low-powered embedded device.

This opens up the world of general purpose computing on graphics processor units (GPGPU), no matter where your application is running. GPGPU means that you can do general computations on your GPU, something that has traditionally been a domain of CPUs. But with GPUs having become more and more powerful and more flexible, many workloads that would require the general purpose capabilities of a CPU can now be done on the GPU in realtime.

A few examples of where the compute capabilities of a GPU can be used are image manipulation and physics, e.g. for a particle system.

## Advantages

Doing computational expensive calculations on the GPU has several advantages. The most obvious one is offloading work from the CPU, esp. when using a GPU that has a dedicated compute queue (more on this later). Another one is not requiring a round-trip to the CPU and it's main memory, so all data can stay on the GPU without having to resolve to slow main memory reads.

Aside from these, GPUs are heavily parallelized with some of them having tens of thousands of small compute units. This often makes them a better fit for highly parallel workflows than a CPU with a few large units.

## An example

An easy to understand example would be a particle system. Such systems are used in many games and often consist of thousands of particles that need to be updated at interactive frame rates. And sometimes even with complex physics applied, e.g. when testing fpr collisions. So rendering such a system requires vertices (passed as vertex buffers) and a way to update them based on some equation.

A "classical" CPU based particle system would store particles in the system's main memory and then use the CPU to update them. And after the update, the vertices need to be transferred to the GPU's memory again, so it'll display the updated particles in the next frame. The most straight-forward way would be recreating the vertex buffer with the new dta each frame. This is obviously very costly. Depending on your implementation, there are other options like mapping GPU memory so it can be written by the CPU (called "resizable BAR" on desktop systems, or unified memory on integrated GPUs) or just using a host local buffer (which would be the slowest method due to PCI-E bandwidth). But no matter what buffer update you'd choose, you always require a "round-trip" to the CPU to update the particles.

With a GPU based particle system, this round-trip is no longer required. Vertices are only uploaded to the GPU once and all updates are done by the GPU using compute shaders inside GPU memory. This is faster than the CPU based method not only, but mostly due to the much higher bandwidth between the GPU and it's memory. And doing this on a GPU with a dedicated compute queue, you can update particles in parallel to the rendering part of the graphics pipeline.

## Data manipulation

An important concept introduced with compute shaders is the possibility to manipulate data passed to it. In this tutorial you already learned about different buffer types like vertex and index buffer for passing primitives and uniform buffers for passing data to a shader (which can also be used to pass data to compute shaders). And you also used images do to texture mapping. But all of these have only been for reading data by the CPU.

But with compute shaders we also want to write data to buffers and images. And for that, Vulkan introduced two new storage types.

### Shader storage buffer objects (SSBO)

### Storage images

### Usage

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

The two flags `VK_BUFFER_USAGE_VERTEX_BUFFER_BIT` and `VK_BUFFER_USAGE_STORAGE_BUFFER_BIT` set with `bufferInfo.usage` tell the implementation that we want to use this buffer for two different scenarios: as a vertex buffer in the vertex shader and as a store buffer.

This is also the case for images:

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


## Synchronization

## Dispatching work

## Compute shaders

## Shared compute shader memory
