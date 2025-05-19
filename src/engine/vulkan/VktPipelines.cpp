#include "engine/vulkan/VktPipelines.h"
#include "engine/GlobalMemory.h"
#include "engine/vulkan/VktCache.h"
#define VK_VALUE_SERIALIZATION_CONFIG_MAIN
#include "extern/vulkan-mini-libs-2/vk_value_serialization.hpp"

#include "extern/pugixml/pugixml.hpp"

void VktPipeline::compileShader(const std::filesystem::path &glslPath, const std::filesystem::path &spirvPath) {
    std::string command = "glslangValidator -V ";
    command.append(glslPath.string());
    command.append(" -o ");
    command.append(spirvPath.string());
    std::system(command.c_str());
}

std::optional<const VktPipeline::Shader *> VktPipeline::loadShader(const pugi::xml_node &node) {
    assert(std::strcmp(node.name(), "shader") == 0);
    STecVkSerializationResult parseResult;

    if(node.child("path").empty()) {
        LOG(LOG_ERROR, "Couldn't find <path> in shader");
        return {};
    }
    const std::filesystem::path path = node.child("path").child_value();
    if(loadedShaders.contains(path)) {
        // If the shader is already loaded, return it
        return &loadedShaders.at(path);
    }
    if(node.child("stage").empty()) {
        LOG(LOG_ERROR, "Couldn't find <stage> in shader");
        return {};
    }
    std::string stageStr = node.child("stage").child_value();
    VkShaderStageFlagBits stage;
    parseResult = vk_parse("VkShaderStageFlagBits", stageStr.c_str(), &stage);

    if(parseResult != STEC_VK_SERIALIZATION_RESULT_SUCCESS) {
        LOG(LOG_ERROR, "Unknown shader stage " << stageStr);
        return {};
    }
    std::filesystem::path spirvPath = path;
    spirvPath.concat(".spv");
    //if(!is_regular_file(spirvPath)) {
    compileShader(path, spirvPath);
    //}
    const VkShaderModule shaderModule = VktUtils::loadShaderModule(spirvPath.c_str());
    loadedShaders[path] = Shader{
            .path = path,
            .stage = stage,
            .module = shaderModule
    };
    return &loadedShaders[path];
}

std::optional<std::vector<const VktPipeline::Shader *>> VktPipeline::loadShaders(const pugi::xml_node &node) {
    assert(std::strcmp(node.name(), "shaders") == 0);
    std::vector<const Shader *> shaders;

    for(pugi::xml_node element: node) {
        if(std::strcmp(element.name(), "shader") == 0) {
            auto shader = loadShader(element);
            if(!shader.has_value()) {
                LOG(LOG_ERROR, "Failed to load shader");
                return {};
            }
            shaders.emplace_back(shader.value());
        }
    }

    return shaders;
}

void VktPipeline::loadTopology(const pugi::xml_node &node) {
    assert(std::strcmp(node.name(), "topology") == 0);
    STecVkSerializationResult parseResult;

    std::string topologyStr = node.child("topology").child_value();
    if(!topologyStr.empty()) {
        parseResult = vk_parse("VkPrimitiveTopology", topologyStr.c_str(), &inputAssembly.topology);
        if(parseResult != STEC_VK_SERIALIZATION_RESULT_SUCCESS) { LOG(LOG_ERROR, "Failed to load topology"); }
    }
}

void VktPipeline::loadRasterizer(const pugi::xml_node &node) {
    assert(std::strcmp(node.name(), "rasterizer") == 0);
    STecVkSerializationResult parseResult;

    const std::string cullingStr = node.child("cull-mode").child_value();
    if(!cullingStr.empty()) {
        parseResult = vk_parse("VkCullModeFlags", cullingStr.c_str(), &rasterizer.cullMode);
        if(parseResult != STEC_VK_SERIALIZATION_RESULT_SUCCESS) { LOG(LOG_ERROR, "Failed to load cull mode"); }
    }

    const std::string frontFaceStr = node.child("front-face").child_value();
    if(!frontFaceStr.empty()) {
        parseResult = vk_parse("VkFrontFace", frontFaceStr.c_str(), &rasterizer.frontFace);
        if(parseResult != STEC_VK_SERIALIZATION_RESULT_SUCCESS) { LOG(LOG_ERROR, "Failed to load front face"); }
    }

    const std::string polygonModeStr = node.child("polygon-mode").child_value();
    if(!polygonModeStr.empty()) {
        parseResult = vk_parse("VkPolygonMode", polygonModeStr.c_str(), &rasterizer.polygonMode);
        if(parseResult != STEC_VK_SERIALIZATION_RESULT_SUCCESS) { LOG(LOG_ERROR, "Failed to load polygon mode"); }
    }

}

