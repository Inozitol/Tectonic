#ifndef TECTONIC_VULKANTYPES_H
#define TECTONIC_VULKANTYPES_H

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "VulkanDestructQueue.h"
#include "VulkanDescriptors.h"

class VulkanCore;

namespace VkTypes{

    struct AllocatedImage{
        VkImage image;
        VkImageView view;
        VmaAllocation allocation;
        VkExtent3D extent;
        VkFormat format;
    };

    struct ComputePushConstants{
        glm::vec4 data1;
        glm::vec4 data2;
        glm::vec4 data3;
        glm::vec4 data4;
    };

    struct ComputeEffect{
        const char* name;

        VkPipeline pipeline;
        VkPipelineLayout layout;

        ComputePushConstants data;
    };

    struct AllocatedBuffer{
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;
    };

    struct Vertex{
        glm::vec3 position;
        float uvX;
        glm::vec3 normal;
        float uvY;
        glm::vec4 color;
    };

    struct FrameData{
        VkCommandPool commandPool{};
        VkCommandBuffer mainCommandBuffer{};

        VkSemaphore swapchainSemaphore{};
        VkSemaphore renderSemaphore{};
        VkFence renderFence{};

        DeletionQueue deletionQueue;
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

#endif //TECTONIC_VULKANTYPES_H
