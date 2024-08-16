#ifndef TECTONIC_VKTSTRUCTS_H
#define TECTONIC_VKTSTRUCTS_H

#include <array>
#include <vulkan/vulkan.h>

/**
 * Various functions that return pre-made Vulkan structures.
 */
namespace VktStructs {
    VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
    VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1);
    VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);
    VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);
    VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
    VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask);
    VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
    VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd);
    VkSubmitInfo2 submitInfo(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signalSemaphoreInfo, VkSemaphoreSubmitInfo *waitSemaphoreInfo);
    VkImageCreateInfo imageCreateInfo(VkFormat format,
                                      VkImageUsageFlags usageFlags,
                                      VkExtent3D extent,
                                      VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
                                      uint32_t mipLevels = 1,
                                      uint32_t layers = 1);
    VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1, uint32_t layers = 1);
    VkRenderingAttachmentInfo attachmentInfo(VkImageView view, VkClearValue *clear, VkImageLayout layout);
    VkRenderingAttachmentInfo depthAttachmentInfo(VkImageView view, VkImageLayout layout);
    VkRenderingInfo renderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo *colorAttachment, VkRenderingAttachmentInfo *depthAttachment, uint32_t layerCount = 1);
    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char *entry = "main");
    VkViewport viewport(VkExtent2D extent);
    VkRect2D scissors(VkExtent2D extent);

    template<size_t N>
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(const std::array<VkDescriptorSetLayout, N> &layouts, const VkPushConstantRange &pushConstantRange) {
        VkPipelineLayoutCreateInfo info{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .pNext = nullptr};
        info.flags = 0;
        info.setLayoutCount = layouts.size();
        info.pSetLayouts = layouts.data();
        info.pushConstantRangeCount = 1;
        info.pPushConstantRanges = &pushConstantRange;
        return info;
    };
}// namespace VktStructs

#endif//TECTONIC_VKTSTRUCTS_H
