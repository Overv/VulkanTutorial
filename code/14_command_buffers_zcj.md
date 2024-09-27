
* EPUB ([English](https://vulkan-tutorial.com/resources/vulkan_tutorial_en.epub), [French](https://vulkan-tutorial.com/resources/vulkan_tutorial_fr.epub))
* PDF ([English](https://vulkan-tutorial.com/resources/vulkan_tutorial_en.pdf), [French](https://vulkan-tutorial.com/resources/vulkan_tutorial_fr.pdf))

* HTML([Command buffers](https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers))
* Commands in Vulkan, like drawing operations and memory transfers, are not executed directly using function calls. You have to record all of the operations you want to perform in command buffer objects. The advantage of this is that when we are ready to tell the Vulkan what we want to do, all of the commands are submitted together and Vulkan can more efficiently process the commands since all of them are available together. In addition, this allows command recording to happen in multiple threads if so desired.
* We have to create a command pool before we can create command buffers.
* We have to create a command pool before we can create command buffers.
  Command pools manage the memory that is used to store the buffers.
  Command buffers are allocated from command pools.
  Add a new class member to store a VkCommandPool:

We will be recording a command buffer every frame, so we will need to create a command pool that can allocate the command buffers. Add a new class member to store a VkCommandPool:

Command buffers are executed by submitting them on one of the device queues, just like the other Vulkan operations we have performed so far. We will need to create a command buffer for each frame, so we will need to create a command pool that can allocate the command buffers. Add a new class member to store a VkCommandPool:

In Vulkan, **command buffers** and **pipelines** are two distinct but interconnected concepts that are fundamental to rendering and compute operations. Here’s a detailed comparison of the two:

## command buffer vs. pipeline
### Command Buffer

1. **Definition**:
    - A command buffer is an object that records a sequence of commands that the GPU will execute. These commands can include drawing commands, state changes, and resource bindings.

2. **Purpose**:
    - Command buffers encapsulate the work that needs to be done by the GPU, allowing you to prepare commands ahead of time for efficient execution.

3. **Recording Commands**:
    - Commands are recorded into a command buffer using functions such as `vkBeginCommandBuffer` and `vkEndCommandBuffer`. You can record various commands like `vkCmdDraw`, state changes, and resource bindings.

4. **Execution**:
    - Command buffers are submitted to a command queue for execution using `vkQueueSubmit`. The GPU processes the commands in the order they were recorded.

5. **Multiple Command Buffers**:
    - You can create multiple command buffers, allowing for parallel recording and execution of commands.

6. **Resetting**:
    - Command buffers can be reset for reuse, enabling you to record new commands without allocating new buffers.

### Pipeline

1. **Definition**:
    - A pipeline is a configuration object that defines the entire state of the graphics or compute processing. It encapsulates fixed-function state and shader stages.

2. **Purpose**:
    - Pipelines dictate how rendering or compute operations are performed. They include details about shader programs, input formats, rasterization, blending, and more.

3. **Pipeline Types**:
    - There are different types of pipelines:
        - **Graphics Pipeline**: Used for rendering operations.
        - **Compute Pipeline**: Used for compute operations.
    - Each type of pipeline has a specific configuration tailored to its purpose.

4. **Creation**:
    - Pipelines are created using `vkCreateGraphicsPipelines` or `vkCreateComputePipelines`, specifying various pipeline states and shader stages.

5. **Binding**:
    - Before drawing commands are issued, a pipeline must be bound to the command buffer using `vkCmdBindPipeline`.

6. **Efficiency**:
    - Once created, pipelines can be reused, but changing the pipeline configuration can incur overhead. Therefore, it's beneficial to minimize pipeline changes during rendering.

### Summary of Differences

| Feature                  | Command Buffer                             | Pipeline                                     |
|--------------------------|-------------------------------------------|---------------------------------------------|
| **Definition**           | Records commands for the GPU to execute. | Defines the state and behavior of rendering or compute operations. |
| **Purpose**              | Encapsulates and organizes GPU commands. | Configures how rendering or computation is performed. |
| **Recording**            | Commands are recorded into a command buffer. | Configured with fixed-function states and shader stages. |
| **Execution**            | Submitted to a command queue for execution. | Must be bound to a command buffer before drawing commands. |
| **Lifecycle**            | Created, recorded into, submitted, and can be reset. | Created once, reused, and can be changed if needed. |

### Conclusion

In summary, command buffers are responsible for recording and submitting the actual commands that the GPU will execute, while pipelines define how those commands are processed. Together, they enable efficient graphics and compute operations in Vulkan, allowing developers to build complex rendering and computation workflows.

## Before drawing commands are issued, a pipeline must be bound to the command buffer using vkCmdBindPipeline.
That's correct! In Vulkan, before issuing any drawing commands (or dispatching compute commands), you must bind a pipeline to the command buffer using the `vkCmdBindPipeline` function. This step is crucial because it specifies the state and configuration that the GPU will use for the subsequent rendering or compute operations.

### Details on Binding a Pipeline

1. **Function Signature**:
   ```cpp
   void vkCmdBindPipeline(
       VkCommandBuffer commandBuffer,
       VkPipelineBindPoint pipelineBindPoint,
       VkPipeline pipeline);
   ```

2. **Parameters**:
    - **`commandBuffer`**: The command buffer into which you are binding the pipeline.
    - **`pipelineBindPoint`**: Specifies the type of pipeline being bound. This can be:
        - `VK_PIPELINE_BIND_POINT_GRAPHICS`: For graphics pipelines.
        - `VK_PIPELINE_BIND_POINT_COMPUTE`: For compute pipelines.
    - **`pipeline`**: The pipeline object to bind. This pipeline must have been created previously using `vkCreateGraphicsPipelines` or `vkCreateComputePipelines`.

3. **Purpose**:
    - Binding a pipeline sets up the GPU with the necessary state for processing vertices and fragments or executing compute shaders.
    - It ensures that all the configurations defined in the pipeline (like shaders, rasterization state, blending state, etc.) are applied to the commands that follow.

4. **Example Usage**:
   Here's an example of how to bind a graphics pipeline before issuing draw commands:
   ```cpp
   // Assume commandBuffer has been recorded and pipeline has been created
   vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

   // Now you can issue drawing commands
   vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
   ```

5. **State Changes**:
    - Changing the bound pipeline incurs overhead, so it's often beneficial to minimize the number of pipeline binds during rendering.
    - If you need to draw with different configurations (like different shaders or blend states), you may want to group draw calls by pipeline to reduce state changes.

### Summary
Binding a pipeline with `vkCmdBindPipeline` is a necessary step before executing drawing or compute commands in Vulkan. It establishes the context for how the GPU will process the commands that follow, enabling the application to leverage the full capabilities of Vulkan's graphics and compute pipeline architecture.