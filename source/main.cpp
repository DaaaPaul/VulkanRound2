#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <algorithm>

constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

class TriangleApp {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	GLFWwindow* window = nullptr;
	vk::raii::Context context;
	vk::raii::Instance instance = nullptr;

	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "VulkanRound2", nullptr, nullptr);
	}

	void createInstnace() {
		const vk::ApplicationInfo appInfo{ {.pApplicationName = "Hello Triangle",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "No Engine",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = vk::ApiVersion14 } };

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<vk::ExtensionProperties> availableExtensions = context.enumerateInstanceExtensionProperties();

		uint32_t matchingExtensions = 0;
		for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
			for (const vk::ExtensionProperties& extension : availableExtensions) {
				if (strcmp(extension.extensionName, glfwExtensions[i]) == 0) ++matchingExtensions;
			}
		}
		if (matchingExtensions == glfwExtensionCount) std::cout << "SUCCESS: Required GLFW extensions exist\n";
		else throw std::runtime_error("FAIL: Some required GLFW extensions do not exist");

		vk::InstanceCreateInfo createInfo{{}, 
			&appInfo, 
			{}, 
			{}, 
			glfwExtensionCount, 
			glfwExtensions};

		instance = vk::raii::Instance(context, createInfo);
	}

	void initVulkan() {
		createInstnace();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
	}

	void cleanup() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}
};

int main() {
	try {
		TriangleApp app;
		app.run();
	} catch (const vk::SystemError& e) {
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	} catch (const std::exception& e) {
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}