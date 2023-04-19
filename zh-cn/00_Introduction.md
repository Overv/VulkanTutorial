## 关于

此教程将教你使用[Vulkan](https://www.khronos.org/vulkan/)图形和计算API的基本知识。Vulkan是[Khronos Group](https://www.khronos.org/)（以OpenGL闻名）推出的一款新API，它为现代图形卡提供了更好的抽象。这个新接口可以让你更好地描述程序如何去执行，与[OpenGL](https://en.wikipedia.org/wiki/OpenGL)和[Direct3D](https://en.wikipedia.org/wiki/Direct3D)等现有API相比，这可以带来更好的性能和更少的奇怪行为。Vulkan背后的想法与[Direct3D 12](https://en.wikipedia.org/wiki/Direct3D#Direct3D_12)和[Metal](https://en.wikipedia.org/wiki/Metal_(API))的想法相似，但Vulkan具有完全跨平台的优势，可以同时为Windows、Linux和Android开发。

然而，你要为这些好处付出一些代价：你必须使用一个更加详细的API。与图形API相关的每个细节都需要由应用程序从头开始设置，包括初始帧缓冲区创建和缓冲区和纹理图像等对象的内存管理。图形驱动程序将减少手动操作，这意味着你必须在应用程序中做更多的工作来确保正确的行为。

这里要传达的信息是，Vulkan并不适合所有人。它针对的是那些热衷于高性能计算机图形并愿意投入一些工作的程序员。如果你对游戏开发而不是计算机图形更感兴趣，那么你可能希望坚持使用OpenGL或Direct3D，这在短期内不会因为Vulkan而受到抨击。另一种选择是使用[虚幻引擎(Unreal Engine)](https://en.wikipedia.org/wiki/Unreal_Engine#Unreal_Engine_4)或[Unity](https://en.wikipedia.org/wiki/Unity_(game_engine))这样的引擎，它将能够使用Vulkan，同时向你展示更高级别的API。

有了这些，让我们介绍一下遵循本教程的一些先决条件：

* 与Vulkan兼容的图形卡和驱动程序 ([NVIDIA](https://developer.nvidia.com/vulkan-driver), [AMD](http://www.amd.com/en-us/innovations/software-technologies/technologies-gaming/vulkan), [Intel](https://software.intel.com/en-us/blogs/2016/03/14/new-intel-vulkan-beta-1540204404-graphics-driver-for-windows-78110-1540), [Apple Silicon (Or the Apple M1)](https://www.phoronix.com/scan.php?page=news_item&px=Apple-Silicon-Vulkan-MoltenVK))
* C++经验（熟悉RAII、初始化器列表）
* 一个对C++17功能有良好支持的编译器（Visual Studio 2017+、GCC 7+或Clang 5+）
* 3D计算机图形学方面的一些现有经验

本教程不会假设你了解OpenGL或Direct3D概念，但它确实要求你了解3D计算机图形的基础知识。例如，它不会解释透视投影背后的数学问题。请参阅[这本在线书籍](https://paroj.github.io/gltut/)，了解计算机图形学概念的精彩介绍。其他一些优秀的计算机图形资源包括：

* [Ray tracing in one weekend](https://github.com/RayTracing/raytracing.github.io)
* [Physically Based Rendering book](http://www.pbr-book.org/)
* Vulkan being used in a real engine in the open-source [Quake](https://github.com/Novum/vkQuake) and [DOOM 3](https://github.com/DustinHLand/vkDOOM3)

如果你愿意，你可以使用C而不是C++，但你必须使用不同的线性代数库，这样你只有自己构建代码结构。
我们将使用类和RAII等C++功能来组织逻辑和资源生命周期。本教程还有一个可供Rust开发人员使用的[替代版本](https://github.com/bwasty/vulkan-tutorial-rs)。

为了让使用其他编程语言的开发人员更容易理解，并获得一些基本API的经验，我们将使用原始的C API来使用Vulkan。然而，如果你使用的是C++，你可能更喜欢使用较新的[Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp)绑定，这些绑定抽象了一些繁琐的工作，并有助于避免某些类的错误。

## 电子书 (暂无中文版)

如果你更喜欢将本教程作为电子书阅读，那么你可以在此处下载EPUB或PDF版本：

* [EPUB(英文版)](https://vulkan-tutorial.com/resources/vulkan_tutorial_en.epub)
* [PDF(英文版)](https://vulkan-tutorial.com/resources/vulkan_tutorial_en.pdf)

## 教程结构

我们将首先概述Vulkan是如何工作的，以及为了在屏幕上显示第一个三角形，我们必须做的工作。在你理解了它们在整个画面中的基本作用后，所有小步骤的目的都会更有意义。接下来，我们将使用[Vulkan SDK](https://lunarg.com/vulkan-sdk/)、用于线性代数运算的[GLM库](http://glm.g-truc.net/)和用于窗口创建的[GLFW](http://www.glfw.org/)来设置开发环境。本教程将介绍如何在带有Visual Studio的Windows和带有GCC的Ubuntu Linux上设置这些。

之后，我们将实现Vulkan程序的所有基本组件，这些组件是渲染第一个三角形所必需的。每一章大致遵循以下结构：
* 引入一个新概念及其目的
* 使用所有相关的API调用将其集成到你的程序中
* 将其部分抽象为辅助函数

尽管每一章都是作为前一章的后续内容编写的，但也可以将这些章节作为介绍某个Vulkan功能的独立文章来阅读。这意味着该网站也可以作为参考。所有Vulkan函数和类型都链接到规范，因此你可以单击它们了解更多信息。Vulkan是一种非常新的API，因此规范本身可能存在一些缺陷。我们鼓励你向 [这个Khronos仓库](https://github.com/KhronosGroup/Vulkan-Docs) 提交反馈.


如前所述，Vulkan API是一个相当详细的API，其中包含许多参数，可以最大限度地控制图形硬件。这导致创建纹理等基本操作需要执行许多步骤，每次都必须重复这些步骤。因此，在整个教程中，我们将创建自己的辅助函数集合。


每一章的结尾都会有一个链接，链接到完整的代码列表。如果你对代码的结构有任何疑问，或者如果你正在处理一个bug并想进行比较，你可以参考它。所有代码文件都已在多家供应商的显卡上进行了测试，以验证其正确性。每一章的结尾都有一个评论部分，你可以在这里提出与特定主题相关的任何问题。请指定你的平台、驱动程序版本、源代码、预期行为和实际行为，以帮助我们和你。

本教程旨在成为一项社区活动。Vulkan仍然是一个非常新的API，最佳实践尚未真正建立。如果你对教程和网站本身有任何类型的反馈，请毫不犹豫地向[GitHub存储库]提交问题或拉取请求(https://github.com/Overv/VulkanTutorial).
你可以*Watch*存储库，以便收到教程更新的通知。

在你完成了在屏幕上绘制第一个Vulkan支持的三角形的程序后，我们将开始扩展该程序，包括线性变换、纹理和3D模型。

如果你以前玩过图形API，那么你就会知道，在第一个几何体出现在屏幕上之前，可能会有很多步骤。Vulkan中有许多这样的初始步骤，但你会看到每个单独的步骤都很容易理解，而且不会觉得多余。同样重要的是要记住，一旦你有了一个看起来无聊的三角形，绘制完全纹理的3D模型就不需要那么多额外的工作，超过这一点的每一步都会更有收获。

如果你在学习本教程时遇到任何问题，请首先查看常见问题解答(FAQ)，看看你的问题及其解决方案是否已在其中列出。如果你在那之后仍然陷入困境，请随时在最相关章节的评论部分寻求帮助。

准备好深入研究未来的高性能图形API了吗？[Let's go!](!zh-cn/Overview)