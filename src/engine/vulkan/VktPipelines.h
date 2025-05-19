#ifndef TECTONIC_VKTPIPELINES_H
#define TECTONIC_VKTPIPELINES_H

#include <array>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <filesystem>
#include <pugixml.hpp>

#include "VktUtils.h"

struct VktPipeline {
    struct Shader {
        std::filesystem::path path;
        VkShaderStageFlagBits stage;
        VkShaderModule module;
    };

    explicit VktPipeline() = default;
    ~VktPipeline() = default;
    bool loadXml(const std::filesystem::path& path);
    static std::optional<const Shader*> loadShader(const pugi::xml_node& node);
    static std::optional<std::vector<const Shader*>> loadShaders(const pugi::xml_node& node);
    static void compileShader(const std::filesystem::path& glslPath, const std::filesystem::path& spirvPath);
    void loadTopology(const pugi::xml_node& node);
    void loadRasterizer(const pugi::xml_node& node);
    void loadRenderInfo(const pugi::xml_node& node);
    void loadBlending(const pugi::xml_node& node);
    void loadDepthTest(const pugi::xml_node& node);
    void loadDynamicStates(const pugi::xml_node& node);
    bool buildPipeline();

    static inline std::unordered_map<std::filesystem::path, Shader> loadedShaders;

    std::vector<const Shader*> pipelineShaders{};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineRasterizationStateCreateInfo rasterizer{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };

    std::vector<VkFormat> colorAttachmentFormats{};
    VkPipelineRenderingCreateInfo renderInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = 0,
        .pColorAttachmentFormats = nullptr,
        .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineMultisampleStateCreateInfo multisampling {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_NEVER,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f
    };

    std::vector<VkDynamicState> dynamicStates;
    VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = 0,
        .pDynamicStates = nullptr,
    };

    std::string name;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

struct VktPipelineBuilder {
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
