#pragma once

#include "Logger.h"
#include "VktDescriptorUtils.h"
#include "utils/utils.h"

#include <unordered_map>
#include <vk_mem_alloc.h>
#include <ktxvulkan.h>
#include <vulkan/vulkan_core.h>

/**
 * Singleton class for caching various resources. \n
 * This class will not maintain the lifetime of these resources.
 */
class VktCache {
public:
    VktCache(VktCache const &) = delete;
    void operator=(VktCache const &) = delete;

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
    static bool storeLayout(Layout id, VkDescriptorSetLayout layout);

    /**
     * @brief Returns a handle for a descriptor layout
     * @param id ID of layout
     * @return Descriptor Layout
     */
    static VkDescriptorSetLayout getLayout(Layout id);

    /**
     * @brief Deletes a handle for a descriptor layout.
     * @note This will not delete it from GPU memory.
     * @param id ID of layout
     * @return True if succesfull
     */
    static bool deleteLayout(Layout id);

    /**
     * @brief Returns the map with stored layouts
     * @return Map of all layouts
     */
    static std::unordered_map<Layout, VkDescriptorSetLayout> &getAllLayouts();

    /**
     * @brief Constructs and returns an array of specified layouts.
     * @note Will return VK_NULL_HANDLE in place of missing layouts.
     * @param id ID of layout
     * @param args Rest of IDs
     * @return Array of layouts
     */
    template<typename... Args>
    static auto getLayouts(Layout id, Args... args) {
        auto &instance = getInstance();

        constexpr size_t N = sizeof...(Args) + 1;
        Layout ids[N] = {id, args...};
        std::array<VkDescriptorSetLayout, N> output;
        for(size_t i = 0; i < N; i++) {
            if(!instance.m_layouts.contains(ids[i])) {
                instance.m_logger(Logger::WARNING) << "Trying to get a layout with ID " << Utils::enumVal(ids[i]) << ", but it's not cached\n";
                output[i] = VK_NULL_HANDLE;
            } else {
                output[i] = instance.m_layouts[ids[i]];
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
    static bool storeSampler(Sampler id, VkSampler sampler);

    /**
     * @brief Returns a handle for an image sampler
     * @param id ID of an image sampler
     * @return Image sampler
     */
    static VkSampler getSampler(Sampler id);

    /**
     * @brief Deletes an image sampler.
     * @note This will not delete it from GPU memory.
     * @param id ID of an image sampler
     * @return True if succesfull
     */
    static bool deleteSampler(Sampler id);

    inline static VkInstance vkInstance = VK_NULL_HANDLE;
    inline static VkDevice vkDevice = VK_NULL_HANDLE;
    inline static VmaAllocator vmaAllocator = VK_NULL_HANDLE;
    inline static DescriptorAllocatorDynamic descriptorAllocator{};
    inline static VkExtent2D drawExtent{};
    inline static ktxVulkanDeviceInfo ktxInfo;

private:
    static VktCache &getInstance();
    VktCache() = default;

    std::unordered_map<Layout, VkDescriptorSetLayout> m_layouts;
    std::unordered_map<Sampler, VkSampler> m_samplers;
    Logger m_logger = Logger("VktCache");
};