
#include "engine/vulkan/VktCore.h"
#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "engine/GlobalMemory.h"
#include "engine/imgui/ImGuiHandler.h"
#include "engine/vulkan/VktCache.h"
#include "extern/stb/stb_image.h"
#include <functional>
#include <vk_mem_alloc.h>
#include "engine/scene/World.h"

void VktCore::init() {

    LOG(LOG_INFO, "Initializing VktCore");

    setExtentDimensions();
    initVulkan();
    initSwapchain();
    initCommands();
    initSyncStructs();
    initDescriptors();
    initPipelines();
    initImGui();
    VktInstantCommands::init(graphicsQueueFamily, graphicsQueue);
    ktxVulkanDeviceInfo_Construct(&VktCachePtr->ktxInfo, VktCachePtr->vkPhysicalDevice, VktCachePtr->vkDevice, graphicsQueue, VktInstantCommands::vkCmdPool, nullptr);
    initDefaultData();

    isInitialized = true;
}

void VktCore::clear() {
    if(isInitialized) {
        for(auto &[id, object]: loadedObjects) {
            delete(object.model);
        }

        for(auto &[id, layout]: VktCachePtr->getAllLayouts()) {
            vkDestroyDescriptorSetLayout(VktCachePtr->vkDevice, layout, nullptr);
        }

        // TODO This should have the possiblity to be cleared by engine at runtime
        for(auto &[id, line]: debugGeometry.lines) {
            VktBuffers::destroy(line->meshBuffers.indexBuffer);
            VktBuffers::destroy(line->meshBuffers.vertexBuffer);
        }
        for(auto &[id, line]: debugGeometry.infLines) {
            VktBuffers::destroy(line->meshBuffers.indexBuffer);
            VktBuffers::destroy(line->meshBuffers.vertexBuffer);
        }
        for(auto &[id, box]: debugGeometry.SAABBs) {
            VktBuffers::destroy(box.mesh.meshBuffers.indexBuffer);
            VktBuffers::destroy(box.mesh.meshBuffers.vertexBuffer);
        }

        VktImages::destroy(m_whiteImage);
        VktImages::destroy(m_blackImage);
        VktImages::destroy(m_greyImage);
        VktImages::destroy(m_errorCheckboardImage);

        VktInstantCommands::clear();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        coreDeletionQueue.flush();
        VktCachePtr->descriptorAllocator.destroyPool();

        for(uint8_t i = 0; i < FRAMES_OVERLAP; i++) {
            vkDestroyCommandPool(VktCachePtr->vkDevice, frames[i].commandPool, nullptr);
            vkDestroyFence(VktCachePtr->vkDevice, frames[i].renderFence, nullptr);
            vkDestroySemaphore(VktCachePtr->vkDevice, frames[i].renderSemaphore, nullptr);
            vkDestroySemaphore(VktCachePtr->vkDevice, frames[i].swapchainSemaphore, nullptr);
        }

        destroySwapchain();

        vkDestroySurfaceKHR(VktCachePtr->vkInstance, surface, nullptr);
        vkDestroyDevice(VktCachePtr->vkDevice, nullptr);

        vkDestroyInstance(VktCachePtr->vkInstance, nullptr);

        isInitialized = false;
    }
}

VktCore::~VktCore() {
    clear();
}

void VktCore::initVulkan() {
    vkb::InstanceBuilder vkBuilder;
    auto instance = vkBuilder.set_app_name("Tectonic")
                            .request_validation_layers(true)
                            .set_debug_callback(VktCore::debugCallback)
                            .require_api_version(1, 3, 0)
                            .build();

    LOG(LOG_INFO, "Finished building vkbInstance");

    vkb::Instance vkbInstance = instance.value();

    VktCachePtr->vkInstance = vkbInstance.instance;
    surface = WindowPtr->createWindowSurface(VktCachePtr->vkInstance);

    VkPhysicalDeviceFeatures features{};

    // Enabling geometry shader for debugging tools
    LOG(LOG_DEBUG, "Enabling geometry shader feature");
    features.geometryShader = true;

    // Enabling line fill mode for debugging tools
    LOG(LOG_DEBUG, "Enabling fillModeNonSolid feature");
    features.fillModeNonSolid = true;

    VkPhysicalDeviceVulkan13Features features13{};

    LOG(LOG_DEBUG, "Enabling dynamicRendering feature");
    features13.dynamicRendering = true;

    LOG(LOG_DEBUG, "Enabling synchronization2 feature");
    features13.synchronization2 = true;

    LOG(LOG_DEBUG, "Enabling maintenance4 feature");
    features13.maintenance4 = true;// TODO remove this, used to keep performance warning quiet

    VkPhysicalDeviceVulkan12Features features12{};

    LOG(LOG_DEBUG, "Enabling bufferDeviceAddress feature");
    features12.bufferDeviceAddress = true;

    LOG(LOG_DEBUG, "Enabling descriptorIndexing feature");
    features12.descriptorIndexing = true;

    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT dynamicFeatures3{};

    LOG(LOG_DEBUG, "Enabling extendedDynamicState3PolygonMode feature");

    dynamicFeatures3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
    dynamicFeatures3.pNext = VK_NULL_HANDLE;
    dynamicFeatures3.extendedDynamicState3PolygonMode = VK_TRUE;

    vkb::PhysicalDeviceSelector vkbSelector{vkbInstance};
    vkb::PhysicalDevice vkbPhysicalDevice =
        vkbSelector.set_minimum_version(1, 3)
                    .add_required_extension_features(dynamicFeatures3)
                    .add_required_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME)
                    .add_required_extension(VK_KHR_MAINTENANCE_5_EXTENSION_NAME)
                    .set_required_features_13(features13)
                    .set_required_features_12(features12)
                    .set_required_features(features)
                    .set_surface(surface)
                    .select()
                    .value();

    vkb::DeviceBuilder vkbDeviceBuilder{vkbPhysicalDevice};
    vkb::Device vkbDevice = vkbDeviceBuilder.build().value();

    LOG(LOG_INFO, "Found physical device: " << vkbDevice.physical_device.name);

    VktCachePtr->vkDevice = vkbDevice.device;// Caching device to be used globablly
    VktCachePtr->vkPhysicalDevice = vkbPhysicalDevice.physical_device;

    graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = VktCachePtr->vkPhysicalDevice;
    allocatorInfo.device = VktCachePtr->vkDevice;
    allocatorInfo.instance = VktCachePtr->vkInstance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    vmaCreateAllocator(&allocatorInfo, &VktCachePtr->vmaAllocator);// Caching allocator to be used globally

    LOG(LOG_INFO, "Finished creating vmaAllocator");

    coreDeletionQueue.pushDeletable(DeletableType::VMA_ALLOCATOR, VktCachePtr->vmaAllocator);
    coreDeletionQueue.pushDeletable(DeletableType::VK_DEBUG_UTILS_MESSENGER, vkbInstance.debug_messenger);

    LOG(LOG_INFO, "Finished initializing Vulkan device");
}

