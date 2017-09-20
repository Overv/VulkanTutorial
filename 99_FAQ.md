This page lists solutions to common problems that you may encounter while
developing Vulkan applications.

* **I get an access violation error in the core validation layer**: Make sure
that MSI Afterburner / RivaTuner Statistics Server is not running, because it
has some compatibility problems with Vulkan.

* **I don't see any messages from the validation layers**: First make sure that
the validation layers get a chance to print errors by keeping the terminal open
after your program exits. You can do this from Visual Studio by running your
program with Ctrl-F5 instead of F5, and on Linux by executing your program from
a terminal window. If there are still no messages and you are sure that
validation layers are turned on, then you should ensure that your Vulkan SDK is
correctly installed by following [these instructions](https://vulkan.lunarg.com/doc/sdk/1.0.61.0/windows/getting_started.html).
