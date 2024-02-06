#include "engine/vulkan/VulkanDestructQueue.h"
#include "engine/vulkan/VulkanDescriptors.h"

void DeletionQueue::setInstance(VkInstance instance) {
    m_instance = instance;
}

void DeletionQueue::setDevice(VkDevice device) {
    m_device = device;
}

void DeletionQueue::setVmaAllocator(VmaAllocator allocator) {
    m_vmaAllocator = allocator;
}

void DeletionQueue::flush() {
    // TODO handle cases of missing instance and allocator
    if(!m_device){
        throw vulkanException("Cannot flush without device");
    }

    // Each DeletableType has command to be executed on provided data
    for(auto it = m_delQueue.rbegin(); it != m_delQueue.rend(); it++){
        switch(it->type){
            case DeletableType::VK_IMAGE_VIEW:
                vkDestroyImageView(m_device, reinterpret_cast<VkImageView>(it->data), nullptr);
                break;
            case DeletableType::VK_COMMAND_POOL:
                vkDestroyCommandPool(m_device, reinterpret_cast<VkCommandPool>(it->data), nullptr);
                break;
            case DeletableType::VK_FENCE:
                vkDestroyFence(m_device, reinterpret_cast<VkFence>(it->data), nullptr);
                break;
            case DeletableType::VK_DESCRIPTOR_POOL:
                vkDestroyDescriptorPool(m_device, reinterpret_cast<VkDescriptorPool>(it->data), nullptr);
                break;
            case DeletableType::VK_DESCRIPTOR_SET_LAYOUT:
                vkDestroyDescriptorSetLayout(m_device, reinterpret_cast<VkDescriptorSetLayout>(it->data), nullptr);
                break;
            case DeletableType::VK_PIPELINE_LAYOUT:
                vkDestroyPipelineLayout(m_device, reinterpret_cast<VkPipelineLayout>(it->data), nullptr);
                break;
            case DeletableType::VK_PIPELINE:
                vkDestroyPipeline(m_device, reinterpret_cast<VkPipeline>(it->data), nullptr);
                break;
            case DeletableType::VK_DEBUG_UTILS_MESSENGER:
                VkUtils::DestroyDebugUtilsMessengerEXT(m_instance, reinterpret_cast<VkDebugUtilsMessengerEXT>(it->data), nullptr);
                break;
            case DeletableType::VK_SAMPLER:
                vkDestroySampler(m_device, reinterpret_cast<VkSampler>(it->data), nullptr);
                break;

            case DeletableType::VMA_ALLOCATOR:
                vmaDestroyAllocator(reinterpret_cast<VmaAllocator>(it->data));
                break;
            case DeletableType::VMA_IMAGE:
                vmaDestroyImage(m_vmaAllocator,
                                reinterpret_cast<VkImage>(it->data),
                                reinterpret_cast<VmaAllocation>(it->cont.at(0)));
                break;
            case DeletableType::VMA_BUFFER:
                vmaDestroyBuffer(m_vmaAllocator,
                                 reinterpret_cast<VkBuffer>(it->data),
                                 reinterpret_cast<VmaAllocation>(it->cont.at(0)));
                break;

            case DeletableType::TEC_DESCRIPTOR_ALLOCATOR_DYNAMIC:
                DescriptorAllocatorDynamic* allocator = reinterpret_cast<DescriptorAllocatorDynamic*>(it->data);
                allocator->destroyPool(m_device);
                break;
        }
    }
}

void DeletionQueue::pushDeletable(DeletableType type, void* data, std::vector<void*> &&cont) {
    if(cont.empty()){
        m_delQueue.push_back({type, data});
    }else{
        m_delQueue.push_back({type, data, cont});
    }
}
