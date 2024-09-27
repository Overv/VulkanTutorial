# [Descriptor layout and buffer](https://vulkan-tutorial.com/Uniform_buffers/Descriptor_layout_and_buffer)

## VkDescriptorSetLayoutBinding

`VkDescriptorSetLayoutBinding` is a structure in Vulkan that specifies the layout of a single descriptor within a descriptor set layout. It describes the type of resource (e.g., uniform buffer, image sampler) that shaders will access, its binding index, and the shader stages that will use it. This structure is used to define how descriptors are organized within a descriptor set, which is then used to create a `VkDescriptorSetLayout`.

### Key Components of VkDescriptorSetLayoutBinding

1. **binding**:
    - Specifies the binding index of the descriptor within the descriptor set. This index is used to refer to the descriptor in the shader code using the `layout(binding = X)` qualifier.

2. **descriptorType**:
    - Specifies the type of descriptor, such as:
        - `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`: For uniform buffers.
        - `VK_DESCRIPTOR_TYPE_STORAGE_BUFFER`: For storage buffers.
        - `VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE`: For images to be sampled by shaders.
        - `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`: For images with an associated sampler.
        - `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC`: For dynamically indexed uniform buffers.

3. **descriptorCount**:
    - Specifies the number of descriptors in the binding. This is typically 1, but can be greater if using an array of descriptors in the shader.

4. **stageFlags**:
    - Specifies which shader stages will use this descriptor. It is a bitmask of `VkShaderStageFlagBits`, such as:
        - `VK_SHADER_STAGE_VERTEX_BIT`: For the vertex shader stage.
        - `VK_SHADER_STAGE_FRAGMENT_BIT`: For the fragment shader stage.
        - `VK_SHADER_STAGE_COMPUTE_BIT`: For the compute shader stage.
        - `VK_SHADER_STAGE_ALL_GRAPHICS`: For all graphics shader stages.

5. **pImmutableSamplers**:
    - A pointer to an array of `VkSampler` handles. This is used when the descriptor type is `VK_DESCRIPTOR_TYPE_SAMPLER` or `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER` to specify immutable samplers, meaning the sampler cannot be changed during the program execution. If `pImmutableSamplers` is `nullptr`, it means the samplers are not immutable.

### Example Usage of VkDescriptorSetLayoutBinding

Here’s an example demonstrating how to set up a `VkDescriptorSetLayoutBinding` for a uniform buffer and a combined image sampler:

```cpp
// Define a uniform buffer binding
VkDescriptorSetLayoutBinding uboLayoutBinding{};
uboLayoutBinding.binding = 0; // Binding index 0 in the shader
uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Type of descriptor
uboLayoutBinding.descriptorCount = 1; // Only one descriptor in this binding
uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Used in the vertex shader
uboLayoutBinding.pImmutableSamplers = nullptr; // Not applicable for uniform buffer

// Define a combined image sampler binding
VkDescriptorSetLayoutBinding samplerLayoutBinding{};
samplerLayoutBinding.binding = 1; // Binding index 1 in the shader
samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // Type of descriptor
samplerLayoutBinding.descriptorCount = 1; // Only one descriptor in this binding
samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Used in the fragment shader
samplerLayoutBinding.pImmutableSamplers = nullptr; // No immutable sampler

// Create an array of bindings for the descriptor set layout
std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

// Create a descriptor set layout
VkDescriptorSetLayoutCreateInfo layoutInfo{};
layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size()); // Number of bindings
layoutInfo.pBindings = bindings.data(); // Pointer to the bindings array

VkDescriptorSetLayout descriptorSetLayout;
if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
}
```

### Explanation

- **Uniform Buffer Binding (`uboLayoutBinding`)**:
    - `binding = 0`: This corresponds to `layout(binding = 0)` in the vertex shader.
    - `descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`: This specifies that this binding will use a uniform buffer.
    - `stageFlags = VK_SHADER_STAGE_VERTEX_BIT`: This descriptor is available to the vertex shader stage.

- **Combined Image Sampler Binding (`samplerLayoutBinding`)**:
    - `binding = 1`: This corresponds to `layout(binding = 1)` in the fragment shader.
    - `descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`: This specifies that this binding will use a combined image sampler.
    - `stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT`: This descriptor is available to the fragment shader stage.

### Summary

- **VkDescriptorSetLayoutBinding** defines the layout of a single descriptor within a descriptor set, including its type, binding index, and shader stages.
- **Components**: `binding`, `descriptorType`, `descriptorCount`, `stageFlags`, and `pImmutableSamplers`.
- **Usage**: Defines how shaders access resources like buffers and textures. Multiple bindings can be combined to create a `VkDescriptorSetLayout`.

By correctly setting up `VkDescriptorSetLayoutBinding`, you enable Vulkan to efficiently manage resources and ensure that shaders have access to the necessary data for rendering and computation.