#include "../include/engine/vulkan/Skybox.h"

#include "../include/engine/vulkan/VktCache.h"
#include "../include/engine/vulkan/VktPipelines.h"
#include "engine/GlobalMemory.h"

#include <engine/vulkan/VktBuffers.h>
#include <engine/vulkan/VktImages.h>
#include <engine/vulkan/VktInstantCommands.h>

Skybox::Skybox(const char *path) {
    load(path);
}

Skybox::~Skybox() {
    clear();
}

void Skybox::load(const char *pth) {
    if(skyboxCount == 0) {
        initDescriptors();
        initPipelines();
        initCube();
        initBRDF();
    }
    skyboxCount++;

    path = pth;

    std::filesystem::path dirPath = path;
    std::filesystem::path colorPath = dirPath / "color.ktx2";
    std::filesystem::path diffPath = dirPath / "diffuse.ktx2";
    std::filesystem::path specPath = dirPath / "specular.ktx2";

    auto cubemap = VktImages::createFromFileKtx(colorPath.c_str(), true);
    if(!cubemap.has_value()) {
        LOG(LOG_ERROR, "Failed to load cubemap from directory [" << path << "]");
    } else {
        LOG(LOG_INFO, "Loaded cubemap from directory [" << path << "]");
    }
    colorCubemap = cubemap.value();

    // TODO check for path after IBL persistnece is done
    if(!exists(diffPath) || !exists(specPath)) {
        generateIBLCubemaps();
        VktImages::writeKtx(specPath.c_str(), IBLSpecularCubemap);
        VktImages::writeKtx(diffPath.c_str(), IBLDiffuseCubemap);
    }else {
        auto cubemap = VktImages::createFromFileKtx(specPath.c_str(), true);
        if(!cubemap.has_value()) {
            LOG(LOG_ERROR, "Failed to load specular IBL cubemap from file [" << path << "]");
        } else {
            LOG(LOG_INFO, "Loaded specular IBL cubemap from file [" << path << "]");
        }
        IBLSpecularCubemap = cubemap.value();

        cubemap = VktImages::createFromFileKtx(diffPath.c_str(), true);
        if(!cubemap.has_value()) {
            LOG(LOG_ERROR, "Failed to load diffuse IBL cubemap from file [" << path << "]");
        } else {
            LOG(LOG_INFO, "Loaded diffuse IBL cubemap from file [" << path << "]");
        }
        IBLDiffuseCubemap = cubemap.value();
    }

    loaded = true;
}

void Skybox::clear() {
    if(loaded) {
        loaded = false;
        skyboxCount--;
        VktImages::destroy(colorCubemap);
        VktImages::destroy(IBLDiffuseCubemap);
        VktImages::destroy(IBLSpecularCubemap);
        if(skyboxCount == 0) {
            clearPipelines();
            clearCube();
            clearBRDF();
        }
    }
}

void Skybox::draw(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptorSet) {
    VktTypes::DrawContext drawContext;
    cube.gatherDrawContext(drawContext);
    VktTypes::RenderObject &cubeRenderObject = drawContext.opaqueSurfaces.at(0);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, colorPipeline.pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            colorPipeline.layout,
                            0, 1,
                            &sceneDescriptorSet, 0,
                            nullptr);

    VkViewport viewport = VktStructs::viewport(VktCachePtr->drawExtent);
    VkRect2D scissors = VktStructs::scissors(VktCachePtr->drawExtent);
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissors);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            colorPipeline.layout,
                            1, 1,
                            &descriptorSet, 0,
                            nullptr);

    vkCmdBindIndexBuffer(cmd, cubeRenderObject.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static> pushConstants;
    pushConstants.vertexBuffer = cubeRenderObject.vertexBufferAddress;
    pushConstants.worldMatrix = cubeRenderObject.transform;
    vkCmdPushConstants(cmd, colorPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>), &pushConstants);

    vkCmdDrawIndexed(cmd, cubeRenderObject.indexCount, 1, cubeRenderObject.firstIndex, 0, 0);
}

