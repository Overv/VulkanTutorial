## General structure

In the previous chapter you've created a Vulkan project with all of the proper
configuration and tested it with the sample code. In this chapter we're starting
from scratch with the following code:

```c++
#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <functional>

class HelloTriangleApplication {
public:
    void run() {
        initVulkan();
        mainLoop();
    }

private:
    void initVulkan() {

    }

    void mainLoop() {

    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

We first include the Vulkan header from the LunarG SDK, which provides the
functions, structures and enumerations. The `stdexcept` and `iostream` headers
are included for reporting and propagating errors. The `functional` headers will
be used for a lambda functions in the resource management section.

The program itself is wrapped into a class where we'll store the Vulkan objects
as private class members and add functions to initiate each of them, which will
be called from the `initVulkan` function. Once everything has been prepared, we
enter the main loop to start rendering frames. We'll fill in the `mainLoop`
function to include a loop that iterates until the window is closed in a moment.

If any kind of fatal error occurs during execution then we'll throw a
`std::runtime_error` exception with a descriptive message, which will propagate
back to the `main` function and be printed to the command prompt. One example of
an error that we will deal with soon is finding out that a certain required
extension is not supported.

Roughly every chapter that follows after this one will add one new function that
will be called from `initVulkan` and one or more new Vulkan objects to the
private class members.

## Resource management

You may have noticed that there's no cleanup function anywhere to be seen and
that is intentional. Every Vulkan object needs to be destroyed with a function
call when it's no longer needed, just like each chunk of memory allocated with
`malloc` requires a call to `free`. Doing that manually is a lot of work and is
very error-prone, but we can completely avoid that by taking advantage of the
C++ [RAII](https://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization)
principle. To do that, we're going to create a class that wraps Vulkan objects
and automatically cleans them up when it goes out of scope, for example because
the application was closed.

First consider the interface we want from this `VDeleter` wrapper class.
Let's say we want to store a `VkInstance` object that should be destroyed with
`vkDestroyInstance` at some point. Then we would add the following class member:

```c++
VDeleter<VkInstance> instance{vkDestroyInstance};
```

The template argument specifies the type of Vulkan object we want to wrap and
the constructor argument specifies the function to use to clean up the object
when it goes out of scope.

To assign an object to the wrapper, we would simply want to pass its pointer to
the creation function as if it was a normal `VkInstance` variable:

```c++
vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
```

Unfortunately, taking the address of the handle in the wrapper doesn't
necessarily mean that we want to overwrite its existing value. A common pattern
is to simply use `&instance` as short-hand for an array of instances with 1
item. If we intend to write a new handle, then the wrapper should clean up any
previous object to not leak memory. Therefore it would be better to have the `&`
operator return a constant pointer and have an explicit function to state that
we wish to replace the handle. The `replace` function calls clean up for any
existing handle and then gives you a non-const pointer to overwrite the handle:

```c++
vkCreateInstance(&instanceCreateInfo, nullptr, instance.replace());
```

Just like that we can now use the `instance` variable wherever a `VkInstance`
would normally be accepted. We no longer have to worry about cleaning up
anymore, because that will automatically happen once the `instance` variable
becomes unreachable! That's pretty easy, right?

The implementation of such a wrapper class is fairly straightforward. It just
requires a bit of lambda magic to shorten the syntax for specifying the cleanup
functions.

```c++
template <typename T>
class VDeleter {
public:
    VDeleter() : VDeleter([](T, VkAllocationCallbacks*) {}) {}

    VDeleter(std::function<void(T, VkAllocationCallbacks*)> deletef) {
        this->deleter = [=](T obj) { deletef(obj, nullptr); };
    }

    VDeleter(const VDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
        this->deleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
    }

    VDeleter(const VDeleter<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
        this->deleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
    }

    ~VDeleter() {
        cleanup();
    }

    const T* operator &() const {
        return &object;
    }

    T* replace() {
        cleanup();
        return &object;
    }

    operator T() const {
        return object;
    }

    void operator=(T rhs) {
        if (rhs != object) {
            cleanup();
            object = rhs;
        }
    }

    template<typename V>
    bool operator==(V rhs) {
        return object == T(rhs);
    }

private:
    T object{VK_NULL_HANDLE};
    std::function<void(T)> deleter;

