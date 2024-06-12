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

    /** @brief Abstraction over Vma allocated cube map. */
    struct AllocatedCubeMap{
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkExtent3D extent {.width = 0, .height = 0, .depth = 0}; // Defined for all 6 sides
        VkFormat format = VK_FORMAT_UNDEFINED;
    };

    /** Helper constants to make templated structs more verbose. */
    inline constexpr bool Static = false;
    inline constexpr bool Skinned = true;

    /**
     * @brief GPU format of vertex data.
     * @tparam S True if the vertex contains joints data.
     */
    template<bool S = Skinned>
    struct Vertex;

    /** @brief Graphics pipeline vertex */
    template<>
    struct Vertex<Static>{
        glm::vec3 position = {0.0f, 0.0f, 0.0f};
        float uvX = 0.0f;
        glm::vec3 normal = {1.0f, 0.0f, 0.0f};
        float uvY = 0.0f;
        glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    };

    /** @brief Graphics pipeline vertex with joints data. */
    template<>
    struct Vertex<Skinned>{
        glm::vec3 position = {0.0f, 0.0f, 0.0f};
        float uvX = 0.0f;
        glm::vec3 normal = {1.0f, 0.0f, 0.0f};
        float uvY = 0.0f;
        glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};

        glm::uvec4 jointIndices = {0, 0, 0, 0};
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

    /** @brief Buffers on GPU holding mesh data */
    struct GPUMeshBuffers{
        AllocatedBuffer indexBuffer{};
        AllocatedBuffer vertexBuffer{};
        VkDeviceAddress vertexBufferAddress = 0;
    };

    /** @brief Buffer on GPU holding joints data */
    struct GPUJointsBuffers{
        AllocatedBuffer jointsBuffer{};
        VkDeviceAddress jointsBufferAddress = 0;
    };

    /**
     * @brief Push constants pushed to GPU every frame.
     * @tparam S True if it should contain an address to joint matrices.
     *
     * This contains the world matrix and GPU pointers to vertices and joints matrices (if available).
     */
    template<bool S>
    struct GPUDrawPushConstants;

    template<>
    struct GPUDrawPushConstants<Static>{
        glm::mat4 worldMatrix = glm::identity<glm::mat4>();
        VkDeviceAddress vertexBuffer = 0;
    };

    template<>
    struct GPUDrawPushConstants<Skinned>{
        glm::mat4 worldMatrix = glm::identity<glm::mat4>();
        VkDeviceAddress vertexBuffer = 0;
        VkDeviceAddress jointsBuffer = 0;
    };

    /** @brief Environmental scene data. */
    struct GPUSceneData{
        glm::mat4 view = glm::identity<glm::mat4>();
        glm::mat4 proj  = glm::identity<glm::mat4>();
        glm::mat4 viewproj = glm::identity<glm::mat4>();
        glm::vec3 ambientColor = {0.0f, 0.0f, 0.0f};
        glm::vec3 sunlightDirection = {0.0f, 0.0f, 0.0f};
        glm::vec3 sunlightColor = {0.0f, 0.0f, 0.0f};
        glm::vec3 cameraPosition = {0.0f, 0.0f, 0.0f};
        glm::vec3 cameraDirection = {0.0f, 0.0f, 0.0f};
        float time = 0.0f;
    };

    /** @brief Holds index, size and material index to buffers inside GPU */
    struct MeshSurface{
        uint32_t startIndex;
        uint32_t count;
        uint32_t materialIndex;
    };

    /** @brief Mesh surfaces with GPU buffers reference */
    struct MeshAsset{
        std::vector<MeshSurface> surfaces;
        VktTypes::GPUMeshBuffers meshBuffers;
    };

    /** @brief Vulkan pipeline for model rendering. */
    struct ModelPipeline {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
    };

    /** @brief Different variations on material rendering. */
    enum class MaterialPass: uint8_t{
        OPAQUE,
        TRANSPARENT,
        OTHER
    };

    /** @brief Structure holding data required to render a glTF Metallic-Roughness PBR material. */
    struct GLTFMetallicRoughness{
        // TODO This isn't scalable, handle it better
        VktTypes::ModelPipeline opaquePipeline{};
        VktTypes::ModelPipeline transparentPipeline{};
        VktTypes::ModelPipeline skinnedOpaquePipeline{};
        VktTypes::ModelPipeline skinnedTransparentPipeline{};

        VkDescriptorSetLayout materialLayout{};

        /**
         * @brief Metallic-Roughness factors
         *
         * The data has to be aligned at 64 bytes (0x40), due to Vulkan constraint.
         */
        struct MaterialConstants {
            glm::vec4 colorFactors = {0.0f, 0.0f, 0.0f, 0.0f};
            glm::vec4 metalRoughFactors = {0.0f, 0.0f, 0.0f, 0.0f};
            std::array<glm::vec4,14> extra;
        };

        /** @brief Handles for material textures and buffers located on GPU. */
        struct MaterialResources {
            VktTypes::AllocatedImage colorImage;
            VkSampler colorSampler = VK_NULL_HANDLE;
            VktTypes::AllocatedImage metalRoughImage;
            VkSampler metalRoughSampler = VK_NULL_HANDLE;
            VkBuffer dataBuffer = VK_NULL_HANDLE;
            uint32_t dataBufferOffset = 0;
        };

        DescriptorWriter writer;
    };

    /** @brief Instance of a single material, generated after loading. */
    struct MaterialInstance{
        VkDescriptorSet materialSet = VK_NULL_HANDLE;
        MaterialPass passType = MaterialPass::OTHER;
        const ModelPipeline* pipeline = nullptr;
    };

    /** @brief Structure that contains all the necessary data to render a mesh. */
    struct RenderObject{
        uint32_t indexCount = 0;
        uint32_t firstIndex = 0;
        VkBuffer indexBuffer = VK_NULL_HANDLE;

        bool isSkinned = false;
        const MaterialInstance* material = nullptr;

        glm::mat4 transform = glm::identity<glm::mat4>();

        VkDeviceAddress vertexBufferAddress = 0;
        VkDeviceAddress jointsBufferAddress = 0;
    };

    /** @brief Separates various kinds of material passes to separate vectors for optimized drawing. */
    struct DrawContext{
        std::vector<RenderObject> opaqueSurfaces;
        std::vector<RenderObject> transparentSurfaces;
    };

}

#endif //TECTONIC_VKTTYPES_H
