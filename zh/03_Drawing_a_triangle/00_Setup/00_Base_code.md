---
title: 基本代码
---

## 通用结构

在上一章你已经使用正确的配置创建了一个 Vulkan 项目，并且已经用一些简单的代码测试过了。在这一章我们会用下面的代码从头开始：

```c++
#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

class HelloTriangleApplication {
public:
    void run() {
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    void initVulkan() {

    }

    void mainLoop() {

    }

    void cleanup() {

    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

首先我们从 LunarG SDK 中引入 Vulkan 的头文件，这个头文件提供了函数、结构体和枚举类型。`stdexcept` 和 `iostream` 头文件用来报告和输出
错误。`functional` 头文件为资源管理部分提供 lambda 函数支持，`cstdlib` 头文件提供 `EXIT_SUCCESS` 和 `EXIT_FAILURE` 宏定义。

程序本身被包装在了一个类里，我们把 Vulkan 对象存储成这个类的私有成员，并且添加成员函数来初始化它们，这些成员函数会被 `initVulkan` 函数调用。
当准备工作都做好了之后，我们进入主循环开始渲染每一帧。我们会用一个循环来填充 `mainLoop` 函数，它会一直循环到窗口被关闭为止。一旦窗口被关闭，
`mainLoop` 返回，我们将确保在 `cleanup` 函数中释放所有用过的资源。

如果在运行过程中有发生了任何致命错误，我们会抛出 `std::runtime_error` 异常并给出异常描述信息，这个异常描述信息会被传递到 `main` 函数，然后
被输出到命令行。为了同时处理各式标准异常，我们捕获更为一般的 `std::exception`。很快就会有一个关于错误处理的例子，我们会检查我们需要的扩展是否
受支持。

粗略地说，此后的每一章我们都会添加一个会被 `initVulkan` 函数调用的新函数以及对应的 Vulkan 对象作为私有类成员，成为私有成员的新 Vulkan 对象
需要在程序末尾通过 `cleanup` 函数释放。

## 资源管理

就像通过 `malloc` 申请到的每一块内存都必须通过 `free` 函数释放一样，每个 Vulkan 对象在当我们不需要它的时候都需要被显式销毁。现代 C++中可以
通过 [RAII](https://zh.wikipedia.org/wiki/RAII) 机制或 `<memory>` 头文件提供的智能指针来进行自动资源管理。但是在此教程中，我选择显式
地分配和回收 Vulkan 对象。毕竟 Vulkan 的卖点就在于显式地进行每一个操作从而避免出错，所以最好明确对象的生命周期来学习 API 如何工作。

在学习此教程之后，你可以通过各种方式实现自动资源管理，例如写一个 C++ 类，在构造函数中产生并持有 Vulkan 对象，在析构函数中释放它们；或者也可以
给 `std::unique_ptr` 或 `std::shared_ptr` 提供自定义删除器，具体使用哪种智能指针取决于你的所有权管理策略。在大型 Vulkan 程序中很推荐
使用 RAII，但是为了学习的目的，知道幕后发生了什么总是好的。

Vulkan 对象要么是直接用形如 `vkCreateXXX` 的函数直接创建的，要么是通过形如 `vkAllocateXXX` 的函数从另一个对象分配的。当你确定一个对象不再
被任何地方所使用的时候。你需要使用相应的 `vkDestroyXXX` 和 `vkFreeXXX` 来销毁它。这些函数的参数通常因对象的类型不同而不同，不过有一个参数是
它们公有的：`pAllocator`。这是一个可选的参数，允许你为自定义的内存分配器指定回调函数。在此教程中我们将忽略这个参数并一直传一个 `nullptr` 作为
参数。

## 集成 GLFW

如果你只想离屏渲染的话，Vulkan 在不创建窗口的情况下也能工作良好，但是事实上显示出点什么东西会更让人兴奋！首先删掉
`#include <vulkan/vulkan.h>` 这一行，换成：

```c++
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
```

这样，GLFW 会使用它自己的定义并且自动加载 Vulkan 头文件。添加一个 `initWindow` 函数并且在 `run` 函数中第一个调用它。我们会用这个函数初始
化 GLFW 并创建一个窗口。

```c++
void run() {
    initWindow();
    initVulkan(); 
    mainLoop(); 
    cleanup(); 
} 

private: 
    void initWindow() { 

    }
```

在 `initWindow` 函数中第一个调用的应该是 `glfwInit()`，这个函数初始化 GLFW 库。因为 GLFW 原本是为创建 OpenGL 上下文设计的，所以我们接下
来需要调用函数告诉 GLFW 不要创建 OpenGL 上下文：

```c++
glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
```

允许窗口调整大小会产生许多额外的问题，这一点日后再谈，现在先通过调用另一个 window hint 调用禁用掉：

```c++
glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
```

现在可以创建真正的窗口了。添加一个 `GLFWwindow* window;` 私有成员变量来保存一个 GLFW 窗口的引用，然后用以下函数初始化它：

```c++
window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
```

前三个参数知名了窗口的长度、宽度和标题。第四个参数是可选的，允许你指定一个显示器来显示这个窗口。最后一个参数只与 OpenGL 有关。

比起硬编码，使用常量来表示长度和宽度显然更好，因为一会儿我们还要用到这些值好几次。我在 `HelloTriangleApplication` 类的定义里加入了如下几行：

```c++
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
```

然后把创建窗口的代码改成这样

```cpp
window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
```

现在 `initWindow` 函数看起来应该长这样：

```c++
void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}
```

为了能让这个程序在不发生错误或者关闭窗口的情况下一直运行下去，我们需要在 `mainLoop` 函数中添加如下所示的事件循环：

```c++
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}
```

这段代码的意思应该不言自明。它是一个循环，每次循环都会检查事件，比如某按钮有没有被按下，一直循环到窗口被用户关闭为止。我们过之后还要在这个循环里
调用绘制单个帧的函数。

一旦窗口被关闭，我们需要销毁资源并退出 GLFW，把资源清理干净。这就是我们最初的 `cleanup` 代码：

```c++
void cleanup() {
    glfwDestroyWindow(window);

    glfwTerminate();
}
```

现在运行这个程序，你应该会看到一个标题为 `Vulkan` 的窗口，它会一直显示着，除非你把它关掉，程序也因此结束。现在我们有了一个 Vulkan 程序的框架，
让我们[创建第一个 Vulkan 对象吧](zn/Drawing_a_triangle/Setup/Instance)！

[C++代码](https://vulkan-tutorial.com/code/00_base_code.cpp)
