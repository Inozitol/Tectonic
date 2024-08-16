
#include "engine/vulkan/VktInstantCommands.h"

#include "engine/vulkan/VktStructs.h"
#include "engine/vulkan/VktUtils.h"

#include <cassert>
#include <engine/vulkan/VktCache.h>

VktInstantCommands &VktInstantCommands::getInstance() {
    static VktInstantCommands instance;
    return instance;
}

void VktInstantCommands::init(uint32_t queueIndex, VkQueue queue) {
    VktInstantCommands &instance = getInstance();
    assert(!instance.isInitialized);

    vkQueueIndex = queueIndex;
    vkQueue = queue;

    // Create VkFence
    const VkFenceCreateInfo fenceCreateInfo = VktStructs::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(VktCache::vkDevice, &fenceCreateInfo, nullptr, &vkFence));

    // Create command pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = VktStructs::commandPoolCreateInfo(queueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(VktCache::vkDevice, &commandPoolCreateInfo, nullptr, &vkCmdPool));

    // Create command buffer
    VkCommandBufferAllocateInfo cmdAllocInfo = VktStructs::commandBufferAllocateInfo(vkCmdPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(VktCache::vkDevice, &cmdAllocInfo, &vkCmdBuffer))

    instance.isInitialized = true;
}

void VktInstantCommands::submitCommands(std::function<void(VkCommandBuffer cmd)> &&func) {
    VktInstantCommands &instance = getInstance();
    assert(instance.isInitialized);

    VK_CHECK(vkResetFences(VktCache::vkDevice, 1, &instance.vkFence))
    VK_CHECK(vkResetCommandBuffer(instance.vkCmdBuffer, 0))

    VkCommandBuffer cmd = vkCmdBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = VktStructs::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo))
    func(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd))

    VkCommandBufferSubmitInfo cmdInfo = VktStructs::commandBufferSubmitInfo(cmd);
    VkSubmitInfo2 submitInfo = VktStructs::submitInfo(&cmdInfo, nullptr, nullptr);

    VK_CHECK(vkQueueSubmit2(instance.vkQueue, 1, &submitInfo, instance.vkFence))
    VK_CHECK(vkWaitForFences(VktCache::vkDevice, 1, &instance.vkFence, VK_TRUE, 9999999999))
}

void VktInstantCommands::clear() {
    VktInstantCommands &instance = getInstance();
    assert(instance.isInitialized);

    vkDestroyFence(VktCache::vkDevice, vkFence, nullptr);
    vkDestroyCommandPool(VktCache::vkDevice, vkCmdPool, nullptr);
    instance.isInitialized = false;
}