void Skybox::writeColorSet() const {
    DescriptorWriter writer;
    writer.writeImage(0, IBLSpecularCubemap.view,
                      VktCachePtr->getSampler(VktCache::Sampler::LINEAR),
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(descriptorSet);
}

void Skybox::writeIBLSets(DescriptorWriter &writer) const {
    writer.writeImage(1, IBLDiffuseCubemap.view,
                      VktCachePtr->getSampler(VktCache::Sampler::LINEAR),
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.writeImage(2, IBLSpecularCubemap.view,
                      VktCachePtr->getSampler(VktCache::Sampler::LINEAR),
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.writeImage(3, BRDFImage.view,
                      VktCachePtr->getSampler(VktCache::Sampler::LINEAR),
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
}

void Skybox::initDescriptors() {
    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// Colors cubemap
        VktCachePtr->storeLayout(VktCache::Layout::SKYBOX, builder.build(VK_SHADER_STAGE_FRAGMENT_BIT));
    }

    descriptorSet = VktCachePtr->descriptorAllocator.allocate(VktCachePtr->getLayout(VktCache::Layout::SKYBOX));
}

void Skybox::initPipelines() {
    VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>)};

    // Create color pipeline
    {
        const auto layouts = VktCachePtr->getLayouts(VktCache::Layout::SCENE, VktCache::Layout::SKYBOX);

        VkPipelineLayoutCreateInfo meshLayoutInfo = VktStructs::pipelineLayoutCreateInfo(layouts, pushConstantRange);

        VkPipelineLayout layout;
        VK_CHECK(vkCreatePipelineLayout(VktCachePtr->vkDevice,
                                        &meshLayoutInfo,
                                        nullptr,
                                        &layout))

        colorPipeline.layout = layout;

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

        colorPipeline.pipeline = pipelineBuilder.buildPipeline();
    }

    // Create IBL diffuse pipeline
    {
        auto layouts = VktCachePtr->getLayouts(VktCache::Layout::SKYBOX);

        VkPipelineLayoutCreateInfo meshLayoutInfo = VktStructs::pipelineLayoutCreateInfo(layouts, pushConstantRange);

        VkPipelineLayout layout;
        VK_CHECK(vkCreatePipelineLayout(VktCachePtr->vkDevice,
                                        &meshLayoutInfo,
                                        nullptr,
                                        &layout))

        IBLDiffusePipeline.layout = layout;

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

        IBLDiffusePipeline.pipeline = pipelineBuilder.buildPipeline();
    }

    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        VktCachePtr->storeLayout(VktCache::Layout::IBL_ROUGHNESS, builder.build(VK_SHADER_STAGE_FRAGMENT_BIT));
    }

    // Create IBL specular pipeline
    {
        auto layouts = VktCachePtr->getLayouts(VktCache::Layout::SKYBOX, VktCache::Layout::IBL_ROUGHNESS);

        // Create pipeline layout with provided descriptors and push constants
        VkPipelineLayoutCreateInfo meshLayoutInfo = VktStructs::pipelineLayoutCreateInfo(layouts, pushConstantRange);

        VkPipelineLayout layout;
        VK_CHECK(vkCreatePipelineLayout(VktCachePtr->vkDevice,
                                        &meshLayoutInfo,
                                        nullptr,
                                        &layout))

        IBLSpecularPipeline.layout = layout;

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

        IBLSpecularPipeline.pipeline = pipelineBuilder.buildPipeline();
    }

    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        VktCachePtr->storeLayout(VktCache::Layout::IBL_BRDF, builder.build(VK_SHADER_STAGE_COMPUTE_BIT));
    }

    {
        auto layouts = VktCachePtr->getLayouts(VktCache::Layout::IBL_BRDF);

        VkPipelineLayoutCreateInfo computeLayout{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .pNext = nullptr};
        computeLayout.pSetLayouts = layouts.data();
        computeLayout.setLayoutCount = layouts.size();

        VkPushConstantRange pushConstant{};
        pushConstant.offset = 0;
        pushConstant.size = sizeof(VktTypes::GPU::ResolutionBuffer);
        pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        computeLayout.pPushConstantRanges = &pushConstant;
        computeLayout.pushConstantRangeCount = 1;

        VK_CHECK(vkCreatePipelineLayout(VktCachePtr->vkDevice, &computeLayout, nullptr, &IBLBRDFPipeline.layout))

        VkShaderModule brdfShader = VktUtils::loadShaderModule("shaders/ibl/ibl_brdf.comp.spv");

        VkPipelineShaderStageCreateInfo stageInfo = VktStructs::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, brdfShader);

        VkComputePipelineCreateInfo computePipelineCreateInfo{.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, .pNext = nullptr};
        computePipelineCreateInfo.layout = IBLBRDFPipeline.layout;
        computePipelineCreateInfo.stage = stageInfo;

        VK_CHECK(vkCreateComputePipelines(VktCachePtr->vkDevice, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &IBLBRDFPipeline.pipeline))

        vkDestroyShaderModule(VktCachePtr->vkDevice, brdfShader, nullptr);
    }
}

