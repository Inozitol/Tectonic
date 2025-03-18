#include "engine/vulkan/VktPipelines.h"

#include "engine/GlobalMemory.h"
#include "engine/vulkan/VktCache.h"


VktPipelineBuilder::VktPipelineBuilder() {
    clear();
}

VktPipelineBuilder::~VktPipelineBuilder() {
    for(const auto &[stage, module]: shaderStages) {
        vkDestroyShaderModule(VktCachePtr->vkDevice, module, nullptr);
    }
}

void VktPipelineBuilder::clear() {
    inputAssembly = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, .pNext = nullptr};
    rasterizer = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .pNext = nullptr};
    colorBlendAttachment = {};
    multisampling = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .pNext = nullptr};
    layout = {};
    depthStencil = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, .pNext = nullptr};
    renderInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    shaderStages.clear();
}

VkPipeline VktPipelineBuilder::buildPipeline() {
    VkPipelineViewportStateCreateInfo viewportState{.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .pNext = nullptr};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineColorBlendStateCreateInfo colorBlending{.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .pNext = nullptr};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineVertexInputStateCreateInfo m_vertexInputInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, .pNext = nullptr};

    std::vector<VkPipelineShaderStageCreateInfo> stages;
    shaderStages.reserve(shaderStages.size());
    for(const auto &[stage, module]: shaderStages) {
        stages.push_back(VktStructs::pipelineShaderStageCreateInfo(stage, module));
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.pNext = &renderInfo;
    pipelineInfo.pStages = stages.data();
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pVertexInputState = &m_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = layout;

    std::array state = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_POLYGON_MODE_EXT
    };

    VkPipelineDynamicStateCreateInfo dynamicInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .pNext = nullptr};
    dynamicInfo.pDynamicStates = state.data();
    dynamicInfo.dynamicStateCount = static_cast<uint32_t>(state.size());

    pipelineInfo.pDynamicState = &dynamicInfo;

    VkPipeline graphicsPipeline;
    VK_CHECK(vkCreateGraphicsPipelines(VktCachePtr->vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));
    return graphicsPipeline;
}

void VktPipelineBuilder::setVertexShader(const char *path) {
    setShader(VK_SHADER_STAGE_VERTEX_BIT, path);
}

void VktPipelineBuilder::setFragmentShader(const char *path) {
    setShader(VK_SHADER_STAGE_FRAGMENT_BIT, path);
}

void VktPipelineBuilder::setGeometryShader(const char *path) {
    setShader(VK_SHADER_STAGE_GEOMETRY_BIT, path);
}

void VktPipelineBuilder::setShaders(const char *vertexShaderPath, const char *fragmentShaderPath, const char *geometryShaderPath) {
    if(vertexShaderPath) {
        setShader(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderPath);
    }
    if(fragmentShaderPath) {
        setShader(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderPath);
    }
    if(geometryShaderPath) {
        setShader(VK_SHADER_STAGE_GEOMETRY_BIT, geometryShaderPath);
    }
}

void VktPipelineBuilder::setShader(VkShaderStageFlagBits stageBit, const char *path) {
    // Delete old shader in case its still cached
    if(shaderStages.contains(stageBit)) {
        vkDestroyShaderModule(VktCachePtr->vkDevice, shaderStages.at(stageBit), nullptr);
    }
    shaderStages[stageBit] = VktUtils::loadShaderModule(path);
}

void VktPipelineBuilder::setInputTopology(VkPrimitiveTopology topology) {
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void VktPipelineBuilder::setPolygonMode(VkPolygonMode mode) {
    rasterizer.polygonMode = mode;
    rasterizer.lineWidth = 1.0f;
}

void VktPipelineBuilder::setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace) {
    rasterizer.cullMode = cullMode;
    rasterizer.frontFace = frontFace;
}

void VktPipelineBuilder::setColorAttachmentFormat(VkFormat format) {
    colorAttachmentFormat = format;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachmentFormats = &colorAttachmentFormat;
}

void VktPipelineBuilder::setDepthFormat(VkFormat format) {
    renderInfo.depthAttachmentFormat = format;
}

void VktPipelineBuilder::setMultisamplingNone() {
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
}

void VktPipelineBuilder::enableDepthTest(bool depthWriteEnable, VkCompareOp op) {
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = depthWriteEnable;
    depthStencil.depthCompareOp = op;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
}

void VktPipelineBuilder::disableBlending() {
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                          VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT |
                                          VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
}

void VktPipelineBuilder::disableDepthTest() {
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
}

void VktPipelineBuilder::setPipelineLayout(VkPipelineLayout l) {
    layout = l;
}

void VktPipelineBuilder::enableBlendingAdditive() {
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                          VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT |
                                          VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void VktPipelineBuilder::enableBlendingAlpha() {
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                          VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT |
                                          VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}
