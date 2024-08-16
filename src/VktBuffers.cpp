#include "engine/vulkan/VktBuffers.h"
#include "engine/vulkan/VktCache.h"

namespace VktBuffers {

    VktTypes::Resources::Buffer create(size_t allocSize,
                                           VkBufferUsageFlags usage,
                                           VmaMemoryUsage memoryUsage) {
        VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .pNext = nullptr};
        bufferInfo.size = allocSize;
        bufferInfo.usage = usage;

        VmaAllocationCreateInfo vmaAllocInfo{};
        vmaAllocInfo.usage = memoryUsage;
        vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VktTypes::Resources::Buffer newBuffer{};
        VK_CHECK(vmaCreateBuffer(VktCache::vmaAllocator,
                                 &bufferInfo,
                                 &vmaAllocInfo,
                                 &newBuffer.buffer,
                                 &newBuffer.allocation,
                                 &newBuffer.info))
        return newBuffer;
    }

    void destroy(VktTypes::Resources::Buffer buffer) {
        vmaDestroyBuffer(VktCache::vmaAllocator, buffer.buffer, buffer.allocation);
    }


}