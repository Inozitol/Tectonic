#pragma once

#include "VktTypes.h"

#include <vulkan/vulkan_core.h>

namespace VktBuffers {

    /**
     * @brief Allocates and returns a GPU buffer of given size and usage.
     *
     * @param allocSize Size of the buffer in bytes
     * @param usage Usage flags
     * @param memoryUsage Memory usage
     * @return Allocated GPU buffer
     */
    VktTypes::Resources::Buffer create(size_t allocSize,
                                             VkBufferUsageFlags usage,
                                             VmaMemoryUsage memoryUsage);

    /**
     * @brief Destroys a GPU buffer
     * @param buffer Allocated GPU buffer
     */
    void destroy(VktTypes::Resources::Buffer buffer);

}// namespace VktBuffers