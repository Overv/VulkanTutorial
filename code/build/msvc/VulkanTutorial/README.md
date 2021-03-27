MS Visual Studio 2019 Solution for x64 (debug+release, no 32 bit).

Required dependencies (may be installed via [vcpkg tool](https://github.com/microsoft/vcpkg)):

- stb
- tinyobjloader
- glfw3
- glm

Of course, we also require the [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/).
It is located via VULKAN_SDK env var set by SDK installer and must be downloaded and
installed manually.

To start each example via F5 shortcut, set Startup Project in Solution Properties
to "Current selection".

If you want a working syntax highlighting for the shader sources, this is what worked for me
at the time of writing this:

- https://github.com/stef-levesque/vscode-shader/ for *VS Code*
- Could not find any working extension for MSVC.
