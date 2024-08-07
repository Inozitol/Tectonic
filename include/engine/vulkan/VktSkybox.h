#pragma once
#include <engine/model/Model.h>

#include "VktDescriptorUtils.h"

class VktSkybox {
public:
    VktSkybox(VkDevice device, VmaAllocator allocator);
private:

    void initPipeline();
    void initDescriptors();

    VktTypes::Resources::Cubemap m_colorCubemap;
    VktTypes::Resources::Cubemap m_diffuseCubemap;
    VktTypes::Resources::Cubemap m_specularCubemap;
    Model m_cube;

    VkDevice m_device;
    VmaAllocator m_allocator;

    static VkDescriptorSetLayout m_descriptorLayout;
    static VkDescriptorSet m_descriptorSet;
    static VktTypes::ModelPipeline m_pipeline;
    static VktTypes::Resources::Image m_BRDFTexture;
    static uint32_t m_skyboxCount;
};
