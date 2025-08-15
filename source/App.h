#pragma once
#include "Window.h"

class App {
public:
	void runLoop();

private:
	Window window{ 800, 600, "VulkanRound2", nullptr };
};