void VktCore::initSwapchain() {
    // TODO better way for this. Grab size from monitor size or something larger than a m_window size.
    createSwapchain(windowExtent.width, windowExtent.height);

    const VkExtent3D drawImageExtent = {
            windowExtent.width,
            windowExtent.height,
            1};

    constexpr VkImageUsageFlags drawImageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                 VK_IMAGE_USAGE_STORAGE_BIT |
                                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    constexpr VkImageUsageFlags depthImageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    // Create draw image
    drawImage = VktImages::createDeviceMemory(drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageFlags).value();
    coreDeletionQueue.pushDeletable(DeletableType::TEC_RESOURCE_IMAGE, &drawImage);

    LOG(LOG_INFO, "Finished creating draw image with format: "
                          << string_VkFormat(VK_FORMAT_R16G16B16A16_SFLOAT)
                          << " and extent dimensions: "
                          << drawImageExtent.width << 'x' << drawImageExtent.height);

    // Create depth image
    depthImage = VktImages::createDeviceMemory(drawImageExtent, VK_FORMAT_D32_SFLOAT, depthImageFlags).value();
    coreDeletionQueue.pushDeletable(DeletableType::TEC_RESOURCE_IMAGE, &depthImage);

    LOG(LOG_INFO, "Finished creating depth image with format: "
                          << string_VkFormat(VK_FORMAT_D32_SFLOAT)
                          << " and extent dimensions: "
                          << drawImageExtent.width << 'x' << drawImageExtent.height);
}

void VktCore::initCommands() {
    VkCommandPoolCreateInfo commandPoolCreateInfo = VktStructs::commandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for(uint8_t i = 0; i < FRAMES_OVERLAP; i++) {
        VK_CHECK(vkCreateCommandPool(VktCachePtr->vkDevice,
                                     &commandPoolCreateInfo,
                                     nullptr,
                                     &frames[i].commandPool))

        VkCommandBufferAllocateInfo cmdAllocInfo = VktStructs::commandBufferAllocateInfo(frames[i].commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(VktCachePtr->vkDevice,
                                          &cmdAllocInfo,
                                          &frames[i].mainCommandBuffer))
    }
}

void VktCore::initSyncStructs() {
    VkFenceCreateInfo fenceCreateInfo = VktStructs::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = VktStructs::semaphoreCreateInfo();

    for(uint8_t i = 0; i < FRAMES_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(VktCachePtr->vkDevice, &fenceCreateInfo, nullptr, &frames[i].renderFence))
        VK_CHECK(vkCreateSemaphore(VktCachePtr->vkDevice, &semaphoreCreateInfo, nullptr, &frames[i].swapchainSemaphore))
        VK_CHECK(vkCreateSemaphore(VktCachePtr->vkDevice, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore))
    }
}

