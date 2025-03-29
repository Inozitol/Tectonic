#include "engine/vulkan/VktStructs.h"

VkCommandPoolCreateInfo VktStructs::commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags){
    VkCommandPoolCreateInfo info{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .pNext = nullptr};
    info.queueFamilyIndex = queueFamilyIndex;
    info.flags = flags;
    return info;
}

VkCommandBufferAllocateInfo VktStructs::commandBufferAllocateInfo(VkCommandPool pool, uint32_t count){
    VkCommandBufferAllocateInfo info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .pNext = nullptr};
    info.commandPool = pool;
    info.commandBufferCount = count;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    return info;
}

VkFenceCreateInfo VktStructs::fenceCreateInfo(VkFenceCreateFlags flags){
    VkFenceCreateInfo info{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = nullptr};
    info.flags = flags;
    return info;
}

VkSemaphoreCreateInfo VktStructs::semaphoreCreateInfo(VkSemaphoreCreateFlags flags){
    VkSemaphoreCreateInfo info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr};
    info.flags = flags;
    return info;
}

VkCommandBufferBeginInfo VktStructs::commandBufferBeginInfo(VkCommandBufferUsageFlags flags){
    VkCommandBufferBeginInfo info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .pNext = nullptr};
    info.pInheritanceInfo = nullptr;
    info.flags = flags;
    return info;
}

VkImageSubresourceRange VktStructs::imageSubresourceRange(VkImageAspectFlags aspectMask){
    VkImageSubresourceRange subImage{};
    subImage.aspectMask = aspectMask;
    subImage.baseMipLevel = 0;
    subImage.levelCount = VK_REMAINING_MIP_LEVELS;
    subImage.baseArrayLayer = 0;
    subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;
    return subImage;
}

VkSemaphoreSubmitInfo VktStructs::semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore){
    VkSemaphoreSubmitInfo info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .pNext = nullptr};
    info.semaphore = semaphore;
    info.stageMask = stageMask;
    info.deviceIndex = 0;
    info.value = 1;
    return info;
}

VkCommandBufferSubmitInfo VktStructs::commandBufferSubmitInfo(VkCommandBuffer cmd){
    VkCommandBufferSubmitInfo info{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .pNext = nullptr};
    info.commandBuffer = cmd;
    info.deviceMask = 0;
    return info;
}

VkSubmitInfo2 VktStructs::submitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo){
    VkSubmitInfo2 info{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2, .pNext = nullptr};
    info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
    info.pWaitSemaphoreInfos = waitSemaphoreInfo;
    info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
    info.pSignalSemaphoreInfos = signalSemaphoreInfo;
    info.commandBufferInfoCount = 1;
    info.pCommandBufferInfos = cmd;
    return info;
}

VkImageCreateInfo VktStructs::imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent,VkImageTiling tiling, uint32_t mipLevels, uint32_t layers){
    VkImageCreateInfo info{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .pNext = nullptr};
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent = extent;
    info.mipLevels = mipLevels;
    info.arrayLayers = layers;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usageFlags;
    info.tiling = tiling;
    return info;
}

VkImageViewCreateInfo VktStructs::imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags, uint32_t mipLevels, uint32_t layers){
    VkImageViewCreateInfo info{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .pNext = nullptr};
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = format;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = mipLevels;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = layers;
    info.subresourceRange.aspectMask = aspectFlags;
    return info;
}

VkRenderingAttachmentInfo VktStructs::attachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout){
    VkRenderingAttachmentInfo info{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, .pNext = nullptr};
    info.imageView = view;
    info.imageLayout = layout;
    info.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    if(clear){
        info.clearValue = *clear;
    }
    return info;
}

VkRenderingAttachmentInfo VktStructs::depthAttachmentInfo(VkImageView view, VkImageLayout layout){
    VkRenderingAttachmentInfo info{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, .pNext = nullptr};
    info.imageView = view;
    info.imageLayout = layout;
    info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    info.clearValue.depthStencil.depth = 1.0f;
    return info;
}

VkRenderingInfo VktStructs::renderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment, uint32_t layerCount){
    VkRenderingInfo info{.sType = VK_STRUCTURE_TYPE_RENDERING_INFO, .pNext = nullptr};
    info.renderArea = VkRect2D{VkOffset2D{0,0}, renderExtent};
    info.layerCount = layerCount;
    info.colorAttachmentCount = 1;
    info.pColorAttachments = colorAttachment;
    info.pDepthAttachment = depthAttachment;
    info.pStencilAttachment = nullptr;
    return info;
}

VkPipelineShaderStageCreateInfo VktStructs::pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entry){
    VkPipelineShaderStageCreateInfo info{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .pNext = nullptr};
    info.stage = stage;
    info.module = shaderModule;
    info.pName = entry;
    return info;
}

VkViewport VktStructs::viewport(VkExtent2D extent) {
    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}


VkRect2D VktStructs::scissors(VkExtent2D extent) {
    VkRect2D scissors{};
    scissors.offset.x = 0;
    scissors.offset.y = 0;
    scissors.extent.width = extent.width;
    scissors.extent.height = extent.height;
    return scissors;
}
