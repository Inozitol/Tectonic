#pragma once
#include "VktTypes.h"

#include <ktx.h>

namespace VktImages {

    /**
     * Contains the necessary information for creating VktTypes::Resources::Image.
     */
    struct VktImageCreateInfo {
        /** Extend of the image. */
        VkExtent3D extent = {0, 0, 0};

        /** Format of the image. */
        VkFormat format = VK_FORMAT_UNDEFINED;

        /** Tiling of the image. */
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

        /** Number of mip layers of the image. */
        uint32_t mipLevels = 1;

        /**
         * Number of layers of the image.\n
         * This member is ignored if isCubemap is true.
         */
        uint32_t layers = 1;

        /**  Makes the image a cubemap with 6 layers and VK_IMAGE_VIEW_TYPE_CUBE image view. */
        bool isCubemap = false;

        /** Usage flags. */
        VkImageUsageFlags usageFlags = 0;

        /** Allocation flags */
        VmaAllocationCreateFlagBits allocationFlags{};

        /** Memory usage flags */
        VmaMemoryUsage memoryUsage{};

        /** Memory properties flags */
        VkMemoryPropertyFlags memoryProperties = 0;
    };

    /**
     * @brief Allocates an empty image on GPU
     * @param info Struct with the necessary information to create an image.
     * @return GPU allocated image.
     */
    std::optional<VktTypes::Resources::Image> create(const VktImageCreateInfo &info);

    /**
     * @brief Allocates an empty image on GPU as device only resource
     * @param allocSize Extent with size of one cubemap side
     * @param format Image format
     * @param usageFlags Usage flags
     * @param mipLevels Levels of mipmaps
     * @param layers Number of array layers of the image
     * @param isCubemap Set to true if the image should be a cubemap. The layers argument is ignored in that case.
     * @return GPU allocated image
     */
    std::optional<VktTypes::Resources::Image> createDeviceMemory(VkExtent3D allocSize,
                                                                 VkFormat format,
                                                                 VkImageUsageFlags usageFlags,
                                                                 uint32_t mipLevels = 1,
                                                                 uint32_t layers = 1,
                                                                 bool isCubemap = false);

    /**
     * @brief Allocates and empty image on GPU with host-visible access
     * @note Devices don't usually support LINEAR HOST-VISIBLE images so it returns image with 1 mip level
     * @param allocSize Extent with size of the image
     * @param format Image format
     * @param usageFlags Usage flags
     * @return Host-visible GPU allocated image
     */
    std::optional<VktTypes::Resources::Image> createHostVisible(VkExtent3D allocSize,
                                                                VkFormat format,
                                                                VkImageUsageFlags usageFlags);

    /**
     * @brief Uploads image data into existing allocated GPU image
     * @param data Image data bytes
     * @param image GPU allocated image
     */
    void copyFromRaw(const VktTypes::Resources::Image &image,
                     size_t size,
                     const char *data);

    /**
     * @brief Creates an image from ktx file
     * @param path Path to ktx file
     * @param isCubemap If true the loader will check if the image contains 6 layers and gives a warning if not.
     * @return GPU allocated filled image
     */
    std::optional<VktTypes::Resources::Image> createFromFileKtx(const char *path, bool isCubemap = false);

    /**
     * @brief Stores an allocated image into a ktx file.
     * @param path Path to ktx file
     * @param image Image to store
     * @return True if the image was successfully stored
     */
    bool writeKtx(const char *path, const VktTypes::Resources::Image &image);

    /**
     * @brief Destroys a GPU image
     * @param image GPU allocated image
     */
    void destroy(const VktTypes::Resources::Image &image);

 size_t totalSize(const VktTypes::Resources::Image& image);

}// namespace VktImages