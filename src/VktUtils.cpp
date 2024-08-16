#include "engine/vulkan/VktUtils.h"

#include "engine/vulkan/VktCache.h"

void VktUtils::transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, uint32_t mipLevels){
    VkImageMemoryBarrier2 imageBarrier {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};

    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;

    VkImageAspectFlags aspectMask =
            (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
            ? VK_IMAGE_ASPECT_DEPTH_BIT
            : VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange = VktStructs::imageSubresourceRange(aspectMask);
    imageBarrier.subresourceRange.levelCount = mipLevels;
    imageBarrier.image = image;

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;

    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void VktUtils::transitionCubeMap(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, uint32_t mipLevels){
    VkImageMemoryBarrier2 imageBarrier {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};

    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;

    VkImageAspectFlags aspectMask =
            (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
            ? VK_IMAGE_ASPECT_DEPTH_BIT
            : VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange = VktStructs::imageSubresourceRange(aspectMask);
    imageBarrier.subresourceRange.layerCount = 6;
    imageBarrier.subresourceRange.levelCount = mipLevels;
    imageBarrier.image = image;

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;

    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void VktUtils::copyImgToImg(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcExtent, VkExtent2D dstExtent) {
    VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

    blitRegion.srcOffsets[1].x = static_cast<int32_t>(srcExtent.width);
    blitRegion.srcOffsets[1].y = static_cast<int32_t>(srcExtent.height);
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = static_cast<int32_t>(dstExtent.width);
    blitRegion.dstOffsets[1].y = static_cast<int32_t>(dstExtent.height);
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;

    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blitInfo {.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
    blitInfo.srcImage = src;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.dstImage = dst;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.filter = VK_FILTER_LINEAR;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blitRegion;

    vkCmdBlitImage2(cmd, &blitInfo);
}

VkShaderModule VktUtils::loadShaderModule(const char* path){
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if(!file.is_open()){
        throw vulkanException("Failed to load a shader file ", path);
    }

    // Reserve memory for SpirV file
    std::size_t fileSize = static_cast<std::size_t>(file.tellg());
    std::vector<uint32_t> buffer(fileSize/sizeof(uint32_t));

    // Read file
    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo{ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .pNext = nullptr };
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    // Create shader module from shader SpirV code
    VkShaderModule shaderModule;
    if(vkCreateShaderModule(VktCache::vkDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS){
        throw vulkanException("Failed to create shader module from shader file ", path);
    }

    return shaderModule;
}

void VktUtils::DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator){
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(VktCache::vkInstance, "vkDestroyDebugUtilsMessengerEXT");
    if(func != nullptr){
        func(VktCache::vkInstance, debugMessenger, pAllocator);
    }
}

std::vector<VkImageView> VktUtils::createImageMipViews(VkImage image, VkFormat format, uint32_t mipLevels) {
    std::vector<VkImageView> views(mipLevels);
    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    if(format == VK_FORMAT_D32_SFLOAT) {
        aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    VkImageViewCreateInfo viewInfo = VktStructs::imageViewCreateInfo(format, image, aspectFlags);

    for(uint32_t level = 0; level < mipLevels; level++) {
        viewInfo.subresourceRange.baseMipLevel = level;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(VktCache::vkDevice, &viewInfo, nullptr, &views[level]))
    }
    return views;
}

std::vector<VkImageView> VktUtils::createCubemapMipViews(VkImage image, VkFormat format, uint32_t mipLevels) {
    std::vector<VkImageView> views(mipLevels);
    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    if(format == VK_FORMAT_D32_SFLOAT) {
        aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    VkImageViewCreateInfo viewInfo = VktStructs::imageViewCreateInfo(format, image, aspectFlags);
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;

    for(uint32_t level = 0; level < mipLevels; level++) {
        viewInfo.subresourceRange.baseMipLevel = level;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 6;
        viewInfo.subresourceRange.baseArrayLayer = 0;

        VK_CHECK(vkCreateImageView(VktCache::vkDevice, &viewInfo, nullptr, &views[level]))
    }
    return views;
}