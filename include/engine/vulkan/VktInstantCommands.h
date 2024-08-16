#pragma once

#include <functional>
#include <vulkan/vulkan_core.h>

class VktInstantCommands {
public:
    VktInstantCommands(VktInstantCommands const &) = delete;
    void operator=(VktInstantCommands const &) = delete;
    static VktInstantCommands &getInstance();

    static void submitCommands(std::function<void(VkCommandBuffer cmd)>&& func);

    static void init(uint32_t queueIndex, VkQueue queue);
    static void clear();

    inline static uint32_t vkQueueIndex = UINT32_MAX;
    inline static VkQueue vkQueue = VK_NULL_HANDLE;
    inline static VkFence vkFence = VK_NULL_HANDLE;
    inline static VkCommandPool vkCmdPool = VK_NULL_HANDLE;
    inline static VkCommandBuffer vkCmdBuffer = VK_NULL_HANDLE;

private:
    VktInstantCommands() = default;

    bool isInitialized = false;
};
