#include "engine/vulkan/VktUtils.h"

#include "engine/GlobalMemory.h"
#include "engine/vulkan/VktCache.h"



void VktUtils::transitionImage(VkCommandBuffer cmd,
                               VkImage image,
                               const VkImageLayout currentLayout,
                               const VkImageLayout newLayout,
                               const uint32_t mipLevels) {
    const VkImageAspectFlags aspectMask =
            newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
                ? VK_IMAGE_ASPECT_DEPTH_BIT
                : VK_IMAGE_ASPECT_COLOR_BIT;

    const VkImageMemoryBarrier2 imageBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
            .oldLayout = currentLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = 0,
            .dstQueueFamilyIndex = 0,
            .image = image,
            .subresourceRange = VktStructs::imageSubresourceRange(aspectMask, mipLevels),
    };

    const VkDependencyInfo depInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imageBarrier,
    };

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void VktUtils::transitionCubeMap(VkCommandBuffer cmd,
                                 VkImage image,
                                 const VkImageLayout currentLayout,
                                 const VkImageLayout newLayout,
                                 const uint32_t mipLevels) {
    const VkImageAspectFlags aspectMask =
            newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
                ? VK_IMAGE_ASPECT_DEPTH_BIT
                : VK_IMAGE_ASPECT_COLOR_BIT;

    const VkImageMemoryBarrier2 imageBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
            .oldLayout = currentLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = 0,
            .dstQueueFamilyIndex = 0,
            .image = image,
            .subresourceRange = VktStructs::imageSubresourceRange(aspectMask, mipLevels, 6),// 6 Array layers
    };

    const VkDependencyInfo depInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imageBarrier,
    };

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void VktUtils::copyImgToImg(VkCommandBuffer cmd,
                            VkImage src,
                            VkImage dst,
                            const VkExtent2D srcExtent,
                            const VkExtent2D dstExtent) {

    const VkImageBlit2 blitRegion{
            .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
            .pNext = nullptr,
            .srcSubresource = VkImageSubresourceLayers{
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
            .srcOffsets = {
                    VkOffset3D{.x = 0, .y = 0, .z = 0},
                    VkOffset3D{.x = static_cast<int32_t>(srcExtent.width), .y = static_cast<int32_t>(srcExtent.height), .z = 1},
            },
            .dstSubresource = VkImageSubresourceLayers{
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
            .dstOffsets = {
                    VkOffset3D{.x = 0, .y = 0, .z = 0},
                    VkOffset3D{.x = static_cast<int32_t>(dstExtent.width), .y = static_cast<int32_t>(dstExtent.height), .z = 1},
            },
    };

    const VkBlitImageInfo2 blitInfo{
            .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
            .pNext = nullptr,
            .srcImage = src,
            .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .dstImage = dst,
            .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .regionCount = 1,
            .pRegions = &blitRegion,
            .filter = VK_FILTER_LINEAR
    };

    vkCmdBlitImage2(cmd, &blitInfo);
}

VkShaderModule VktUtils::loadShaderModule(const char *path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if(!file.is_open()) {
        LOG(LOG_ERROR, "Failed to load shader file " << path);
        return VK_NULL_HANDLE;
    }

    // Reserve memory for SpirV file
    const std::size_t fileSize = static_cast<std::size_t>(file.tellg());
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    // Read file
    file.seekg(0);
    file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
    file.close();

    const VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = buffer.size() * sizeof(uint32_t),
        .pCode = buffer.data()
    };

    // Create shader module from shader SpirV code
    VkShaderModule shaderModule;
    if(vkCreateShaderModule(VktCachePtr->vkDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) { throw vulkanException("Failed to create shader module from shader file ", path); }

    return shaderModule;
}

void VktUtils::DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
    static auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(VktCachePtr->vkInstance, "vkDestroyDebugUtilsMessengerEXT"));
    if(func != nullptr) { func(VktCachePtr->vkInstance, debugMessenger, pAllocator); }
}

void VktUtils::CmdSetPolygonModeEXT(VkCommandBuffer cmd, const VkPolygonMode polygonMode) {
    static auto func = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(vkGetInstanceProcAddr(VktCachePtr->vkInstance, "vkCmdSetPolygonModeEXT"));
    if(func != nullptr) { func(cmd, polygonMode); }
}

std::vector<VkImageView> VktUtils::createImageMipViews(VkImage image, const VkFormat format, const uint32_t mipLevels) {
    std::vector<VkImageView> views(mipLevels);
    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    if(format == VK_FORMAT_D32_SFLOAT) { aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT; }
    VkImageViewCreateInfo viewInfo = VktStructs::imageViewCreateInfo(format, image, aspectFlags, 1, 1);

    for(uint32_t level = 0; level < mipLevels; level++) {
        viewInfo.subresourceRange.baseMipLevel = level;

        VK_CHECK(vkCreateImageView(VktCachePtr->vkDevice, &viewInfo, nullptr, &views[level]))
    }
    return views;
}

std::vector<VkImageView> VktUtils::createCubemapMipViews(VkImage cubemapImage, const VkFormat format, const uint32_t mipLevels) {
    std::vector<VkImageView> views(mipLevels);
    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    if(format == VK_FORMAT_D32_SFLOAT) { aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT; }
    VkImageViewCreateInfo viewInfo = VktStructs::imageViewCreateInfo(format, cubemapImage, aspectFlags, 1, 6);
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;

    for(uint32_t level = 0; level < mipLevels; level++) {
        viewInfo.subresourceRange.baseMipLevel = level;

        VK_CHECK(vkCreateImageView(VktCachePtr->vkDevice, &viewInfo, nullptr, &views[level]))
    }
    return views;
}

void VktUtils::reinitCommandBuffer(VkCommandBuffer cmd, const VkCommandBufferUsageFlags flags) {
    VK_CHECK(vkResetCommandBuffer(cmd, 0))
    const VkCommandBufferBeginInfo cmdBeginInfo = VktStructs::commandBufferBeginInfo(flags);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo))
}