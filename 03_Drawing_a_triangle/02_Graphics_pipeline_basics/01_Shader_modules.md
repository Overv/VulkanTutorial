Unlike earlier APIs, shader code in Vulkan has to be specified in a bytecode
format as opposed to human-readable syntax like [GLSL](https://en.wikipedia.org/wiki/OpenGL_Shading_Language)
and [HLSL](https://en.wikipedia.org/wiki/High-Level_Shading_Language). This
bytecode format is called [SPIR-V](https://www.khronos.org/spir) and is designed
to be used with both Vulkan and OpenCL (both Khronos APIs). It is a format that
can be used to write graphics and compute shaders, but we will focus on shaders
used in Vulkan's graphics pipelines in this tutorial.

The advantage of using a bytecode format is that the compilers written by GPU
vendors to turn shader code into native code are significantly less complex. The
past has shown that with human-readable syntax like GLSL, some GPU vendors were
rather flexible with their interpretation of the standard. If you happen to
write non-trivial shaders with a GPU from one of these vendors, then you'd risk
other vendor's drivers rejecting your code due to syntax errors, or worse, your
shader running differently because of compiler bugs. With a straightforward
bytecode format like SPIR-V that will hopefully be avoided.

However, that does not mean that we need to write this bytecode by hand. Khronos
has released their own vendor-independent compiler that compiles GLSL to SPIR-V.
This compiler is designed to verify that your shader code is fully standards
compliant and produces one SPIR-V binary that you can ship with your program.
You can also include this compiler as a library to produce SPIR-V at runtime,
but we won't be doing that in this tutorial. The compiler is already included
with the LunarG SDK as `glslangValidator.exe`, so you don't need to download
anything extra.

GLSL is a shading language with a C-style syntax. Programs written in it have a
`main` function that is invoked for every object. Instead of using parameters
for input and a return value as output, GLSL uses global variables to handle
input and output. The language includes many features to aid in graphics
programming, like built-in vector and matrix primitives. Functions for
operations like cross products, matrix-vector products and reflections around a
vector are included. The vector type is called `vec` with a number indicating
the amount of elements. For example, a 3D position would be stored in a `vec3`.
It is possible to access single components through members like `.x`, but it's
also possible to create a new vector from multiple components at the same time.
For example, the expression `vec3(1.0, 2.0, 3.0).xy` would result in `vec2`. The
constructors of vectors can also take combinations of vector objects and scalar
values. For example, a `vec3` can be constructed with
`vec3(vec2(1.0, 2.0), 3.0)`.

As the previous chapter mentioned, we need to write a vertex shader and a
fragment shader to get a triangle on the screen. The next two sections will
cover the GLSL code of each of those and after that I'll show you how to produce
two SPIR-V binaries and load them into the program.

## Vertex shader

The vertex shader processes each incoming vertex. It takes its attributes, like
world position, color, normal and texture coordinates as input. The output is
the final position in clip coordinates and the attributes that need to be passed
on to the fragment shader, like color and texture coordinates. These values will
then be interpolated over the fragments by the rasterizer to produce a smooth
gradient.

Clip coordinates are [homogeneous coordinates](https://en.wikipedia.org/wiki/Homogeneous_coordinates)
that map the framebuffer to a [-1, 1] by [-1, 1] coordinate system that looks
like the following:

![](/images/clip_coordinates.svg)

You should already be familiar with these if you have dabbed in computer
graphics before. If you have used OpenGL before, then you'll notice that the
sign of the Y coordinates is now flipped. The Z coordinate now uses the same
range as it does in Direct3D, from 0 to 1.

For our first triangle we won't be applying any transformations, we'll just
specify the positions of the three vertices directly in clip coordinates to
create the following shape:

![](/images/triangle_coordinates.svg)

Normally these coordinates would be stored in a vertex buffer, but creating a
vertex buffer in Vulkan and filling it with data is not trivial. Therefore I've
decided to postpone that until after we've had the satisfaction of seeing a
triangle pop up on the screen. We're going to do something a little unorthodox
in the meanwhile: include the coordinates directly inside the vertex shader. The
code looks like this:

```glsl
#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
```

The `main` function is invoked for every vertex. The built-in `gl_VertexIndex`
variable contains the index of the current vertex. This is usually an index into
the vertex buffer, but in our case it will be an index into a hardcoded array
of vertex data. The position of each vertex is accessed from the constant array
in the shader and combined with dummy `z` and `w` components to produce a
position in clip coordinates. The built-in variable `gl_Position` functions as
the output. The `GL_ARB_separate_shader_objects` extension is required for
Vulkan shaders to work.

## Fragment shader

The triangle that is formed by the positions from the vertex shader fills an
area on the screen with fragments. The fragment shader is invoked on these
fragments to produce a color and depth for the framebuffer (or framebuffers). A
simple fragment shader that outputs the color red for the entire triangle looks
like this:

```glsl
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
```

The `main` function is called for every fragment just like the vertex shader
`main` function is called for every vertex. Colors in GLSL are 4-component
vectors with the R, G, B and alpha channels within the [0, 1] range. Unlike
`gl_Position` in the vertex shader, there is no built-in variable to output a
color for the current fragment. You have to specify your own output variable for
each framebuffer where the `layout(location = 0)` modifier specifies the index
of the framebuffer. The color red is written to this `outColor` variable that is
linked to the first (and only) framebuffer at index `0`.

## Per-vertex colors

Making the entire triangle red is not very interesting, wouldn't something like
the following look a lot nicer?

![](/images/triangle_coordinates_colors.png)

We have to make a couple of changes to both shaders to accomplish this. First
off, we need to specify a distinct color for each of the three vertices. The
vertex shader should now include an array with colors just like it does for
positions:

```glsl
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);
```

Now we just need to pass these per-vertex colors to the fragment shader so it
can output their interpolated values to the framebuffer. Add an output for color
to the vertex shader and write to it in the `main` function:

```glsl
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```

Next, we need to add a matching input in the fragment shader:

```glsl
layout(location = 0) in vec3 fragColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

The input variable does not necessarily have to use the same name, they will be
linked together using the indexes specified by the `location` directives. The
`main` function has been modified to output the color along with an alpha value.
As shown in the image above, the values for `fragColor` will be automatically
interpolated for the fragments between the three vertices, resulting in a smooth
gradient.

## Compiling the shaders

Create a directory called `shaders` in the root directory of your project and
store the vertex shader in a file called `shader.vert` and the fragment shader
in a file called `shader.frag` in that directory. GLSL shaders don't have an
official extension, but these two are commonly used to distinguish them.

The contents of `shader.vert` should be:

```glsl
#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```

And the contents of `shader.frag` should be:

```glsl
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

We're now going to compile these into SPIR-V bytecode using the
`glslangValidator` program.

**Windows**

Create a `compile.bat` file with the following contents:

```bash
C:/VulkanSDK/1.0.17.0/Bin32/glslangValidator.exe -V shader.vert
C:/VulkanSDK/1.0.17.0/Bin32/glslangValidator.exe -V shader.frag
pause
```

Replace the path to `glslangValidator.exe` with the path to where you installed
the Vulkan SDK. Double click the file to run it.

**Linux**

Create a `compile.sh` file with the following contents:

```bash
/home/user/VulkanSDK/x.x.x.x/x86_64/bin/glslangValidator -V shader.vert
/home/user/VulkanSDK/x.x.x.x/x86_64/bin/glslangValidator -V shader.frag
```

Replace the path to `glslangValidator` with the path to where you installed the
Vulkan SDK. Make the script executable with `chmod +x compile.sh` and run it.

**End of platform-specific instructions**

These two commands invoke the compiler with the `-V` flag, which tells it to
compile the GLSL source files to SPIR-V bytecode. When you run the compile
script, you'll see that two SPIR-V binaries are created: `vert.spv` and
`frag.spv`. The names are automatically derived from the type of shader, but you
can rename them to anything you like. You may get a warning about some missing
features when compiling your shaders, but you can safely ignore that.

If your shader contains a syntax error then the compiler will tell you the line
number and problem, as you would expect. Try leaving out a semicolon for example
and run the compile script again. Also try running the compiler without any
arguments to see what kinds of flags it supports. It can, for example, also
output the bytecode into a human-readable format so you can see exactly what
your shader is doing and any optimizations that have been applied at this stage.

## Loading a shader

Now that we have a way of producing SPIR-V shaders, it's time to load them into
our program to plug them into the graphics pipeline at some point. We'll first
write a simple helper function to load the binary data from the files.

```c++
#include <fstream>

...

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
}
```

The `readFile` function will read all of the bytes from the specified file and
return them in a byte array managed by `std::vector`. We start by opening the
file with two flags:

* `ate`: Start reading at the end of the file
* `binary`: Read the file as binary file (avoid text transformations)

The advantage of starting to read at the end of the file is that we can use the
read position to determine the size of the file and allocate a buffer:

```c++
size_t fileSize = (size_t) file.tellg();
std::vector<char> buffer(fileSize);
```

After that, we can seek back to the beginning of the file and read all of the
bytes at once:

```c++
file.seekg(0);
file.read(buffer.data(), fileSize);
```

And finally close the file and return the bytes:

```c++
file.close();

return buffer;
```

We'll now call this function from `createGraphicsPipeline` to load the bytecode
of the two shaders:

```c++
void createGraphicsPipeline() {
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");
}
```

Make sure that the shaders are loaded correctly by printing the size of the
buffers and checking if they match the actual file size in bytes.

## Creating shader modules

Before we can pass the code to the pipeline, we have to wrap it in a
`VkShaderModule` object. Let's create a helper function `createShaderModule` to
do that.

```c++
void createShaderModule(const std::vector<char>& code, VDeleter<VkShaderModule>& shaderModule) {

}
```

The function will take a buffer with the bytecode as parameter and create a
`VkShaderModule` from it. Instead of returning this handle directly, it's
written to the variable specified for the second parameter, which makes it
easier to wrap it in a deleter variable when calling `createShaderModule`.

Creating a shader module is simple, we only need to specify a pointer to the
buffer with the bytecode and the length of it. This information is specified in
a `VkShaderModuleCreateInfo` structure.

```c++
VkShaderModuleCreateInfo createInfo = {};
createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
createInfo.codeSize = code.size();
createInfo.pCode = (uint32_t*) code.data();
```

The `VkShaderModule` can then be created with a call to `vkCreateShaderModule`:

```c++
if (vkCreateShaderModule(device, &createInfo, nullptr, shaderModule.replace()) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
}
```

The parameters are the same as those in previous object creation functions: the
logical device, pointer to create info structure, optional pointer to custom
allocators and handle output variable. The buffer with the code can be freed
immediately after creating the shader module.

The shader module objects are only required during the pipeline creation
process, so instead of declaring them as class members, we'll make them local
variables in the `createGraphicsPipeline` function:

```c++
VDeleter<VkShaderModule> vertShaderModule{device, vkDestroyShaderModule};
VDeleter<VkShaderModule> fragShaderModule{device, vkDestroyShaderModule};
```

They will be automatically cleaned up when the graphics pipeline has been
created and `createGraphicsPipeline` returns. Now just call the helper function
we created and we're done:

```c++
createShaderModule(vertShaderCode, vertShaderModule);
createShaderModule(fragShaderCode, fragShaderModule);
```

## Shader stage creation

The `VkShaderModule` object is just a dumb wrapper around the bytecode buffer.
The shaders aren't linked to each other yet and they haven't even been given a
purpose yet. Assigning a shader module to either the vertex or fragment shader
stage in the pipeline happens through a `VkPipelineShaderStageCreateInfo`
structure, which is part of the actual pipeline creation process.

We'll start by filling in the structure for the vertex shader, again in the
`createGraphicsPipeline` function.

```c++
VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
```

The first step, besides the obligatory `sType` member, is telling Vulkan in
which pipeline stage the shader is going to be used. There is an enum value for
each of the programmable stages described in the previous chapter.

```c++
vertShaderStageInfo.module = vertShaderModule;
vertShaderStageInfo.pName = "main";
```

The next two members specify the shader module containing the code, and the
function to invoke. That means that it's possible to combine multiple fragment
shaders into a single shader module and use different entry points to
differentiate between their behaviors. In this case we'll stick to the standard
`main`, however.

There is one more (optional) member, `pSpecializationInfo`, which we won't be
using here, but is worth discussing. It allows you to specify values for shader
constants. You can use a single shader module where its behavior can be
configured at pipeline creation by specifying different values for the constants
used in it. This is more efficient than configuring the shader using variables
at render time, because the compiler can do optimizations like eliminating `if`
statements that depend on these values. If you don't have any constants like
that, then you can set the member to `nullptr`, which our struct initialization
does automatically.

Modifying the structure to suit the fragment shader is easy:

```c++
VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
fragShaderStageInfo.module = fragShaderModule;
fragShaderStageInfo.pName = "main";
```

Finish by defining an array that contains these two structs, which we'll later
use to reference them in the actual pipeline creation step.

```c++
VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
```

That's all there is to describing the programmable stages of the pipeline. In
the next chapter we'll look at the fixed-function stages.

[C++ code](/code/shader_modules.cpp) /
[Vertex shader](/code/shader_base.vert) /
[Fragment shader](/code/shader_base.frag)
