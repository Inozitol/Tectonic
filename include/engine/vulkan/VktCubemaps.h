#pragma once

#include "VktBuffers.h"
#include "VktInstantCommands.h"
#include "VktTypes.h"

#include <array>
#include <vulkan/vulkan_core.h>

namespace VktCubemaps {

    /**
     * @brief Allocates an empty 6 layered image on GPU
     * @param device Vulkan device
     * @param allocator VMA allocator
     * @param allocSize Extent with size of the image
     * @param format Image format
     * @param usageFlags Usage flags
     * @param mipMapped If true the images will be mipmapped, defaults to false
     * @return GPU allocated cubemap
     */
    std::optional<VktTypes::Resources::Cubemap> createCubemap(VkDevice device,
                                                            VmaAllocator allocator,
                                                            VkExtent3D allocSize,
                                                            VkFormat format,
                                                            VkImageUsageFlags usageFlags,
                                                            uint32_t mipLevels = 1);
    /**
     * @brief Fills an existing cubemap on GPU with data
     * @param allocator VMA allocator
     * @param cubemap GPU allocated cubemap
     * @param data Array of 6 images with cubemap data
     */
    void fillCubemap(VmaAllocator allocator,
                     VktTypes::Resources::Cubemap cubemap,
                     std::array<void *, 6> data);

    /**
     * @brief Creates a cubemap from a directory with 6 images
     * @param device Vulkan device
     * @param allocator VMA allocator
     * @param directory Cubemap directory
     * @return GPU allocated and filled cubemap
     */
    std::optional<VktTypes::Resources::Cubemap> createCubemapFromDir(VkDevice device, VmaAllocator allocator, const char *directory);

    /**
     * @brief Destroys a GPU allocated cubemap
     * @param device Vulkan device
     * @param allocator VMA allocator
     * @param cubemap GPU allocated cubemap
     */
    void destroyCubemap(VkDevice device, VmaAllocator allocator, VktTypes::Resources::Cubemap cubemap);
}// namespace VktCubemaps