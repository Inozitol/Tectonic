#pragma once

#include "Logger.h"

#include <unordered_map>
#include <vulkan/vulkan_core.h>

/**
 * Singleton class for caching various resources. \n
 * This class will not maintain the lifetime of these resources.
 */
class VktCache {
public:
    VktCache(VktCache const&) = delete;
    void operator=(VktCache const&) = delete;

    enum Layout : uint8_t {
        DRAW_IMAGE = 0,
        SCENE,
        SKYBOX,
        MAT_METAL_ROUGHNESS,
        IBL_ROUGHNESS,
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
    static std::unordered_map<Layout, VkDescriptorSetLayout>& getAllLayouts();

    /**
     * @brief Constructs and returns an array of specified layouts.
     * @notes Will return VK_NULL_HANDLE in place of missing layouts.
     * @param id ID of layout
     * @param args Rest of IDs
     * @return Array of layouts
     */
    template<typename ...Args>
    static auto getLayouts(Layout id, Args... args) {
        auto& instance = getInstance();

        constexpr size_t N = sizeof...(Args)+1;
        Layout ids[N] = {id, args...};
        std::array<VkDescriptorSetLayout, N> output;
        for(size_t i = 0; i < N; i++) {
            if(!instance.m_layouts.contains(ids[i])) {
                instance.m_logger(Logger::WARNING) << "Trying to get a layout with ID " << ids[i] << ", but it's not cached\n";
                output[i] = VK_NULL_HANDLE;
            }else {
                output[i] = instance.m_layouts[ids[i]];
            }
        }
        return output;
    }

private:
    static VktCache& getInstance();
    VktCache() = default;

    std::unordered_map<Layout, VkDescriptorSetLayout> m_layouts;
    Logger m_logger = Logger("VktCache");
};

