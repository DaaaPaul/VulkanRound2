#include <vulkan/vulkan.hpp>
#include <iostream>
#include <stdexcept>
#include "App.h"

int main() {
	try {
		
		App app{};
		app.runLoop();
	} catch (const std::exception& e) {
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}