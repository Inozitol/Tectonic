#ifndef TECTONIC_VKTTYPES_H
#define TECTONIC_VKTTYPES_H

#include "Logger.h"
#include "VktDeletableQueue.h"
#include "VktDescriptorUtils.h"
#include "extern/imgui/imgui_impl_glfw.h"
#include <glm/detail/type_quat.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vk_mem_alloc.h>

namespace VktTypes{
    namespace Resources {
        /** @brief Abstraction over Vma buffer allocation. */
        struct Buffer{
            VkBuffer buffer = VK_NULL_HANDLE;
            VmaAllocation allocation = VK_NULL_HANDLE;
            VmaAllocationInfo info{};
        };

        /** @brief Abstraction over Vma image allocation.
         *
         * For cubemap images when isCubemap is true, the layers should be 6 with each cube side.
         */
        struct Image{
            VkImage image = VK_NULL_HANDLE;
            VkImageView view = VK_NULL_HANDLE;
            VmaAllocation allocation = VK_NULL_HANDLE;
            VmaAllocationInfo info{};
            VkExtent3D extent {.width = 0, .height = 0, .depth = 0};
            VkFormat format = VK_FORMAT_UNDEFINED;
            VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;    // TODO use this
            uint32_t mipLevels = 1;
            uint32_t layers = 1;
            bool isCubemap = false;
        };
    }

    namespace GPU {
        /** @brief Buffers on GPU holding mesh data */
        struct MeshBuffers{
            Resources::Buffer indexBuffer{};
            Resources::Buffer vertexBuffer{};
            VkDeviceAddress vertexBufferAddress = 0;
        };

        /** Helper constants to make templated structs more verbose. */
        inline constexpr bool Static = false;
        inline constexpr bool Skinned = true;

        /**
         * @brief Push constants pushed to GPU every frame.
         * @tparam S True if it should contain an address to joint matrices.
         *
         * This contains the world matrix and GPU pointers to vertices and joints matrices (if available).
         */
        template<bool S>
        struct DrawPushConstants;

        template<>
        struct DrawPushConstants<Static>{
            alignas(16) glm::mat4 worldMatrix = glm::identity<glm::mat4>();
            VkDeviceAddress vertexBuffer = 0;
        };

        template<>
        struct DrawPushConstants<Skinned>{
            alignas(16) glm::mat4 worldMatrix = glm::identity<glm::mat4>();
            VkDeviceAddress vertexBuffer = 0;
            VkDeviceAddress jointsBuffer = 0;
        };

        enum class VertexType {
            STATIC,     // For statically placed verticies of 3D meshes
            SKINNED,    // For skinned 3D meshes
            POINT       // For colored 3D points
        };

        /**
         * @brief GPU format of vertex data.
         * @tparam vType Type of vertex data.
         */
        template<VertexType vType>
        struct Vertex;

        /** @brief Graphics pipeline vertex */
        template<>
        struct Vertex<VertexType::STATIC>{
            alignas(16) glm::vec3 position = {0.0f, 0.0f, 0.0f};
            alignas(4) float uvX = 0.0f;
            alignas(16) glm::vec3 normal = {1.0f, 0.0f, 0.0f};
            alignas(4) float uvY = 0.0f;
            alignas(16) glm::vec3 color = {1.0f, 1.0f, 1.0f};
        };

        /** @brief Graphics pipeline vertex with joints data. */
        template<>
        struct Vertex<VertexType::SKINNED>{
            alignas(16) glm::vec3 position = {0.0f, 0.0f, 0.0f};
            alignas(4) float uvX = 0.0f;
            alignas(16)glm::vec3 normal = {1.0f, 0.0f, 0.0f};
            alignas(4) float uvY = 0.0f;
            alignas(16) glm::vec3 color = {1.0f, 1.0f, 1.0f};

            alignas(16) glm::uvec4 jointIndices = {0, 0, 0, 0};
            alignas(16) glm::vec4 jointWeights = {0.0f, 0.0f, 0.0f, 0.0f};
        };

        /** @brief Graphics pipeline vertex for colored point */
        template<>
        struct Vertex<VertexType::POINT>{
            alignas(16) glm::vec3 position = {0.0f, 0.0f, 0.0f};
            alignas(16) glm::vec3 color = {1.0f, 1.0f, 1.0f};
        };


        /** @brief Buffer on GPU holding joints data */
        struct JointsBuffers{
            Resources::Buffer jointsBuffer{};
            VkDeviceAddress jointsBufferAddress = 0;
        };

        /** @brief Environmental scene data. */
        struct SceneData {
            alignas(16) glm::mat4 view = glm::identity<glm::mat4>();
            alignas(16) glm::mat4 proj  = glm::identity<glm::mat4>();
            alignas(16) glm::mat4 viewproj = glm::identity<glm::mat4>();
            alignas(16) glm::vec3 ambientColor = {0.0f, 0.0f, 0.0f};
            alignas(16) glm::vec3 sunlightDirection = {0.0f, 0.0f, 0.0f};
            alignas(16) glm::vec3 sunlightColor = {0.0f, 0.0f, 0.0f};
            alignas(16) glm::vec3 cameraPosition = {0.0f, 0.0f, 0.0f};
            alignas(16) glm::vec3 cameraDirection = {0.0f, 0.0f, 0.0f};
            alignas(4) float time = 0.0f;
        };

        /** @brief Buffer with roughness value used in IBL specular rendering */
        struct RoughnessBuffer {
            float roughness;
        };

        /** @brief Buffer with resolution data */
        struct ResolutionBuffer {
            glm::vec2 data;
        };

    }

    struct PointMesh {
        std::vector<GPU::Vertex<GPU::VertexType::POINT>> vertices;
        std::vector<uint32_t> indices;
        GPU::MeshBuffers meshBuffers;
    };

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
        Resources::Buffer sceneUniformBuffer{};
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
        VktTypes::GPU::MeshBuffers meshBuffers;
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

        /**
         * @brief Metallic-Roughness factors
         *
         * The data has to be aligned at 64 bytes (0x40), due to Vulkan constraint.
         */
        struct MaterialConstants {
            enum class Flags : uint64_t{
                ColorTex             = 1 << 0,
                MetallicRoughnessTex = 1 << 1,
            };
            glm::vec4 colorFactors = {0.0f, 0.0f, 0.0f, 0.0f};
            glm::vec2 metalRoughFactors = {0.0f, 0.0f};
            Flags bitFlags{};
            std::array<glm::vec4,14> extra;
        };

        /** @brief Handles for material textures and buffers located on GPU. */
        struct MaterialResources {
            VktTypes::Resources::Image colorImage;
            VkSampler colorSampler = VK_NULL_HANDLE;
            VktTypes::Resources::Image metalRoughImage;
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
