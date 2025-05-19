#pragma once

#include "pugixml.hpp"

#include <filesystem>
#include <vulkan/vulkan_core.h>
#include <vector>

struct VktLayout {
    bool loadXml(const std::filesystem::path& path);
    void loadBinding(const pugi::xml_node& node);
    void loadStages(const pugi::xml_node& node);
    bool buildLayout();
    std::string name;
    VkShaderStageFlags stageFlags;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    VkDescriptorSetLayout layout;
};