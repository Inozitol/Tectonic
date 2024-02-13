#ifndef TECTONIC_VKTTYPES_H
#define TECTONIC_VKTTYPES_H

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <memory>
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
        glm::vec4 color = {0.0f, 0.0f, 0.0f, 0.0f};
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

    struct GPUDrawPushConstants{
        glm::mat4 worldMatrix = glm::mat4(1.0f);
        VkDeviceAddress vertexBuffer = 0;
    };

    struct GPUSceneData{
        glm::mat4 view = glm::identity<glm::mat4>();
        glm::mat4 proj  = glm::identity<glm::mat4>();;
        glm::mat4 viewproj = glm::identity<glm::mat4>();;
        glm::vec4 ambientColor = {0.0f, 0.0f, 0.0f, 0.0f};
        glm::vec4 sunlightDirection = {0.0f, 0.0f, 0.0f, 0.0f};
        glm::vec4 sunlightColor = {0.0f, 0.0f, 0.0f, 0.0f};
    };

    struct MaterialPipeline{
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
    };

    enum class MaterialPass: uint8_t{
        OPAQUE,
        TRANSPARENT,
        OTHER
    };

    struct MaterialInstance{
        MaterialInstance() = default;
        MaterialPipeline* pipeline = nullptr;
        VkDescriptorSet materialSet = VK_NULL_HANDLE;
        MaterialPass passType = MaterialPass::OTHER;
    };

    struct RenderObject{
        uint32_t indexCount = 0;
        uint32_t firstIndex = 0;
        VkBuffer indexBuffer = VK_NULL_HANDLE;

        MaterialInstance* material = nullptr;

        glm::mat4 transform = glm::identity<glm::mat4>();
        VkDeviceAddress vertexBufferAddress = 0;
    };

    struct DrawContext{
        std::vector<RenderObject> opaqueSurfaces;
        std::vector<RenderObject> transparentSurfaces;
    };

    class IRenderable{
        virtual void draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
    };

    struct Node : public IRenderable{
        std::weak_ptr<Node> parent;
        std::vector<std::shared_ptr<Node>> children;

        glm::mat4 localTransform;
        glm::mat4 worldTransform;

        void refreshTransform(const glm::mat4& parentMatrix){
            worldTransform = parentMatrix * localTransform;
            for(const auto& c : children){
                c->refreshTransform(worldTransform);
            }
        }

        virtual void draw(const glm::mat4& topMatrix, DrawContext& ctx) {
            for(auto& c : children){
                c->draw(topMatrix, ctx);
            }
        }
    };
}

#endif //TECTONIC_VKTTYPES_H
