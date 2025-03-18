#ifndef TECTONIC_VKTCORE_H
#define TECTONIC_VKTCORE_H

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/gtx/transform.hpp>

#include <deque>
#include <memory>

#include "extern/vkbootstrap/VkBootstrap.h"

#include "extern/imgui/imgui.h"
#include "extern/imgui/imgui_impl_glfw.h"
#include "extern/imgui/imgui_impl_vulkan.h"
#include <vk_mem_alloc.h>

#include "Logger.h"
#include "Transformation.h"
#include "VktBuffers.h"
#include "VktDeletableQueue.h"
#include "VktDescriptorUtils.h"
#include "VktImages.h"
#include "VktInstantCommands.h"
#include "VktPipelines.h"
#include "VktStructs.h"
#include "VktTypes.h"
#include "VktUtils.h"
#include "engine/Window.h"
#include "engine/model/Model.h"
#include "engine/model/ModelTypes.h"
#include "exceptions.h"

struct VktCore {
    VktCore() = default;
    ~VktCore();

    /**
     * @brief Initializes Vulkan.
     * @note Has to be called after setWindow.
     */
    void init();

    /**
     * @brief De-initializes Vulkan.
     */
    void clear();

    /** @brief Initialized window dimensions from existing Window struct in memory. */
    void setExtentDimensions();

    /** @brief Creates MeshBuffers for the provided mesh data, uploads the primitives and returns created buffers. */
    template <VktTypes::GPU::VertexType vType>
    static VktTypes::GPU::MeshBuffers createPrimitivesDeviceMemory(const std::span<uint32_t>& indices, const std::span<VktTypes::GPU::Vertex<vType>>& vertices);

    /** @brief Creates MeshBuffers for the provided mesh data, uploads the primitives and returns created host-visible buffers. */
    template <VktTypes::GPU::VertexType vType>
    static VktTypes::GPU::MeshBuffers createPrimitivesHostVisible(const std::span<uint32_t>& indices, const std::span<VktTypes::GPU::Vertex<vType>>& vertices);


    /** @brief Uploads a primitives into the provided host-visible buffers. */
    template <VktTypes::GPU::VertexType vType>
    static void uploadPrimitivesToHostVisible(const VktTypes::GPU::MeshBuffers& meshBuffers, std::span<uint32_t> indices, std::span<VktTypes::GPU::Vertex<vType>> vertices);


    /** @brief Uploads joint matrices */
    static VktTypes::GPU::JointsBuffers uploadJoints(const std::span<glm::mat4>& jointMatrices);

    /**
     * @brief Checks if the Window inside should close.
     * @return True if the m_window wants to close.
     *
     * It only calls the Window::shouldClose. It would be better to check it from the actual Window,
     * or setup a signal to get the data without polling glfwWindowShouldClose.
     */
    bool shouldClose();

    /**
     * @brief Main run function that should be called every frame.
     */
    void run();

    /**
     * @brief Returns current VkDevice (if initialized).
     * @return Current VkDevice.
     */
    static VkDevice device();

    /**
     * @brief Returns current VMA allocator (if initialized).
     * @return Current VMA allocator.
     */
    static VmaAllocator allocator();

    /** Number of in-flight frames being generated in parallel */
    static constexpr uint8_t FRAMES_OVERLAP = 2;

    VktTypes::MaterialInstance writeMaterial(VktTypes::MaterialPass pass, const VktTypes::GLTFMetallicRoughness::MaterialResources& resources, DescriptorAllocatorDynamic& descriptorAllocator, bool isSkinned);


    using objectID_t = uint32_t;

    /**
     * @brief Represents a objects created from 3D model.
     */
    struct EngineObject {
        objectID_t objectID = 0;
        static objectID_t lastID;

        std::string name;
        Model* model;
    };

    VktCore::EngineObject* createObject(const std::string& name, const std::filesystem::path& filePath);
    VktCore::EngineObject* createObject(const std::string& name, Model* model);

    /**
     * @brief Stores performance measurements
     */
    struct PerfStats {
        float frametime = 0.0f;
        uint32_t trigDrawCount = 0;
        uint32_t drawCallCount = 0;
        float sceneUpdateTime = 0.0;
        float meshDrawTime = 0.0;
    };

    /**
     * @brief Various configurations to enable debugging information in pipeline
     */
    struct DebugConfig {
        bool enableDebugNormals = false;
        bool enableDebugVectors = true;
    };

    uint32_t addDebugPointMesh(VktTypes::PointMesh* pointMesh);

    // TODO REMOVE LATER
    VktTypes::Resources::Image m_errorCheckboardImage{};
    VktTypes::Resources::Image m_whiteImage{};
    VktTypes::Resources::Image m_blackImage{};
    VktTypes::Resources::Image m_greyImage{};
    VktTypes::GLTFMetallicRoughness metalRoughMaterial;

    std::unordered_map<objectID_t, EngineObject> loadedObjects;

    void initVulkan();
    void initSwapchain();
    void initCommands();
    void initSyncStructs();
    void initDescriptors();
    void initPipelines();
    void initDebugPipeline();
    void initMaterialPipelines();
    void initImGui();
    void initDefaultData();

    void createSwapchain(uint32_t width, uint32_t height);
    void destroySwapchain();

    VktTypes::FrameData& getCurrentFrame();

    void draw();

    void drawImGui(VkCommandBuffer cmd, VkImageView targetView);
    void drawGeometry(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptorSet);
    void drawDebugNormals(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptorSet);
    void drawDebugLines(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptorSet);

    static VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    void resizeSwapchain();
    void updateScene();

    bool m_isInitialized = false;
    uint32_t m_frameNumber = 0;

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkExtent2D m_windowExtent = {.width = 0, .height = 0};

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_swapchainImageFormat = VK_FORMAT_UNDEFINED;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    VkExtent2D m_swapchainExtent = {.width = 0, .height = 0};
    bool m_resizeSwapchain = false;

    VktTypes::Resources::Image m_drawImage;
    VktTypes::Resources::Image m_depthImage;
    float m_renderScale = 1.0f;

    std::array<VktTypes::FrameData, FRAMES_OVERLAP> m_frames;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueFamily = 0;

    VktDeletableQueue m_coreDeletionQueue;

    VkDescriptorSet m_drawImageDescriptors = VK_NULL_HANDLE;

    VktTypes::DrawContext m_mainDrawContext;

    VktTypes::ModelPipeline m_normalsDebugStaticPipeline;
    VktTypes::ModelPipeline m_normalsDebugSkinnedPipeline;
    VktTypes::ModelPipeline m_lineStripDebugPipeline;

    PerfStats stats;
    DebugConfig debugConf;

    uint32_t m_lastPointMeshIndex = 0;
    std::unordered_map<uint32_t, VktTypes::PointMesh*> m_debugLines;

    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;

    Slot<int32_t, int32_t> slt_windowResize{
        [this](int32_t, int32_t) {
            m_resizeSwapchain = true;
        }
    };

    Slot<> slt_polygonModeToggle{
        [this]() {
            if (polygonMode == VK_POLYGON_MODE_FILL) {
                polygonMode = VK_POLYGON_MODE_LINE;
            } else {
                polygonMode = VK_POLYGON_MODE_FILL;
            }

            /*VktInstantCommands::submitCommands([this](VkCommandBuffer cmd) {
                VktUtils::CmdSetPolygonModeEXT(cmd,polygonMode);
            });*/
        }
    };
};

#endif//TECTONIC_VKTCORE_H
