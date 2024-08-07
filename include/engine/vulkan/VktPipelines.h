#ifndef TECTONIC_VKTPIPELINES_H
#define TECTONIC_VKTPIPELINES_H

#include <array>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "VktUtils.h"

class VktPipelineBuilder{
public:
    explicit VktPipelineBuilder(VkDevice device);
    ~VktPipelineBuilder();
    void clear();
    VkPipeline buildPipeline();
    void setVertexShader(const char* path);
    void setFragmentShader(const char* path);
    void setGeometryShader(const char* path);
    void setShaders(const char* vertexShaderPath, const char* fragmentShaderPath = nullptr, const char* geometryShaderPath = nullptr);
    void setShader(VkShaderStageFlagBits stageBit, const char* path);
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
    VkDevice m_device = VK_NULL_HANDLE;

    std::unordered_map<VkShaderStageFlagBits, VkShaderModule> m_shaderStages;

    VkPipelineInputAssemblyStateCreateInfo m_inputAssembly{};
    VkPipelineRasterizationStateCreateInfo m_rasterizer{};
    VkPipelineColorBlendAttachmentState m_colorBlendAttachment{};
    VkPipelineMultisampleStateCreateInfo m_multisampling{};
    VkPipelineDepthStencilStateCreateInfo m_depthStencil{};
    VkPipelineRenderingCreateInfo m_renderInfo{};
    VkFormat m_colorAttachmentFormat{};

    VkPipelineLayout m_layout{};
};

#endif //TECTONIC_VKTPIPELINES_H
