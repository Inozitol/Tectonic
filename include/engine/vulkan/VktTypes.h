#ifndef TECTONIC_VKTTYPES_H
#define TECTONIC_VKTTYPES_H

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "extern/imgui/imgui_impl_glfw.h"
#include "VktDeletableQueue.h"
#include "VktDescriptors.h"
#include "meta/meta.h"


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
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;
    };

    /** @brief Abstraction over Vma image allocation. */
    struct AllocatedImage{
        VkImage image;
        VkImageView view;
        VmaAllocation allocation;
        VkExtent3D extent;
        VkFormat format;
    };

    /** @brief Graphics pipeline vertex. */
    struct Vertex{
        glm::vec3 position;
        float uvX;
        glm::vec3 normal;
        float uvY;
        glm::vec4 color;
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
        AllocatedBuffer indexBuffer;
        AllocatedBuffer vertexBuffer;
        VkDeviceAddress vertexBufferAddress;
    };

    struct GPUDrawPushConstants{
        glm::mat4 worldMatrix = glm::mat4(1.0f);
        VkDeviceAddress vertexBuffer{};
    };

    struct GPUSceneData{
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewproj;
        glm::vec4 ambientColor;
        glm::vec4 sunlightDirection;
        glm::vec4 sunlightColor;
    };

    struct MaterialPipeline{
        VkPipeline pipeline;
        VkPipelineLayout layout;
    };

    enum class MaterialPass: uint8_t{
        OPAQUE,
        TRANSPARENT,
        OTHER
    };

    struct MaterialInstance{
        MaterialPipeline* pipeline;
        VkDescriptorSet materialSet;
        MaterialPass passType;
    };

    struct RenderObject{
        uint32_t indexCount;
        uint32_t firstIndex;
        VkBuffer indexBuffer;

        MaterialInstance* material;

        glm::mat4 transform = glm::identity<glm::mat4>();
        VkDeviceAddress vertexBufferAddress;
    };

}

#endif //TECTONIC_VKTTYPES_H
