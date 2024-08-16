#include "../include/engine/vulkan/VktSkybox.h"

#include "../include/engine/vulkan/VktCache.h"
#include "../include/engine/vulkan/VktPipelines.h"

#include <engine/vulkan/VktBuffers.h>
#include <engine/vulkan/VktImages.h>
#include <engine/vulkan/VktInstantCommands.h>

VktSkybox::VktSkybox(const char *path) {
    load(path);
}

VktSkybox::~VktSkybox() {
    clear();
}

void VktSkybox::load(const char *path) {
    if(m_skyboxCount == 0) {
        initDescriptors();
        initPipelines();
        initCube();
        initBRDF();
    }
    m_skyboxCount++;

    std::filesystem::path dirPath = path;
    std::filesystem::path colorPath = dirPath / "color.ktx2";
    std::filesystem::path diffPath = dirPath / "diffuse.ktx2";
    std::filesystem::path specPath = dirPath / "specular.ktx2";

    auto cubemap = VktImages::createFromFileKtx(colorPath.c_str(), true);
    if(!cubemap.has_value()) {
        m_logger(Logger::ERROR) << "Failed to load cubemap from directory [" << path << "]\n";
    } else {
        m_logger(Logger::INFO) << "Loaded cubemap from directory [" << path << "]\n";
    }
    m_colorCubemap = cubemap.value();

    // TODO check for path after IBL persistnece is done
    if(!exists(diffPath) || !exists(specPath)) {
        generateIBLCubemaps();
        VktImages::writeKtx(specPath.c_str(), m_IBLSpecularCubemap);
        VktImages::writeKtx(diffPath.c_str(), m_IBLDiffuseCubemap);
    }else {
        auto cubemap = VktImages::createFromFileKtx(specPath.c_str(), true);
        if(!cubemap.has_value()) {
            m_logger(Logger::ERROR) << "Failed to load specular IBL cubemap from file [" << path << "]\n";
        } else {
            m_logger(Logger::INFO) << "Loaded specular IBL cubemap from file [" << path << "]\n";
        }
        m_IBLSpecularCubemap = cubemap.value();

        cubemap = VktImages::createFromFileKtx(diffPath.c_str(), true);
        if(!cubemap.has_value()) {
            m_logger(Logger::ERROR) << "Failed to load diffuse IBL cubemap from file [" << path << "]\n";
        } else {
            m_logger(Logger::INFO) << "Loaded diffuse IBL cubemap from file [" << path << "]\n";
        }
        m_IBLDiffuseCubemap = cubemap.value();

    }

    m_loaded = true;
}

void VktSkybox::clear() {
    if(m_loaded) {
        m_loaded = false;
        m_skyboxCount--;
        VktImages::destroy(m_colorCubemap);
        VktImages::destroy(m_IBLDiffuseCubemap);
        VktImages::destroy(m_IBLSpecularCubemap);
        if(m_skyboxCount == 0) {
            clearPipelines();
            clearCube();
            clearBRDF();
        }
    }
}

void VktSkybox::draw(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptorSet) {
    VktTypes::DrawContext drawContext;
    m_cube.gatherDrawContext(drawContext);
    VktTypes::RenderObject &cubeRenderObject = drawContext.opaqueSurfaces.at(0);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_colorPipeline.pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_colorPipeline.layout,
                            0, 1,
                            &sceneDescriptorSet, 0,
                            nullptr);

    VkViewport viewport = VktStructs::viewport(VktCache::drawExtent);
    VkRect2D scissors = VktStructs::scissors(VktCache::drawExtent);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissors);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_colorPipeline.layout,
                            1, 1,
                            &m_descriptorSet, 0,
                            nullptr);

    vkCmdBindIndexBuffer(cmd, cubeRenderObject.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static> pushConstants;
    pushConstants.vertexBuffer = cubeRenderObject.vertexBufferAddress;
    pushConstants.worldMatrix = cubeRenderObject.transform;
    vkCmdPushConstants(cmd, m_colorPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>), &pushConstants);

    vkCmdDrawIndexed(cmd, cubeRenderObject.indexCount, 1, cubeRenderObject.firstIndex, 0, 0);
}

