#include "engine/vulkan/VktImages.h"
#include "engine/vulkan/VktBuffers.h"
#include "engine/vulkan/VktInstantCommands.h"

namespace VktImages {
    static Logger logger("VktImages");

    std::optional<VktTypes::Resources::Image> createImage(VkDevice device,
                                                          VmaAllocator allocator,
                                                          VkExtent3D allocSize,
                                                          VkFormat format,
                                                          VkImageUsageFlags usageFlags,
                                                          uint32_t mipLevels) {
        VktTypes::Resources::Image newImage{
                .extent = allocSize,
                .format = format
        };

        VkImageCreateInfo imgInfo = VktStructs::imageCreateInfo(format, usageFlags, allocSize);
        if(mipLevels == UINT32_MAX) {
            imgInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(allocSize.width, allocSize.height)))) + 1;
        }else {
            imgInfo.mipLevels = mipLevels;
        }

        const VmaAllocationCreateInfo allocInfo{
                .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        };

        VK_CHECK(vmaCreateImage(allocator, &imgInfo, &allocInfo, &newImage.image, &newImage.allocation, nullptr))

        VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        if(format == VK_FORMAT_D32_SFLOAT) {
            aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        const VkImageViewCreateInfo viewInfo = VktStructs::imageViewCreateInfo(format, newImage.image, aspectFlags, mipLevels);

        VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &newImage.view))

        return newImage;
    }

    void fillImage(VmaAllocator allocator,
                   const void *data,
                   VktTypes::Resources::Image image) {

        size_t dataSize = image.extent.width * image.extent.height * image.extent.depth * 4;
        VktTypes::Resources::Buffer stagingBuffer = VktBuffers::createBuffer(allocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        // Copying data to staging buffer
        memcpy(stagingBuffer.info.pMappedData, data, dataSize);

        // Transfer data to image through staging buffer
        VktInstantCommands::submitCommands([&](VkCommandBuffer cmd) {
            VktUtils::transitionImage(cmd, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBufferImageCopy copyRegion{};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;
            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.imageExtent = image.extent;

            vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
            VktUtils::transitionImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });
        VktBuffers::destroyBuffer(allocator, stagingBuffer);
    }

    std::optional<VktTypes::Resources::Image> createFilledImage(VkDevice device,
                                                                VmaAllocator allocator,
                                                                const void *data,
                                                                VkExtent3D allocSize,
                                                                VkFormat format,
                                                                VkImageUsageFlags usageFlags,
                                                                uint32_t mipLevels) {
        auto image = createImage(device, allocator, allocSize, format, usageFlags | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
        if(!image.has_value()) {
            return {};
        }
        fillImage(allocator, data, image.value());
        return image;
    }

    void destroyImage(VkDevice device, VmaAllocator allocator, VktTypes::Resources::Image image) {
        vkDestroyImageView(device, image.view, nullptr);
        vmaDestroyImage(allocator, image.image, image.allocation);
    }

}// namespace VktImages