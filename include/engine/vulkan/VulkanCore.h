#ifndef TECTONIC_VULKANCORE_H
#define TECTONIC_VULKANCORE_H

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <GLFW/glfw3.h>
#include <memory>
#include "extern/VkBootstrap/VkBootstrap.h"
#include "exceptions.h"
#include "Window.h"
#include "VulkanStructs.h"
#include "VulkanUtils.h"

#define VK_CHECK(x)                                                             \
    {                                                                           \
        VkResult err = x;                                                       \
        if (err) {                                                              \
            fprintf(stderr, "Detected Vulkan error: %s", string_VkResult(err)); \
            abort();                                                            \
        }                                                                       \
    }

class VulkanCore {
public:
    VulkanCore(VulkanCore const&) = delete;
    void operator=(VulkanCore const&) = delete;
    static VulkanCore& getInstance();

    struct frameData{
        VkCommandPool commandPool;
        VkCommandBuffer mainCommandBuffer;

        VkSemaphore swapchainSemaphore;
        VkSemaphore renderSemaphore;
        VkFence renderFence;
    };

    frameData& getCurrentFrame();

    static constexpr uint8_t FRAMES_OVERLAP = 2;

    void draw();

private:
    VulkanCore();
    ~VulkanCore();

    void initGLFW();
    void initVulkan();
    void initSwapchain();
    void initCommands();
    void initSyncStructs();

    void createSwapchain(uint32_t width, uint32_t height);
    void destroySwapchain();

    static void glfwErrorCallback(int, const char *msg);

    bool m_isInitialized = false;
    uint32_t m_frameNumber = 0;

    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;

    VkSurfaceKHR m_surface;
    VkExtent2D m_windowExtent;
    std::unique_ptr<Window> m_window;

    VkSwapchainKHR m_swapchain;
    VkFormat m_swapchainImageFormat;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    VkExtent2D m_swapchainExtent;

    frameData m_frames[FRAMES_OVERLAP];
    VkQueue m_graphicsQueue;
    uint32_t m_graphicsQueueFamily;
};

#endif //TECTONIC_VULKANCORE_H
