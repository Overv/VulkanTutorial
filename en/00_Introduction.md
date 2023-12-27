## About

This tutorial will teach you the basics of using the [Vulkan](https://www.khronos.org/vulkan/)
graphics and compute API. Vulkan is a new API by the [Khronos group](https://www.khronos.org/)
(known for OpenGL) that provides a much better abstraction of modern graphics
cards. This new interface allows you to better describe what your application
intends to do, which can lead to better performance and less surprising driver
behavior compared to existing APIs like [OpenGL](https://en.wikipedia.org/wiki/OpenGL)
and [Direct3D](https://en.wikipedia.org/wiki/Direct3D). The ideas behind Vulkan
are similar to those of [Direct3D 12](https://en.wikipedia.org/wiki/Direct3D#Direct3D_12)
and [Metal](https://en.wikipedia.org/wiki/Metal_(API)), but Vulkan has the
advantage of being fully cross-platform and allows you to develop for Windows,
Linux and Android at the same time.

However, the price you pay for these benefits is that you have to work with a
significantly more verbose API. Every detail related to the graphics API needs
to be set up from scratch by your application, including initial frame buffer
creation and memory management for objects like buffers and texture images. The
graphics driver will do a lot less hand holding, which means that you will have
to do more work in your application to ensure correct behavior.

The takeaway message here is that Vulkan is not for everyone. It is targeted at
programmers who are enthusiastic about high performance computer graphics, and
are willing to put some work in. If you are more interested in game development,
rather than computer graphics, then you may wish to stick to OpenGL or Direct3D,
which will not be deprecated in favor of Vulkan anytime soon. Another
alternative is to use an engine like [Unreal Engine](https://en.wikipedia.org/wiki/Unreal_Engine#Unreal_Engine_4)
or [Unity](https://en.wikipedia.org/wiki/Unity_(game_engine)), which will be
able to use Vulkan while exposing a much higher level API to you.

With that out of the way, let's cover some prerequisites for following this
tutorial:

* A graphics card and driver compatible with Vulkan ([NVIDIA](https://developer.nvidia.com/vulkan-driver), [AMD](http://www.amd.com/en-us/innovations/software-technologies/technologies-gaming/vulkan), [Intel](https://software.intel.com/en-us/blogs/2016/03/14/new-intel-vulkan-beta-1540204404-graphics-driver-for-windows-78110-1540), [Apple Silicon (Or the Apple M1)](https://www.phoronix.com/scan.php?page=news_item&px=Apple-Silicon-Vulkan-MoltenVK))
* Experience with C++ (familiarity with RAII, initializer lists)
* A compiler with decent support of C++17 features (Visual Studio 2017+, GCC 7+, Or Clang 5+)
* Some existing experience with 3D computer graphics

This tutorial will not assume knowledge of OpenGL or Direct3D concepts, but it
does require you to know the basics of 3D computer graphics. It will not explain
the math behind perspective projection, for example. See [this online book](https://paroj.github.io/gltut/)
for a great introduction of computer graphics concepts. Some other great computer graphics resources are:

* [Ray tracing in one weekend](https://github.com/RayTracing/raytracing.github.io)
* [Physically Based Rendering book](http://www.pbr-book.org/)
* Vulkan being used in a real engine in the open-source [Quake](https://github.com/Novum/vkQuake) and [DOOM 3](https://github.com/DustinHLand/vkDOOM3)

You can use C instead of C++ if you want, but you will have to use a different
linear algebra library and you will be on your own in terms of code structuring.
We will use C++ features like classes and RAII to organize logic and resource
lifetimes. There are also two alternative versions of this tutorial available for Rust developers: [Vulkano based](https://github.com/bwasty/vulkan-tutorial-rs), [Vulkanalia based](https://kylemayes.github.io/vulkanalia).

To make it easier to follow along for developers using other programming languages, and to get some experience with the base API we'll be using the original C API to work with Vulkan. If you are using C++, however, you may prefer using the newer [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp) bindings that abstract some of the dirty work and help prevent certain classes of errors.

## E-book

If you prefer to read this tutorial as an e-book, then you can download an EPUB
or PDF version here:

* [EPUB](https://vulkan-tutorial.com/resources/vulkan_tutorial_en.epub)
* [PDF](https://vulkan-tutorial.com/resources/vulkan_tutorial_en.pdf)

## Tutorial structure

We'll start with an overview of how Vulkan works and the work we'll have to do
to get the first triangle on the screen. The purpose of all the smaller steps
will make more sense after you've understood their basic role in the whole
picture. Next, we'll set up the development environment with the [Vulkan SDK](https://lunarg.com/vulkan-sdk/),
the [GLM library](http://glm.g-truc.net/) for linear algebra operations and
[GLFW](http://www.glfw.org/) for window creation. The tutorial will cover how
to set these up on Windows with Visual Studio, and on Ubuntu Linux with GCC.

After that we'll implement all of the basic components of a Vulkan program that
are necessary to render your first triangle. Each chapter will follow roughly
the following structure:

* Introduce a new concept and its purpose
* Use all of the relevant API calls to integrate it into your program
* Abstract parts of it into helper functions

Although each chapter is written as a follow-up on the previous one, it is also
possible to read the chapters as standalone articles introducing a certain
Vulkan feature. That means that the site is also useful as a reference. All of
the Vulkan functions and types are linked to the specification, so you can click
them to learn more. Vulkan is a very new API, so there may be some shortcomings
in the specification itself. You are encouraged to submit feedback to
[this Khronos repository](https://github.com/KhronosGroup/Vulkan-Docs).

As mentioned before, the Vulkan API has a rather verbose API with many
parameters to give you maximum control over the graphics hardware. This causes
basic operations like creating a texture to take a lot of steps that have to be
repeated every time. Therefore we'll be creating our own collection of helper
functions throughout the tutorial.

Every chapter will also conclude with a link to the full code listing up to that
point. You can refer to it if you have any doubts about the structure of the
code, or if you're dealing with a bug and want to compare. All of the code files
have been tested on graphics cards from multiple vendors to verify correctness.
Each chapter also has a comment section at the end where you can ask any
questions that are relevant to the specific subject matter. Please specify your
platform, driver version, source code, expected behavior and actual behavior to
help us help you.

This tutorial is intended to be a community effort. Vulkan is still a very new
API and best practices have not really been established yet. If you have any
type of feedback on the tutorial and site itself, then please don't hesitate to
submit an issue or pull request to the [GitHub repository](https://github.com/Overv/VulkanTutorial).
You can *watch* the repository to be notified of updates to the tutorial.

After you've gone through the ritual of drawing your very first Vulkan powered
triangle onscreen, we'll start expanding the program to include linear
transformations, textures and 3D models.

If you've played with graphics APIs before, then you'll know that there can be a
lot of steps until the first geometry shows up on the screen. There are many of
these initial steps in Vulkan, but you'll see that each of the individual steps
is easy to understand and does not feel redundant. It's also important to keep
in mind that once you have that boring looking triangle, drawing fully textured
3D models does not take that much extra work, and each step beyond that point is
much more rewarding.

If you encounter any problems while following the tutorial, then first check the
FAQ to see if your problem and its solution is already listed there. If you are
still stuck after that, then feel free to ask for help in the comment section of
the closest related chapter.

Ready to dive into the future of high performance graphics APIs? [Let's go!](!en/Overview)

## License

Copyright (C) 2015-2023, Alexander Overvoorde

The contents are licensed under [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/),
unless stated otherwise. By contributing, you agree to license
your contributions to the public under that same license.

The code listings in the `code` directory in the source repository are licensed 
under [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/).
By contributing to that directory, you agree to license your contributions to
the public under that same public domain-like license.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
