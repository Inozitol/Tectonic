
#include "../include/engine/vulkan/VktCache.h"

VktCache & VktCache::getInstance() {
    static VktCache instance;
    return instance;
}

bool VktCache::storeLayout(Layout id, VkDescriptorSetLayout layout) {
    auto& instance = getInstance();
    if(instance.m_layouts.contains(id)) {
        instance.m_logger(Logger::WARNING) << "Trying to store a layout with ID " << Utils::enumVal(id) << ", but it's already cached\n";
        return false;
    }
    instance.m_layouts[id] = layout;
    instance.m_logger(Logger::DEBUG) << "Stored layout with id " << Utils::enumVal(id) << "\n";
    return true;
}

VkDescriptorSetLayout VktCache::getLayout(Layout id) {
    auto& instance = getInstance();
    if(!instance.m_layouts.contains(id)) {
        instance.m_logger(Logger::WARNING) << "Trying to get a layout with ID " << Utils::enumVal(id) << ", but it's not cached\n";
        return VK_NULL_HANDLE;
    }
    return instance.m_layouts[id];
}

bool VktCache::deleteLayout(Layout id) {
    auto &instance = getInstance();
    if(!instance.m_layouts.contains(id)) {
        instance.m_logger(Logger::WARNING) << "Trying to delete a layout with ID " << Utils::enumVal(id) << ", but it's not cached\n";
        return false;
    }
    instance.m_layouts.erase(id);
    instance.m_logger(Logger::DEBUG) << "Deleted a layout with id " << Utils::enumVal(id) << "\n";
    return true;
}

std::unordered_map<VktCache::Layout, VkDescriptorSetLayout> &VktCache::getAllLayouts() {
    return getInstance().m_layouts;
}

bool VktCache::storeSampler(Sampler id, VkSampler sampler) {
    auto& instance = getInstance();
    if(instance.m_samplers.contains(id)) {
        instance.m_logger(Logger::WARNING) << "Trying to store an image sampler with ID " << Utils::enumVal(id) << ", but it's already cached\n";
        return false;
    }
    instance.m_samplers[id] = sampler;
    instance.m_logger(Logger::DEBUG) << "Stored an image sampler with id " << Utils::enumVal(id) << "\n";
    return true;
}

VkSampler VktCache::getSampler(Sampler id) {
    auto& instance = getInstance();
    if(!instance.m_samplers.contains(id)) {
        instance.m_logger(Logger::WARNING) << "Trying to get an image sampler with ID " << Utils::enumVal(id) << ", but it's not cached\n";
        return VK_NULL_HANDLE;
    }
    return instance.m_samplers[id];
}

bool VktCache::deleteSampler(Sampler id) {
    auto &instance = getInstance();
    if(!instance.m_samplers.contains(id)) {
        instance.m_logger(Logger::WARNING) << "Trying to delete an image sampler with ID " << Utils::enumVal(id) << ", but it's not cached\n";
        return false;
    }
    instance.m_samplers.erase(id);
    instance.m_logger(Logger::DEBUG) << "Deleted an image sampler with id " << Utils::enumVal(id) << "\n";
    return true;
}