void VktCore::createSwapchain(uint32_t width, uint32_t height) {
    LOG(LOG_INFO, "Creating swapchain of dimensions: " << width << 'x' << height);

    vkb::SwapchainBuilder vkbSwapchainBuilder(VktCachePtr->vkPhysicalDevice, VktCachePtr->vkDevice, surface);
    swapchainImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    vkb::Swapchain vkbSwapchain = vkbSwapchainBuilder
                                          .set_desired_format(VkSurfaceFormatKHR{
                                                  .format = swapchainImageFormat,
                                                  .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                          .set_desired_extent(width, height)
                                          .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                          .build()
                                          .value();

    swapchainExtent = vkbSwapchain.extent;
    swapchain = vkbSwapchain.swapchain;
    swapchainImages = vkbSwapchain.get_images().value();
    swapchainImageViews = vkbSwapchain.get_image_views().value();
    swapchainImageFormat = vkbSwapchain.image_format;// Builder might choose different format
    LOG(LOG_INFO, "Finished building swapchain with format: " << string_VkFormat(vkbSwapchain.image_format));
}

void VktCore::destroySwapchain() {
    vkDestroySwapchainKHR(VktCachePtr->vkDevice, swapchain, nullptr);

    for(auto &imageView: swapchainImageViews) {
        vkDestroyImageView(VktCachePtr->vkDevice, imageView, nullptr);
    }
}

VktTypes::FrameData &VktCore::getCurrentFrame() {
    return frames[frameNumber % FRAMES_OVERLAP];
}

void VktCore::draw() {
    // Updates relevant scene rendering data buffers
    updateScene();

    VK_CHECK(vkWaitForFences(VktCachePtr->vkDevice, 1, &getCurrentFrame().renderFence, true, 1000000000))
    VK_CHECK(vkResetFences(VktCachePtr->vkDevice, 1, &getCurrentFrame().renderFence))
    getCurrentFrame().deletionQueue.flush();
    getCurrentFrame().descriptors.clearPools();

    uint32_t swapchainIndex;
    VkResult error = vkAcquireNextImageKHR(VktCachePtr->vkDevice,
                                           swapchain,
                                           1000000000,
                                           getCurrentFrame().swapchainSemaphore,
                                           nullptr,
                                           &swapchainIndex);
    if(error == VK_ERROR_OUT_OF_DATE_KHR) {
        m_resizeSwapchain = true;
        return;
    }
    if(error) {
        throw vulkanException("Failed to acquire next image from swapchain, error code ", string_VkResult(error));
    }

    VkCommandBuffer cmd = getCurrentFrame().mainCommandBuffer;

    VK_CHECK(vkResetCommandBuffer(cmd, 0))
    VkCommandBufferBeginInfo cmdBeginInfo = VktStructs::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo))

    VktCachePtr->drawExtent.width = static_cast<uint32_t>(static_cast<float>(std::min(drawImage.extent.width, swapchainExtent.width)) * renderScale);
    VktCachePtr->drawExtent.height = static_cast<uint32_t>(static_cast<float>(std::min(drawImage.extent.height, swapchainExtent.height)) * renderScale);

    // Transition color and depth image into correct layouts
    VktUtils::transitionImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    VktUtils::transitionImage(cmd, depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkClearValue clrValue {
        .color = VkClearColorValue{0.0f, 0.0f, 0.0f, 1.0f}
    };
    VkRenderingAttachmentInfo colorAttachment = VktStructs::attachmentInfo(drawImage.view, &clrValue, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingAttachmentInfo depthAttachment = VktStructs::depthAttachmentInfo(depthImage.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderingInfo = VktStructs::renderingInfo(VktCachePtr->drawExtent, &colorAttachment, &depthAttachment);

    perfStats.drawCallCount = 0;
    perfStats.trigDrawCount = 0;
    auto startTime = std::chrono::system_clock::now();

    vkCmdBeginRendering(cmd, &renderingInfo);

    // Fetch scene uniform buffer allocated memory
    VktTypes::Resources::Buffer &gpuSceneDataBuffer = getCurrentFrame().sceneUniformBuffer;

    // Copy scene data to buffer
    memcpy(gpuSceneDataBuffer.info.pMappedData, &TecCorePtr->world->sceneData, sizeof(VktTypes::GPU::SceneData));

    VkDescriptorSet sceneDescriptorSet = getCurrentFrame().descriptors.allocate(VktCachePtr->getLayout(VktCache::Layout::SCENE));

    // Write scene data to uniforms
    {
        DescriptorWriter writer;
        writer.writeBuffer(0, gpuSceneDataBuffer.buffer,
                           sizeof(VktTypes::GPU::SceneData),
                           0,
                           VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        TecCorePtr->world->skybox->writeIBLSets(writer);
        /*
        writer.writeImage(1, m_skyboxIBLDiffuse.view,
                          VktCachePtr->getSampler(VktCachePtr->Sampler::LINEAR),
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImage(2, m_skyboxIBLSpecular.view,
                          VktCachePtr->getSampler(VktCachePtr->Sampler::LINEAR),
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImage(3, m_skyboxBRDF.view,
                          VktCachePtr->getSampler(VktCachePtr->Sampler::LINEAR),
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);*/
        writer.updateSet(sceneDescriptorSet);
    }

    VktUtils::CmdSetPolygonModeEXT(cmd,polygonMode);

    Skybox::draw(cmd, sceneDescriptorSet);
    //drawSkybox(cmd, sceneDescriptorSet);
    drawGeometry(cmd, sceneDescriptorSet);
    if(debugConfig.enableDebugNormals) {
        drawDebugNormals(cmd, sceneDescriptorSet);
    }
    if(debugConfig.enableDebugVectors) {
        drawDebugLines(cmd, sceneDescriptorSet);
        drawDebugBoxes(cmd, sceneDescriptorSet);
    }

    vkCmdEndRendering(cmd);

    auto endTime = std::chrono::system_clock::now();
    auto drawDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    perfStats.meshDrawTime = static_cast<float>(drawDuration.count()) / 1000.0f;

    mainDrawContext.opaqueSurfaces.clear();
    mainDrawContext.transparentSurfaces.clear();

    // Transition draw image and swap chain into correct transfer layouts
    VktUtils::transitionImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VktUtils::transitionImage(cmd, swapchainImages[swapchainIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy main image into swapchain
    VktUtils::copyImgToImg(cmd, drawImage.image, swapchainImages[swapchainIndex], VktCachePtr->drawExtent, swapchainExtent);

    VktUtils::transitionImage(cmd, swapchainImages[swapchainIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Draw ImGui
    drawImGui(cmd, swapchainImageViews[swapchainIndex]);

    // Transition swapchain image to present layout
    VktUtils::transitionImage(cmd, swapchainImages[swapchainIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd))

    VkCommandBufferSubmitInfo cmdInfo = VktStructs::commandBufferSubmitInfo(cmd);

    VkSemaphoreSubmitInfo waitInfo = VktStructs::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame().swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = VktStructs::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().renderSemaphore);

    VkSubmitInfo2 submit = VktStructs::submitInfo(&cmdInfo, &signalInfo, &waitInfo);

    VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, getCurrentFrame().renderFence))

    VkPresentInfoKHR presentInfo{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, .pNext = nullptr};
    presentInfo.pSwapchains = &swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainIndex;

    error = vkQueuePresentKHR(graphicsQueue, &presentInfo);
    if(error == VK_ERROR_OUT_OF_DATE_KHR) {
        m_resizeSwapchain = true;
    } else if(error) {
        throw vulkanException("Failed to queue swapchain image for present, error code ", string_VkResult(error));
    }

    frameNumber++;
}

void VktCore::drawGeometry(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptorSet) {

    auto draw = [&](const VktTypes::RenderObject &renderObject) {
        perfStats.drawCallCount++;
        perfStats.trigDrawCount += renderObject.indexCount / 3;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderObject.material->pipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                renderObject.material->pipeline->layout,
                                0, 1,
                                &sceneDescriptorSet, 0,
                                nullptr);

        VkViewport viewport = VktStructs::viewport(VktCachePtr->drawExtent);
        VkRect2D scissors = VktStructs::scissors(VktCachePtr->drawExtent);
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissors);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                renderObject.material->pipeline->layout,
                                1, 1,
                                &renderObject.material->materialSet, 0,
                                nullptr);

        vkCmdBindIndexBuffer(cmd, renderObject.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        if(renderObject.isSkinned) {
            VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Skinned> pushConstants;
            pushConstants.vertexBuffer = renderObject.vertexBufferAddress;
            pushConstants.jointsBuffer = renderObject.jointsBufferAddress;
            pushConstants.worldMatrix = renderObject.transform;
            vkCmdPushConstants(cmd, renderObject.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                               sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Skinned>), &pushConstants);
        } else {
            VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static> pushConstants;
            pushConstants.vertexBuffer = renderObject.vertexBufferAddress;
            pushConstants.worldMatrix = renderObject.transform;
            vkCmdPushConstants(cmd, renderObject.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                               sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>), &pushConstants);
        }

        vkCmdDrawIndexed(cmd, renderObject.indexCount, 1, renderObject.firstIndex, renderObject.vertexOffset, 0);
    };

    for(const VktTypes::RenderObject &renderObject: mainDrawContext.opaqueSurfaces) {
        draw(renderObject);
    }

    for(const VktTypes::RenderObject &renderObject: mainDrawContext.transparentSurfaces) {
        draw(renderObject);
    }
}

void VktCore::drawDebugNormals(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptorSet) {
    auto draw = [&](const VktTypes::RenderObject &renderObject) {
        perfStats.drawCallCount++;
        perfStats.trigDrawCount += renderObject.indexCount / 3;

        if(renderObject.isSkinned) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, debugPipelines.normalsSkinned.pipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    debugPipelines.normalsSkinned.layout,
                                    0, 1,
                                    &sceneDescriptorSet, 0,
                                    nullptr);
        } else {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, debugPipelines.normalsStatic.pipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    debugPipelines.normalsStatic.layout,
                                    0, 1,
                                    &sceneDescriptorSet, 0,
                                    nullptr);
        }

        VkViewport viewport = VktStructs::viewport(VktCachePtr->drawExtent);
        VkRect2D scissors = VktStructs::scissors(VktCachePtr->drawExtent);
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissors);

        vkCmdBindIndexBuffer(cmd, renderObject.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        if(renderObject.isSkinned) {
            VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Skinned> pushConstants;
            pushConstants.vertexBuffer = renderObject.vertexBufferAddress;
            pushConstants.jointsBuffer = renderObject.jointsBufferAddress;
            pushConstants.worldMatrix = renderObject.transform;
            vkCmdPushConstants(cmd, debugPipelines.normalsSkinned.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0,
                               sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Skinned>), &pushConstants);
        } else {
            VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static> pushConstants;
            pushConstants.vertexBuffer = renderObject.vertexBufferAddress;
            pushConstants.worldMatrix = renderObject.transform;
            vkCmdPushConstants(cmd, debugPipelines.normalsStatic.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0,
                               sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>), &pushConstants);
        }

        vkCmdDrawIndexed(cmd, renderObject.indexCount, 1, renderObject.firstIndex, 0, 0);
    };

    for(const VktTypes::RenderObject &renderObject: mainDrawContext.opaqueSurfaces) {
        draw(renderObject);
    }

    for(const VktTypes::RenderObject &renderObject: mainDrawContext.transparentSurfaces) {
        draw(renderObject);
    }
}

