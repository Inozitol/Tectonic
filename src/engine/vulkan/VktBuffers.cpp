#include "engine/vulkan/VktBuffers.h"

#include "engine/GlobalMemory.h"
#include "engine/vulkan/VktCache.h"

namespace VktBuffers {

    VktTypes::Resources::Buffer create(const size_t allocSize,
                                       const VkBufferUsageFlags usage,
                                       const VmaMemoryUsage memoryUsage) {
        const VkBufferCreateInfo bufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = allocSize,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr
        };

        VmaAllocationCreateInfo vmaAllocInfo{};
        vmaAllocInfo.usage = memoryUsage;
        vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VktTypes::Resources::Buffer newBuffer{};
        VK_CHECK(vmaCreateBuffer(VktCachePtr->vmaAllocator,
            &bufferInfo,
            &vmaAllocInfo,
            &newBuffer.buffer,
            &newBuffer.allocation,
            &newBuffer.info))
#ifdef VKT_DEBUG_ALLOCATION_NAMES
        const std::string debugName = "VkBuffer_size_" + std::to_string(allocSize);
        vmaSetAllocationName(VktCachePtr->vmaAllocator,newBuffer.allocation,debugName.c_str());
#endif
        return newBuffer;
    }

    void destroy(const VktTypes::Resources::Buffer &buffer) { vmaDestroyBuffer(VktCachePtr->vmaAllocator, buffer.buffer, buffer.allocation); }


}