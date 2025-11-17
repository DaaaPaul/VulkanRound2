#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

constexpr uint32_t WIDTH = 1200;
constexpr uint32_t HEIGHT = 900;

constexpr bool enableValidationLayers = true;
const std::vector<const char*> requiredValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

class HelloTriangleApplication {
public:
    HelloTriangleApplication() : window{ nullptr }, context{}, instance{ nullptr } {}

    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    vk::raii::Context context;
    vk::raii::Instance instance;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan tutorial", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
    }

    void createInstance() {
        vk::ApplicationInfo appInfo{
            .pApplicationName = "Vulkan tutorial",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = vk::ApiVersion14
        };

        std::vector<const char*> requiredValLayers;
        if(enableValidationLayers) {
            requiredValLayers = requiredValidationLayers;

            std::vector<vk::LayerProperties> layerProperties = context.enumerateInstanceLayerProperties();

            for (uint32_t i = 0; i < requiredValLayers.size(); ++i) {
                bool found = false;

                for (vk::LayerProperties const& property : layerProperties) {
                    if (strcmp(property.layerName, requiredValLayers[i]) == 0) {
                        found = true; break;
                    }
                }

                if (!found) {
                    throw std::runtime_error("Required validation layer not supported:" + std::string(requiredValLayers[i]));
                }
            }
        }

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<vk::ExtensionProperties> extensionProperties = context.enumerateInstanceExtensionProperties();
        for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
            bool found = false;

            for(vk::ExtensionProperties const& property : extensionProperties) {
                if (strcmp(property.extensionName, glfwExtensions[i]) == 0) {
                    found = true; break;
                }
            }

            if(!found) {
                throw std::runtime_error("Required GLFW extension not supported:" + std::string(glfwExtensions[i]));
            }
        }

        vk::InstanceCreateInfo instanceCreateInfo;
        if(enableValidationLayers) {
            vk::InstanceCreateInfo info{
                .pApplicationInfo = &appInfo,
                .enabledLayerCount = static_cast<uint32_t>(requiredValLayers.size()),
                .ppEnabledLayerNames = requiredValLayers.data(),
                .enabledExtensionCount = glfwExtensionCount,
                .ppEnabledExtensionNames = glfwExtensions,
            };

            instanceCreateInfo = info;
        } else {
            vk::InstanceCreateInfo info{
                .pApplicationInfo = &appInfo,
                .enabledExtensionCount = glfwExtensionCount,
                .ppEnabledExtensionNames = glfwExtensions,
            };

            instanceCreateInfo = info;
        }

        instance = vk::raii::Instance(context, instanceCreateInfo);
    }

    void mainLoop() {
        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        glfwDestroyWindow(window);
        glfwTerminate();
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