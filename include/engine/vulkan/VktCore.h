#ifndef TECTONIC_VKTCORE_H
#define TECTONIC_VKTCORE_H

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
#include "engine/Window.h"
#include "VktStructs.h"
#include "VktTypes.h"
#include "VktUtils.h"
#include "VktDescriptors.h"
#include "VktPipelines.h"
#include "VktDeletableQueue.h"
#include "VktLoader.h"

class VktCore {
public:
    VktCore(VktCore const&) = delete;
    void operator=(VktCore const&) = delete;
    static VktCore& getInstance();

    /**
     * @brief Initializes Vulkan.
     * @note Has to be called after setWindow.
     */
    void init();

    /** @brief Inserts initialized Window. */
    void setWindow(Window* window);

    /** @brief Uploads a mesh into the created Vulkan device memory. */
    VktTypes::GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<VktTypes::Vertex> vertices);

    static constexpr uint8_t FRAMES_OVERLAP = 2;

    /**
     * @brief Checks if the Window inside should close.
     * @return True if the window wants to close, False if not.
     *
     * It only calls the Window::shouldClose. It would be better to check it from the actual Window,
     * or setup a signal to get the data without polling glfwWindowShouldClose.
     */
    bool shouldClose();

    /**
     * @brief Main run function that should be called every frame.
     */
    void run();

private:
    VktCore() = default;
    ~VktCore();

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

    VktTypes::FrameData& getCurrentFrame();

    void draw();

    void drawBackground(VkCommandBuffer cmd);
    void drawGeometry(VkCommandBuffer cmd);
    void drawImGui(VkCommandBuffer cmd, VkImageView targetView);

    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& func);

    static VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                  VkDebugUtilsMessageTypeFlagsEXT type,
                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                  void* pUserData);

    VktTypes::AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    VktTypes::AllocatedImage  createImage(VkExtent3D allocSize, VkFormat format, VkImageUsageFlags usage, bool mipMapped = false);
    VktTypes::AllocatedImage  createImage(void* data, VkExtent3D allocSize, VkFormat format, VkImageUsageFlags usage, bool mipMapped = false);
    void destroyBuffer(const VktTypes::AllocatedBuffer& buffer);
    void destroyImage(const VktTypes::AllocatedImage& img);

    void resizeSwapchain();

    bool m_isInitialized = false;
    uint32_t m_frameNumber = 0;

    VkInstance m_instance{};
    VkPhysicalDevice m_physicalDevice{};
    VkDevice m_device{};

    VkSurfaceKHR m_surface{};
    VkExtent2D m_windowExtent{};
    Window* m_window = nullptr;

    VkSwapchainKHR m_swapchain{};
    VkFormat m_swapchainImageFormat = VK_FORMAT_UNDEFINED;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    VkExtent2D m_swapchainExtent{};
    bool m_resizeSwapchain = false;

    VktTypes::AllocatedImage m_drawImage{};
    VktTypes::AllocatedImage m_depthImage{};
    VkExtent2D m_drawExtent{};
    float m_renderScale = 1.0f;

    VktTypes::FrameData m_frames[FRAMES_OVERLAP];
    VkQueue m_graphicsQueue{};
    uint32_t m_graphicsQueueFamily{};

    VktDeletableQueue m_coreDeletionQueue;

    VmaAllocator m_vmaAllocator{};

    DescriptorAllocatorDynamic m_globDescriptorAllocator{};
    VkDescriptorSet m_drawImageDescriptors{};
    VkDescriptorSetLayout m_drawImageDescriptorLayout{};

    VkPipeline m_gradientPipeline{};
    VkPipelineLayout m_gradientPipelineLayout{};

    VkFence m_immFence{};
    VkCommandBuffer m_immCommandBuffer{};
    VkCommandPool m_immCommandPool{};

    std::vector<VktTypes::ComputeEffect> m_backgroundEffect;
    int m_currentBackgroundEffect = 0;

    VkPipelineLayout m_meshPipelineLayout{};
    VkPipeline m_meshPipeline{};

    std::vector<std::shared_ptr<MeshAsset>> m_testMeshes;

    VktTypes::GPUSceneData m_sceneData{};
    VkDescriptorSetLayout m_gpuSceneDataDescriptorLayout{};

    VktTypes::AllocatedImage m_whiteImage{};
    VktTypes::AllocatedImage m_blackImage{};
    VktTypes::AllocatedImage m_greyImage{};
    VktTypes::AllocatedImage m_errorCheckboardImage{};
    VkSampler m_defaultSamplerLinear{};
    VkSampler m_defaultSamplerNearest{};
    VkDescriptorSetLayout m_singleImageDescriptorLayout{};

    struct GLTFMetallicRoughness{
        VktTypes::MaterialPipeline opaquePipeline{};
        VktTypes::MaterialPipeline transparentPipeline{};

        VkDescriptorSetLayout materialLayout{};

        struct MaterialConstants{
            glm::vec4 colorFactors;
            glm::vec4 metalRoughFactors;
            glm::vec4 extra[14];
        };

        struct MaterialResources{
            VktTypes::AllocatedImage colorImage;
            VkSampler colorSampler = VK_NULL_HANDLE;
            VktTypes::AllocatedImage metalRoughImage;
            VkSampler metalRoughSampler = VK_NULL_HANDLE;
            VkBuffer dataBuffer = VK_NULL_HANDLE;
            uint32_t dataBufferOffset = 0;
        };

        DescriptorWriter writer;

        void buildPipelines(VktCore* core);
        void clearResources(VkDevice device);

        VktTypes::MaterialInstance writeMaterial(VkDevice device,
                                                 VktTypes::MaterialPass pass,
                                                 const MaterialResources& resources,
                                                 DescriptorAllocatorDynamic& descriptorAllocator);
    };

    VktTypes::MaterialInstance m_defaultData;
    GLTFMetallicRoughness m_metalRoughMaterial;
};

#endif //TECTONIC_VKTCORE_H
