#pragma once
#include <GLFW/glfw3.h>

class Window {
public:
	Window(const int w, const int h, const char* name, GLFWmonitor* monitor);
	~Window();
	Window(const Window const&) = delete;
	Window& operator=(const Window) = delete;

	bool shouldClose() const { return glfwWindowShouldClose(glfwWindow); };
	void processInput() const;

private:
	const int WIDTH{};
	const int HEIGHT{};
	GLFWwindow* glfwWindow{};
};