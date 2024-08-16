#ifndef TECTONIC_VKTDESCRIPTORS_H
#define TECTONIC_VKTDESCRIPTORS_H

#include <vulkan/vulkan.h>
#include <vector>
#include <span>
#include <deque>

#include "VktUtils.h"

struct DescriptorLayoutBuilder {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void addBinding(uint32_t binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(VkShaderStageFlags shaderStages);
};

struct DescriptorAllocator{
    struct PoolSizeRatio{
        VkDescriptorType type;
        float ratio;
    };

    VkDescriptorPool pool;

    void initPool(uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
    void clearDescriptors();
    void destroyPool();

    VkDescriptorSet allocate(VkDescriptorSetLayout layout);
};

struct DescriptorAllocatorDynamic{
    struct PoolSizeRatio{
        VkDescriptorType type;
        float ratio;
    };

    void initPool(uint32_t initSets, std::span<PoolSizeRatio> poolRatios);
    void clearPools();
    void destroyPool();

    VkDescriptorSet allocate(VkDescriptorSetLayout layout);

    static constexpr uint32_t MAX_POOL_SETS = 4092;

private:
    VkDescriptorPool getPool();
    VkDescriptorPool createPool(uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio> m_ratios;
    std::vector<VkDescriptorPool> m_fullPools;
    std::vector<VkDescriptorPool> m_readyPools;
    uint32_t m_setsPerPool = 0;
};

struct DescriptorWriter{
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;

    void writeImage(uint32_t binding, VkImageView view, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    void writeImages(uint32_t binding, std::vector<VkImageView> views, std::vector<VkSampler> samplers, std::vector<VkImageLayout> layouts, VkDescriptorType type);
    void writeBuffer(uint32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

    void clear();
    void updateSet(VkDescriptorSet set);
};


#endif //TECTONIC_VKTDESCRIPTORS_H
