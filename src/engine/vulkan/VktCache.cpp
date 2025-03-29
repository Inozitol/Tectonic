
#include "engine/vulkan/VktCache.h"
#include "utils/Logger.h"

bool VktCache::storeLayout(Layout id, VkDescriptorSetLayout layout) {
    if(m_layouts.contains(id)) {
        LOG(LOG_WARNING,"Trying to store a layout with ID " << Utils::enumVal(id) << ", but it's already cached");
        return false;
    }
    m_layouts[id] = layout;
    LOG(LOG_DEBUG, "Stored layout with id " << static_cast<uint32_t>(Utils::enumVal(id)));
    return true;
}

VkDescriptorSetLayout VktCache::getLayout(Layout id) {
    if(!m_layouts.contains(id)) {
        LOG(LOG_WARNING, "Trying to get a layout with ID " << Utils::enumVal(id) << ", but it's not cached\n");
        return VK_NULL_HANDLE;
    }
    return m_layouts[id];
}

bool VktCache::deleteLayout(Layout id) {
    if(!m_layouts.contains(id)) {
        LOG(LOG_WARNING, "Trying to delete a layout with ID " << Utils::enumVal(id) << ", but it's not cached\n");
        return false;
    }
    m_layouts.erase(id);
    LOG(LOG_DEBUG, "Deleted a layout with id " << Utils::enumVal(id));
    return true;
}

std::unordered_map<VktCache::Layout, VkDescriptorSetLayout> &VktCache::getAllLayouts() {
    return m_layouts;
}

bool VktCache::storeSampler(Sampler id, VkSampler sampler) {
    if(m_samplers.contains(id)) {
        LOG(LOG_WARNING, "Trying to store an image sampler with ID " << Utils::enumVal(id) << ", but it's already cached\n");
        return false;
    }
    m_samplers[id] = sampler;
    LOG(LOG_DEBUG, "Stored an image sampler with id " << Utils::enumVal(id));
    return true;
}

VkSampler VktCache::getSampler(Sampler id) {
    if(!m_samplers.contains(id)) {
        LOG(LOG_WARNING, "Trying to get an image sampler with ID " << Utils::enumVal(id) << ", but it's not cached\n");
        return VK_NULL_HANDLE;
    }
    return m_samplers[id];
}

bool VktCache::deleteSampler(Sampler id) {
    if(!m_samplers.contains(id)) {
        LOG(LOG_WARNING, "Trying to delete an image sampler with ID " << Utils::enumVal(id) << ", but it's not cached\n");
        return false;
    }
    m_samplers.erase(id);
    LOG(LOG_DEBUG, "Deleted an image sampler with id " << Utils::enumVal(id));
    return true;
}