void VktSkybox::writeColorSet() const {
    DescriptorWriter writer;
    writer.writeImage(0, m_IBLSpecularCubemap.view,
                      VktCache::getSampler(VktCache::Sampler::LINEAR),
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(m_descriptorSet);
}

void VktSkybox::writeIBLSets(DescriptorWriter &writer) const {
    writer.writeImage(1, m_IBLDiffuseCubemap.view,
                      VktCache::getSampler(VktCache::Sampler::LINEAR),
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.writeImage(2, m_IBLSpecularCubemap.view,
                      VktCache::getSampler(VktCache::Sampler::LINEAR),
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.writeImage(3, m_BRDFImage.view,
                      VktCache::getSampler(VktCache::Sampler::LINEAR),
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
}

void VktSkybox::initDescriptors() {
    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// Colors cubemap
        VktCache::storeLayout(VktCache::Layout::SKYBOX, builder.build(VK_SHADER_STAGE_FRAGMENT_BIT));
    }

    m_descriptorSet = VktCache::descriptorAllocator.allocate(VktCache::getLayout(VktCache::Layout::SKYBOX));
}

void VktSkybox::initPipelines() {
    VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>)};

    // Create color pipeline
    {
        const auto layouts = VktCache::getLayouts(VktCache::Layout::SCENE, VktCache::Layout::SKYBOX);

        VkPipelineLayoutCreateInfo meshLayoutInfo = VktStructs::pipelineLayoutCreateInfo(layouts, pushConstantRange);

        VkPipelineLayout layout;
        VK_CHECK(vkCreatePipelineLayout(VktCache::vkDevice,
                                        &meshLayoutInfo,
                                        nullptr,
                                        &layout))

        m_colorPipeline.layout = layout;

        VktPipelineBuilder pipelineBuilder;
        pipelineBuilder.setShaders("shaders/skybox.vert.spv", "shaders/skybox.frag.spv");
        pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.setMultisamplingNone();
        pipelineBuilder.disableBlending();
        pipelineBuilder.disableDepthTest();
        pipelineBuilder.setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
        pipelineBuilder.setDepthFormat(VK_FORMAT_D32_SFLOAT);
        pipelineBuilder.setPipelineLayout(layout);

        m_colorPipeline.pipeline = pipelineBuilder.buildPipeline();
    }

    // Create IBL diffuse pipeline
    {
        auto layouts = VktCache::getLayouts(VktCache::Layout::SKYBOX);

        VkPipelineLayoutCreateInfo meshLayoutInfo = VktStructs::pipelineLayoutCreateInfo(layouts, pushConstantRange);

        VkPipelineLayout layout;
        VK_CHECK(vkCreatePipelineLayout(VktCache::vkDevice,
                                        &meshLayoutInfo,
                                        nullptr,
                                        &layout))

        m_IBLDiffusePipeline.layout = layout;

        VktPipelineBuilder pipelineBuilder;
        pipelineBuilder.setShaders("shaders/ibl/ibl.vert.spv", "shaders/ibl/ibl_diffuse.frag.spv", "shaders/ibl/ibl.geom.spv");
        pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.setMultisamplingNone();
        pipelineBuilder.disableBlending();
        pipelineBuilder.disableDepthTest();
        pipelineBuilder.setColorAttachmentFormat(VK_FORMAT_R8G8B8A8_UNORM);
        pipelineBuilder.setPipelineLayout(layout);

        m_IBLDiffusePipeline.pipeline = pipelineBuilder.buildPipeline();
    }

    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        VktCache::storeLayout(VktCache::Layout::IBL_ROUGHNESS, builder.build(VK_SHADER_STAGE_FRAGMENT_BIT));
    }

    // Create IBL specular pipeline
    {
        auto layouts = VktCache::getLayouts(VktCache::Layout::SKYBOX, VktCache::Layout::IBL_ROUGHNESS);

        // Create pipeline layout with provided descriptors and push constants
        VkPipelineLayoutCreateInfo meshLayoutInfo = VktStructs::pipelineLayoutCreateInfo(layouts, pushConstantRange);

        VkPipelineLayout layout;
        VK_CHECK(vkCreatePipelineLayout(VktCache::vkDevice,
                                        &meshLayoutInfo,
                                        nullptr,
                                        &layout))

        m_IBLSpecularPipeline.layout = layout;

        VktPipelineBuilder pipelineBuilder;
        pipelineBuilder.setShaders("shaders/ibl/ibl.vert.spv", "shaders/ibl/ibl_specular.frag.spv", "shaders/ibl/ibl.geom.spv");
        pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.setMultisamplingNone();
        pipelineBuilder.disableBlending();
        pipelineBuilder.disableDepthTest();
        pipelineBuilder.setColorAttachmentFormat(VK_FORMAT_R8G8B8A8_UNORM);
        pipelineBuilder.setPipelineLayout(layout);

        m_IBLSpecularPipeline.pipeline = pipelineBuilder.buildPipeline();
    }

    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        VktCache::storeLayout(VktCache::Layout::IBL_BRDF, builder.build(VK_SHADER_STAGE_COMPUTE_BIT));
    }

    {
        auto layouts = VktCache::getLayouts(VktCache::Layout::IBL_BRDF);

        VkPipelineLayoutCreateInfo computeLayout{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .pNext = nullptr};
        computeLayout.pSetLayouts = layouts.data();
        computeLayout.setLayoutCount = layouts.size();

        VkPushConstantRange pushConstant{};
        pushConstant.offset = 0;
        pushConstant.size = sizeof(VktTypes::GPU::ResolutionBuffer);
        pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        computeLayout.pPushConstantRanges = &pushConstant;
        computeLayout.pushConstantRangeCount = 1;

        VK_CHECK(vkCreatePipelineLayout(VktCache::vkDevice, &computeLayout, nullptr, &m_IBLBRDFPipeline.layout))

        VkShaderModule brdfShader = VktUtils::loadShaderModule("shaders/ibl/ibl_brdf.comp.spv");

        VkPipelineShaderStageCreateInfo stageInfo = VktStructs::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, brdfShader);

        VkComputePipelineCreateInfo computePipelineCreateInfo{.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, .pNext = nullptr};
        computePipelineCreateInfo.layout = m_IBLBRDFPipeline.layout;
        computePipelineCreateInfo.stage = stageInfo;

        VK_CHECK(vkCreateComputePipelines(VktCache::vkDevice, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_IBLBRDFPipeline.pipeline))

        vkDestroyShaderModule(VktCache::vkDevice, brdfShader, nullptr);
    }
}