    void cleanup() {
        if (object != VK_NULL_HANDLE) {
            deleter(object);
        }
        object = VK_NULL_HANDLE;
    }
};
```

The three non-default constructors allow you to specify all three types of
deletion functions used in Vulkan:

* `vkDestroyXXX(object, callbacks)`: Only the object itself needs to be passed
to the cleanup function, so we can simply construct a `VDeleter` with just the
function as argument.
* `vkDestroyXXX(instance, object, callbacks)`: A `VkInstance` also
needs to be passed to the cleanup function, so we use the `VDeleter` constructor
that takes the `VkInstance` reference and cleanup function as parameters.
* `vkDestroyXXX(device, object, callbacks)`: Similar to the previous case, but a
`VkDevice` must be passed instead of a `VkInstance`.

The `callbacks` parameter is optional and we always pass `nullptr` to it, as you
can see in the `VDeleter` definition.

All of the constructors initialize the object handle with the equivalent of
`nullptr` in Vulkan: `VK_NULL_HANDLE`. Any extra arguments that are needed for
the deleter functions must also be passed, usually the parent object. It
overloads the address-of, assignment, comparison and casting operators to make
the wrapper as transparent as possible. When the wrapped object goes out of
scope, the destructor is invoked, which in turn calls the cleanup function we
specified.

The address-of operator returns a constant pointer to make sure that the object
within the wrapper is not unexpectedly changed. If you want to replace the
handle within the wrapper through a pointer, then you should use the `replace()`
function instead. It will invoke the cleanup function for the existing handle so
that you can safely overwrite it afterwards.

There is also a default constructor with a dummy deleter function that can be
used to initialize it later, which will be useful for lists of deleters.

I've added the class code between the headers and the `HelloTriangleApplication`
class definition. You can also choose to put it in a separate header file. We'll
use it for the first time in the next chapter where we'll create the very first
Vulkan object!

## Integrating GLFW

Vulkan works perfectly fine without a creating a window if you want to use it
off-screen rendering, but it's a lot more exciting to actually show something!
First replace the `#include <vulkan/vulkan.h>` line with

```c++
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
```

That way GLFW will include its own definitions and automatically load the Vulkan
header with it. Add a `initWindow` function and add a call to it from the `run`
function before the other calls. We'll use that function to initialize GLFW and
create a window.

```c++
void run() {
    initWindow();
    initVulkan();
    mainLoop();
}

private:
    void initWindow() {

    }
```

The very first call in `initWindow` should be `glfwInit()`, which initializes
the GLFW library. Because GLFW was originally designed to create an OpenGL
context, we need to tell it to not create an OpenGL context with a subsequent
call:

```c++
glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
```

Because handling resized windows takes special care that we'll look into later,
disable it for now with another window hint call:

```c++
glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
```

All that's left now is creating the actual window. Add a `GLFWwindow* window;`
private class member to store a reference to it and initialize the window with:

```c++
window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
```

The first three parameters specify the width, height and title of the window.
The fourth parameter allows you to optionally specify a monitor to open the
window on and the last parameter is only relevant to OpenGL.

It's a good idea to use constants instead of hardcoded width and height numbers
because we'll be referring to these values a couple of times in the future. I've
added the following lines above the `HelloTriangleApplication` class definition:

```c++
const int WIDTH = 800;
const int HEIGHT = 600;
```

and replaced the window creation call with

```c++
window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
```

You should now have a `initWindow` function that looks like this:

```c++
void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}
```

To keep the application running until either an error occurs or the window is
closed, we need to add an event loop to the `mainLoop` function as follows:

```c++
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
}
```

This code should be fairly self-explanatory. It loops and checks for events like
pressing the X button until the window has been closed by the user. This is also
the loop where we'll later call a function to render a single frame. Once the
window is closed, we need to clean up resources by destroying it and GLFW]
itself.

When you run the program now you should see a window titled `Vulkan` show up
until the application is terminated by closing the window. Now that we have the
skeleton for the Vulkan application, let's [create the first Vulkan object](!Drawing_a_triangle/Setup/Instance)!

[C++ code](/code/base_code.cpp)
