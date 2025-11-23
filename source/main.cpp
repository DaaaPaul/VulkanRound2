#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <limits>
#include <algorithm>
#include <fstream>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

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
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window = nullptr;
    vk::raii::Context context{};
    vk::raii::Instance instance = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;
    vk::raii::Queue graphicsQueue = nullptr;
    uint32_t graphicsQueueIndex = ~0;
    vk::raii::SwapchainKHR swapchain = nullptr;
    vk::SurfaceFormatKHR swapchainFormat{};
    vk::PresentModeKHR swapchainPresentMode{};
    vk::Extent2D swapchainExtent{};
    std::vector<vk::Image> swapchainImages{};
    std::vector<vk::raii::ImageView> swapchainImageViews{};
    vk::raii::Pipeline graphicsPipeline = nullptr;
    vk::raii::CommandPool commandPool = nullptr;
    vk::raii::CommandBuffer commandBuffer = nullptr;
    vk::raii::Semaphore renderComplete = nullptr;
    vk::raii::Semaphore presentComplete = nullptr;
    vk::raii::Fence commandBufferDone = nullptr;

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
        createSwapchain();
        createSwapchainImageViews();
        createGraphicsPipeline();
        createCommandPool();
        createCommandBuffer();
        createSyncObjects();
    }

    void createSyncObjects() {
        renderComplete = vk::raii::Semaphore(device, vk::SemaphoreCreateInfo());
        presentComplete = vk::raii::Semaphore(device, vk::SemaphoreCreateInfo());
        commandBufferDone = vk::raii::Fence(device, { .flags = vk::FenceCreateFlagBits::eSignaled });
    }

    void transitionImageLayout(
        uint32_t imageIndex,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask) {

        vk::ImageMemoryBarrier2 barrier = {
            .srcStageMask = srcStageMask,
            .srcAccessMask = srcAccessMask,
            .dstStageMask = dstStageMask,
            .dstAccessMask = dstAccessMask,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = swapchainImages[imageIndex],
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        vk::DependencyInfo dependencyInfo = {
            .dependencyFlags = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
        };

        commandBuffer.pipelineBarrier2(dependencyInfo);
    }

    void recordCommandBuffer(uint32_t imageIndex) {
        commandBuffer.begin({});

        transitionImageLayout(
            imageIndex,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput
        );

        vk::ClearColorValue clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        vk::RenderingAttachmentInfo colorAttachmentInfo = {
            .imageView = swapchainImageViews[imageIndex],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearValue,
        };

        vk::RenderingInfo renderingInfo = {
            .renderArea = {.offset = {0, 0}, .extent = swapchainExtent },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentInfo
        };

        commandBuffer.beginRendering(renderingInfo);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
        commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height), 0.0f, 1.0f));
        commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchainExtent));
        
        commandBuffer.draw(3, 1, 0, 0);
        commandBuffer.endRendering();

        transitionImageLayout(
            imageIndex,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            {},
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eBottomOfPipe
        );

        commandBuffer.end();
    }

    void createCommandBuffer() {
        vk::CommandBufferAllocateInfo commandBuffersInfo = {
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };

        commandBuffer = std::move(vk::raii::CommandBuffers(device, commandBuffersInfo).front());
    }

    void createCommandPool() {
        vk::CommandPoolCreateInfo commandPoolInfo = {
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = graphicsQueueIndex
        };

        commandPool = vk::raii::CommandPool(device, commandPoolInfo);
    }

    std::vector<char> readBinaryFile(std::string const& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file!\n");
        }

        std::vector<char> buffer(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

        file.close();

        return buffer;
    }

    void createGraphicsPipeline() {
        std::vector<char> shaderBytecode = readBinaryFile(R"(shaders\slang.spv)");
        vk::ShaderModuleCreateInfo moduleInfo = {
            .codeSize = shaderBytecode.size() * sizeof(char),
            .pCode = reinterpret_cast<const uint32_t*>(shaderBytecode.data()),
        };
        vk::raii::ShaderModule shader(device, moduleInfo);

        vk::PipelineShaderStageCreateInfo vertex = {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = shader,
            .pName = "vertMain",
        };

        vk::PipelineShaderStageCreateInfo fragment = {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = shader,
            .pName = "fragMain",
        };

        vk::PipelineShaderStageCreateInfo shaderStages[] = { vertex, fragment };

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
            .topology = vk::PrimitiveTopology::eTriangleList
        };

        vk::PipelineViewportStateCreateInfo viewportInfo = {
            .viewportCount = 1,
            .scissorCount = 1,
        };

        vk::PipelineRasterizationStateCreateInfo rasterizationInfo = {
            .depthClampEnable = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eClockwise,
            .depthBiasEnable = vk::False,
            .depthBiasSlopeFactor = 1.0f,
            .lineWidth = 1.0f
        };

        vk::PipelineMultisampleStateCreateInfo multisamplingInfo = { 
            .rasterizationSamples = vk::SampleCountFlagBits::e1, 
            .sampleShadingEnable = vk::False 
        };

        vk::PipelineColorBlendAttachmentState colorBlendAttachmentInfo = { 
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA 
        };

        vk::PipelineColorBlendStateCreateInfo colorBlendingInfo = { 
            .logicOpEnable = vk::False,
            .logicOp = vk::LogicOp::eCopy, 
            .attachmentCount = 1, 
            .pAttachments = &colorBlendAttachmentInfo
        };

        std::vector<vk::DynamicState> dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
        };
        vk::PipelineDynamicStateCreateInfo dynamicStateInfo = {
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        };

        vk::raii::PipelineLayout pipelineLayout = nullptr;
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo = { 
            .setLayoutCount = 0, 
            .pushConstantRangeCount = 0 
        };
        pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

        vk::PipelineRenderingCreateInfo attachmentInfo = {
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapchainFormat.format
        };

        vk::GraphicsPipelineCreateInfo pipelineInfo = {
            .pNext = &attachmentInfo,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pViewportState = &viewportInfo,
            .pRasterizationState = &rasterizationInfo,
            .pMultisampleState = &multisamplingInfo,
            .pColorBlendState = &colorBlendingInfo,
            .pDynamicState = &dynamicStateInfo,
            .layout = pipelineLayout,
            .renderPass = nullptr
        };

        graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
        std::cout << "Created graphics pipeline\n";
    }

    void createSwapchainImageViews() {
        swapchainImageViews.clear();

        vk::ImageViewCreateInfo imageViewInfo = {
            .viewType = vk::ImageViewType::e2D,
            .format = swapchainFormat.format,
            .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
        };

        for(vk::Image const& im : swapchainImages) {
            imageViewInfo.image = im;
            swapchainImageViews.push_back(vk::raii::ImageView(device, imageViewInfo));
        }
    }

    void createSwapchain() {
        vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(surface);
        std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

        uint32_t imageCount;

        bool foundWantedFormat = false;
        for(vk::SurfaceFormatKHR const& f : formats) {
            if(f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                swapchainFormat = f;
                foundWantedFormat = true;
                std::cout << "Found best surface format\n";
                break;
            }
        }
        if(!foundWantedFormat) {
            swapchainFormat = formats[0];
            std::cout << "Didn't find best surface format, defaulting...\n";
        }

        bool foundWantedPresentMode = false;
        for (vk::PresentModeKHR const& pm : presentModes) {
            if(pm == vk::PresentModeKHR::eMailbox) {
                swapchainPresentMode = pm;
                foundWantedPresentMode = true;
                std::cout << "Found best present mode\n";
                break;
            }
        }
        if (!foundWantedPresentMode) {
            swapchainPresentMode = vk::PresentModeKHR::eFifo;
            std::cout << "Didn't find best present mode, defaulting...\n";
        }

        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            swapchainExtent = capabilities.currentExtent;
            std::cout << "No discrepency in extent vs screen coordinates, setting extent directly\n";
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            swapchainExtent = {
                std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
            };
            std::cout << "Discrepency in extent vs screen coordinates, adjusting extent accordingly\n";
        }

        imageCount = capabilities.minImageCount + 1;
        if(capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR swapchainCreateInfo = {
            .flags = vk::SwapchainCreateFlagsKHR(),
            .surface = surface,
            .minImageCount = capabilities.minImageCount,
            .imageFormat = swapchainFormat.format,
            .imageColorSpace = swapchainFormat.colorSpace,
            .imageExtent = swapchainExtent,
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment, 
            .imageSharingMode = vk::SharingMode::eExclusive,
            .preTransform = capabilities.currentTransform, 
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = swapchainPresentMode,
            .clipped = true, 
            .oldSwapchain = nullptr 
        };

        swapchain = vk::raii::SwapchainKHR(device, swapchainCreateInfo);
        swapchainImages = swapchain.getImages();
        std::cout << "Swapchain created with " << swapchainImages.size() << " images\n";
    }

    void createDevice() {
        std::vector<vk::QueueFamilyProperties> qfProperties = physicalDevice.getQueueFamilyProperties();

        for(uint32_t i = 0; i < qfProperties.size(); ++i) {
            if ((qfProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                physicalDevice.getSurfaceSupportKHR(i, *surface)) {
                graphicsQueueIndex = i;
                std::cout << "Graphics queue that supports presenting to surface at " << graphicsQueueIndex << " found in " << physicalDevice.getProperties().deviceName << '\n';
                break;
            }
        }

        float priority = 0.5f;
        vk::DeviceQueueCreateInfo queueCreateInfo = {
            .queueFamilyIndex = graphicsQueueIndex,
            .queueCount = 1,
            .pQueuePriorities = &priority
        };

        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
            {},
            {.shaderDrawParameters = true},
            {.synchronization2 = true, .dynamicRendering = true },    
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

        graphicsQueue = vk::raii::Queue(device, graphicsQueueIndex, 0);
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

            vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> features = d.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
            bool hasRequiredFeatures = 
                features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
                features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering && 
                features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
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
            drawFrame();
        }

        device.waitIdle();
    }

    void drawFrame() {
        graphicsQueue.waitIdle();
        std::pair<vk::Result, uint32_t> freshImage = swapchain.acquireNextImage(UINT64_MAX, *presentComplete, nullptr);

        recordCommandBuffer(freshImage.second);
        device.resetFences(*commandBufferDone);

        vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        vk::SubmitInfo submitInfo = {
            .waitSemaphoreCount = 1, 
            .pWaitSemaphores = &*presentComplete,
            .pWaitDstStageMask = &waitDestinationStageMask, 
            .commandBufferCount = 1, 
            .pCommandBuffers = &*commandBuffer, 
            .signalSemaphoreCount = 1, 
            .pSignalSemaphores = &*renderComplete,
        };

        graphicsQueue.submit(submitInfo, *commandBufferDone);
        while (vk::Result::eTimeout == device.waitForFences(*commandBufferDone, vk::True, UINT64_MAX));

        vk::PresentInfoKHR presentInfoKHR = { 
            .waitSemaphoreCount = 1, 
            .pWaitSemaphores = &*renderComplete,
            .swapchainCount = 1, 
            .pSwapchains = &*swapchain, 
            .pImageIndices = &freshImage.second 
        };

        freshImage.first = graphicsQueue.presentKHR(presentInfoKHR);

        switch (freshImage.first) {
            case vk::Result::eSuccess:
                break;
            case vk::Result::eSuboptimalKHR:
                std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR!\n";
                break;
            default:
                break;
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