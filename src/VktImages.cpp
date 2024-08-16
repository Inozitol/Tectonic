#include "engine/vulkan/VktImages.h"
#include "engine/vulkan/VktBuffers.h"
#include "engine/vulkan/VktCache.h"
#include "engine/vulkan/VktInstantCommands.h"

#include <filesystem>
#include <vulkan/utility/vk_format_utils.h>

namespace VktImages {
    static Logger logger("VktImages");

    std::optional<VktTypes::Resources::Image> create(const VktImageCreateInfo &info) {
        VktTypes::Resources::Image newImage{
                .extent = info.extent,
                .format = info.format,
                .mipLevels = info.mipLevels,
                .layers = info.layers};

        VkImageCreateInfo imgInfo = VktStructs::imageCreateInfo(info.format, info.usageFlags, info.extent, info.tiling, info.mipLevels, info.layers);
        if(info.isCubemap) {
            imgInfo.arrayLayers = 6;
            imgInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.flags = info.allocationFlags;
        allocInfo.usage = info.memoryUsage;
        allocInfo.requiredFlags = info.memoryProperties;

        VK_CHECK(vmaCreateImage(VktCache::vmaAllocator, &imgInfo, &allocInfo, &newImage.image, &newImage.allocation, &newImage.info))

        VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        if(info.format == VK_FORMAT_D32_SFLOAT) {
            aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        VkImageViewCreateInfo viewInfo = VktStructs::imageViewCreateInfo(info.format, newImage.image, aspectFlags, info.mipLevels, info.layers);
        if(info.isCubemap) {
            viewInfo.subresourceRange.layerCount = 6;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        }

        VK_CHECK(vkCreateImageView(VktCache::vkDevice, &viewInfo, nullptr, &newImage.view))
        return newImage;
    }

    std::optional<VktTypes::Resources::Image> createDeviceMemory(VkExtent3D allocSize,
                                                                 VkFormat format,
                                                                 VkImageUsageFlags usageFlags,
                                                                 uint32_t mipLevels,
                                                                 uint32_t layers,
                                                                 bool isCubemap) {
        VktImageCreateInfo info{
                .extent = allocSize,
                .format = format,
                .mipLevels = mipLevels,
                .layers = layers,
                .isCubemap = isCubemap,
                .usageFlags = usageFlags,
                .memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

        return create(info);
    }

    void copyFromRaw(const VktTypes::Resources::Image &image,
                     size_t size,
                     const char *data) {

        size_t mipOffset = 0;

        // Copying data to staging buffer
        VktTypes::Resources::Buffer stagingBuffer = VktBuffers::create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        memcpy(stagingBuffer.info.pMappedData, data, size);

        VktInstantCommands::submitCommands([&](VkCommandBuffer cmd) {
            VktUtils::transitionImage(cmd, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        });
        for(uint32_t mipLevel = image.mipLevels - 1; mipLevel != UINT32_MAX; mipLevel--) {
            VkExtent3D mipExtent = {
                    .width = static_cast<uint32_t>(image.extent.width * std::pow(0.5, mipLevel)),
                    .height = static_cast<uint32_t>(image.extent.height * std::pow(0.5, mipLevel)),
                    .depth = image.extent.depth};

            // Transfer data to image through staging buffer
            VktInstantCommands::submitCommands([&](VkCommandBuffer cmd) {
                VkBufferImageCopy copyRegion{};
                copyRegion.bufferOffset = mipOffset;
                copyRegion.bufferRowLength = 0;
                copyRegion.bufferImageHeight = 0;
                copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.imageSubresource.mipLevel = mipLevel;
                copyRegion.imageSubresource.baseArrayLayer = 0;
                copyRegion.imageSubresource.layerCount = image.layers;
                copyRegion.imageExtent = mipExtent;

                vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
            });
            mipOffset += mipExtent.width * mipExtent.height * mipExtent.depth * vkuFormatComponentCount(image.format) * image.layers;
        }
        VktInstantCommands::submitCommands([&](VkCommandBuffer cmd) {
            VktUtils::transitionImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });
        VktBuffers::destroy(stagingBuffer);
    }

    std::optional<VktTypes::Resources::Image> createFromFileKtx(const char *path, bool isCubemap) {
        auto imagePath = std::filesystem::path(path);
        if(!exists(imagePath)) {
            logger(Logger::ERROR) << "Failed to create image. Cannot find ktx file " << imagePath << '\n';
            return {};
        }

        ktxTexture2 *ktxImage;
        ktxResult result = ktxTexture2_CreateFromNamedFile(imagePath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxImage);
        if(result != KTX_SUCCESS) {
            logger(Logger::ERROR) << "Failed to create image. ktxTexture_CreateFromNamedFile returned " << ktxErrorString(result) << '\n';
            return {};
        }

        const VkExtent3D allocSize = {.width = ktxImage->baseWidth, .height = ktxImage->baseHeight, .depth = ktxImage->baseDepth};
        const VkFormat format = ktxTexture2_GetVkFormat(ktxImage);
        const std::optional<VktTypes::Resources::Image> optImage = createDeviceMemory(allocSize,
                                                                                      format,
                                                                                      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                                                                      ktxImage->numLevels,
                                                                                      ktxImage->numLayers,
                                                                                      isCubemap);
        if(!optImage.has_value()) {
            return {};
        }

        if(isCubemap && ktxImage->numLayers != 6) {
            logger(Logger::WARNING) << "Loaded ktx " << path << " file with " << ktxImage->numLayers << " layers but expected 6 because isCubemap set to true. Are you sure this is a cubemap?\n";
        }

        const char *ktxImageData = reinterpret_cast<char *>(ktxImage->pData);
        const size_t ktxImageSize = ktxImage->dataSize;
        copyFromRaw(optImage.value(), ktxImageSize, ktxImageData);

        ktxTexture2_Destroy(ktxImage);

        return optImage;
    }

    bool writeKtx(const char *path, const VktTypes::Resources::Image &image) {
        const VkExtent3D &extent = image.extent;

        ktxTexture2 *texture;
        ktxTextureCreateInfo ktxCreateInfo{};
        ktx_error_code_e ktxResult;
        ktx_uint32_t mipLevel, layer;

        ktxCreateInfo.vkFormat = image.format;
        ktxCreateInfo.baseWidth = extent.width;
        ktxCreateInfo.baseHeight = extent.height;
        ktxCreateInfo.baseDepth = extent.depth;
        ktxCreateInfo.numDimensions = 2;
        ktxCreateInfo.numLevels = image.mipLevels;
        ktxCreateInfo.numLayers = image.layers;
        ktxCreateInfo.numFaces = 1;
        ktxCreateInfo.isArray = image.layers > 1 ? KTX_TRUE : KTX_FALSE;
        ktxCreateInfo.generateMipmaps = KTX_FALSE;

        ktxResult = ktxTexture2_Create(&ktxCreateInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture);
        if(ktxResult != KTX_SUCCESS) {
            logger(Logger::ERROR) << "Failed to create ktxTexture2. ktxTexture2_Create returned [" << ktxErrorString(ktxResult) << "]\n";
            return false;
        }
        VktInstantCommands::submitCommands([&](VkCommandBuffer cmd) {
            VktUtils::transitionImage(cmd, image.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        });

        for(mipLevel = 0; mipLevel < image.mipLevels; mipLevel++) {
            VkExtent3D mipExtent = {
                    .width = static_cast<uint32_t>(extent.width * std::pow(0.5, mipLevel)),
                    .height = static_cast<uint32_t>(extent.height * std::pow(0.5, mipLevel)),
                    .depth = extent.depth};
            for(layer = 0; layer < image.layers; layer++) {
                size_t layerSize = mipExtent.width * mipExtent.height * mipExtent.depth * vkuFormatComponentCount(image.format);

                VktTypes::Resources::Buffer copyBuffer = VktBuffers::create(layerSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);
                VktInstantCommands::submitCommands([&](VkCommandBuffer cmd) {
                    VkBufferImageCopy copyRegion{};
                    copyRegion.bufferOffset = 0;
                    copyRegion.bufferRowLength = 0;
                    copyRegion.bufferImageHeight = 0;
                    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.imageSubresource.mipLevel = mipLevel;
                    copyRegion.imageSubresource.baseArrayLayer = layer;
                    copyRegion.imageSubresource.layerCount = 1;
                    copyRegion.imageExtent = mipExtent;

                    vkCmdCopyImageToBuffer(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, copyBuffer.buffer, 1, &copyRegion);
                });
                VktBuffers::destroy(copyBuffer);

                ktxResult = ktxTexture_SetImageFromMemory(ktxTexture(texture), mipLevel, layer, 0, static_cast<ktx_uint8_t *>(copyBuffer.info.pMappedData), layerSize);
                if(ktxResult != KTX_SUCCESS) {
                    logger(Logger::ERROR) << "Failed to set ktx image memory. ktxTexture_SetImageFromMemory returned [" << ktxErrorString(ktxResult) << "]\n";
                    VktInstantCommands::submitCommands([&](VkCommandBuffer cmd) {
                        VktUtils::transitionImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                    });

                    return false;
                }
            }
        }
        VktInstantCommands::submitCommands([&](VkCommandBuffer cmd) {
            VktUtils::transitionImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

        ktxResult = ktxTexture_WriteToNamedFile(ktxTexture(texture), path);
        if(ktxResult != KTX_SUCCESS) {
            logger(Logger::ERROR) << "Failed to create ktx file. ktxTexture_WriteToNamedFile returned [" << ktxErrorString(ktxResult) << "]\n";
            return false;
        }

        ktxTexture2_Destroy(texture);

        return true;
    }

    void destroy(const VktTypes::Resources::Image &image) {
        vkDestroyImageView(VktCache::vkDevice, image.view, nullptr);
        vmaDestroyImage(VktCache::vmaAllocator, image.image, image.allocation);
    }

}// namespace VktImages