#include "engine/vulkan/VktCubemaps.h"

#include <extern/stb/stb_image.h>

namespace VktCubemaps {

    void fillCubemap(VmaAllocator allocator,
                     VktTypes::Resources::Cubemap cubemap,
                     std::array<void *, 6> data) {

        // 4 channels and 6 layers of cube
        size_t dataSize = cubemap.extent.width * cubemap.extent.height * cubemap.extent.depth * 4 * 6;

        // Copy data to staging buffer
        VktTypes::Resources::Buffer stagingBuffer = VktBuffers::createBuffer(allocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        for(uint8_t buffer = 0; buffer < 6; buffer++) {
            memcpy(static_cast<char *>(stagingBuffer.info.pMappedData) + (buffer * (dataSize / 6)), data[buffer], (dataSize / 6));
        }

        // Transfer data to cubemap layers through staging buffer
        VktInstantCommands::submitCommands([&cubemap, &stagingBuffer](VkCommandBuffer cmd) {
            VktUtils::transitionCubeMap(cmd, cubemap.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBufferImageCopy copyRegion{};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;
            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 6;
            copyRegion.imageExtent = cubemap.extent;

            vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, cubemap.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
            VktUtils::transitionImage(cmd, cubemap.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });
        VktBuffers::destroyBuffer(allocator, stagingBuffer);
    }

    std::optional<VktTypes::Resources::Cubemap> createCubemap(VkDevice device,
                                                            VmaAllocator allocator,
                                                            VkExtent3D allocSize,
                                                            VkFormat format,
                                                            VkImageUsageFlags usageFlags,
                                                            uint32_t mipLevels) {
        VktTypes::Resources::Cubemap newCubeMap{};
        newCubeMap.format = format;
        newCubeMap.extent = allocSize;

        VkImageCreateInfo imgInfo = VktStructs::imageCreateInfo(format, usageFlags, allocSize, mipLevels);
        imgInfo.arrayLayers = 6;
        imgInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vmaCreateImage(allocator, &imgInfo, &allocInfo, &newCubeMap.image, &newCubeMap.allocation, nullptr))

        VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        if(format == VK_FORMAT_D32_SFLOAT) {
            aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        VkImageViewCreateInfo viewInfo = VktStructs::imageViewCreateInfo(format, newCubeMap.image, aspectFlags);
        viewInfo.subresourceRange.levelCount = imgInfo.mipLevels;
        viewInfo.subresourceRange.layerCount = 6;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;

        VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &newCubeMap.view))

        return newCubeMap;
    }

    std::optional<VktTypes::Resources::Cubemap> createCubemapFromDir(VkDevice device, VmaAllocator allocator, const char *directory) {
        std::array<void *, 6> cubeData{};

        std::array cubeFiles = {
                std::string(directory) + "/xpos.png",
                std::string(directory) + "/xneg.png",
                std::string(directory) + "/ypos.png",
                std::string(directory) + "/yneg.png",
                std::string(directory) + "/zpos.png",
                std::string(directory) + "/zneg.png",
        };

        // Currently assuming that every picture has same width/height/channels
        int width, height, channels;
        for(uint8_t cubeSide = 0; cubeSide < 6; cubeSide++) {
            unsigned char *data = stbi_load(cubeFiles[cubeSide].c_str(), &width, &height, &channels, 4);
            if(data) {
                cubeData[cubeSide] = data;
            } else {
                return {};
            }
        }

        VkExtent3D allocSize = {.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height), .depth = 1};
        std::optional<VktTypes::Resources::Cubemap> cubeMap = createCubemap(device, allocator, allocSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        if(!cubeMap.has_value()) {
            return {};
        }
        fillCubemap(allocator, cubeMap.value(), cubeData);

        return cubeMap;
    }
}