void VktCore::drawDebugLines(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptorSet) {

    for(auto &[id, line]: debugGeometry.lines) {
        if(line->meshBuffers.vertexBufferAddress == 0) {
            line->meshBuffers = createPrimitivesHostVisible<VktTypes::GPU::VertexType::POINT>(line->indices, line->vertices);
        } else {
            uploadPrimitivesToHostVisible<VktTypes::GPU::VertexType::POINT>(line->meshBuffers, line->indices, line->vertices);
        }
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, debugPipelines.lineStrip.pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                debugPipelines.lineStrip.layout,
                                0, 1,
                                &sceneDescriptorSet, 0,
                                nullptr);

        VkViewport viewport = VktStructs::viewport(VktCachePtr->drawExtent);
        VkRect2D scissors = VktStructs::scissors(VktCachePtr->drawExtent);
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissors);

        vkCmdBindIndexBuffer(cmd, line->meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static> pushConstants;
        pushConstants.worldMatrix = glm::identity<glm::mat4>();
        pushConstants.vertexBuffer = line->meshBuffers.vertexBufferAddress;
        vkCmdPushConstants(cmd, debugPipelines.lineStrip.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>), &pushConstants);
        vkCmdDrawIndexed(cmd, line->indices.size(), 1, 0, 0, 0);
        //vkCmdDraw(cmd, line->vertices.size(), 1, 0, 0);
    }

    for(auto &[id, line]: debugGeometry.infLines) {
        if(line->meshBuffers.vertexBufferAddress == 0) {
            line->meshBuffers = createPrimitivesHostVisible<VktTypes::GPU::VertexType::POINT>(line->indices, line->vertices);
        } else {
            uploadPrimitivesToHostVisible<VktTypes::GPU::VertexType::POINT>(line->meshBuffers, line->indices, line->vertices);
        }
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, debugPipelines.infLine.pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                debugPipelines.infLine.layout,
                                0, 1,
                                &sceneDescriptorSet, 0,
                                nullptr);

        VkViewport viewport = VktStructs::viewport(VktCachePtr->drawExtent);
        VkRect2D scissors = VktStructs::scissors(VktCachePtr->drawExtent);
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissors);

        vkCmdBindIndexBuffer(cmd, line->meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static> pushConstants;
        pushConstants.worldMatrix = glm::identity<glm::mat4>();
        pushConstants.vertexBuffer = line->meshBuffers.vertexBufferAddress;
        vkCmdPushConstants(cmd, debugPipelines.infLine.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>), &pushConstants);
        vkCmdDrawIndexed(cmd, line->indices.size(), 1, 0, 0, 0);
        //vkCmdDraw(cmd, line->vertices.size(), 1, 0, 0);
    }

}

void VktCore::drawDebugBoxes(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptorSet) {
    for(auto &[id, box]: debugGeometry.SAABBs) {
        if(box.mesh.meshBuffers.vertexBufferAddress == 0) {
            createSAABBMesh(box);
        }else{
            updateSAABBMesh(box);
        }
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, debugPipelines.box.pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                debugPipelines.box.layout,
                                0, 1,
                                &sceneDescriptorSet, 0,
                                nullptr);

        VkViewport viewport = VktStructs::viewport(VktCachePtr->drawExtent);
        VkRect2D scissors = VktStructs::scissors(VktCachePtr->drawExtent);
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissors);

        vkCmdBindIndexBuffer(cmd, box.mesh.meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static> pushConstants;
        pushConstants.worldMatrix = glm::identity<glm::mat4>();
        pushConstants.vertexBuffer = box.mesh.meshBuffers.vertexBufferAddress;
        vkCmdPushConstants(cmd, debugPipelines.box.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>), &pushConstants);
        vkCmdDrawIndexed(cmd, box.mesh.indices.size(), 1, 0, 0, 0);
    }
}

void VktCore::createSAABBMesh(VktTypes::PrimitiveMeshCache<SAABB>& boxCache) {
    BB::Corners_t points = boxCache.object->getCorners();
    boxCache.mesh.vertices.reserve(BB::BOX_CORNERS);
    for(const auto &p : points) {
        boxCache.mesh.vertices.push_back(VktTypes::GPU::Vertex<VktTypes::GPU::VertexType::POINT>{
            .position = p,
            .color = {1.0f, 1.0f, 1.0f}
        });
    }
    boxCache.mesh.indices.resize(BB::BOX_INDICES);
    boxCache.mesh.indices = {
        0, 4, 5, 0, 5, 1,
        0, 1, 3, 0, 3, 2,
        0, 6, 4, 0, 2, 6,
        7, 5, 1, 7, 3, 1,
        7, 5, 4, 7, 4, 6,
        7, 2, 3, 7, 6, 2
    };
    boxCache.mesh.meshBuffers = createPrimitivesHostVisible<VktTypes::GPU::VertexType::POINT>(boxCache.mesh.indices, boxCache.mesh.vertices);
}

void VktCore::updateSAABBMesh(VktTypes::PrimitiveMeshCache<SAABB>& boxCache) {
    BB::Corners_t points = boxCache.object->getCorners();
    for(std::size_t pIndex = 0; pIndex < BB::BOX_CORNERS; pIndex++) {
        boxCache.mesh.vertices[pIndex].position = points[pIndex];
    }
    uploadPrimitivesToHostVisible<VktTypes::GPU::VertexType::POINT>(boxCache.mesh.meshBuffers, boxCache.mesh.indices, boxCache.mesh.vertices);
}

uint32_t VktCore::addDebugPointMesh(VktTypes::PointMesh *pointMesh) {
    while(debugGeometry.lines.contains(lastPointMeshIndex)) lastPointMeshIndex++;
    debugGeometry.lines[lastPointMeshIndex] = pointMesh;
    return lastPointMeshIndex;
}

