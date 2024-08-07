#pragma once

#include <functional>
#include <vulkan/vulkan_core.h>

class VktInstantCommands {
public:
    VktInstantCommands(VktInstantCommands const &) = delete;
    void operator=(VktInstantCommands const &) = delete;
    static VktInstantCommands &getInstance();

    static void submitCommands(std::function<void(VkCommandBuffer cmd)>&& func);

    static void init(VkDevice device, uint32_t queueIndex, VkQueue queue);
    static void clear();

private:
    VktInstantCommands() = default;

    uint32_t m_queueIndex = UINT32_MAX;
    VkQueue m_queue = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkFence m_fence = VK_NULL_HANDLE;
    VkCommandPool m_cmdPool = VK_NULL_HANDLE;
    VkCommandBuffer m_cmdBuffer = VK_NULL_HANDLE;
    bool isInitialized = false;
};
