//
// Created by hadriel on 8/7/24.
//

#include "../include/engine/vulkan/VktCache.h"

VktCache& VktCache::getInstance() {
    static VktCache instance;
    return instance;
}

bool VktCache::storeLayout(Layout id, VkDescriptorSetLayout layout) {
    auto& instance = getInstance();
    if(instance.m_layouts.contains(id)) {
        instance.m_logger(Logger::WARNING) << "Trying to store a layout with ID " << id << ", but it's already cached\n";
        return false;
    }
    instance.m_layouts[id] = layout;
    instance.m_logger(Logger::DEBUG) << "Stored layout with id " << id << "\n";
    return true;
}

VkDescriptorSetLayout VktCache::getLayout(Layout id) {
    auto& instance = getInstance();
    if(!instance.m_layouts.contains(id)) {
        instance.m_logger(Logger::WARNING) << "Trying to get a layout with ID " << id << ", but it's not cached\n";
        return VK_NULL_HANDLE;
    }
    return instance.m_layouts[id];
}

bool VktCache::deleteLayout(Layout id) {
    auto &instance = getInstance();
    if(!instance.m_layouts.contains(id)) {
        instance.m_logger(Logger::WARNING) << "Trying to delete a layout with ID " << id << ", but it's not cached\n";
        return false;
    }
    instance.m_layouts.erase(id);
    instance.m_logger(Logger::DEBUG) << "Deleted a layout with id " << id << "\n";
    return true;
}

std::unordered_map<VktCache::Layout, VkDescriptorSetLayout> &VktCache::getAllLayouts() {
    return getInstance().m_layouts;
}
