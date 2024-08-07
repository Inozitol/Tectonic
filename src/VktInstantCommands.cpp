
#include "engine/vulkan/VktInstantCommands.h"

#include "engine/vulkan/VktStructs.h"
#include "engine/vulkan/VktUtils.h"

#include <cassert>

VktInstantCommands &VktInstantCommands::getInstance() {
    static VktInstantCommands instance;
    return instance;
}

void VktInstantCommands::init(VkDevice device, uint32_t queueIndex, VkQueue queue) {
    VktInstantCommands &instance = getInstance();
    assert(!instance.isInitialized);

    instance.m_device = device;
    instance.m_queueIndex = queueIndex;
    instance.m_queue = queue;

    // Create VkFence
    const VkFenceCreateInfo fenceCreateInfo = VktStructs::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(instance.m_device, &fenceCreateInfo, nullptr, &instance.m_fence));

    // Create command pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = VktStructs::commandPoolCreateInfo(queueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(instance.m_device, &commandPoolCreateInfo, nullptr, &instance.m_cmdPool));

    // Create command buffer
    VkCommandBufferAllocateInfo cmdAllocInfo = VktStructs::commandBufferAllocateInfo(instance.m_cmdPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(instance.m_device, &cmdAllocInfo, &instance.m_cmdBuffer))

    instance.isInitialized = true;
}

void VktInstantCommands::submitCommands(std::function<void(VkCommandBuffer cmd)> &&func) {
    VktInstantCommands &instance = getInstance();
    assert(instance.isInitialized);

    VK_CHECK(vkResetFences(instance.m_device, 1, &instance.m_fence))
    VK_CHECK(vkResetCommandBuffer(instance.m_cmdBuffer, 0))

    VkCommandBuffer cmd = instance.m_cmdBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = VktStructs::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo))
    func(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd))

    VkCommandBufferSubmitInfo cmdInfo = VktStructs::commandBufferSubmitInfo(cmd);
    VkSubmitInfo2 submitInfo = VktStructs::submitInfo(&cmdInfo, nullptr, nullptr);

    VK_CHECK(vkQueueSubmit2(instance.m_queue, 1, &submitInfo, instance.m_fence))
    VK_CHECK(vkWaitForFences(instance.m_device, 1, &instance.m_fence, VK_TRUE, 9999999999))
}

void VktInstantCommands::clear() {
    VktInstantCommands &instance = getInstance();
    assert(instance.isInitialized);

    vkDestroyFence(instance.m_device, instance.m_fence, nullptr);
    vkDestroyCommandPool(instance.m_device, instance.m_cmdPool, nullptr);
    instance.isInitialized = false;
}