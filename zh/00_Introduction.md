---
title: 背景介绍
---

## 关于本教程

本教程将帮助读者学习使用 [Vulkan](https://www.khronos.org/vulkan/) 的基本知识。Vulkan 是一种新近的用于图形和计算的应用程序接口 (API)，
由 [Khronos 组织](https://www.khronos.org/)（以OpenGL闻名）推出，其为现代显卡提供了很好的抽象。相比于其它已有的 API 如
[OpenGL](https://zh.wikipedia.org/wiki/OpenGL) 和 [Direct3D](https://zh.wikipedia.org/wiki/Direct3D)，Vulkan 能让你更明确地
表述程序要做什么，从而带来性能提升，减少因为驱动程序导致的意外行为。Vulkan 背后的理念和
[Direct3D 12](https://zh.wikipedia.org/wiki/Direct3D#Direct3D_12) 以及 [Metal](https://zh.wikipedia.org/wiki/Metal_(API))
相似，但 Vulkan 具有全面跨平台的优点，使你能够同时为 Windows，Linux 和 Android 进行开发。

然而，为了享受这些好处所支付的代价是，开发者不得不面对巨量繁复的 API，亲自从头处理与图形编程有关的每个细节，包括初始化帧缓冲，管理缓冲区和纹理
对象内存的内存分配。图形驱动需要插手之处大大减少，这也意味着开发者不得不在程序代码中做更多工作来保证程序行为正确。

在这里要说的是，Vulkan 并不适合所有人。它的目标是热衷于高性能计算机图形学，并且愿意在这方面投入精力的程序员。如果相比于计算机图形学，你对游戏开
发更感兴趣，那么您可能希望继续使用 OpenGL 或 Direct3D，它们在短期内不会被 Vulkan 取代。另一种选择是使用图形引擎，例如
[虚幻引擎](https://en.wikipedia.org/wiki/Unreal_Engine#Unreal_Engine_4)
或 [Unity](https://en.wikipedia.org/wiki/Unity_(game_engine))，它们在可以底层使用 Vulkan 而对开发者提供更高层次的开发接口。

了解完以上内容，让我们看看学习本教程需要哪些准备：

* 兼容 Vulkan 的显卡和驱动 ([NVIDIA](https://developer.nvidia.com/vulkan-driver)，
  [AMD](http://www.amd.com/en-us/innovations/software-technologies/technologies-gaming/vulkan)，
  [Intel](https://software.intel.com/en-us/blogs/2016/03/14/new-intel-vulkan-beta-1540204404-graphics-driver-for-windows-78110-1540)，
  [Apple Silicon（也叫 Apple M1）](https://www.phoronix.com/scan.php?page=news_item&px=Apple-Silicon-Vulkan-MoltenVK))
* C++ 经验（熟悉 RAII 和初始化列表）
* 支持 C++17 的编译器（Visual Studio 2017+，GCC 7+ 或 Clang 5+）
* 3D 计算机图形学经验

本教程不会假定读者了解 OpenGL 或 Direct3D 的相关概念，但仍要求具备 3D 计算机图形学的基础知识。例如，本教程不会解释透视投影背后的数学原理。
[这本在线书籍](https://paroj.github.io/gltut/)很好地介绍了计算机图形学的概念。其他一些优秀的计算机图形学资源包括：

* [用一个周末实现光线追踪（Ray tracing in one weekend）](https://github.com/RayTracing/raytracing.github.io)
* [基于物理的渲染（Physically Based Rendering book）](http://www.pbr-book.org/)
* Vulkan 被应用于开源图形引擎 [Quake](https://github.com/Novum/vkQuake) 和 
  [DOOM 3](https://github.com/DustinHLand/vkDOOM3) 中

如果你想，你可以使用 C 语言而不是 C++，但这样你就需要使用不同的线性代数库，而且要自己组织代码结构。本教程将使用类和 RAII 等 C++ 特性来组织
逻辑，管理资源生命周期。这里还有为 Rust 开发者提供的本教程的[替代版本](https://github.com/bwasty/vulkan-tutorial-rs)。

为了让使用其他编程语言的开发者更容易理解，并获得一些使用基本 API 的经验，我们将使用原始的 C API 编写 Vulkan。但是，如果你使用C++，那你可能更
喜欢使用更新的 [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp) 绑定，它封装了一些费力的工作，还能防止几类错误。

## 电子书

如果你想阅读本教程的电子书，可以在这里下载 EPUB 或 PDF 版本：

<!-- TODO: Chinese version -->
* [EPUB](https://vulkan-tutorial.com/resources/vulkan_tutorial_en.epub)
* [PDF](https://vulkan-tutorial.com/resources/vulkan_tutorial_en.pdf)

## 教程结构

我们首先会概述 Vulkan 如何工作，要怎么在屏幕上绘制我们的第一个三角形。在你了解每一小步在整个流程中的作用后，你对它们的理解会更深刻。接下来我们
搭建开发环境，包括 [Vulkan SDK](https://lunarg.com/vulkan-sdk/)，用于线性代数操作的 [GLM](http://glm.g-truc.net/) 和用于创建窗口
的 [GLFW](http://www.glfw.org/)。本教程将说明如何设置上述依赖，在 Windows 上使用 Visual Studio，而在 Ubuntu 上使用 GCC。

然后我们将实现要渲染你的第一个三角形所必需的 Vulkan 程序的所有基本组件。每章将大致遵循以下结构：

* 介绍一个新概念以及使用它的目的
* 使用与之相关的 API 调用将其集成到程序中
* 将其部分地抽象为辅助函数

虽然每章都承接上一章的内容，但仍可以将每章作为介绍相应 Vulkan 特性的独立文章阅读。这意味着本教程也可以作为参考手册使用。所有的 Vulkan 函数和
类型都超链接到了 Vulkan 规范中相应位置，你可以点击链接来了解更多。Vulkan 是非常新近的 API，所以其规范中也可能有些许不足。鼓励大家去
[科纳斯组织的代码仓库](https://github.com/KhronosGroup/Vulkan-Docs) 进行反馈。

如前所述，为了让开发者能最大限度地控制图形硬件， Vulkan 的接口相当冗长，带有许多参数。这导致像创建纹理这样的基本操作每次都需要重复大量步骤。
因此本教程中我们将自己创建一系列辅助函数。

每章末尾将附上截止该章用到的完整代码的链接。如果对代码结构有疑问，或者想要和自己的代码比较来解决其中的错误，可以参考本教程所附代码。所有代码文件
都已在来自多个厂商的显卡上验证过正确性。还有每章后的评论区，可以在此询问与相应章节主题相关的任何问题。为了便于我们帮助你，提问时请指明你的开发环境，
驱动程序版本，源代码，预期的行为和实际行为。

本教程欢迎来自社区的踊跃参与。Vulkan 仍然是非常新近的接口，最佳实践尚未完全得到归纳。如果你对本教程和网站本身有任何类型的反馈，请不要犹豫，向
[GitHub 仓库](https://github.com/Overv/VulkanTutorial)提交 issue 或 pull request。你可以 *watch* 该存储库，这样当教程更新时就能收
到通知。

经历了用 Vulkan 在屏幕上画出第一个三角形的“仪式”后，我们会开始扩展简单的画三角形的程序，包括进行线性变换，加载纹理和三维模型。

如果你曾经使用过图形接口进行开发，你应该明白在第一个几何图元显示在屏幕上之前有许多步骤要进行。Vulkan 里有许多初始化步骤，但你将发现每个独立的
步骤都是易懂且必需的。要记住，一旦你能够画出一个看似无聊的三角形，绘制完整的带纹理的 3D 模型并不需要花费太多的额外工作，在此之外的每一步都会
更有价值。

如果你在学习本教程的过程中遇到了任何问题，首先查看“常见问题”页面，看看是否已经列出了你遇到的问题及其解决方案。如果在这之后仍然卡住，请在相关章节
的评论部分寻求帮助吧。

准备好深入研究高性能图形 API 的未来了吗？[让我们开始吧！](!zh/Overview)
