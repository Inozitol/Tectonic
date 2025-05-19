#include "VktLayout.h"

#include "VktUtils.h"
#include "engine/GlobalMemory.h"
#include "extern/vulkan-mini-libs-2/vk_value_serialization.hpp"

#include <cassert>
#include <utils/Logger.h>
#include <cstring>

bool VktLayout::loadXml(const std::filesystem::path &path) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(path.c_str());
    if(!result) {
        LOG(LOG_ERROR, "Failed to load XML file " << path << " | " << result.description());
        return false;
    }
    const auto layoutNode = doc.child("layout");
    if(layoutNode.attribute("name").empty()) {
        LOG(LOG_ERROR, "Failed to load layout name");
        return false;
    }
    name = layoutNode.attribute("name").as_string();

    if(layoutNode.child("stages").empty()) {
        LOG(LOG_ERROR, "Failed to load layout stages");
        return false;
    }
    loadStages(layoutNode.child("stages"));

    if(layoutNode.child("binding").empty()) {
        LOG(LOG_ERROR, "Failed to load layout bindings");
        return false;
    }
    for(pugi::xml_node bindingNode : layoutNode.children("binding")) {
        loadBinding(bindingNode);
    }

    return true;
}

void VktLayout::loadBinding(const pugi::xml_node &node) {
    assert(std::strcmp(node.name(), "binding") == 0);
    assert(!node.attribute("id").empty());
    uint32_t binding = node.attribute("id").as_uint();
    STecVkSerializationResult parseResult;

    VkDescriptorSetLayoutBinding newBinding{
        .binding = binding,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = stageFlags,
    };

    const std::string descriptorType = node.child_value();
    if(!descriptorType.empty()) {
        parseResult = vk_parse("VkDescriptorType", descriptorType.c_str(), &newBinding.descriptorType);
        if(parseResult != STEC_VK_SERIALIZATION_RESULT_SUCCESS) { LOG(LOG_ERROR, "Failed to load stencil attachment format"); }
    }

    bindings.push_back(newBinding);
}

void VktLayout::loadStages(const pugi::xml_node &node) {
    assert(std::strcmp(node.name(), "stages") == 0);
    STecVkSerializationResult parseResult;

    for(pugi::xml_node stageNode: node.children("stage")) {
        VkShaderStageFlags stage;
        parseResult = vk_parse("VkShaderStageFlags", stageNode.child_value(), &stage);
        if(parseResult != STEC_VK_SERIALIZATION_RESULT_SUCCESS) { LOG(LOG_ERROR, "Failed to load layout stage"); }
        stageFlags |= stage;
    }
}

bool VktLayout::buildLayout() {
    VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(VktCachePtr->vkDevice, &info, nullptr, &set));
    return true;
}
