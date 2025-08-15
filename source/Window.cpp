#include "Window.h"
#include <stdexcept>

Window::Window(const int w, const int h, const char* name, GLFWmonitor* monitor) : WIDTH{ w }, HEIGHT{ h } {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindow = glfwCreateWindow(WIDTH, HEIGHT, name, monitor, nullptr);
	if (glfwWindow == NULL) {
		throw std::runtime_error("Failed: Winow creation");
	}
}

Window::~Window() {
	glfwDestroyWindow(glfwWindow);
	glfwTerminate();
}

void Window::processInput() const {
	if (glfwGetKey(glfwWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
	}
}