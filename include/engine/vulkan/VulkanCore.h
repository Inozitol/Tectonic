#ifndef TECTONIC_VULKANCORE_H
#define TECTONIC_VULKANCORE_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/gtx/transform.hpp>

#include <memory>
#include <deque>

#include "extern/vkbootstrap/VkBootstrap.h"

#include "extern/vma/vk_mem_alloc.h"
#include "extern/imgui/imgui.h"
#include "extern/imgui/imgui_impl_vulkan.h"
#include "extern/imgui/imgui_impl_glfw.h"

#include "exceptions.h"
#include "Window.h"
#include "VulkanStructs.h"
#include "VulkanTypes.h"
#include "VulkanUtils.h"
#include "VulkanDescriptors.h"
#include "VulkanPipelines.h"
#include "VulkanDestructQueue.h"
#include "VulkanLoader.h"

class VulkanCore {
public:
    VulkanCore(VulkanCore const&) = delete;
    void operator=(VulkanCore const&) = delete;
    static VulkanCore& getInstance();

    VkTypes::FrameData& getCurrentFrame();

    VkTypes::GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<VkTypes::Vertex> vertices);

    static constexpr uint8_t FRAMES_OVERLAP = 2;

    bool shouldClose();
    void run();

private:
    VulkanCore();
    ~VulkanCore();

    void initGLFW();
    void initVulkan();
    void initSwapchain();
    void initCommands();
    void initSyncStructs();
    void initDescriptors();
    void initPipelines();
    void initBackgroundPipelines();
    void initMeshPipeline();
    void initImGui();
    void initDefaultData();

    void createSwapchain(uint32_t width, uint32_t height);
    void destroySwapchain();

    void draw();

    void drawBackground(VkCommandBuffer cmd);
    void drawGeometry(VkCommandBuffer cmd);
    void drawImGui(VkCommandBuffer cmd, VkImageView targetView);

    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& func);

    static VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                  VkDebugUtilsMessageTypeFlagsEXT type,
                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                  void* pUserData);

    VkTypes::AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    void destroyBuffer(const VkTypes::AllocatedBuffer& buffer);
    VkTypes::AllocatedImage createImage(VkExtent3D allocSize, VkFormat format, VkImageUsageFlags usage, bool mipMapped = false);
    VkTypes::AllocatedImage createImage(void* data, VkExtent3D allocSize, VkFormat format, VkImageUsageFlags usage, bool mipMapped = false);
    void destroyImage(const VkTypes::AllocatedImage& img);

    void resizeSwapchain();

    static void glfwErrorCallback(int, const char *msg);

    bool m_isInitialized = false;
    uint32_t m_frameNumber = 0;

    VkInstance m_instance;
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
    bool m_resizeSwapchain = false;

    VkTypes::AllocatedImage m_drawImage;
    VkTypes::AllocatedImage m_depthImage;
    VkExtent2D m_drawExtent;
    float m_renderScale = 1.0f;

    VkTypes::FrameData m_frames[FRAMES_OVERLAP];
    VkQueue m_graphicsQueue;
    uint32_t m_graphicsQueueFamily;

    DeletionQueue m_coreDeletionQueue;

    VmaAllocator m_vmaAllocator;

    DescriptorAllocator m_globDescriptorAllocator;
    VkDescriptorSet m_drawImageDescriptors;
    VkDescriptorSetLayout m_drawImageDescriptorLayout;

    VkPipeline m_gradientPipeline;
    VkPipelineLayout m_gradientPipelineLayout;

    VkFence m_immFence;
    VkCommandBuffer m_immCommandBuffer;
    VkCommandPool m_immCommandPool;

    std::vector<VkTypes::ComputeEffect> m_backgroundEffect;
    int m_currentBackgroundEffect = 0;

    VkPipelineLayout m_meshPipelineLayout;
    VkPipeline m_meshPipeline;

    std::vector<std::shared_ptr<MeshAsset>> m_testMeshes;

    VkTypes::GPUSceneData m_sceneData{};
    VkDescriptorSetLayout m_gpuSceneDataDescriptorLayout;

    VkTypes::AllocatedImage m_whiteImage;
    VkTypes::AllocatedImage m_blackImage;
    VkTypes::AllocatedImage m_greyImage;
    VkTypes::AllocatedImage m_errorCheckboardImage;
    VkSampler m_defaultSamplerLinear;
    VkSampler m_defaultSamplerNearest;
    VkDescriptorSetLayout m_singleImageDescriptorLayout;

    struct GLTFMetallicRoughness{
        VkTypes::MaterialPipeline opaquePipeline{};
        VkTypes::MaterialPipeline transparentPipeline{};

        VkDescriptorSetLayout materialLayout{};

        struct MaterialConstants{
            glm::vec4 colorFactors;
            glm::vec4 metalRoughFactors;
            glm::vec4 extra[14];
        };

        struct MaterialResources{
            VkTypes::AllocatedImage colorImage;
            VkSampler colorSampler;
            VkTypes::AllocatedImage metalRoughImage;
            VkSampler metalRoughSampler;
            VkBuffer dataBuffer;
            uint32_t dataBufferOffset;
        };

        DescriptorWriter writer;

        void buildPipelines(VulkanCore* core);
        void clearResources(VkDevice device);

        VkTypes::MaterialInstance writeMaterial(VkDevice device,
                                                VkTypes::MaterialPass pass,
                                                const MaterialResources& resources,
                                                DescriptorAllocatorDynamic& descriptorAllocator);
    };


};

#endif //TECTONIC_VULKANCORE_H