void VktPipeline::loadRenderInfo(const pugi::xml_node &node) {
    assert(std::strcmp(node.name(), "render-info") == 0);
    STecVkSerializationResult parseResult;

    const pugi::xml_node colorAttachments = node.child("color-attachment-formats");
    if(!colorAttachments.empty()) {
        uint32_t formatCount = 0;
        for(pugi::xml_node format: colorAttachments) {
            assert(std::strcmp(format.name(), "format") == 0);
            if(!format.empty()) {
                formatCount++;
                colorAttachmentFormats.emplace_back();
                parseResult = vk_parse("VkFormat", format.child_value(), &colorAttachmentFormats.back());
                if(parseResult != STEC_VK_SERIALIZATION_RESULT_SUCCESS) { LOG(LOG_ERROR, "Failed to load color attachment format"); }
            }
        }
        renderInfo.colorAttachmentCount = formatCount;
        renderInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
    }

    const std::string depthAttachmentFormat = node.child("depth-attachment-format").child_value();
    if(!depthAttachmentFormat.empty()) {
        parseResult = vk_parse("VkFormat", depthAttachmentFormat.c_str(), &renderInfo.depthAttachmentFormat);
        if(parseResult != STEC_VK_SERIALIZATION_RESULT_SUCCESS) { LOG(LOG_ERROR, "Failed to load depth attachment format"); }
    }
    const std::string stencilAttachmentFormat = node.child("stencil-attachment-format").child_value();
    if(!stencilAttachmentFormat.empty()) {
        parseResult = vk_parse("VkFormat", stencilAttachmentFormat.c_str(), &renderInfo.stencilAttachmentFormat);
        if(parseResult != STEC_VK_SERIALIZATION_RESULT_SUCCESS) { LOG(LOG_ERROR, "Failed to load stencil attachment format"); }
    }
}

