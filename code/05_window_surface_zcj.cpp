#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers_02 = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT_02(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT_02(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

struct QueueFamilyIndices_03 {
    std::optional<uint32_t> graphicsFamily_03;
    std::optional<uint32_t> presentFamily_05;

    bool isComplete() {
        return graphicsFamily_03.has_value() && presentFamily_05.has_value();
    }
};

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
    VkDebugUtilsMessengerEXT gbl_02_debugMessenger;
    VkSurfaceKHR gbl_05_surface;

    VkPhysicalDevice gbl_03_physicalDevice = VK_NULL_HANDLE;
    VkDevice gbl_04_device;

    VkQueue gbl_04_graphicsQueue;
    VkQueue gbl_05_presentQueue;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance_01();
        setupDebugMessenger_02();
        createSurface_05();
        pickPhysicalDevice_03();
        createLogicalDevice_04();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup_00() {
        vkDestroyDevice(gbl_04_device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT_02(instance, gbl_02_debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, gbl_05_surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void createInstance_01() {
        if (enableValidationLayers && !checkValidationLayerSupport_02()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions_02();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers_02.size());
            createInfo.ppEnabledLayerNames = validationLayers_02.data();

            populateDebugMessengerCreateInfo_02(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    void populateDebugMessengerCreateInfo_02(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback_02;
    }

    void setupDebugMessenger_02() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo_02(createInfo);

        if (CreateDebugUtilsMessengerEXT_02(instance, &createInfo, nullptr, &gbl_02_debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    /**
 * \brief Creates a Vulkan window surface.
 *
 * This function creates a Vulkan window surface using GLFW. It attempts to create the surface
 * and assigns it to the global variable `gbl_05_surface`. If the surface creation fails, it throws
 * a runtime error.
 *
 * \throws std::runtime_error if the window surface creation fails.
 */
void createSurface_05() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &gbl_05_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

    void pickPhysicalDevice_03() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable_03(device)) {
                gbl_03_physicalDevice = device;
                break;
            }
        }

        if (gbl_03_physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice_04() {
        QueueFamilyIndices_03 indices = findQueueFamilies_03(gbl_03_physicalDevice);

        // Vector to hold the queue creation information structures.
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        // Set to store unique queue family indices.
        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily_03.value(),
            indices.presentFamily_05.value()
        };

        // Priority for the device queues.
        float queuePriority = 1.0f;

        // Loop through each unique queue family index and create a queue creation info structure.
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        // Set the number of queue creation info structures.
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

        // Assign the array of queue creation info structures.
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = 0;

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers_02.size());
            createInfo.ppEnabledLayerNames = validationLayers_02.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(gbl_03_physicalDevice, &createInfo, nullptr, &gbl_04_device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(gbl_04_device, indices.graphicsFamily_03.value(), 0, &gbl_04_graphicsQueue);
// Retrieve the handle to the present queue from the logical device.
// The present queue is used to submit presentation commands to the swap chain.
// The queue family index is obtained from the `presentFamily_05` member of the `indices` structure.
// The queue index is set to 0, as only one queue is created for each family.
// The handle to the present queue is stored in the `gbl_05_presentQueue` global variable.
vkGetDeviceQueue(gbl_04_device, indices.presentFamily_05.value(), 0, &gbl_05_presentQueue);
    }

    bool isDeviceSuitable_03(VkPhysicalDevice device) {
        QueueFamilyIndices_03 indices = findQueueFamilies_03(device);

        return indices.isComplete();
    }

    QueueFamilyIndices_03 findQueueFamilies_03(VkPhysicalDevice device) {
        QueueFamilyIndices_03 indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily_03 = i;
            }

// Check if the physical device supports presentation to the surface.
// The `presentSupport` variable will be set to `VK_TRUE` if the device supports presentation.
VkBool32 presentSupport = false;
vkGetPhysicalDeviceSurfaceSupportKHR(device, i, gbl_05_surface, &presentSupport);

// If the device supports presentation, assign the queue family index to `presentFamily_05`.
if (presentSupport) {
    indices.presentFamily_05 = i;
}
            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    std::vector<const char*> getRequiredExtensions_02() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    bool checkValidationLayerSupport_02() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers_02) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback_02(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
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