void VktCore::initDescriptors() {
    std::vector<DescriptorAllocatorDynamic::PoolSizeRatio> sizes = {
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}};

    VktCachePtr->descriptorAllocator.initPool(5, sizes);
    //VktCachePtr->descriptorAllocator.initPool(m_device, 5, sizes);

    // Build draw image descriptor layout
    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        VktCachePtr->storeLayout(VktCache::Layout::DRAW_IMAGE, builder.build(VK_SHADER_STAGE_COMPUTE_BIT));
    }

    // Allocate draw image
    drawImageDescriptors = VktCachePtr->descriptorAllocator.allocate(VktCachePtr->getLayout(VktCache::Layout::DRAW_IMAGE));

    // Write draw image
    {
        DescriptorWriter writer;
        writer.writeImage(0, drawImage.view,
                          VK_NULL_HANDLE,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(drawImageDescriptors);
    }

    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);        // SceneData
        builder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// IBLCube
        builder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// Specular cube
        builder.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// BRDF texture
        VktCachePtr->storeLayout(VktCache::Layout::SCENE, builder.build(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_GEOMETRY_BIT));
    }

    for(uint8_t i = 0; i < FRAMES_OVERLAP; i++) {
        std::vector<DescriptorAllocatorDynamic::PoolSizeRatio> frameSizes = {
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4}};

        frames[i].descriptors = DescriptorAllocatorDynamic{};
        frames[i].descriptors.initPool(1000, frameSizes);

        frames[i].sceneUniformBuffer = VktBuffers::create(sizeof(VktTypes::GPU::SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        coreDeletionQueue.pushDeletable(DeletableType::TEC_RESOURCE_BUFFER, &frames[i].sceneUniformBuffer);
        coreDeletionQueue.pushDeletable(DeletableType::TEC_DESCRIPTOR_ALLOCATOR_DYNAMIC, &frames[i].descriptors);
    }
}

void VktCore::initPipelines() {
    initMaterialPipelines();
    initDebugPipeline();
    LOG(LOG_INFO, "Finished initializing graphics pipelines");
}

void VktCore::initMaterialPipelines() {
    // Create vertex shader push constants range
    VkPushConstantRange matrixRange{};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPushConstantRange skinnedMatrixRange{};
    skinnedMatrixRange.offset = 0;
    skinnedMatrixRange.size = sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Skinned>);
    skinnedMatrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    {
        DescriptorLayoutBuilder layoutBuilder;
        // TODO Create enums/defs/constexpr for binding numbers
        layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);        // MaterialData
        layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// ColorTexture
        layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// MetalRoughnessTexture

        VktCachePtr->storeLayout(VktCache::Layout::MAT_METAL_ROUGHNESS, layoutBuilder.build(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
    }

    auto layouts = VktCachePtr->getLayouts(VktCache::Layout::SCENE, VktCache::Layout::MAT_METAL_ROUGHNESS);

    // Create pipeline layout with provided descriptors and push constants
    VkPipelineLayoutCreateInfo meshLayoutInfo = VktStructs::pipelineLayoutCreateInfo(layouts, matrixRange);

    VkPipelineLayout staticLayout;
    VK_CHECK(vkCreatePipelineLayout(VktCachePtr->vkDevice,
                                    &meshLayoutInfo,
                                    nullptr,
                                    &staticLayout))
    coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE_LAYOUT, staticLayout);

    meshLayoutInfo.pPushConstantRanges = &skinnedMatrixRange;

    VkPipelineLayout skinnedLayout;
    VK_CHECK(vkCreatePipelineLayout(VktCachePtr->vkDevice,
                                    &meshLayoutInfo,
                                    nullptr,
                                    &skinnedLayout))
    coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE_LAYOUT, skinnedLayout);

    // Set the pipeline layout for both opaque and transparent material pipeline
    metalRoughMaterial.opaquePipeline.layout = staticLayout;
    metalRoughMaterial.transparentPipeline.layout = staticLayout;
    metalRoughMaterial.skinnedOpaquePipeline.layout = skinnedLayout;
    metalRoughMaterial.skinnedTransparentPipeline.layout = skinnedLayout;

    // Build four pipelines fo opaque/transparent and static/skinned material rendering
    VktPipelineBuilder pipelineBuilder;
    pipelineBuilder.setShaders("shaders/mesh.vert.spv", "shaders/mesh.frag.spv");
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.setMultisamplingNone();
    pipelineBuilder.disableBlending();
    pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_LESS);
    pipelineBuilder.setColorAttachmentFormat(drawImage.format);
    pipelineBuilder.setDepthFormat(depthImage.format);
    pipelineBuilder.setPipelineLayout(staticLayout);

    metalRoughMaterial.opaquePipeline.pipeline = pipelineBuilder.buildPipeline();
    coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, metalRoughMaterial.opaquePipeline.pipeline);

    // Switch from opaque to transparent
    pipelineBuilder.enableBlendingAdditive();
    pipelineBuilder.enableDepthTest(false, VK_COMPARE_OP_LESS);

    metalRoughMaterial.transparentPipeline.pipeline = pipelineBuilder.buildPipeline();
    coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, metalRoughMaterial.transparentPipeline.pipeline);

    // Switch from static vertex shaders to skinned with joint matrices
    pipelineBuilder.setVertexShader("shaders/mesh_skin.vert.spv");
    pipelineBuilder.setPipelineLayout(skinnedLayout);

    // Maintain the pipeline order
    pipelineBuilder.disableBlending();
    pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_LESS);

    metalRoughMaterial.skinnedOpaquePipeline.pipeline = pipelineBuilder.buildPipeline();
    coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, metalRoughMaterial.skinnedOpaquePipeline.pipeline);

    pipelineBuilder.enableBlendingAdditive();
    pipelineBuilder.enableDepthTest(false, VK_COMPARE_OP_LESS);

    metalRoughMaterial.skinnedTransparentPipeline.pipeline = pipelineBuilder.buildPipeline();
    coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, metalRoughMaterial.skinnedTransparentPipeline.pipeline);
}

