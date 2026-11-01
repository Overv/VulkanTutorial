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

	void initWindow() {
		SDL_Init(SDL_INIT_VIDEO);
		window = SDL_CreateWindow("Vulkan", WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
	}

	void initVulkan() {
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
		SDL_DestroyWindow(window);
		SDL_Quit();
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
