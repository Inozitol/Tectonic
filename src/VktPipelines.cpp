#include "engine/vulkan/VktPipelines.h"

#include "engine/vulkan/VktCache.h"


VktPipelineBuilder::VktPipelineBuilder(){
    clear();
}

VktPipelineBuilder::~VktPipelineBuilder() {
    for(const auto& [stage, module] : m_shaderStages){
        vkDestroyShaderModule(VktCache::vkDevice, module, nullptr);
    }
}

void VktPipelineBuilder::clear() {
    m_inputAssembly = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, .pNext = nullptr};
    m_rasterizer = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .pNext = nullptr};
    m_colorBlendAttachment = {};
    m_multisampling = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .pNext = nullptr};
    m_layout = {};
    m_depthStencil = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, .pNext = nullptr};
    m_renderInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    m_shaderStages.clear();
}

VkPipeline VktPipelineBuilder::buildPipeline() {
    VkPipelineViewportStateCreateInfo viewportState{.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .pNext = nullptr};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineColorBlendStateCreateInfo colorBlending{.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .pNext = nullptr};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &m_colorBlendAttachment;

    VkPipelineVertexInputStateCreateInfo m_vertexInputInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, .pNext = nullptr};

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.reserve(m_shaderStages.size());
    for(const auto& [stage, module] : m_shaderStages){
        shaderStages.push_back(VktStructs::pipelineShaderStageCreateInfo(stage, module));
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.pNext = &m_renderInfo;
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pVertexInputState = &m_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &m_inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &m_rasterizer;
    pipelineInfo.pMultisampleState = &m_multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &m_depthStencil;
    pipelineInfo.layout = m_layout;

    std::array<VkDynamicState, 2> state = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .pNext = nullptr};
    dynamicInfo.pDynamicStates = state.data();
    dynamicInfo.dynamicStateCount = static_cast<uint32_t>(state.size());

    pipelineInfo.pDynamicState = &dynamicInfo;

    VkPipeline graphicsPipeline;
    VK_CHECK(vkCreateGraphicsPipelines(VktCache::vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));
    return graphicsPipeline;
}

void VktPipelineBuilder::setVertexShader(const char* path) {
    setShader(VK_SHADER_STAGE_VERTEX_BIT, path);
}

void VktPipelineBuilder::setFragmentShader(const char* path) {
    setShader(VK_SHADER_STAGE_FRAGMENT_BIT, path);
}

void VktPipelineBuilder::setGeometryShader(const char* path) {
    setShader(VK_SHADER_STAGE_GEOMETRY_BIT, path);
}

void VktPipelineBuilder::setShaders(const char* vertexShaderPath, const char* fragmentShaderPath, const char* geometryShaderPath) {
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

void VktPipelineBuilder::setShader(VkShaderStageFlagBits stageBit, const char* path){
    // Delete old shader in case its still cached
    if(m_shaderStages.contains(stageBit)){
        vkDestroyShaderModule(VktCache::vkDevice, m_shaderStages.at(stageBit), nullptr);
    }
    m_shaderStages[stageBit] = VktUtils::loadShaderModule(path);
}

void VktPipelineBuilder::setInputTopology(VkPrimitiveTopology topology) {
    m_inputAssembly.topology = topology;
    m_inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void VktPipelineBuilder::setPolygonMode(VkPolygonMode mode) {
    m_rasterizer.polygonMode = mode;
    m_rasterizer.lineWidth = 1.0f;
}

void VktPipelineBuilder::setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace) {
    m_rasterizer.cullMode = cullMode;
    m_rasterizer.frontFace = frontFace;
}

void VktPipelineBuilder::setColorAttachmentFormat(VkFormat format) {
    m_colorAttachmentFormat = format;
    m_renderInfo.colorAttachmentCount = 1;
    m_renderInfo.pColorAttachmentFormats = &m_colorAttachmentFormat;
}

void VktPipelineBuilder::setDepthFormat(VkFormat format) {
    m_renderInfo.depthAttachmentFormat = format;
}

void VktPipelineBuilder::setMultisamplingNone() {
    m_multisampling.sampleShadingEnable = VK_FALSE;
    m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    m_multisampling.minSampleShading = 1.0f;
    m_multisampling.pSampleMask = nullptr;
    m_multisampling.alphaToCoverageEnable = VK_FALSE;
    m_multisampling.alphaToOneEnable = VK_FALSE;
}

void VktPipelineBuilder::enableDepthTest(bool depthWriteEnable, VkCompareOp op) {
    m_depthStencil.depthTestEnable = VK_TRUE;
    m_depthStencil.depthWriteEnable = depthWriteEnable;
    m_depthStencil.depthCompareOp = op;
    m_depthStencil.depthBoundsTestEnable = VK_FALSE;
    m_depthStencil.stencilTestEnable = VK_FALSE;
    m_depthStencil.front = {};
    m_depthStencil.back = {};
    m_depthStencil.minDepthBounds = 0.0f;
    m_depthStencil.maxDepthBounds = 1.0f;
}

void VktPipelineBuilder::disableBlending() {
    m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;
    m_colorBlendAttachment.blendEnable = VK_FALSE;
}

void VktPipelineBuilder::disableDepthTest() {
    m_depthStencil.depthTestEnable = VK_FALSE;
    m_depthStencil.depthWriteEnable = VK_FALSE;
    m_depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    m_depthStencil.stencilTestEnable = VK_FALSE;
    m_depthStencil.front = {};
    m_depthStencil.back = {};
    m_depthStencil.minDepthBounds = 0.0f;
    m_depthStencil.maxDepthBounds = 1.0f;
}

void VktPipelineBuilder::setPipelineLayout(VkPipelineLayout layout) {
    m_layout = layout;
}

void VktPipelineBuilder::enableBlendingAdditive() {
    m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;
    m_colorBlendAttachment.blendEnable = VK_TRUE;
    m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
    m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void VktPipelineBuilder::enableBlendingAlpha() {
    m_colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;
    m_colorBlendAttachment.blendEnable = VK_TRUE;
    m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
    m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}