void VktCore::initDebugPipeline() {
    VkPushConstantRange normalStaticPushConstants{};
    normalStaticPushConstants.offset = 0;
    normalStaticPushConstants.size = sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>);
    normalStaticPushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;

    VkPushConstantRange normalSkinnedPushConstants{};
    normalSkinnedPushConstants.offset = 0;
    normalSkinnedPushConstants.size = sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Skinned>);
    normalSkinnedPushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;

    VkPushConstantRange linesPushConstants{};
    linesPushConstants.offset = 0;
    linesPushConstants.size = sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>);
    linesPushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPushConstantRange boxPushConstants{};
    boxPushConstants.offset = 0;
    boxPushConstants.size = sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>);
    boxPushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Debug pipeline use scene data for mesh transformations
    const auto layouts = VktCachePtr->getLayouts(VktCache::Layout::SCENE);

    VkPipelineLayoutCreateInfo meshLayoutInfo = VktStructs::pipelineLayoutCreateInfo(layouts, normalStaticPushConstants);

    VkPipelineLayout normalStaticLayout;
    VK_CHECK(vkCreatePipelineLayout(VktCachePtr->vkDevice,
                                    &meshLayoutInfo,
                                    nullptr,
                                    &normalStaticLayout))
    coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE_LAYOUT, normalStaticLayout);

    meshLayoutInfo.pPushConstantRanges = &normalSkinnedPushConstants;

    VkPipelineLayout normalSkinnedLayout;
    VK_CHECK(vkCreatePipelineLayout(VktCachePtr->vkDevice,
                                    &meshLayoutInfo,
                                    nullptr,
                                    &normalSkinnedLayout))
    coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE_LAYOUT, normalSkinnedLayout);

    meshLayoutInfo.pPushConstantRanges = &linesPushConstants;

    VkPipelineLayout linesLayout;
    VK_CHECK(vkCreatePipelineLayout(VktCachePtr->vkDevice,
                                    &meshLayoutInfo,
                                    nullptr,
                                    &linesLayout))
    coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE_LAYOUT, linesLayout);

    VkPipelineLayout infLinesLayout;
    VK_CHECK(vkCreatePipelineLayout(VktCachePtr->vkDevice,
                                    &meshLayoutInfo,
                                    nullptr,
                                    &infLinesLayout))
    coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE_LAYOUT, infLinesLayout);

    VkPipelineLayout boxLayout;
    VK_CHECK(vkCreatePipelineLayout(VktCachePtr->vkDevice,
                                    &meshLayoutInfo,
                                    nullptr,
                                    &boxLayout))
    coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE_LAYOUT, boxLayout);


    debugPipelines.normalsStatic.layout = normalStaticLayout;
    debugPipelines.normalsSkinned.layout = normalSkinnedLayout;
    debugPipelines.lineStrip.layout = linesLayout;
    debugPipelines.infLine.layout = infLinesLayout;
    debugPipelines.box.layout = boxLayout;

    {
        VktPipelineBuilder pipelineBuilder;
        pipelineBuilder.setVertexShader("shaders/debug/normal.vert.spv");
        pipelineBuilder.setFragmentShader("shaders/debug/mesh.frag.spv");
        pipelineBuilder.setGeometryShader("shaders/debug/normal.geom.spv");
        pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.setMultisamplingNone();
        pipelineBuilder.disableBlending();
        pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineBuilder.setColorAttachmentFormat(drawImage.format);
        pipelineBuilder.setDepthFormat(depthImage.format);
        pipelineBuilder.setPipelineLayout(normalStaticLayout);

        debugPipelines.normalsStatic.pipeline = pipelineBuilder.buildPipeline();
        coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, debugPipelines.normalsStatic.pipeline);

        pipelineBuilder.setVertexShader("shaders/debug/normal_skin.vert.spv");
        pipelineBuilder.setPipelineLayout(normalSkinnedLayout);

        debugPipelines.normalsSkinned.pipeline = pipelineBuilder.buildPipeline();
        coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, debugPipelines.normalsSkinned.pipeline);
    }

    {
        VktPipelineBuilder pipelineBuilder;
        pipelineBuilder.setVertexShader("shaders/debug/mesh.vert.spv");
        pipelineBuilder.setFragmentShader("shaders/debug/mesh.frag.spv");
        pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_LINE);
        pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.setMultisamplingNone();
        pipelineBuilder.disableBlending();
        pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineBuilder.disableDepthTest();
        pipelineBuilder.setColorAttachmentFormat(drawImage.format);
        pipelineBuilder.setDepthFormat(depthImage.format);
        pipelineBuilder.setPipelineLayout(linesLayout);

        debugPipelines.lineStrip.pipeline = pipelineBuilder.buildPipeline();
        coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, debugPipelines.lineStrip.pipeline);
    }

    {
        VktPipelineBuilder pipelineBuilder;
        // TODO change shader to infinite lines
        pipelineBuilder.setVertexShader("shaders/debug/line_inf.vert.spv");
        pipelineBuilder.setFragmentShader("shaders/debug/mesh.frag.spv");
        pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_LINE);
        pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.setMultisamplingNone();
        pipelineBuilder.disableBlending();
        pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineBuilder.disableDepthTest();
        pipelineBuilder.setColorAttachmentFormat(drawImage.format);
        pipelineBuilder.setDepthFormat(depthImage.format);
        pipelineBuilder.setPipelineLayout(linesLayout);

        debugPipelines.infLine.pipeline = pipelineBuilder.buildPipeline();
        coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, debugPipelines.infLine.pipeline);
    }

    {
        VktPipelineBuilder pipelineBuilder;
        pipelineBuilder.setVertexShader("shaders/debug/mesh.vert.spv");
        pipelineBuilder.setFragmentShader("shaders/debug/mesh.frag.spv");
        pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_LINE);
        pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.setMultisamplingNone();
        pipelineBuilder.disableBlending();
        pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineBuilder.disableDepthTest();
        pipelineBuilder.setColorAttachmentFormat(drawImage.format);
        pipelineBuilder.setDepthFormat(depthImage.format);
        pipelineBuilder.setPipelineLayout(linesLayout);
        pipelineBuilder.dynamicStates.pop_back(); // Disable dynamic polygons

        debugPipelines.box.pipeline = pipelineBuilder.buildPipeline();
        coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, debugPipelines.box.pipeline);
    }

}

void VktCore::initImGui() {

    // Overkill pool resources but only for dev so whatever
    VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
    VkDescriptorPoolCreateInfo poolInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .pNext = nullptr};
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkDescriptorPool imguiPool;
    VK_CHECK(vkCreateDescriptorPool(VktCachePtr->vkDevice, &poolInfo, nullptr, &imguiPool))

    ImGuiContext* ctx = ImGui::CreateContext();
    WindowPtr->initImGuiVulkan();

    ImGuiIO &io = ImGui::GetIO();

    // Enable keyboard controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Enable docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = VktCachePtr->vkInstance;
    initInfo.PhysicalDevice = VktCachePtr->vkPhysicalDevice;
    initInfo.Device = VktCachePtr->vkDevice;
    initInfo.Queue = graphicsQueue;
    initInfo.QueueFamily = graphicsQueueFamily;
    initInfo.DescriptorPool = imguiPool;
    initInfo.MinImageCount = swapchainImages.size();
    initInfo.ImageCount = swapchainImages.size();
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineRenderingCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &drawImage.format,
            .depthAttachmentFormat = depthImage.format};
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.CheckVkResultFn = &ImGuiHandler::vkDebugCallback;

    ImGui_ImplVulkan_Init(&initInfo);
    ImGui_ImplVulkan_CreateFontsTexture();
    coreDeletionQueue.pushDeletable(DeletableType::VK_DESCRIPTOR_POOL, imguiPool);

    ImGuiHandler::initMenus();
    ImGuiHandler::mainWindow.Surface = surface;

    const VkFormat requestSurfaceImageFormat[] = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    ImGuiHandler::mainWindow.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(VktCachePtr->vkPhysicalDevice, surface, requestSurfaceImageFormat, ARRAY_SIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);
    VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};

    ImGuiHandler::mainWindow.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(VktCachePtr->vkPhysicalDevice, surface, &present_modes[0], ARRAY_SIZE(present_modes));

    // Create SwapChain, RenderPass, Framebuffer, etc.
    /*ImGui_ImplVulkanH_CreateOrResizeWindow(VktCachePtr->vkInstance,
                                           VktCachePtr->vkPhysicalDevice,
                                           VktCachePtr->vkDevice,
                                           &ImGuiHandler::mainWindow,
                                           graphicsQueueFamily,
                                           nullptr,
                                           swapchainExtent.width,
                                           swapchainExtent.height,
                                           swapchainImages.size());*/
}