void VktPipeline::loadBlending(const pugi::xml_node &node) {
    assert(std::strcmp(node.name(), "color-blending") == 0);

    const std::string blendingStr = node.child("color-blending").child_value();
    if(!blendingStr.empty()) {
        if(blendingStr == "NONE") {
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                                  VK_COLOR_COMPONENT_G_BIT |
                                                  VK_COLOR_COMPONENT_B_BIT |
                                                  VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;
        } else if(blendingStr == "ADDITIVE") {
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
        } else if(blendingStr == "ALPHA") {
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
    }
}

void VktPipeline::loadDepthTest(const pugi::xml_node &node) {
    assert(std::strcmp(node.name(), "depth-test") == 0);
    STecVkSerializationResult parseResult;

    const std::string testEnableStr = node.child("test-enable").child_value();
    if(!testEnableStr.empty()) {
        if(testEnableStr == "TRUE") {
            depthStencil.depthTestEnable = VK_TRUE;
        }else if(testEnableStr == "FALSE") {
            depthStencil.depthTestEnable = VK_FALSE;
        }
    }

    const std::string writeEnableStr = node.child("write-enable").child_value();
    if(!writeEnableStr.empty()) {
        if(writeEnableStr == "TRUE") {
            depthStencil.depthWriteEnable = VK_TRUE;
        }else if(writeEnableStr == "FALSE") {
            depthStencil.depthWriteEnable = VK_FALSE;
        }
    }

    const std::string compareOpStr = node.child("compare-op").child_value();
    if(!compareOpStr.empty()) {
        parseResult = vk_parse("VkCompareOp", compareOpStr.c_str(), &depthStencil.depthCompareOp);
        if(parseResult != STEC_VK_SERIALIZATION_RESULT_SUCCESS) { LOG(LOG_ERROR, "Failed to load depth compare op"); }
    }

    const std::string minBoundsStr = node.child("min-bounds").child_value();
    if(!minBoundsStr.empty()) {
        depthStencil.minDepthBounds = std::stof(minBoundsStr);
    }

    const std::string maxBoundsStr = node.child("max-bounds").child_value();
    if(!maxBoundsStr.empty()) {
        depthStencil.maxDepthBounds = std::stof(maxBoundsStr);
    }
}

void VktPipeline::loadDynamicStates(const pugi::xml_node& node) {
    assert(std::strcmp(node.name(), "dynamic-states") == 0);
    STecVkSerializationResult parseResult;
    for(pugi::xml_node state: node) {
        if(std::strcmp(state.name(), "dynamic-state") == 0) {
            dynamicStates.emplace_back();
            parseResult = vk_parse("VkDynamicState", state.child_value(), &dynamicStates.back());
            if(parseResult != STEC_VK_SERIALIZATION_RESULT_SUCCESS) { LOG(LOG_ERROR, "Failed to load dynamic state"); }
        }
    }
    dynamicState.dynamicStateCount = dynamicStates.size();
    dynamicState.pDynamicStates = dynamicStates.data();
};

bool VktPipeline::buildPipeline() {
    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    std::vector<VkPipelineShaderStageCreateInfo> stages;
    stages.reserve(pipelineShaders.size());
    for(auto& shader : pipelineShaders) {
        stages.push_back(VktStructs::pipelineShaderStageCreateInfo(shader->stage, shader->module));
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = static_cast<uint32_t>(stages.size()),
        .pStages = stages.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState = nullptr,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = 0,
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0
    };

    VK_CHECK(vkCreateGraphicsPipelines(VktCachePtr->vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
    return true;
}


bool VktPipeline::loadXml(const std::filesystem::path &path) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(path.c_str());
    if(!result) {
        LOG(LOG_ERROR, "Failed to load XML file " << path << " | " << result.description());
        return false;
    }
    const auto pipelineNode = doc.child("pipeline");
    if(pipelineNode.empty()) {
        LOG(LOG_ERROR, "Failed to load pipeline node");
        return false;
    }
    if(pipelineNode.attribute("name").empty()) {
        LOG(LOG_ERROR, "Failed to load pipeline name");
        return false;
    }
    name = pipelineNode.attribute("name").as_string();
    if(pipelineNode.child("shaders").empty()) {
        LOG(LOG_ERROR, "Failed to load pipeline shaders");
        return false;
    }
    const auto shaders = loadShaders(pipelineNode.child("shaders"));
    if(!shaders.has_value()) {
        LOG(LOG_ERROR, "Failed to load shaders");
        return false;
    }
    pipelineShaders = shaders.value();
    if(!pipelineNode.child("topology").empty()) { loadTopology(pipelineNode.child("topology")); }
    if(!pipelineNode.child("rasterizer").empty()) { loadRasterizer(pipelineNode.child("rasterizer")); }
    if(!pipelineNode.child("render-info").empty()) { loadRenderInfo(pipelineNode.child("render-info")); }
    if(!pipelineNode.child("color-blending").empty()) { loadBlending(pipelineNode.child("color-blending")); }
    if(!pipelineNode.child("depth-test").empty()) { loadDepthTest(pipelineNode.child("depth-test")); }
    if(!pipelineNode.child("dynamic-states").empty()) { loadDynamicStates(pipelineNode.child("dynamic-states")); }

    return true;
}

VktPipelineBuilder::VktPipelineBuilder() { clear(); }

VktPipelineBuilder::~VktPipelineBuilder() { for(const auto &module: shaderStages | std::views::values) { vkDestroyShaderModule(VktCachePtr->vkDevice, module, nullptr); } }

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
    stages.reserve(shaderStages.size());
    for(const auto &[stage, module]: shaderStages) { stages.push_back(VktStructs::pipelineShaderStageCreateInfo(stage, module)); }

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

    VkPipelineDynamicStateCreateInfo dynamicInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .pNext = nullptr};
    dynamicInfo.pDynamicStates = dynamicStates.data();
    dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());

    pipelineInfo.pDynamicState = &dynamicInfo;

    VkPipeline graphicsPipeline;
    VK_CHECK(vkCreateGraphicsPipelines(VktCachePtr->vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));
    return graphicsPipeline;
}

void VktPipelineBuilder::setVertexShader(const char *path) { setShader(VK_SHADER_STAGE_VERTEX_BIT, path); }

void VktPipelineBuilder::setFragmentShader(const char *path) { setShader(VK_SHADER_STAGE_FRAGMENT_BIT, path); }

void VktPipelineBuilder::setGeometryShader(const char *path) { setShader(VK_SHADER_STAGE_GEOMETRY_BIT, path); }

void VktPipelineBuilder::setShaders(const char *vertexShaderPath, const char *fragmentShaderPath, const char *geometryShaderPath) {
    if(vertexShaderPath) { setShader(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderPath); }
    if(fragmentShaderPath) { setShader(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderPath); }
    if(geometryShaderPath) { setShader(VK_SHADER_STAGE_GEOMETRY_BIT, geometryShaderPath); }
}

void VktPipelineBuilder::setShader(VkShaderStageFlagBits stageBit, const char *path) {
    // Delete old shader in case its still cached
    if(shaderStages.contains(stageBit)) { vkDestroyShaderModule(VktCachePtr->vkDevice, shaderStages.at(stageBit), nullptr); }
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

void VktPipelineBuilder::setDepthFormat(VkFormat format) { renderInfo.depthAttachmentFormat = format; }

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

void VktPipelineBuilder::setPipelineLayout(VkPipelineLayout l) { layout = l; }

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