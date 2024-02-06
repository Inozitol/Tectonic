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
    VkImageCreateInfo               imageCreateInfo(VkFormat format, VkImageUsageFlags flags, VkExtent3D extent);
    VkImageViewCreateInfo           imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
    VkRenderingAttachmentInfo       attachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout);
    VkRenderingAttachmentInfo       depthAttachmentInfo(VkImageView view, VkImageLayout layout);
    VkRenderingInfo                 renderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment);
    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry = "main");
    VkPipelineLayoutCreateInfo      pipelineLayoutCreateInfo();
}

#endif //TECTONIC_VULKANSTRUCTS_H
