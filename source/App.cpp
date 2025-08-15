#include "App.h"

void App::runLoop() {
	while (!window.shouldClose()) {
		glfwPollEvents();
		window.processInput();
	}
}