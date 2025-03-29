#pragma once

#include "VktDescriptorUtils.h"
#include "utils/Utils.h"
#include "utils/Logger.h"

#include <ktxvulkan.h>
#include <unordered_map>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

/**
 * Singleton class for caching various resources. \n
 * This class will not maintain the lifetime of these resources.
 */
struct VktCache {
    VktCache() = default;

    enum class Layout : uint8_t {
        DRAW_IMAGE = 0,
        SCENE,
        SKYBOX,
        MAT_METAL_ROUGHNESS,
        IBL_ROUGHNESS,
        IBL_BRDF
    };

    enum class Sampler : uint8_t {
        LINEAR,
        NEAREST
    };

    /**
     * @brief Stores a descriptor layout.
     * @param id ID of layout
     * @param layout Descriptor Layout
     * @return True if succesfull
     */
    bool storeLayout(Layout id, VkDescriptorSetLayout layout);

    /**
     * @brief Returns a handle for a descriptor layout
     * @param id ID of layout
     * @return Descriptor Layout
     */
    VkDescriptorSetLayout getLayout(Layout id);

    /**
     * @brief Deletes a handle for a descriptor layout.
     * @note This will not delete it from GPU memory.
     * @param id ID of layout
     * @return True if succesfull
     */
    bool deleteLayout(Layout id);

    /**
     * @brief Returns the map with stored layouts
     * @return Map of all layouts
     */
    std::unordered_map<Layout, VkDescriptorSetLayout> &getAllLayouts();

    /**
     * @brief Constructs and returns an array of specified layouts.
     * @note Will return VK_NULL_HANDLE in place of missing layouts.
     * @param id ID of layout
     * @param args Rest of IDs
     * @return Array of layouts
     */
    template<typename... Args>
    auto getLayouts(Layout id, Args... args) {
        constexpr size_t N = sizeof...(Args) + 1;
        Layout ids[N] = {id, args...};
        std::array<VkDescriptorSetLayout, N> output;
        for(size_t i = 0; i < N; i++) {
            if(!m_layouts.contains(ids[i])) {
                LOG(LOG_WARNING, "Trying to get a layout with ID " << Utils::enumVal(ids[i]) << ", but it's not cached");
                output[i] = VK_NULL_HANDLE;
            } else {
                output[i] = m_layouts[ids[i]];
            }
        }
        return output;
    }


    /**
     * @brief Stores an image sampler.
     * @param id ID of sampler
     * @param sampler Sampler
     * @return True if succesfull
     */
    bool storeSampler(Sampler id, VkSampler sampler);

    /**
     * @brief Returns a handle for an image sampler
     * @param id ID of an image sampler
     * @return Image sampler
     */
    VkSampler getSampler(Sampler id);

    /**
     * @brief Deletes an image sampler.
     * @note This will not delete it from GPU memory.
     * @param id ID of an image sampler
     * @return True if succesfull
     */
    bool deleteSampler(Sampler id);

    VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
    VkInstance vkInstance = VK_NULL_HANDLE;
    VkDevice vkDevice = VK_NULL_HANDLE;
    VmaAllocator vmaAllocator = VK_NULL_HANDLE;
    DescriptorAllocatorDynamic descriptorAllocator{};
    VkExtent2D drawExtent{};
    ktxVulkanDeviceInfo ktxInfo;

    std::unordered_map<Layout, VkDescriptorSetLayout> m_layouts;
    std::unordered_map<Sampler, VkSampler> m_samplers;
};