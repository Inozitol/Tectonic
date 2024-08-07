#pragma once

#include "VktTypes.h"

#include <vulkan/vulkan_core.h>

namespace VktBuffers {

    /**
     * @brief Allocates and returns a GPU buffer of given size and usage.
     *
     * @param allocator VMA allocator
     * @param allocSize Size of the buffer in bytes
     * @param usage Usage flags
     * @param memoryUsage Memory usage
     * @return Allocated GPU buffer
     */
    VktTypes::Resources::Buffer createBuffer(VmaAllocator allocator,
                                           size_t allocSize,
                                           VkBufferUsageFlags usage,
                                           VmaMemoryUsage memoryUsage);

    /**
     * @brief Destroys a GPU buffer
     * @param allocator VMA allocator
     * @param buffer Allocated GPU buffer
     */
    void destroyBuffer(VmaAllocator allocator, VktTypes::Resources::Buffer buffer);

}// namespace VktBuffers