void VktCore::drawImGui(VkCommandBuffer cmd, VkImageView targetView) {
    VkRenderingAttachmentInfo colorAttachment = VktStructs::attachmentInfo(targetView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingInfo renderInfo = VktStructs::renderingInfo(swapchainExtent, &colorAttachment, nullptr);

    vkCmdBeginRendering(cmd, &renderInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRendering(cmd);
}

bool VktCore::shouldClose() {
    return WindowPtr->shouldClose();
}

void VktCore::run() {

    auto startTime = std::chrono::system_clock::now();

    if(m_resizeSwapchain) {
        resizeSwapchain();
        ImGui_ImplVulkan_SetMinImageCount(swapchainImages.size());
    }

    ImGuiHandler::run();
    //runImGui();

    draw();

    auto endTime = std::chrono::system_clock::now();
    auto drawDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    perfStats.frametime = static_cast<float>(drawDuration.count()) / 1000.0f;
}

template VktTypes::GPU::MeshBuffers VktCore::createPrimitivesDeviceMemory<VktTypes::GPU::VertexType::SKINNED>(const std::span<uint32_t> &indices, const std::span<VktTypes::GPU::Vertex<VktTypes::GPU::VertexType::SKINNED>> &vertices);
template VktTypes::GPU::MeshBuffers VktCore::createPrimitivesDeviceMemory<VktTypes::GPU::VertexType::STATIC>(const std::span<uint32_t> &indices, const std::span<VktTypes::GPU::Vertex<VktTypes::GPU::VertexType::STATIC>> &vertices);

template<VktTypes::GPU::VertexType vType>
VktTypes::GPU::MeshBuffers VktCore::createPrimitivesDeviceMemory(const std::span<uint32_t> &indices, const std::span<VktTypes::GPU::Vertex<vType>> &vertices) {
    const size_t vertexBufferSize = vertices.size() * sizeof(VktTypes::GPU::Vertex<vType>);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    VktTypes::GPU::MeshBuffers newBuffers{};
    newBuffers.vertexBuffer = VktBuffers::create(vertexBufferSize,
                                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                 VMA_MEMORY_USAGE_GPU_ONLY);

    VkBufferDeviceAddressInfo deviceAddressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .pNext = nullptr};
    deviceAddressInfo.buffer = newBuffers.vertexBuffer.buffer;
    newBuffers.vertexBufferAddress = vkGetBufferDeviceAddress(VktCachePtr->vkDevice, &deviceAddressInfo);

    newBuffers.indexBuffer = VktBuffers::create(indexBufferSize,
                                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                VMA_MEMORY_USAGE_GPU_ONLY);

    VktTypes::Resources::Buffer staging = VktBuffers::create(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void *data = staging.info.pMappedData;
    memcpy(data, vertices.data(), vertexBufferSize);
    memcpy((char *) data + vertexBufferSize, indices.data(), indexBufferSize);

    VktInstantCommands::submitCommands([&](VkCommandBuffer cmd) {
        VkBufferCopy vertexCopy{};
        vertexCopy.srcOffset = 0;
        vertexCopy.dstOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newBuffers.vertexBuffer.buffer, 1, &vertexCopy);

        VkBufferCopy indexCopy{};
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.dstOffset = 0;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newBuffers.indexBuffer.buffer, 1, &indexCopy);
    });
    VktBuffers::destroy(staging);

    return newBuffers;
}

template VktTypes::GPU::MeshBuffers VktCore::createPrimitivesHostVisible<VktTypes::GPU::VertexType::POINT>(const std::span<uint32_t> &indices, const std::span<VktTypes::GPU::Vertex<VktTypes::GPU::VertexType::POINT>> &vertices);

template<VktTypes::GPU::VertexType vType>
VktTypes::GPU::MeshBuffers VktCore::createPrimitivesHostVisible(const std::span<uint32_t> &indices, const std::span<VktTypes::GPU::Vertex<vType>> &vertices) {
    const size_t vertexBufferSize = vertices.size() * sizeof(VktTypes::GPU::Vertex<vType>);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    VktTypes::GPU::MeshBuffers newBuffers{};
    newBuffers.vertexBuffer = VktBuffers::create(vertexBufferSize,
                                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                 VMA_MEMORY_USAGE_CPU_TO_GPU);

    VkBufferDeviceAddressInfo deviceAddressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .pNext = nullptr};
    deviceAddressInfo.buffer = newBuffers.vertexBuffer.buffer;
    newBuffers.vertexBufferAddress = vkGetBufferDeviceAddress(VktCachePtr->vkDevice, &deviceAddressInfo);

    newBuffers.indexBuffer = VktBuffers::create(indexBufferSize,
                                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                VMA_MEMORY_USAGE_CPU_TO_GPU);

    void *vertexBufferData = newBuffers.vertexBuffer.info.pMappedData;
    memcpy(vertexBufferData, vertices.data(), vertexBufferSize);

    void *indexBufferData = newBuffers.indexBuffer.info.pMappedData;
    memcpy(indexBufferData, indices.data(), indexBufferSize);

    return newBuffers;
}

template void VktCore::uploadPrimitivesToHostVisible<VktTypes::GPU::VertexType::POINT>(const VktTypes::GPU::MeshBuffers &meshBuffers,
                                                                                       std::span<uint32_t> indices,
                                                                                       std::span<VktTypes::GPU::Vertex<VktTypes::GPU::VertexType::POINT>> vertices);

template<VktTypes::GPU::VertexType vType>
void VktCore::uploadPrimitivesToHostVisible(const VktTypes::GPU::MeshBuffers &meshBuffers, std::span<uint32_t> indices, std::span<VktTypes::GPU::Vertex<vType>> vertices) {
    const size_t vertexBufferSize = vertices.size() * sizeof(VktTypes::GPU::Vertex<vType>);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    void *vertexBufferData = meshBuffers.vertexBuffer.info.pMappedData;
    memcpy(vertexBufferData, vertices.data(), vertexBufferSize);

    void *indexBufferData = meshBuffers.indexBuffer.info.pMappedData;
    memcpy(indexBufferData, indices.data(), indexBufferSize);
}

