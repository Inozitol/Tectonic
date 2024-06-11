#ifndef TECTONIC_VKTPIPELINES_H
#define TECTONIC_VKTPIPELINES_H

#include <vector>
#include <array>
#include <vulkan/vulkan.h>
#include "VktUtils.h"

class VktPipelineBuilder{
public:
    VktPipelineBuilder();
    void clear();
    VkPipeline buildPipeline(VkDevice device);
    void setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader, VkShaderModule geometryShader = VK_NULL_HANDLE);
    void setInputTopology(VkPrimitiveTopology topology);
    void setPolygonMode(VkPolygonMode mode);
    void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void setColorAttachmentFormat(VkFormat format);
    void setDepthFormat(VkFormat format);
    void setPipelineLayout(VkPipelineLayout layout);
    void setMultisamplingNone();
    void enableDepthTest(bool depthWriteEnable, VkCompareOp op);
    void enableBlendingAdditive();
    void enableBlendingAlpha();
    void disableBlending();
    void disableDepthTest();
private:
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;

    VkPipelineInputAssemblyStateCreateInfo m_inputAssembly;
    VkPipelineRasterizationStateCreateInfo m_rasterizer;
    VkPipelineColorBlendAttachmentState m_colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo m_multisampling;
    VkPipelineDepthStencilStateCreateInfo m_depthStencil;
    VkPipelineRenderingCreateInfo m_renderInfo;
    VkFormat m_colorAttachmentFormat;

    VkPipelineLayout m_layout;
};

#endif //TECTONIC_VKTPIPELINES_H
