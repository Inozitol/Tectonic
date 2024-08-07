#ifndef TECTONIC_VKTUTILS_H
#define TECTONIC_VKTUTILS_H

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <fstream>
#include <vector>

#include "VktStructs.h"
#include "exceptions.h"

#define VK_CHECK(x)                                                             \
    do{                                                                         \
        VkResult err = x;                                                       \
        if (err) {                                                              \
            fprintf(stderr, "Detected Vulkan error: %s", string_VkResult(err)); \
            abort();                                                            \
        }                                                                       \
    }while(0);

namespace VktUtils{
    void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, uint32_t mipLevels = VK_REMAINING_MIP_LEVELS);
    void transitionCubeMap(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, uint32_t mipLevels = VK_REMAINING_MIP_LEVELS);
    void copyImgToImg(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcExtent, VkExtent2D dstExtent);
    VkShaderModule loadShaderModule(const char* path, VkDevice device);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    std::vector<VkImageView> createImageMipViews(VkDevice device, VkImage image, VkFormat format, uint32_t mipLevels);
    std::vector<VkImageView> createCubemapMipViews(VkDevice device, VkImage cubemapImage, VkFormat format, uint32_t mipLevels);

}

#endif //TECTONIC_VKTUTILS_H
