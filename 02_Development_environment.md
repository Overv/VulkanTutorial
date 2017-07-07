In this chapter we'll set up your environment for developing Vulkan applications
and install some useful libraries. All of the tools we'll use, with the
exception of the compiler, are compatible with both Windows and Linux, but the
steps for installing them differ a bit, which is why they're described
separately here.

## Windows

If you're developing for Windows, then I will assume that you are using Visual
Studio 2013 or 2015 to compile your code. The steps are the same for both
versions, but the Vulkan SDK currently only includes debug symbols that are
compatible with Visual Studio 2013. That isn't really a problem in practice, but
it's something that you may wish to take into account.

### Vulkan SDK

The most important component you'll need for developing Vulkan applications is
the SDK. It includes the headers, standard validation layers, debugging tools
and a loader for the Vulkan functions. The loader looks up the functions in the
driver at runtime, similarly to GLEW for OpenGL - if you're familiar with that.

The SDK can be downloaded from [the LunarG website](https://vulkan.lunarg.com/)
using the buttons at the bottom of the page. You don't have to create an
account, but it will give you access to some additional documentation that may
be useful to you.

![](/images/vulkan_sdk_download_buttons.png)

Proceed through the installation and pay attention to the install location of
the SDK. The first thing we'll do is verify that your graphics card and driver
properly support Vulkan. Go to the directory where you installed the SDK, open
the `Bin32` directory and run the `cube.exe` demo. You should see the following:

![](/images/cube_demo.png)

If you receive an error message then ensure that your drivers are up-to-date,
include the Vulkan runtime and that your graphics card is supported. See the
[introduction chapter](!Introduction) for links to drivers from the major
vendors.

There are two other programs in this directory that will be useful for
development. The `vkjson_info.exe` program generates a JSON file with a detailed
description of the capabilities of your hardware when using Vulkan. If you are
wondering what support is like for extensions and other optional features among
the graphics cards of your end users, then you can use [this website](http://vulkan.gpuinfo.org/)
to view the results of a wide range of GPUs.

The `glslangValidator.exe` program will be used to compile shaders from the
human-readable [GLSL](https://en.wikipedia.org/wiki/OpenGL_Shading_Language) to
bytecode. We'll cover this in depth in the [shader modules](!Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules)
chapter. The `Bin32` directory also contains the binaries of the Vulkan loader
and the validation layers, while the `Lib32` directory contains the libraries.

The `Doc` directory contains useful information about the Vulkan SDK and an
offline version of the entire Vulkan specification. Lastly, there's the
`Include` directory that contains the Vulkan headers. Feel free to explore the
other files, but we won't need them for this tutorial.

### GLFW

As mentioned before, Vulkan by itself is a platform agnostic API and does not
include tools for creating a window to display the rendered results. To benefit
from the cross-platform advantages of Vulkan and to avoid the horrors of Win32,
we'll use the [GLFW library](http://www.glfw.org/) to create a window, which
supports both Windows and Linux. There are other libraries available for this
purpose, like [SDL](https://www.libsdl.org/), but the advantage of GLFW is that
it also abstracts away some of the other platform-specific things in Vulkan
besides just window creation.

You can find the latest release of GLFW on the [official website](http://www.glfw.org/download.html).
In this tutorial we'll be using the 32-bit binaries, but you can of course also
choose to build in 64 bit mode. In that case make sure to link with the Vulkan
SDK binaries in the `Bin` directory. After downloading it, extract the archive
to a convenient location. I've chosen to create a `Libraries` directory in the
Visual Studio directory under documents.

![](/images/glfw_directory.png)

### GLM

Unlike DirectX 12, Vulkan does not include a library for linear algebra
operations, so we'll have to download one. [GLM](http://glm.g-truc.net/) is a
nice library that is designed for use with graphics APIs and is also commonly
used with OpenGL.

GLM is a header-only library, so just download the [latest version](https://github.com/g-truc/glm/releases)
and store it in a convenient location. You should have a directory structure
similar to the following now:

![](/images/library_directory.png)

### Setting up Visual Studio

Now that you've installed all of the dependencies we can set up a basic Visual
Studio project for Vulkan and write a little bit of code to make sure that
everything works.

Start Visual Studio and create a new C++ Win32 project.

![](/images/vs_new_cpp_project.png)

Click `Next`, select `Console application` as application type and make sure
that `Empty project` is checked.

![](/images/vs_application_settings.png)

Press `Finish` to create the project and add a C++ source file. You should
already know how to do that, but the steps are included here for completeness.

![](/images/vs_new_item.png)

![](/images/vs_new_source_file.png)

Now add the following code to the file. Don't worry about trying to
understand it right now; we're just making sure that you can compile and run
Vulkan applications. We'll start from scratch in the next chapter.

```c++
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported" << std::endl;

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
```

Let's now configure the project to get rid of the errors. Open the project
properties dialog and ensure that `All Configurations` is selected, because most
of the settings apply to both `Debug` and `Release` mode.

![](/images/vs_open_project_properties.png)

![](/images/vs_all_configs.png)

Go to `C++ -> General -> Additional Include Directories` and press `<Edit...>`
in the dropdown box.

![](/images/vs_cpp_general.png)

Add the header directories for Vulkan, GLFW and GLM:

![](/images/vs_include_dirs.png)

Next, open the editor for library directories:

![](/images/vs_link_settings.png)

And add the locations of the object files for Vulkan and GLFW:

![](/images/vs_link_dirs.png)

Go to `Linker -> Input` and press `<Edit...>` in the `Additional Dependencies`
dropdown box.

![](/images/vs_link_input.png)

Enter the names of the Vulkan and GLFW object files:

![](/images/vs_dependencies.png)

You can now close the project properties dialog. If you did everything right
then you should no longer see any more errors being highlighted in the code.

Press `F5` to compile and run the project and you should see a command prompt
and a window pop up like this:

![](/images/vs_test_window.png)

The number of extensions should be non-zero. Congratulations, you're all set for
playing with Vulkan!

To avoid having to repeat this work all over again every time, you can create a
template from it. Select `File -> Export Template...` in Visual Studio 2015 or
`Project -> Export Template...` in Visual Studio 2017. Then select
`Project template` and fill in a nice name and description for the template.

![](/images/vs_export_template.png)

Press `Finish` and you should now have a handy template in the `New Project`
dialog!  Use it to create a `Hello Triangle` project as preparation for the next
chapter.

![](/images/vs_template.png)

You are now all set for [the real adventure](!Drawing_a_triangle/Setup/Base_code).

## Linux

These instructions will be aimed at Ubuntu users, but you may be able to follow
along by compiling the LunarG SDK yourself and changing the `apt` commands to
the package manager commands that are appropriate for you. You should already
have a version of GCC installed that supports modern C++ (4.8 or later). You
also need both CMake and make.

### Vulkan SDK

The most important component you'll need for developing Vulkan applications is
the SDK. It includes the headers, standard validation layers, debugging tools
and a loader for the Vulkan functions. The loader looks up the functions in the
driver at runtime, similarly to GLEW for OpenGL - if you're familiar with that.

The SDK can be downloaded from [the LunarG website](https://vulkan.lunarg.com/)
using the buttons at the bottom of the page. You don't have to create an
account, but it will give you access to some additional documentation that may
be useful to you.

![](/images/vulkan_sdk_download_buttons.png)

Open a terminal in the directory where you've downloaded the `.run` script, make
it executable and run it:

```bash
chmod +x vulkansdk-linux-x86_64-xxx.run
./vulkansdk-linux-x86_64-xxx.run
```

It will extract all of the files in the SDK to a `VulkanSDK` subdirectory in the
working directory. Move the `VulkanSDK` directory to a convenient place and take
note of its path. Open a terminal in the root directory of the SDK, which will
contain files like `build_examples.sh`.

The samples in the SDK and one of the libraries that you will later use for your
program depend on the XCB library. This is a C library that is used to interface
with the X Window System. It can be installed in Ubuntu from the `libxcb1-dev`
package. You also need the generic X development files that come with the
`xorg-dev` package.

```bash
sudo apt install libxcb1-dev xorg-dev
```

You can now build the Vulkan examples in the SDK by running:

```bash
./build_examples.sh
```

If compilation was successful, then you should now have a
`./examples/build/cube` executable. Run it from the `examples/build` directory
with `./cube` and ensure that you see the following pop up in a window:

![](/images/cube_demo_nowindow.png)

If you receive an error message then ensure that your drivers are up-to-date,
include the Vulkan runtime and that your graphics card is supported. See the
[introduction chapter](!Introduction) for links to drivers from the major
vendors.

### GLFW

As mentioned before, Vulkan by itself is a platform agnostic API and does not
include tools for creation a window to display the rendered results. To benefit
from the cross-platform advantages of Vulkan and to avoid the horrors of X11,
we'll use the [GLFW library](http://www.glfw.org/) to create a window, which
supports both Windows and Linux. There are other libraries available for this
purpose, like [SDL](https://www.libsdl.org/), but the advantage of GLFW is that
it also abstracts away some of the other platform-specific things in Vulkan
besides just window creation.

We'll be installing GLFW from source instead of using a package, because the
Vulkan support requires a recent version. You can find the sources on the [official website](http://www.glfw.org/).
Extract the source code to a convenient directory and open a terminal in the
directory with files like `CMakeLists.txt`.

Run the following commands to generate a makefile and compile GLFW:

```bash
cmake .
make
```

You may see a warning stating `Could NOT find Vulkan`, but you can safely ignore
this message. If compilation was successful, then you can install GLFW into the
system libraries by running:

```bash
sudo make install
```

### GLM

Unlike DirectX 12, Vulkan does not include a library for linear algebra
operations, so we'll have to download one. [GLM](http://glm.g-truc.net/) is a
nice library that is designed for use with graphics APIs and is also commonly
used with OpenGL.

It is a header-only library that can be installed from the `libglm-dev` package:

```bash
sudo apt install libglm-dev
```

### Setting up a makefile project

Now that you have installed all of the dependencies, we can set up a basic
makefile project for Vulkan and write a little bit of code to make sure that
everything works.

Create a new directory at a convenient location with a name like `VulkanTest`.
Create a source file called `main.cpp` and insert the following code. Don't
worry about trying to understand it right now; we're just making sure that you
can compile and run Vulkan applications. We'll start from scratch in the next
chapter.

```c++
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported" << std::endl;

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
```

Next, we'll write a makefile to compile and run this basic Vulkan code. Create a
new empty file called `Makefile`. I will assume that you already have some basic
experience with makefiles, like how variables and rules work. If not, you can
get up to speed very quickly with [this tutorial](http://mrbook.org/blog/tutorials/make/).

We'll first define a couple of variables to simplify the remainder of the file.
Define a `VULKAN_SDK_PATH` variable that refers to the location of the `x86_64`
directory in the LunarG SDK, for example:

```make
VULKAN_SDK_PATH = /home/user/VulkanSDK/x.x.x.x/x86_64
```

Next, define a `CFLAGS` variable that will specify the basic compiler flags:

```make
CFLAGS = -std=c++11 -I$(VULKAN_SDK_PATH)/include
```

We're going to use modern C++ (`-std=c++11` or `std=c++14`), and we need to be
able to locate `vulkan.h` in the LunarG SDK.

Similarly, define the linker flags in a `LDFLAGS` variable:

```make
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan
```

The first flag specifies that we want to be able to find libraries like
`libvulkan.so` in the LunarG SDK's `x86_64/lib` directory. The second component
invokes `pkg-config` to automatically retrieve all of the linker flags necessary
to build an application with GLFW. Finally, `-lvulkan` links with the Vulkan
function loader that comes with the LunarG SDK.

Specifying the rule to compile `VulkanTest` is straightforward now. Make sure to
use tabs for indentation instead of spaces.

```make
VulkanTest: main.cpp
    g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS)
```

Verify that this rule works by saving the makefile and running `make` in the
directory with `main.cpp` and `Makefile`. This should result in a `VulkanTest`
executable.

We'll now define two more rules, `test` and `clean`, where the former will
run the executable and the latter will remove a built executable:

```make
.PHONY: test clean

test: VulkanTest
    ./VulkanTest

clean:
    rm -f VulkanTest
```

You will find that `make clean` works perfectly fine, but `make test` will most
likely fail with the following error message:

```text
./VulkanTest: error while loading shared libraries: libvulkan.so.1: cannot open shared object file: No such file or directory
```

That's because `libvulkan.so` is not installed as system library. To alleviate
this problem, explicitly specify the library loading path using the
`LD_LIBRARY_PATH` environment variable:

```make
test: VulkanTest
    LD_LIBRARY_PATH=$(VULKAN_SDK_PATH)/lib ./VulkanTest
```

The program should now run successfully, and display the number of Vulkan
extensions. The application should exit with the success return code (`0`) when
you close the empty window. However, there is one more variable that you need to
set. We will start using validation layers in Vulkan and you need to tell the
Vulkan library where to load these from using the `VK_LAYER_PATH` variable:

```make
test: VulkanTest
    LD_LIBRARY_PATH=$(VULKAN_SDK_PATH)/lib VK_LAYER_PATH=$(VULKAN_SDK_PATH)/etc/explicit_layer.d ./VulkanTest
```

You should now have a complete makefile that resembles the following:

```make
VULKAN_SDK_PATH = /home/user/VulkanSDK/x.x.x.x/x86_64

CFLAGS = -std=c++11 -I$(VULKAN_SDK_PATH)/include
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan

VulkanTest: main.cpp
    g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
    LD_LIBRARY_PATH=$(VULKAN_SDK_PATH)/lib VK_LAYER_PATH=$(VULKAN_SDK_PATH)/etc/explicit_layer.d ./VulkanTest

clean:
    rm -f VulkanTest
```

You can now use this directory as a template for your Vulkan projects. Make a
copy, rename it to something like `HelloTriangle` and remove all of the code
in `main.cpp`.

Before we move on, let's explore the Vulkan SDK a bit more. There are two
programs in it that will be very useful for development. The
`x86_64/bin/vkjson_info` program generates a JSON file with a detailed
description of the capabilities of your hardware when using Vulkan. If you are
wondering what support is like for extensions and other optional features among
the graphics cards of your end users, then you can use [this website](http://vulkan.gpuinfo.org/)
to view the results of a wide range of GPUs. This program needs to be run with
the same `LD_LIBRARY_PATH` variable as your own programs:

```bash
LD_LIBRARY_PATH=../lib ./vkjson_info
```

The `x86_64/bin/glslangValidator` program will be used to compile shaders from
the human-readable [GLSL](https://en.wikipedia.org/wiki/OpenGL_Shading_Language)
to bytecode. We'll cover this in depth in the [shader modules](!Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules)
chapter. It does not depend on the Vulkan library.

The `Doc` directory contains useful information about the Vulkan SDK and an
offline version of the entire Vulkan specification. Feel free to explore the
other files, but we won't need them for this tutorial.

You are now all set for [the real adventure](!Drawing_a_triangle/Setup/Base_code).
