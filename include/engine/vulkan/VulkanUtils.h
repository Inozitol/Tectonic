#ifndef TECTONIC_VULKANUTILS_H
#define TECTONIC_VULKANUTILS_H

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <fstream>
#include <vector>

#include "VulkanStructs.h"
#include "exceptions.h"

#define VK_CHECK(x)                                                             \
    do{                                                                         \
        VkResult err = x;                                                       \
        if (err) {                                                              \
            fprintf(stderr, "Detected Vulkan error: %s", string_VkResult(err)); \
            abort();                                                            \
        }                                                                       \
    }while(0);

namespace VkUtils{
    void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
    void copyImgToImg(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcExtent, VkExtent2D dstExtent);
    VkShaderModule loadShaderModule(const char* path, VkDevice device);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
}

#endif //TECTONIC_VULKANUTILS_H
