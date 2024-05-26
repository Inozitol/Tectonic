#ifndef TECTONIC_VKTCORE_H
#define TECTONIC_VKTCORE_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

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
#include "Logger.h"
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

    /**
     * @brief De-initializes Vulkan.
     */
    void clean();

    /** @brief Inserts initialized Window. */
    void setWindow(Window* window);

    /** @brief Uploads a mesh into the created Vulkan device memory. */
    static VktTypes::GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<VktTypes::Vertex> vertices);

    /**
     * @brief Checks if the Window inside should close.
     * @return True if the m_window wants to close, False if not.
     *
     * It only calls the Window::shouldClose. It would be better to check it from the actual Window,
     * or setup a signal to get the data without polling glfwWindowShouldClose.
     */
    bool shouldClose();

    /**
     * @brief Main run function that should be called every frame.
     */
    void run();

    void setViewMatrix(const glm::mat4& viewMatrix);
    void setProjMatrix(const glm::mat4& projMatrix);

    /**
     * @brief Returns current VkDevice (if initialized).
     * @return Current VkDevice.
     */
    static VkDevice device();

    static constexpr uint8_t FRAMES_OVERLAP = 2;

    static VktTypes::AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    static VktTypes::AllocatedImage  createImage(VkExtent3D allocSize, VkFormat format, VkImageUsageFlags usage, bool mipMapped = false);
    static VktTypes::AllocatedImage  createImage(void* data, VkExtent3D allocSize, VkFormat format, VkImageUsageFlags usage, bool mipMapped = false);
    static void destroyBuffer(const VktTypes::AllocatedBuffer& buffer);
    static void destroyImage(const VktTypes::AllocatedImage& img);

    struct GLTFMetallicRoughness{
        VktTypes::MaterialPipeline opaquePipeline{};
        VktTypes::MaterialPipeline transparentPipeline{};

        VkDescriptorSetLayout materialLayout{};

        struct MaterialConstants {
            glm::vec4 colorFactors = {0.0f, 0.0f, 0.0f, 0.0f};
            glm::vec4 metalRoughFactors = {0.0f, 0.0f, 0.0f, 0.0f};
            glm::vec4 extra[14];
        };

        struct MaterialResources {
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

    struct ObjectHandle{

    };

    struct PerfStats{
        float frametime;
        uint32_t trigDrawCount;
        uint32_t drawCallCount;
        float sceneUpdateTime;
        float meshDrawTime;
    };

    // TODO REMOVE LATER
    VktTypes::AllocatedImage m_errorCheckboardImage{};
    VktTypes::AllocatedImage m_whiteImage{};
    VktTypes::AllocatedImage m_blackImage{};
    VktTypes::AllocatedImage m_greyImage{};
    VkSampler m_defaultSamplerLinear{};
    VkSampler m_defaultSamplerNearest{};
    GLTFMetallicRoughness m_metalRoughMaterial;

    std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;

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
    void drawImGui(VkCommandBuffer cmd, VkImageView targetView);
    void drawGeometry(VkCommandBuffer cmd);

    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& func);

    static VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                  VkDebugUtilsMessageTypeFlagsEXT type,
                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                  void* pUserData);

    void resizeSwapchain();

    void updateScene();

    bool m_isInitialized = false;
    uint32_t m_frameNumber = 0;

    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkExtent2D m_windowExtent = {.width = 0, .height = 0};
    Window* m_window = nullptr;

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_swapchainImageFormat = VK_FORMAT_UNDEFINED;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    VkExtent2D m_swapchainExtent = {.width = 0, .height = 0};
    bool m_resizeSwapchain = false;

    VktTypes::AllocatedImage m_drawImage;
    VktTypes::AllocatedImage m_depthImage;
    VkExtent2D m_drawExtent = {.width = 0, .height = 0};
    float m_renderScale = 1.0f;

    std::array<VktTypes::FrameData, FRAMES_OVERLAP> m_frames;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueFamily = 0;

    VktDeletableQueue m_coreDeletionQueue;

    VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;

    DescriptorAllocatorDynamic m_globDescriptorAllocator;
    VkDescriptorSet m_drawImageDescriptors = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_drawImageDescriptorLayout = VK_NULL_HANDLE;

    VkPipeline m_gradientPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_gradientPipelineLayout = VK_NULL_HANDLE;

    VkFence m_immFence = VK_NULL_HANDLE;
    VkCommandBuffer m_immCommandBuffer = VK_NULL_HANDLE;
    VkCommandPool m_immCommandPool = VK_NULL_HANDLE;

    std::vector<VktTypes::ComputeEffect> m_backgroundEffect;
    int m_currentBackgroundEffect = 0;

    VkPipelineLayout m_meshPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_meshPipeline = VK_NULL_HANDLE;

    VktTypes::GPUSceneData m_sceneData;
    VkDescriptorSetLayout m_gpuSceneDataDescriptorLayout{};

    VkDescriptorSetLayout m_singleImageDescriptorLayout{};

    VktTypes::DrawContext m_mainDrawContext;
    //std::unordered_map<std::string, std::shared_ptr<VktTypes::Node>> m_loadedNodes;

    glm::mat4 m_viewMatrix = glm::identity<glm::mat4>();
    glm::mat4 m_projMatrix = glm::identity<glm::mat4>();

    PerfStats m_stats;

    Logger m_logger = Logger("VulkanCore");
};

#endif //TECTONIC_VKTCORE_H
