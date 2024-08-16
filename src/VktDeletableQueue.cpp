#include "engine/vulkan/VktDeletableQueue.h"

#include "engine/vulkan/VktDescriptorUtils.h"
#include "engine/vulkan/VktTypes.h"
#include "engine/vulkan/VktCache.h"

void VktDeletableQueue::flush() {
    // Each DeletableType has command to be executed on provided data
    for(auto it = m_delQueue.rbegin(); it != m_delQueue.rend(); it++){
        switch(it->type){
            case DeletableType::VK_IMAGE_VIEW:
                vkDestroyImageView(VktCache::vkDevice, reinterpret_cast<VkImageView>(it->data), nullptr);
                break;
            case DeletableType::VK_COMMAND_POOL:
                vkDestroyCommandPool(VktCache::vkDevice, reinterpret_cast<VkCommandPool>(it->data), nullptr);
                break;
            case DeletableType::VK_FENCE:
                vkDestroyFence(VktCache::vkDevice, reinterpret_cast<VkFence>(it->data), nullptr);
                break;
            case DeletableType::VK_DESCRIPTOR_POOL:
                vkDestroyDescriptorPool(VktCache::vkDevice, reinterpret_cast<VkDescriptorPool>(it->data), nullptr);
                break;
            case DeletableType::VK_DESCRIPTOR_SET_LAYOUT:
                vkDestroyDescriptorSetLayout(VktCache::vkDevice, reinterpret_cast<VkDescriptorSetLayout>(it->data), nullptr);
                break;
            case DeletableType::VK_PIPELINE_LAYOUT:
                vkDestroyPipelineLayout(VktCache::vkDevice, reinterpret_cast<VkPipelineLayout>(it->data), nullptr);
                break;
            case DeletableType::VK_PIPELINE:
                vkDestroyPipeline(VktCache::vkDevice, reinterpret_cast<VkPipeline>(it->data), nullptr);
                break;
            case DeletableType::VK_DEBUG_UTILS_MESSENGER:
                VktUtils::DestroyDebugUtilsMessengerEXT(reinterpret_cast<VkDebugUtilsMessengerEXT>(it->data), nullptr);
                break;
            case DeletableType::VK_SAMPLER:
                vkDestroySampler(VktCache::vkDevice, reinterpret_cast<VkSampler>(it->data), nullptr);
                break;
            case DeletableType::VMA_ALLOCATOR:
                vmaDestroyAllocator(reinterpret_cast<VmaAllocator>(it->data));
                break;
            case DeletableType::VMA_IMAGE:
                vmaDestroyImage(VktCache::vmaAllocator,
                                reinterpret_cast<VkImage>(it->data),
                                reinterpret_cast<VmaAllocation>(it->cont.at(0)));
                break;
            case DeletableType::VMA_BUFFER:
                vmaDestroyBuffer(VktCache::vmaAllocator,
                                 reinterpret_cast<VkBuffer>(it->data),
                                 reinterpret_cast<VmaAllocation>(it->cont.at(0)));
                break;
            case DeletableType::TEC_DESCRIPTOR_ALLOCATOR_DYNAMIC: {
                auto* allocator = reinterpret_cast<DescriptorAllocatorDynamic*>(it->data);
                allocator->destroyPool();
                break;
            }
            case DeletableType::TEC_RESOURCE_BUFFER: {
                auto* buffer = reinterpret_cast<VktTypes::Resources::Buffer*>(it->data);
                vmaDestroyBuffer(VktCache::vmaAllocator, buffer->buffer, buffer->allocation);
                break;
            }
            case DeletableType::TEC_RESOURCE_IMAGE: {
                auto* image = reinterpret_cast<VktTypes::Resources::Image*>(it->data);
                vmaDestroyImage(VktCache::vmaAllocator, image->image, image->allocation);
                vkDestroyImageView(VktCache::vkDevice, image->view, nullptr);
                break;
            }
        }
    }
}

void VktDeletableQueue::pushDeletable(DeletableType type, void* data, std::vector<void*> &&cont) {
    if(cont.empty()){
        m_delQueue.push_back({type, data});
    }else{
        m_delQueue.push_back({type, data, cont});
    }
}