void Skybox::clearPipelines() {
    vkDestroyPipelineLayout(VktCachePtr->vkDevice, colorPipeline.layout, nullptr);
    vkDestroyPipeline(VktCachePtr->vkDevice, colorPipeline.pipeline, nullptr);

    vkDestroyPipelineLayout(VktCachePtr->vkDevice, IBLDiffusePipeline.layout, nullptr);
    vkDestroyPipeline(VktCachePtr->vkDevice, IBLDiffusePipeline.pipeline, nullptr);

    vkDestroyPipelineLayout(VktCachePtr->vkDevice, IBLSpecularPipeline.layout, nullptr);
    vkDestroyPipeline(VktCachePtr->vkDevice, IBLSpecularPipeline.pipeline, nullptr);

    vkDestroyPipelineLayout(VktCachePtr->vkDevice, IBLBRDFPipeline.layout, nullptr);
    vkDestroyPipeline(VktCachePtr->vkDevice, IBLBRDFPipeline.pipeline, nullptr);
}

void Skybox::initCube() {
    cube = Model(CUBE_PATH);
}

void Skybox::clearCube() {
    cube.clear();
}

void Skybox::generateIBLCubemaps() {
    VkDescriptorSet skyboxDescriptorSet = VktCachePtr->descriptorAllocator.allocate(VktCachePtr->getLayout(VktCache::Layout::SKYBOX));
    auto cubemap = VktImages::createDeviceMemory(colorCubemap.extent,
                                                 colorCubemap.format,
                                                 VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                 1, 6, true);
    if(!cubemap.has_value()) {
        LOG(LOG_ERROR, "Failed to allocate IBL diffuse cubemap");
        return;
    }
    IBLDiffuseCubemap = cubemap.value();

    cubemap = VktImages::createDeviceMemory(colorCubemap.extent,
                                            colorCubemap.format,
                                            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                            5, 6, true);
    if(!cubemap.has_value()) {
        LOG(LOG_ERROR, "Failed to allocate IBL specular cubemap");
        return;
    }
    IBLSpecularCubemap = cubemap.value();

    // Create diffuse cubemap
    VktInstantCommands::submitCommands({[&, this](VkCommandBuffer cmd) {
        VktUtils::transitionCubeMap(cmd, IBLDiffuseCubemap.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderingAttachmentInfo colorAttachment = VktStructs::attachmentInfo(IBLDiffuseCubemap.view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // Gather cube's render context
        VktTypes::DrawContext drawContext;
        cube.gatherDrawContext(drawContext);
        VktTypes::RenderObject &renderObject = drawContext.opaqueSurfaces[0];

        VkExtent2D extent2D = {.width = colorCubemap.extent.width, .height = colorCubemap.extent.height};
        VkRenderingInfo renderingInfo = VktStructs::renderingInfo(extent2D, &colorAttachment, nullptr, 6);

        {
            DescriptorWriter writer;

            writer.writeImage(0, colorCubemap.view,
                              VktCachePtr->getSampler(VktCache::Sampler::LINEAR),
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.updateSet(skyboxDescriptorSet);
        }

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                IBLDiffusePipeline.layout,
                                0, 1,
                                &skyboxDescriptorSet, 0,
                                nullptr);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, IBLDiffusePipeline.pipeline);
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
        vkCmdPushConstants(cmd, IBLDiffusePipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>), &pushConstants);

        vkCmdDrawIndexed(cmd, renderObject.indexCount, 1, renderObject.firstIndex, 0, 0);
        vkCmdEndRendering(cmd);

        VktUtils::transitionCubeMap(cmd, IBLDiffuseCubemap.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }});
    LOG(LOG_INFO, "Generated IBL diffuse cubemap");


    // Create specular cubemap
    std::vector<VkImageView> cubeViews = VktUtils::createCubemapMipViews(IBLSpecularCubemap.image, IBLSpecularCubemap.format, 5);
    VkDescriptorSet roughnessBufferSet = VktCachePtr->descriptorAllocator.allocate(VktCachePtr->getLayout(VktCache::Layout::IBL_ROUGHNESS));
    VktTypes::Resources::Buffer roughnessBuffer = VktBuffers::create(sizeof(VktTypes::GPU::RoughnessBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    VktInstantCommands::submitCommands({[&, this](VkCommandBuffer cmd) {
        VktUtils::transitionCubeMap(cmd, IBLSpecularCubemap.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 5);
    }});
    for(uint32_t mipLevel = 0; mipLevel < cubeViews.size(); mipLevel++) {
        VktInstantCommands::submitCommands({[&, this](VkCommandBuffer cmd) {
            VkRenderingAttachmentInfo colorAttachment = VktStructs::attachmentInfo(cubeViews[mipLevel], nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            // Gather cube's render context
            VktTypes::DrawContext drawContext;
            cube.gatherDrawContext(drawContext);
            VktTypes::RenderObject &renderObject = drawContext.opaqueSurfaces[0];

            VkExtent2D extent2D = {
                    .width = static_cast<uint32_t>(colorCubemap.extent.width * std::pow(0.5, mipLevel)),
                    .height = static_cast<uint32_t>(colorCubemap.extent.height * std::pow(0.5, mipLevel))};
            VkRenderingInfo renderingInfo = VktStructs::renderingInfo(extent2D, &colorAttachment, nullptr, 6);

            {
                DescriptorWriter writer;

                writer.writeImage(0, colorCubemap.view,
                                  VktCachePtr->getSampler(VktCache::Sampler::LINEAR),
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


            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, IBLSpecularPipeline.pipeline);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    IBLSpecularPipeline.layout,
                                    0, 1,
                                    &skyboxDescriptorSet, 0,
                                    nullptr);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    IBLSpecularPipeline.layout,
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
            vkCmdPushConstants(cmd, IBLSpecularPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                               sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>), &pushConstants);

            vkCmdDrawIndexed(cmd, renderObject.indexCount, 1, renderObject.firstIndex, 0, 0);
            vkCmdEndRendering(cmd);
        }});
    }
    VktInstantCommands::submitCommands({[&, this](VkCommandBuffer cmd) {
        VktUtils::transitionCubeMap(cmd, IBLSpecularCubemap.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 5);
    }});

    for(auto &view: cubeViews) {
        vkDestroyImageView(VktCachePtr->vkDevice, view, nullptr);
    }

    LOG(LOG_INFO, "Generated IBL specular cubemap");

    VktBuffers::destroy(roughnessBuffer);
}

void Skybox::initBRDF() {
    VkExtent3D extent{
            .width = BRDF_SIZE,
            .height = BRDF_SIZE,
            .depth = 1};
    auto image = VktImages::createDeviceMemory(extent, VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    if(!image.has_value()) {
        LOG(LOG_ERROR, "Failed to allocate BRDF image");
        return;
    }
    BRDFImage = image.value();

    VkDescriptorSet imageSet = VktCachePtr->descriptorAllocator.allocate(VktCachePtr->getLayout(VktCache::Layout::IBL_BRDF));

    {
        DescriptorWriter writer;
        writer.writeImage(0, BRDFImage.view,
                          VK_NULL_HANDLE,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(imageSet);
    }

    VktTypes::GPU::ResolutionBuffer resolutionBuffer{.data = {extent.width, extent.height}};

    {
        VktInstantCommands::submitCommands([&](VkCommandBuffer cmd) {
            VktUtils::transitionImage(cmd, BRDFImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

            // Draw gradient with compute shader
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, IBLBRDFPipeline.pipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, IBLBRDFPipeline.layout, 0, 1, &imageSet, 0, nullptr);

            // Push constants
            vkCmdPushConstants(cmd, IBLBRDFPipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(VktTypes::GPU::ResolutionBuffer), &resolutionBuffer);

            vkCmdDispatch(cmd, std::ceil(extent.width / 16.0), std::ceil(extent.height / 16.0), 1);

            VktUtils::transitionImage(cmd, BRDFImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });
    }
}

void Skybox::clearBRDF() {
    VktImages::destroy(BRDFImage);
}
