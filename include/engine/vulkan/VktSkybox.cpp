#include "VktSkybox.h"

uint32_t VktSkybox::m_skyboxCount = 0;

VktSkybox::VktSkybox(VkDevice device, VmaAllocator allocator)
    : m_device(device), m_allocator(allocator) {
    if(m_skyboxCount == 0) {
    }
}

void VktSkybox::initPipeline() {
    VkPushConstantRange pushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>)
    };

    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER); // Colors cubemap
        //m_descriptorLayout = builder.build(m_device, VK_SHADER_STAGE_FRAGMENT_BIT);
    }

}

void VktSkybox::initDescriptors() {

}