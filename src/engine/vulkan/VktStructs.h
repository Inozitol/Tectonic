#ifndef TECTONIC_VKTSTRUCTS_H
#define TECTONIC_VKTSTRUCTS_H

#include <array>

/**
 * Various functions that return pre-made Vulkan structures.
 */
namespace VktStructs {
    inline VkBufferDeviceAddressInfo bufferDeviceAddressInfo(const VkBuffer buffer) {
        return VkBufferDeviceAddressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext = nullptr,
            .buffer = buffer
        };
    }

    inline VkCommandPoolCreateInfo commandPoolCreateInfo(const uint32_t queueFamilyIndex, const VkCommandPoolCreateFlags flags = 0) {
        return VkCommandPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = flags,
            .queueFamilyIndex = queueFamilyIndex,
        };
    }

    inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(const VkCommandPool pool, const uint32_t count = 1) {
        return VkCommandBufferAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = count
        };
    }

    inline VkFenceCreateInfo fenceCreateInfo(const VkFenceCreateFlags flags = 0) {
        return VkFenceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = flags,
        };
    }

    inline VkSemaphoreCreateInfo semaphoreCreateInfo(const VkSemaphoreCreateFlags flags = 0) {
        return VkSemaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = flags,
        };
    }

    inline VkCommandBufferBeginInfo commandBufferBeginInfo(const VkCommandBufferUsageFlags flags = 0) {
        return VkCommandBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = flags,
            .pInheritanceInfo = nullptr
        };
    }

    inline VkImageSubresourceRange imageSubresourceRange(const VkImageAspectFlags aspectMask, const uint32_t mipLevels = VK_REMAINING_MIP_LEVELS, const uint32_t arrayLayers = VK_REMAINING_ARRAY_LAYERS) {
        return VkImageSubresourceRange{
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = arrayLayers
        };
    }

    inline VkSemaphoreSubmitInfo semaphoreSubmitInfo(const VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) {
        return VkSemaphoreSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = semaphore,
            .value = 1,
            .stageMask = stageMask,
            .deviceIndex = 0
        };
    }

    inline VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd) {
        return VkCommandBufferSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBuffer = cmd,
            .deviceMask = 0
        };
    }

    inline VkSubmitInfo2 submitInfo(const VkCommandBufferSubmitInfo* cmd,
                                    const VkSemaphoreSubmitInfo* signalSemaphoreInfo,
                                    const VkSemaphoreSubmitInfo* waitSemaphoreInfo) {
        return VkSubmitInfo2{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext = nullptr,
            .flags = 0,
            .waitSemaphoreInfoCount = static_cast<uint32_t>(waitSemaphoreInfo == nullptr ? 0 : 1),
            .pWaitSemaphoreInfos = waitSemaphoreInfo,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = cmd,
            .signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphoreInfo == nullptr ? 0 : 1),
            .pSignalSemaphoreInfos = signalSemaphoreInfo,
        };
    }

    inline VkImageCreateInfo imageCreateInfo(const VkFormat format,
                                             const VkImageUsageFlags usageFlags,
                                             const VkExtent3D extent,
                                             const VkImageTiling tiling,
                                             const uint32_t mipLevels = 1,
                                             const uint32_t arrayLayers = 1) {
        return VkImageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = extent,
            .mipLevels = mipLevels,
            .arrayLayers = arrayLayers,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = tiling,
            .usage = usageFlags,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
    }

    inline VkImageViewCreateInfo imageViewCreateInfo(const VkFormat format,
                                                     VkImage image,
                                                     const VkImageAspectFlags aspectFlags,
                                                     const uint32_t mipLevels = 1,
                                                     const uint32_t arrayLayers = 1) {
        return VkImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = aspectFlags,
                .baseMipLevel = 0,
                .levelCount = mipLevels,
                .baseArrayLayer = 0,
                .layerCount = arrayLayers
            }
        };
    }

    inline VkRenderingAttachmentInfo attachmentInfo(VkImageView view, const VkClearValue* clear,
                                                    const VkImageLayout layout) {
        return VkRenderingAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = view,
            .imageLayout = layout,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = clear
                              ? *clear
                              : VkClearValue{
                                  .color = {.uint32 = {0, 0, 0, 0}},
                              },
        };
    }

    inline VkRenderingAttachmentInfo depthAttachmentInfo(VkImageView view,
                                                         const VkImageLayout layout) {
        return VkRenderingAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = view,
            .imageLayout = layout,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {
                .depthStencil = {
                    .depth = 1.0f,
                    .stencil = 0
                }
            },
        };
    }

    inline VkRenderingInfo renderingInfo(const VkExtent2D renderExtent,
                                         const VkRenderingAttachmentInfo* colorAttachment,
                                         const VkRenderingAttachmentInfo* depthAttachment,
                                         const uint32_t layerCount = 1) {
        return VkRenderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = VkRect2D{
                VkOffset2D{.x = 0, .y = 0},
                renderExtent
            },
            .layerCount = layerCount,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = colorAttachment,
            .pDepthAttachment = depthAttachment,
            .pStencilAttachment = nullptr
        };
    }

    inline VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(const VkShaderStageFlagBits stage,
                                                                         const VkShaderModule shaderModule,
                                                                         const char* entry = "main") {
        return VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = stage,
            .module = shaderModule,
            .pName = entry,
            .pSpecializationInfo = nullptr
        };
    }

    inline VkViewport viewport(const VkExtent2D extent) {
        return VkViewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(extent.width),
            .height = static_cast<float>(extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
    }

    inline VkRect2D scissors(const VkExtent2D extent) {
        return VkRect2D{
            .offset = VkOffset2D{.x = 0, .y = 0},
            .extent = extent,
        };
    }

    template <size_t N>
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(const std::array<VkDescriptorSetLayout, N>& layouts, const VkPushConstantRange& pushConstantRange) {
        VkPipelineLayoutCreateInfo info{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .pNext = nullptr};
        info.flags = 0;
        info.setLayoutCount = layouts.size();
        info.pSetLayouts = layouts.data();
        info.pushConstantRangeCount = 1;
        info.pPushConstantRanges = &pushConstantRange;
        return info;
    };
} // namespace VktStructs

#endif//TECTONIC_VKTSTRUCTS_H
