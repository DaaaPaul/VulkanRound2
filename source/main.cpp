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

const std::vector<const char*> requiredPhyDeviceExtensions = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName
};

std::vector<const char*> deviceExtensions = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName
};

class HelloTriangleApplication {
public:
    HelloTriangleApplication() : window{ nullptr }, context{}, instance{ nullptr }, surface{ nullptr }, physicalDevice { nullptr }, device{ nullptr }, graphicsQueue{ nullptr } {}

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
    vk::raii::SurfaceKHR surface;
    vk::raii::PhysicalDevice physicalDevice;
    vk::raii::Device device;
    vk::raii::Queue graphicsQueue;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan tutorial", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createDevice();
    }

    void createDevice() {
        std::vector<vk::QueueFamilyProperties> qfProperties = physicalDevice.getQueueFamilyProperties();
        uint32_t firstGraphicsQfIndex = ~0;

        for(uint32_t i = 0; i < qfProperties.size(); ++i) {
            if ((qfProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                physicalDevice.getSurfaceSupportKHR(i, *surface)) {
                firstGraphicsQfIndex = i;
                std::cout << "Graphics queue that supports presenting to surface at " << firstGraphicsQfIndex << " found in " << physicalDevice.getProperties().deviceName << '\n';
                break;
            }
        }

        float priority = 0.5f;
        vk::DeviceQueueCreateInfo queueCreateInfo = {
            .queueFamilyIndex = firstGraphicsQfIndex,
            .queueCount = 1,
            .pQueuePriorities = &priority
        };

        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
            {},
            {.dynamicRendering = true },    
            {.extendedDynamicState = true }
        };

        vk::DeviceCreateInfo deviceCreateInfo = {
            .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data()
        };

        device = vk::raii::Device(physicalDevice, deviceCreateInfo);
        std::cout << "Logical device created for " << physicalDevice.getProperties().deviceName << '\n';

        graphicsQueue = vk::raii::Queue(device, firstGraphicsQfIndex, 0);
    }

    void pickPhysicalDevice() {
        std::vector<vk::raii::PhysicalDevice> phyDevices = instance.enumeratePhysicalDevices();

        for(vk::raii::PhysicalDevice const& d : phyDevices) {
            uint32_t suitability = 0;

            if (d.getProperties().apiVersion >= VK_API_VERSION_1_3) {
                suitability++;
                std::cout << "Vulkan version supported" << '\n';
            }

            std::vector<vk::QueueFamilyProperties> qfProperties = d.getQueueFamilyProperties();
            for(vk::QueueFamilyProperties const& qf : qfProperties) {
                if (qf.queueFlags & vk::QueueFlagBits::eGraphics) {
                    suitability++;
                    std::cout << "Graphics card supports graphics" << '\n';
                    break;
                }
            }

            bool hasRequiredExtensions = true;
            std::vector<vk::ExtensionProperties> extensionProperties = d.enumerateDeviceExtensionProperties();
            for (uint32_t i = 0; i < requiredPhyDeviceExtensions.size(); ++i) {
                bool found = false;

                for(vk::ExtensionProperties const& eP : extensionProperties) {
                    if (strcmp(eP.extensionName, requiredPhyDeviceExtensions[i]) == 0) {
                        found = true;
                        break;
                    }
                }

                if(!found) hasRequiredExtensions = false;
            }

            if (hasRequiredExtensions) {
                std::cout << "Graphics card extensions supported" << '\n';
                suitability++;
            }

            vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> features = d.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
            bool hasRequiredFeatures = features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering && 
                features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

            if (hasRequiredFeatures) {
                std::cout << "Graphics card features supported" << '\n';
                suitability++;
            }

            if(suitability == 4) {
                physicalDevice = d;
                std::cout << "Physical device selected:" << physicalDevice.getProperties().deviceName << '\n';
                break;
            }
        }

        if (physicalDevice == nullptr) throw std::runtime_error("No suitable graphics card found");
    }

    void createSurface() {
        VkSurfaceKHR cSurface;
        if (glfwCreateWindowSurface(*instance, window, nullptr, &cSurface) != 0) {
            throw std::runtime_error("Failed to create window surface");
        }
        surface = vk::raii::SurfaceKHR(instance, cSurface);
        std::cout << "Created surface for windows" << '\n';
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
                        found = true;
                        break;
                    }
                }

                if(!found) throw std::runtime_error("Required validation layer not supported:" + std::string(requiredValLayers[i]));
                else std::cout << "Required validation layer supported:" + std::string(requiredValLayers[i]) << '\n';
            }
        }

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<vk::ExtensionProperties> extensionProperties = context.enumerateInstanceExtensionProperties();
        for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
            bool found = false;

            for(vk::ExtensionProperties const& property : extensionProperties) {
                if (strcmp(property.extensionName, glfwExtensions[i]) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) throw std::runtime_error("Required GLFW extension not supported:" + std::string(glfwExtensions[i]));
            else std::cout << "Required GLFW extension supported:" + std::string(glfwExtensions[i]) << '\n';
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
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}