VktTypes::GPU::JointsBuffers VktCore::uploadJoints(const std::span<glm::mat4> &jointMatrices) {
    const size_t jointsBufferSize = jointMatrices.size() * sizeof(glm::mat4);

    VktTypes::GPU::JointsBuffers newJoints{};
    newJoints.jointsBuffer = VktBuffers::create(jointsBufferSize,
                                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                VMA_MEMORY_USAGE_CPU_TO_GPU);

    VkBufferDeviceAddressInfo deviceAddressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .pNext = nullptr};
    deviceAddressInfo.buffer = newJoints.jointsBuffer.buffer;
    newJoints.jointsBufferAddress = vkGetBufferDeviceAddress(VktCachePtr->vkDevice, &deviceAddressInfo);

    memcpy(newJoints.jointsBuffer.info.pMappedData, jointMatrices.data(), jointsBufferSize);

    return newJoints;
}

void VktCore::initDefaultData() {
    std::array<u_char, 4> white = {0xFF, 0xFF, 0xFF, 0xFF};
    std::array<u_char, 4> black = {0x00, 0x00, 0x00, 0x00};
    std::array<u_char, 4> gray = {0xAA, 0xAA, 0xAA, 0xFF};

    m_whiteImage = VktImages::createDeviceMemory(VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT).value();
    m_blackImage = VktImages::createDeviceMemory(VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT).value();
    m_greyImage = VktImages::createDeviceMemory(VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT).value();
    VktImages::copyFromRaw(m_whiteImage, 4, reinterpret_cast<char *>(white.data()));
    VktImages::copyFromRaw(m_blackImage, 4, reinterpret_cast<char *>(black.data()));
    VktImages::copyFromRaw(m_greyImage, 4, reinterpret_cast<char *>(gray.data()));

    uint32_t pink = 0xFF00DC;
    std::array<uint32_t, 16 * 16> pixels{};
    for(uint32_t x = 0; x < 16; x++) {
        for(uint32_t y = 0; y < 16; y++) {
            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? pink : 0;
        }
    }
    m_errorCheckboardImage = VktImages::createDeviceMemory(VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT).value();
    VktImages::copyFromRaw(m_errorCheckboardImage, 16 * 16 * 4, reinterpret_cast<char *>(pixels.data()));

    VkSamplerCreateInfo samplerInfo{.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};

    VkSampler tmpSamplerHandle;

    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(VktCachePtr->vkDevice, &samplerInfo, nullptr, &tmpSamplerHandle);
    VktCachePtr->storeSampler(VktCache::Sampler::NEAREST, tmpSamplerHandle);
    coreDeletionQueue.pushDeletable(DeletableType::VK_SAMPLER, tmpSamplerHandle);

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(VktCachePtr->vkDevice, &samplerInfo, nullptr, &tmpSamplerHandle);
    VktCachePtr->storeSampler(VktCache::Sampler::LINEAR, tmpSamplerHandle);
    coreDeletionQueue.pushDeletable(DeletableType::VK_SAMPLER, tmpSamplerHandle);
}

VkBool32 VktCore::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
                                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
    auto severityStr = vkb::to_string_message_severity(severity);
    auto typeStr = vkb::to_string_message_type(type);
    fprintf(stderr, "[%s: %s] %s\n", severityStr, typeStr, pCallbackData->pMessage);
    return VK_FALSE;
}

void VktCore::resizeSwapchain() {
    vkDeviceWaitIdle(VktCachePtr->vkDevice);
    destroySwapchain();
    setExtentDimensions();
    createSwapchain(windowExtent.width, windowExtent.height);
    m_resizeSwapchain = false;
}

void VktCore::setExtentDimensions() {
    Utils::WindowDimension winDimensions = WindowPtr->getSize();
    windowExtent.width = winDimensions.width;
    windowExtent.height = winDimensions.height;
}

void VktCore::updateScene() {
    mainDrawContext.opaqueSurfaces.clear();

    for(auto &[oID, object]: loadedObjects) {
        if(object.model->isSkinned() && object.model->currentAnimation() != ModelTypes::NULL_ID) {
            object.model->updateAnimationTime();
            object.model->updateJoints();
        }
        object.model->gatherDrawContext(mainDrawContext);
    }
    TecCorePtr->world->gatherDrawContext(mainDrawContext);
}

/**
 * Initialize first object identifiers to 0.
 */
VktCore::objectID_t VktCore::EngineObject::lastID = 0;

VktCore::EngineObject *VktCore::createObject(const std::string &name, const std::filesystem::path &filePath) {
    // Find free identifier
    while(loadedObjects.contains(EngineObject::lastID)) EngineObject::lastID++;
    objectID_t freeID = EngineObject::lastID++;

    // Create object
    loadedObjects[freeID] = EngineObject{
            .objectID = freeID,
            .name = name,
            .model = new Model(filePath),
    };

    // Upload default position
    if(loadedObjects[freeID].model->isSkinned()) {
        loadedObjects[freeID].model->updateJoints();
    }

    LOG(LOG_INFO, "Created an object " << name << " with ID " << freeID);
    return &loadedObjects[freeID];
}

VktCore::EngineObject *VktCore::createObject(const std::string &name, Model *model) {
    // Find free identifier
    while(loadedObjects.contains(EngineObject::lastID)) EngineObject::lastID++;
    objectID_t freeID = EngineObject::lastID++;

    // Create object
    loadedObjects[freeID] = EngineObject{
            .objectID = freeID,
            .name = name,
            .model = model};

    // Upload default position
    if(loadedObjects[freeID].model->isSkinned()) {
        loadedObjects[freeID].model->updateJoints();
    }

    LOG(LOG_INFO, "Created an object " << name << " with ID " << freeID);
    return &loadedObjects[freeID];
}

VktTypes::MaterialInstance VktCore::writeMaterial(VktTypes::MaterialPass pass,
                                                  const VktTypes::GLTFMetallicRoughness::MaterialResources &resources,
                                                  DescriptorAllocatorDynamic &descriptorAllocator,
                                                  bool isSkinned) {
    VktTypes::MaterialInstance matData{};
    matData.passType = pass;
    switch(pass) {
        case VktTypes::MaterialPass::OPAQUE:
            matData.pipeline = isSkinned ? &metalRoughMaterial.skinnedOpaquePipeline : &metalRoughMaterial.opaquePipeline;
            break;
        case VktTypes::MaterialPass::TRANSPARENT:
            matData.pipeline = isSkinned ? &metalRoughMaterial.skinnedTransparentPipeline : &metalRoughMaterial.transparentPipeline;
            break;
        default:
            break;
    }

    matData.materialSet = descriptorAllocator.allocate(VktCachePtr->getLayout(VktCache::Layout::MAT_METAL_ROUGHNESS));

    metalRoughMaterial.writer.clear();
    metalRoughMaterial.writer.writeBuffer(0, resources.dataBuffer,
                                          sizeof(VktTypes::GLTFMetallicRoughness::MaterialConstants),
                                          resources.dataBufferOffset,
                                          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    metalRoughMaterial.writer.writeImage(1, resources.colorImage.view,
                                         resources.colorSampler,
                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    metalRoughMaterial.writer.writeImage(2, resources.metalRoughImage.view,
                                         resources.metalRoughSampler,
                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    metalRoughMaterial.writer.updateSet(matData.materialSet);

    return matData;
}