void VktSkybox::clearPipelines() {
    vkDestroyPipelineLayout(VktCache::vkDevice, m_colorPipeline.layout, nullptr);
    vkDestroyPipeline(VktCache::vkDevice, m_colorPipeline.pipeline, nullptr);

    vkDestroyPipelineLayout(VktCache::vkDevice, m_IBLDiffusePipeline.layout, nullptr);
    vkDestroyPipeline(VktCache::vkDevice, m_IBLDiffusePipeline.pipeline, nullptr);

    vkDestroyPipelineLayout(VktCache::vkDevice, m_IBLSpecularPipeline.layout, nullptr);
    vkDestroyPipeline(VktCache::vkDevice, m_IBLSpecularPipeline.pipeline, nullptr);

    vkDestroyPipelineLayout(VktCache::vkDevice, m_IBLBRDFPipeline.layout, nullptr);
    vkDestroyPipeline(VktCache::vkDevice, m_IBLBRDFPipeline.pipeline, nullptr);
}

void VktSkybox::initCube() {
    m_cube = Model(CUBE_PATH);
}

void VktSkybox::clearCube() {
    m_cube.clear();
}

void VktSkybox::generateIBLCubemaps() {
    VkDescriptorSet skyboxDescriptorSet = VktCache::descriptorAllocator.allocate(VktCache::getLayout(VktCache::Layout::SKYBOX));
    auto cubemap = VktImages::createDeviceMemory(m_colorCubemap.extent,
                                                   m_colorCubemap.format,
                                                   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                   1, 6, true);
    if(!cubemap.has_value()) {
        m_logger(Logger::ERROR) << "Failed to allocate IBL diffuse cubemap\n";
        return;
    }
    m_IBLDiffuseCubemap = cubemap.value();

    cubemap = VktImages::createDeviceMemory(m_colorCubemap.extent,
                                              m_colorCubemap.format,
                                              VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                              5, 6, true);
    if(!cubemap.has_value()) {
        m_logger(Logger::ERROR) << "Failed to allocate IBL specular cubemap\n";
        return;
    }
    m_IBLSpecularCubemap = cubemap.value();

    // Create diffuse cubemap
    VktInstantCommands::submitCommands({[&, this](VkCommandBuffer cmd) {
        VktUtils::transitionCubeMap(cmd, m_IBLDiffuseCubemap.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderingAttachmentInfo colorAttachment = VktStructs::attachmentInfo(m_IBLDiffuseCubemap.view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // Gather cube's render context
        VktTypes::DrawContext drawContext;
        m_cube.gatherDrawContext(drawContext);
        VktTypes::RenderObject &renderObject = drawContext.opaqueSurfaces[0];

        VkExtent2D extent2D = {.width = m_colorCubemap.extent.width, .height = m_colorCubemap.extent.height};
        VkRenderingInfo renderingInfo = VktStructs::renderingInfo(extent2D, &colorAttachment, nullptr, 6);

        {
            DescriptorWriter writer;

            writer.writeImage(0, m_colorCubemap.view,
                              VktCache::getSampler(VktCache::Sampler::LINEAR),
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.updateSet(skyboxDescriptorSet);
        }

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_IBLDiffusePipeline.layout,
                                0, 1,
                                &skyboxDescriptorSet, 0,
                                nullptr);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_IBLDiffusePipeline.pipeline);
        vkCmdBeginRendering(cmd, &renderingInfo);

        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = static_cast<float>(extent2D.width);
        viewport.height = static_cast<float>(extent2D.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissors{};
        scissors.offset.x = 0;
        scissors.offset.y = 0;
        scissors.extent.width = extent2D.width;
        scissors.extent.height = extent2D.height;

        vkCmdSetScissor(cmd, 0, 1, &scissors);

        vkCmdBindIndexBuffer(cmd, renderObject.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static> pushConstants;
        pushConstants.vertexBuffer = renderObject.vertexBufferAddress;
        pushConstants.worldMatrix = renderObject.transform;
        vkCmdPushConstants(cmd, m_IBLDiffusePipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>), &pushConstants);

        vkCmdDrawIndexed(cmd, renderObject.indexCount, 1, renderObject.firstIndex, 0, 0);
        vkCmdEndRendering(cmd);

        VktUtils::transitionCubeMap(cmd, m_IBLDiffuseCubemap.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }});
    m_logger(Logger::INFO) << "Generated IBL diffuse cubemap\n";


    // Create specular cubemap
    std::vector<VkImageView> cubeViews = VktUtils::createCubemapMipViews(m_IBLSpecularCubemap.image, m_IBLSpecularCubemap.format, 5);
    VkDescriptorSet roughnessBufferSet = VktCache::descriptorAllocator.allocate(VktCache::getLayout(VktCache::Layout::IBL_ROUGHNESS));
    VktTypes::Resources::Buffer roughnessBuffer = VktBuffers::create(sizeof(VktTypes::GPU::RoughnessBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    VktInstantCommands::submitCommands({[&, this](VkCommandBuffer cmd) {
        VktUtils::transitionCubeMap(cmd, m_IBLSpecularCubemap.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 5);
    }});
    for(uint32_t mipLevel = 0; mipLevel < cubeViews.size(); mipLevel++) {
        VktInstantCommands::submitCommands({[&, this](VkCommandBuffer cmd) {
            VkRenderingAttachmentInfo colorAttachment = VktStructs::attachmentInfo(cubeViews[mipLevel], nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            // Gather cube's render context
            VktTypes::DrawContext drawContext;
            m_cube.gatherDrawContext(drawContext);
            VktTypes::RenderObject &renderObject = drawContext.opaqueSurfaces[0];

            VkExtent2D extent2D = {
                    .width = static_cast<uint32_t>(m_colorCubemap.extent.width * std::pow(0.5, mipLevel)),
                    .height = static_cast<uint32_t>(m_colorCubemap.extent.height * std::pow(0.5, mipLevel))};
            VkRenderingInfo renderingInfo = VktStructs::renderingInfo(extent2D, &colorAttachment, nullptr, 6);

            {
                DescriptorWriter writer;

                writer.writeImage(0, m_colorCubemap.view,
                                  VktCache::getSampler(VktCache::Sampler::LINEAR),
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                writer.updateSet(skyboxDescriptorSet);
            }

            VktTypes::GPU::RoughnessBuffer roughnessBufferData{.roughness = ((float) mipLevel / (float) (cubeViews.size() - 1))};
            memcpy(roughnessBuffer.info.pMappedData, &roughnessBufferData, sizeof(VktTypes::GPU::RoughnessBuffer));

            {
                DescriptorWriter writer;
                writer.writeBuffer(0,
                                   roughnessBuffer.buffer,
                                   sizeof(VktTypes::GPU::RoughnessBuffer),
                                   0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
                writer.updateSet(roughnessBufferSet);
            }


            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_IBLSpecularPipeline.pipeline);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_IBLSpecularPipeline.layout,
                                    0, 1,
                                    &skyboxDescriptorSet, 0,
                                    nullptr);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_IBLSpecularPipeline.layout,
                                    1, 1,
                                    &roughnessBufferSet, 0,
                                    nullptr);

            vkCmdBeginRendering(cmd, &renderingInfo);

            VkViewport viewport{};
            viewport.x = 0;
            viewport.y = 0;
            viewport.width = static_cast<float>(extent2D.width);
            viewport.height = static_cast<float>(extent2D.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            vkCmdSetViewport(cmd, 0, 1, &viewport);

            VkRect2D scissors{};
            scissors.offset.x = 0;
            scissors.offset.y = 0;
            scissors.extent.width = extent2D.width;
            scissors.extent.height = extent2D.height;

            vkCmdSetScissor(cmd, 0, 1, &scissors);

            vkCmdBindIndexBuffer(cmd, renderObject.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static> pushConstants;
            pushConstants.vertexBuffer = renderObject.vertexBufferAddress;
            pushConstants.worldMatrix = renderObject.transform;
            vkCmdPushConstants(cmd, m_IBLSpecularPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                               sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>), &pushConstants);

            vkCmdDrawIndexed(cmd, renderObject.indexCount, 1, renderObject.firstIndex, 0, 0);
            vkCmdEndRendering(cmd);
        }});
    }
    VktInstantCommands::submitCommands({[&, this](VkCommandBuffer cmd) {
        VktUtils::transitionCubeMap(cmd, m_IBLSpecularCubemap.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 5);
    }});

    for(auto &view: cubeViews) {
        vkDestroyImageView(VktCache::vkDevice, view, nullptr);
    }

    m_logger(Logger::INFO) << "Generated IBL specular cubemap\n";

    VktBuffers::destroy(roughnessBuffer);
}

void VktSkybox::initBRDF() {
    VkExtent3D extent{
            .width = BRDF_SIZE,
            .height = BRDF_SIZE,
            .depth = 1};
    auto image = VktImages::createDeviceMemory(extent, VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    if(!image.has_value()) {
        m_logger(Logger::ERROR) << "Failed to allocate BRDF image\n";
        return;
    }
    m_BRDFImage = image.value();

    VkDescriptorSet imageSet = VktCache::descriptorAllocator.allocate(VktCache::getLayout(VktCache::Layout::IBL_BRDF));

    {
        DescriptorWriter writer;
        writer.writeImage(0, m_BRDFImage.view,
                          VK_NULL_HANDLE,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(imageSet);
    }

    VktTypes::GPU::ResolutionBuffer resolutionBuffer{.data = {extent.width, extent.height}};

    {
        VktInstantCommands::submitCommands([&](VkCommandBuffer cmd) {
            VktUtils::transitionImage(cmd, m_BRDFImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

            // Draw gradient with compute shader
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_IBLBRDFPipeline.pipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_IBLBRDFPipeline.layout, 0, 1, &imageSet, 0, nullptr);

            // Push constants
            vkCmdPushConstants(cmd, m_IBLBRDFPipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(VktTypes::GPU::ResolutionBuffer), &resolutionBuffer);

            vkCmdDispatch(cmd, std::ceil(extent.width / 16.0), std::ceil(extent.height / 16.0), 1);

            VktUtils::transitionImage(cmd, m_BRDFImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });
    }
}

void VktSkybox::clearBRDF() {
    VktImages::destroy(m_BRDFImage);
}
