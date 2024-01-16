#ifndef TECTONIC_VULKANUTILS_H
#define TECTONIC_VULKANUTILS_H

#include <vulkan/vulkan.h>

#include "VulkanStructs.h"

namespace VkUtils{
    void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
}

#endif //TECTONIC_VULKANUTILS_H
