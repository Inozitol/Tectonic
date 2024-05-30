#ifndef TECTONIC_VKTTYPES_H
#define TECTONIC_VKTTYPES_H

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <memory>
#include <glm/detail/type_quat.hpp>
#include "extern/imgui/imgui_impl_glfw.h"
#include "VktDeletableQueue.h"
#include "VktDescriptors.h"
#include "Logger.h"

namespace VktTypes{

    /** Very work-in-progress push constants. */
    struct ComputePushConstants{
        glm::vec4 data1;
        glm::vec4 data2;
        glm::vec4 data3;
        glm::vec4 data4;
    };

    /** @brief Abstraction over Vulkan compute pipeline.
     *
     * Holds a whole pipeline and pipeline layout.
     */
    struct ComputeEffect{
        const char* name;

        VkPipeline pipeline;
        VkPipelineLayout layout;

        ComputePushConstants data;
    };

    /** @brief Abstraction over Vma buffer allocation. */
    struct AllocatedBuffer{
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocationInfo info{};
    };

    /** @brief Abstraction over Vma image allocation. */
    struct AllocatedImage{
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkExtent3D extent {.width = 0, .height = 0, .depth = 0};
        VkFormat format = VK_FORMAT_UNDEFINED;
    };

    /** @brief Graphics pipeline vertex. */
    struct Vertex{
        glm::vec3 position = {0.0f, 0.0f, 0.0f};
        float uvX = 0.0f;
        glm::vec3 normal = {1.0f, 0.0f, 0.0f};
        float uvY = 0.0f;
        glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};

        glm::vec4 jointIndices = {0.0f, 0.0f, 0.0f, 0.0f};
        glm::vec4 jointWeights = {0.0f, 0.0f, 0.0f, 0.0f};
    };

    /**
     * @brief Holds data needed for Vulkan to render a single frame.
     *
     * Because the rendering is overlapped and parallelized over multiple frames (see VktCore::FRAMES_OVERLAP),
     * the engine needs to hold separate data for each of those frames.
     */
    struct FrameData{
        VkCommandPool commandPool{};
        VkCommandBuffer mainCommandBuffer{};

        VkSemaphore swapchainSemaphore{};
        VkSemaphore renderSemaphore{};
        VkFence renderFence{};

        VktDeletableQueue deletionQueue;
        DescriptorAllocatorDynamic descriptors;
        AllocatedBuffer sceneUniformBuffer{};
    };

    struct GPUMeshBuffers{
        AllocatedBuffer indexBuffer{};
        AllocatedBuffer vertexBuffer{};
        VkDeviceAddress vertexBufferAddress = 0;
    };

    struct GPUJointsBuffers{
        AllocatedBuffer jointsBuffer{};
        VkDeviceAddress jointsBufferAddress = 0;
    };

    /**
     * @brief Push constants pushed to GPU every frame.
     *
     * This contains the world matrix and GPU pointers to vertices and joints matrices (if available).
     */
    struct GPUDrawPushConstants{
        glm::mat4 worldMatrix = glm::mat4(1.0f);
        VkDeviceAddress vertexBuffer = 0;
        VkDeviceAddress jointsBuffer = 0;
    };

    /**
     * @brief Environmental scene data.
     */
    struct GPUSceneData{
        glm::mat4 view = glm::identity<glm::mat4>();
        glm::mat4 proj  = glm::identity<glm::mat4>();;
        glm::mat4 viewproj = glm::identity<glm::mat4>();;
        glm::vec4 ambientColor = {0.0f, 0.0f, 0.0f, 0.0f};
        glm::vec4 sunlightDirection = {0.0f, 0.0f, 0.0f, 0.0f};
        glm::vec4 sunlightColor = {0.0f, 0.0f, 0.0f, 0.0f};
    };

    /**
     * @brief Vulkan pipeline for model rendering.
     */
    struct ModelPipeline {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
    };

    enum class MaterialPass: uint8_t{
        OPAQUE,
        TRANSPARENT,
        OTHER
    };

    struct MaterialInstance{
        ModelPipeline* pipeline = nullptr;
        VkDescriptorSet materialSet = VK_NULL_HANDLE;
        MaterialPass passType = MaterialPass::OTHER;
    };

    struct RenderObject{
        uint32_t indexCount = 0;
        uint32_t firstIndex = 0;
        VkBuffer indexBuffer = VK_NULL_HANDLE;

        const MaterialInstance* material = nullptr;

        glm::mat4 transform = glm::identity<glm::mat4>();
        VkDeviceAddress vertexBufferAddress = 0;
        VkDeviceAddress jointsBufferAddress = 0;
    };

    struct DrawContext{
        std::vector<RenderObject> opaqueSurfaces;
        std::vector<RenderObject> transparentSurfaces;
    };

    struct GLTFMaterial{
        explicit GLTFMaterial(VktTypes::MaterialInstance data): data(data){}
        GLTFMaterial() = default;
        VktTypes::MaterialInstance data;
    };

    struct GeoSurface{
        uint32_t startIndex;
        uint32_t count;
        const GLTFMaterial* material;
    };

    struct MeshAsset{
        std::vector<GeoSurface> surfaces;
        VktTypes::GPUMeshBuffers meshBuffers;
    };

    struct Skin;

    struct Node{
        const Node* parent = nullptr;
        std::vector<Node*> children;
        const MeshAsset* mesh = nullptr;
        std::string name;

        glm::mat4 localTransform = glm::identity<glm::mat4>();
        //glm::mat4 worldTransform;

        glm::vec3 translation;
        glm::vec3 scale{1.0f};
        glm::quat rotation{};
        Skin* skin = nullptr;

        glm::mat4 localMatrix() const;
        glm::mat4 nodeMatrix() const;

        /**
         * @brief Fills the DrawContext with rendering data.
         * @param root Root of the tree.
         * @param topMatrix Transformation matrix.
         * @param ctx DrawContext to fill with rendering data.
         */
        static void gatherContext(const Node& root, const glm::mat4& topMatrix, const std::vector<VktTypes::GPUJointsBuffers>& jointsBuffers, DrawContext& ctx);

        static void updateJoints(const Node& root, const std::vector<VktTypes::GPUJointsBuffers>& jointsBuffers);
    };

    struct Skin{
        std::size_t index;
        std::string name;
        Node* skeletonRoot = nullptr;
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<Node*> joints;
    };

    struct AnimationSampler{
        enum class Interpolation{
            LINEAR,
            STEP,
            CUBICSPLINE
        };

        Interpolation interpolation;
        std::vector<float> inputs;
        std::vector<glm::vec4> outputsVec4;
    };

    struct AnimationChannel{
        enum class Path{
            TRANSLATION,
            ROTATION,
            SCALE,
            WEIGHTS
        };
        Path path;
        Node* node;
        uint32_t samplerIndex;
    };

    struct Animation{
        std::string name;
        std::vector<AnimationSampler> samplers;
        std::vector<AnimationChannel> channels;
        float start = std::numeric_limits<float>::max();
        float end = std::numeric_limits<float>::min();
        float currTime = 0.0f;
    };

}

#endif //TECTONIC_VKTTYPES_H
