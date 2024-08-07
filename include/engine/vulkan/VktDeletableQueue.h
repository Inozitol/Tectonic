#ifndef TECTONIC_VKTDELETABLEQUEUE_H
#define TECTONIC_VKTDELETABLEQUEUE_H

#include <deque>
#include <functional>
#include <optional>
#include <span>
#include <vulkan/vulkan.h>

#include "VktUtils.h"
#include "exceptions.h"
#include "extern/vma/vk_mem_alloc.h"

enum class DeletableType {
    /**
     * Destroys VkImageView \n
     * data: VkImageView \n
     * cont: {}
     */
    VK_IMAGE_VIEW,

    /**
     * Destroys VkCommandPool \n
     * data: VkCommandPool \n
     * cont: {}
     */
    VK_COMMAND_POOL,

    /**
     * Destroys VkFence \n
     * data: VkFence \n
     * cont: {}
     */
    VK_FENCE,

    /**
     * Destroys VkDescriptorPool \n
     * data: VkDescriptorPool \n
     * cont: {}
     */
    VK_DESCRIPTOR_POOL,

    /**
     * Destroys VkDescriptorSetLayout \n
     * data: VkDescriptorSetLayout \n
     * cont: {}
     */
    VK_DESCRIPTOR_SET_LAYOUT,

    /**
     * Destroys VkPipelineLayout \n
     * data: VkPipelineLayout \n
     * cont: {}
     */
    VK_PIPELINE_LAYOUT,

    /**
     * Destroys VkPipeline \n
     * data: VkPipeline \n
     * cont: {}
     */
    VK_PIPELINE,

    /**
     * Destroys VkDebugUtilsMessengerEXT \n
     * data: VkDebugUtilsMessengerEXT \n
     * cont: {}
     */
    VK_DEBUG_UTILS_MESSENGER,

    /**
     * Destroys VkSampler \n
     * data: VkSampler \n
     * cont: {}
     */
    VK_SAMPLER,

    /**
     * Destroys VmaAllocator \n
     * data: VmaAllocator \n
     * cont: {}
     */
    VMA_ALLOCATOR,

    /**
     * Destroys VkImage \n
     * data: VkImage \n
     * cont: {VmaAllocation}
     */
    VMA_IMAGE,

    /**
     * Destroys VkBuffer \n
     * data: VkBuffer \n
     * cont: {VmaAllocation}
     */
    VMA_BUFFER,

    /**
     * Destroys DescriptorAllocatorDynamic \n
     * data: DescriptorAllocatorDynamic* \n
     * cont: {}
     */
    TEC_DESCRIPTOR_ALLOCATOR_DYNAMIC,

    /**
     * Destroys VktTypes::Resources::Buffer \n
     * data: VktTypes::Resources::Buffer \n
     * cont: {}
     */
    TEC_RESOURCE_BUFFER,

    /**
     * Destroys VktTypes::Resources::Image \n
     * data: VktTypes::Resources::Image \n
     * cont: {}
     */
    TEC_RESOURCE_IMAGE,

    /**
     * Destroys VktTypes::Resources::Cubemap \n
     * data: VktTypes::Resources::Cubemap \n
     * cont: {}
     */
    TEC_RESOURCE_CUBEMAP
};

class VktDeletableQueue {
public:
    VktDeletableQueue() = default;

    void setInstance(VkInstance instance);
    void setDevice(VkDevice device);
    void setVmaAllocator(VmaAllocator allocator);
    void pushDeletable(DeletableType type, void *data, std::vector<void *> &&cont = {});
    void flush();

private:
    struct VktDeletable {
        DeletableType type{};
        void *data{};
        std::vector<void *> cont;
    };

    VkInstance m_instance = nullptr;
    VkDevice m_device = nullptr;
    VmaAllocator m_vmaAllocator = nullptr;
    std::deque<VktDeletable> m_delQueue;
};


#endif//TECTONIC_VKTDELETABLEQUEUE_H
