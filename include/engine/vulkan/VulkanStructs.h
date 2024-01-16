#ifndef TECTONIC_VULKANSTRUCTS_H
#define TECTONIC_VULKANSTRUCTS_H

#include <vulkan/vulkan.h>

namespace VkStructs{
    VkCommandPoolCreateInfo         commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
    VkCommandBufferAllocateInfo     commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1);
    VkFenceCreateInfo               fenceCreateInfo(VkFenceCreateFlags flags = 0);
    VkSemaphoreCreateInfo           semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);
    VkCommandBufferBeginInfo        commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
    VkImageSubresourceRange         imageSubresourceRange(VkImageAspectFlags aspectMask);
    VkSemaphoreSubmitInfo           semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
    VkCommandBufferSubmitInfo       commandBufferSubmitInfo(VkCommandBuffer cmd);
    VkSubmitInfo2                   submitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo);
}

#endif //TECTONIC_VULKANSTRUCTS_H
