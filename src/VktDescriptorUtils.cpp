#include "engine/vulkan/VktDescriptorUtils.h"

#include "engine/vulkan/VktCache.h"

void DescriptorLayoutBuilder::addBinding(uint32_t binding, VkDescriptorType type) {
    VkDescriptorSetLayoutBinding newbind{};
    newbind.binding = binding;
    newbind.descriptorCount = 1;
    newbind.descriptorType = type;

    bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::clear() {
    bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkShaderStageFlags shaderStages) {
    for(auto& binding : bindings){
        binding.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .pNext = nullptr };
    info.pBindings = bindings.data();
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.flags = 0;

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(VktCache::vkDevice, &info, nullptr, &set));

    return set;
}

void DescriptorAllocator::initPool(uint32_t maxSets, std::span<PoolSizeRatio> poolRatios) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{
                .type = ratio.type,
                .descriptorCount = static_cast<uint32_t>(ratio.ratio * static_cast<float>(maxSets))
        });
    }

    VkDescriptorPoolCreateInfo poolInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .pNext = nullptr };
    poolInfo.flags = 0;
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    vkCreateDescriptorPool(VktCache::vkDevice, &poolInfo, nullptr, &pool);
}

void DescriptorAllocator::clearDescriptors() {
    vkResetDescriptorPool(VktCache::vkDevice, pool, 0);
}

void DescriptorAllocator::destroyPool() {
    vkDestroyDescriptorPool(VktCache::vkDevice, pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::allocate(VkDescriptorSetLayout layout) {
    VkDescriptorSetAllocateInfo allocInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .pNext = nullptr };
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet set;
    VK_CHECK(vkAllocateDescriptorSets(VktCache::vkDevice, &allocInfo, &set));
    return set;
}

void DescriptorAllocatorDynamic::initPool(uint32_t initSets, std::span<PoolSizeRatio> poolRatios) {
    m_ratios.clear();

    for(PoolSizeRatio ratio : poolRatios){
        m_ratios.push_back(ratio);
    }

    VkDescriptorPool newPool = createPool(initSets, poolRatios);
    m_setsPerPool = static_cast<uint32_t>(static_cast<float>(initSets) * 1.5f);
    m_readyPools.push_back(newPool);
}

void DescriptorAllocatorDynamic::clearPools() {
    for(VkDescriptorPool pool : m_readyPools){
        vkResetDescriptorPool(VktCache::vkDevice, pool, 0);
    }
    for(VkDescriptorPool pool : m_fullPools){
        vkResetDescriptorPool(VktCache::vkDevice, pool, 0);
        m_readyPools.push_back(pool);
    }
    m_fullPools.clear();
}

void DescriptorAllocatorDynamic::destroyPool() {
    for(VkDescriptorPool pool : m_readyPools){
        vkDestroyDescriptorPool(VktCache::vkDevice, pool, nullptr);
    }
    m_readyPools.clear();
    for(VkDescriptorPool pool : m_fullPools){
        vkDestroyDescriptorPool(VktCache::vkDevice, pool, nullptr);
    }
    m_fullPools.clear();
}

VkDescriptorSet DescriptorAllocatorDynamic::allocate(VkDescriptorSetLayout layout) {
    VkDescriptorPool poolToUse = getPool();

    VkDescriptorSetAllocateInfo allocInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .pNext = nullptr};
    allocInfo.descriptorPool = poolToUse;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descSet;
    VkResult result = vkAllocateDescriptorSets(VktCache::vkDevice, &allocInfo, &descSet);

    if(result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL){
        m_fullPools.push_back(poolToUse);
        poolToUse = getPool();
        allocInfo.descriptorPool = poolToUse;
        VK_CHECK(vkAllocateDescriptorSets(VktCache::vkDevice, &allocInfo, &descSet));
    }
    m_readyPools.push_back(poolToUse);
    return descSet;
}

VkDescriptorPool DescriptorAllocatorDynamic::getPool() {
    VkDescriptorPool newPool;
    if(!m_readyPools.empty()){
        newPool = m_readyPools.back();
        m_readyPools.pop_back();
    }else{
        newPool = createPool( m_setsPerPool, m_ratios);
        m_setsPerPool = static_cast<uint32_t>(static_cast<float>(m_setsPerPool) * 1.5f);
        if(m_setsPerPool > MAX_POOL_SETS){
            m_setsPerPool = MAX_POOL_SETS;
        }
    }
    return newPool;
}

VkDescriptorPool DescriptorAllocatorDynamic::createPool(uint32_t setCount, std::span<PoolSizeRatio> poolRatios) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for(PoolSizeRatio ratio : poolRatios){
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = static_cast<uint32_t>(static_cast<float>(setCount) * ratio.ratio)
        });
    }

    VkDescriptorPoolCreateInfo poolInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .pNext = nullptr};
    poolInfo.maxSets = setCount;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    VkDescriptorPool newPool;
    VK_CHECK(vkCreateDescriptorPool(VktCache::vkDevice, &poolInfo, nullptr, &newPool));
    return newPool;
}

void DescriptorWriter::writeBuffer(uint32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type) {
    VkDescriptorBufferInfo& info = bufferInfos.emplace_back(VkDescriptorBufferInfo{
        .buffer = buffer,
        .offset = offset,
        .range = size
    });
    VkWriteDescriptorSet write{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr};
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;

    writes.push_back(write);
}

void DescriptorWriter::writeImage(uint32_t binding, VkImageView view, VkSampler sampler, VkImageLayout layout, VkDescriptorType type) {
    VkDescriptorImageInfo& info = imageInfos.emplace_back(VkDescriptorImageInfo{
            .sampler = sampler,
            .imageView = view,
            .imageLayout = layout
    });
    VkWriteDescriptorSet write{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr};
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;

    writes.push_back(write);
}

void DescriptorWriter::clear() {
    imageInfos.clear();
    bufferInfos.clear();
    writes.clear();
}

void DescriptorWriter::updateSet(VkDescriptorSet set) {
    for(VkWriteDescriptorSet& write: writes){
        write.dstSet = set;
    }
    vkUpdateDescriptorSets(VktCache::vkDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
