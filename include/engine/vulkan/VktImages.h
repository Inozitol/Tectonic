#pragma once
#include "VktTypes.h"

namespace VktImages {

    /**
     * @brief Allocates an empty image on GPU
     * @param device Vulkan device
     * @param allocator VMA allocator
     * @param allocSize Extent with size of the image
     * @param format Image format
     * @param usageFlags Usage flags
     * @param mipLevels Contains the number of mipmap levels. If UINT32_MAX, it will create mip level for each halfed size of allocSize.
     * @return GPU allocated image
     */
    std::optional<VktTypes::Resources::Image> createImage(VkDevice device,
                                                          VmaAllocator allocator,
                                                          VkExtent3D allocSize,
                                                          VkFormat format,
                                                          VkImageUsageFlags usageFlags,
                                                          uint32_t mipLevels = 1);

    /**
     * @brief Uploads image data into existing allocated GPU image
     * @param allocator VMA allocator
     * @param data Image data bytes
     * @param image GPU allocated image
     */
    void fillImage(VmaAllocator allocator,
                   const void *data,
                   VktTypes::Resources::Image image);

    /**
     * @brief Equivalent to createImage and fillImage, with VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL set to usageFlags
     * @param device Vulkan device
     * @param allocator VMA allocator
     * @param data Image data bytes
     * @param allocSize Extent with size of the image
     * @param format Image format
     * @param usageFlags Usage flags
     * @param mipMapped If true the image will be mipmapped, defaults to false
     * @return
     */
    std::optional<VktTypes::Resources::Image> createFilledImage(VkDevice device,
                                                                VmaAllocator allocator,
                                                                const void *data,
                                                                VkExtent3D allocSize,
                                                                VkFormat format,
                                                                VkImageUsageFlags usageFlags,
                                                                uint32_t mipLevels = 1);

    /**
     * @brief Destroys a GPU image
     * @param device Vulkan device
     * @param allocator VMA allocator
     * @param image GPU allocated image
     */
    void destroyImage(VkDevice device, VmaAllocator allocator, VktTypes::Resources::Image image);

}// namespace VktImages