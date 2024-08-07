#include "engine/vulkan/VktBuffers.h"

namespace VktBuffers {

    VktTypes::Resources::Buffer createBuffer(VmaAllocator allocator,
                                           size_t allocSize,
                                           VkBufferUsageFlags usage,
                                           VmaMemoryUsage memoryUsage) {
        VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .pNext = nullptr};
        bufferInfo.size = allocSize;
        bufferInfo.usage = usage;

        VmaAllocationCreateInfo vmaAllocInfo{};
        vmaAllocInfo.usage = memoryUsage;
        vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VktTypes::Resources::Buffer newBuffer{};
        VK_CHECK(vmaCreateBuffer(allocator,
                                 &bufferInfo,
                                 &vmaAllocInfo,
                                 &newBuffer.buffer,
                                 &newBuffer.allocation,
                                 &newBuffer.info))
        return newBuffer;
    }

    void destroyBuffer(VmaAllocator allocator, VktTypes::Resources::Buffer buffer) {
        vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
    }


}