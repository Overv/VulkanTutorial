#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup_00();
    }

private:
    GLFWwindow* window;

    VkInstance instance;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance_01();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup_00() {
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

/**
* @brief Creates a Vulkan instance.
*
* This function sets up the necessary information and attempts to create a Vulkan instance,
* which is the connection between the application and the Vulkan library.
*/
void createInstance_01() {
    // Application information structure
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // Type of the structure
    appInfo.pApplicationName = "Hello Triangle"; // Name of the application
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // Application version
    appInfo.pEngineName = "No Engine"; // Name of the engine (if any)
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0); // Engine version
    appInfo.apiVersion = VK_API_VERSION_1_0; // Vulkan API version

    // Instance creation information structure
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // Type of the structure
    createInfo.pApplicationInfo = &appInfo; // Pointer to the application information

    // Get required extensions for GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Set the required extensions in the instance creation info
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    // No validation layers are enabled
    createInfo.enabledLayerCount = 0;

    // Create the Vulkan instance
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!"); // Throw an error if instance creation fails
    }
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
