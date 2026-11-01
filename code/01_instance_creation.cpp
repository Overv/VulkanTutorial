#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;

class HelloTriangleApplication {
  public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

  private:
	SDL_Window *window = nullptr;
	VkInstance instance;

	void initWindow() {
		SDL_Init(SDL_INIT_VIDEO);
		window = SDL_CreateWindow("Vulkan", WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
	}

	void initVulkan() {
		createInstance();
	}

	void mainLoop() {
		bool running = true;
		while (running) {
			SDL_Event e;
			while (SDL_PollEvent(&e)) {
				if (e.type == SDL_EVENT_QUIT) {
					running = false;
				}
				if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
					running = false;
				}
			}
		}
	}

	void cleanup() {
		vkDestroyInstance(instance, nullptr);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	void createInstance() {
		VkApplicationInfo appInfo{};
		appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName   = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName        = "No Engine";
		appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion         = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		uint32_t     sdlExtensionCount = 0;
		const char *const *sdlExtensions;
		sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

		createInfo.enabledExtensionCount   = sdlExtensionCount;
		createInfo.ppEnabledExtensionNames = sdlExtensions;

		createInfo.enabledLayerCount = 0;

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}
};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
