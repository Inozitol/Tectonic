#ifndef TECTONIC_VKTPIPELINES_H
#define TECTONIC_VKTPIPELINES_H

#include <array>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "VktUtils.h"

struct VktPipelineBuilder{
    explicit VktPipelineBuilder();
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

    std::unordered_map<VkShaderStageFlagBits, VkShaderModule> shaderStages;
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_POLYGON_MODE_EXT
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineMultisampleStateCreateInfo multisampling{};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineRenderingCreateInfo renderInfo{};
    VkFormat colorAttachmentFormat{};

    VkPipelineLayout layout{};
};

#endif //TECTONIC_VKTPIPELINES_H
