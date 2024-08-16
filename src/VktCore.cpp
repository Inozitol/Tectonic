
#include "engine/vulkan/VktCore.h"
#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "engine/vulkan/VktCache.h"
#include "extern/stb/stb_image.h"
#include <vk_mem_alloc.h>

VktCore &VktCore::getInstance() {
    static VktCore instance;
    return instance;
}

void VktCore::init() {
    assert(m_window != nullptr);

    m_logger(Logger::INFO) << "Initializing VktCore\n";

    initVulkan();
    initSwapchain();
    initCommands();
    initSyncStructs();
    initDescriptors();
    initPipelines();
    initImGui();
    VktInstantCommands::init(m_graphicsQueueFamily, m_graphicsQueue);
    ktxVulkanDeviceInfo_Construct(&VktCache::ktxInfo, m_physicalDevice, VktCache::vkDevice, m_graphicsQueue, VktInstantCommands::vkCmdPool, nullptr);
    initDefaultData();

    m_isInitialized = true;
}

void VktCore::clear() {
    if(m_isInitialized) {
        vkDeviceWaitIdle(VktCache::vkDevice);

        for(auto &[k, object]: loadedObjects) {
            object.model.clear();
        }

        for(auto &[id, layout]: VktCache::getAllLayouts()) {
            vkDestroyDescriptorSetLayout(VktCache::vkDevice, layout, nullptr);
        }

        m_skybox.clear();

        VktImages::destroy(m_whiteImage);
        VktImages::destroy(m_blackImage);
        VktImages::destroy(m_greyImage);
        VktImages::destroy(m_errorCheckboardImage);

        VktInstantCommands::clear();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_coreDeletionQueue.flush();
        VktCache::descriptorAllocator.destroyPool();

        for(uint8_t i = 0; i < FRAMES_OVERLAP; i++) {
            vkDestroyCommandPool(VktCache::vkDevice, m_frames[i].commandPool, nullptr);
            vkDestroyFence(VktCache::vkDevice, m_frames[i].renderFence, nullptr);
            vkDestroySemaphore(VktCache::vkDevice, m_frames[i].renderSemaphore, nullptr);
            vkDestroySemaphore(VktCache::vkDevice, m_frames[i].swapchainSemaphore, nullptr);
        }

        destroySwapchain();

        vkDestroySurfaceKHR(VktCache::vkInstance, m_surface, nullptr);
        vkDestroyDevice(VktCache::vkDevice, nullptr);

        vkDestroyInstance(VktCache::vkInstance, nullptr);

        m_isInitialized = false;
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

    m_logger(Logger::INFO) << "Finished building vkbInstance\n";

    vkb::Instance vkbInstance = instance.value();

    VktCache::vkInstance = vkbInstance.instance;
    m_surface = m_window->createWindowSurface(VktCache::vkInstance);

    VkPhysicalDeviceFeatures features{};

    // Enabling geometry shader for debugging tools
    m_logger(Logger::DEBUG) << "Enabling geometry shader feature\n";
    features.geometryShader = true;

    VkPhysicalDeviceVulkan13Features features13{};

    m_logger(Logger::DEBUG) << "Enabling dynamicRendering feature\n";
    features13.dynamicRendering = true;

    m_logger(Logger::DEBUG) << "Enabling synchronization2 feature\n";
    features13.synchronization2 = true;

    m_logger(Logger::DEBUG) << "Enabling maintenance4 feature\n";
    features13.maintenance4 = true; // TODO remove this, used to keep performance warning quiet

    VkPhysicalDeviceVulkan12Features features12{};

    m_logger(Logger::DEBUG) << "Enabling bufferDeviceAddress feature\n";
    features12.bufferDeviceAddress = true;

    m_logger(Logger::DEBUG) << "Enabling descriptorIndexing feature\n";
    features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector vkbSelector{vkbInstance};
    vkb::PhysicalDevice vkbPhysicalDevice = vkbSelector
                                                    .set_minimum_version(1, 3)
                                                    .add_required_extension(VK_KHR_MAINTENANCE_5_EXTENSION_NAME)
                                                    .set_required_features_13(features13)
                                                    .set_required_features_12(features12)
                                                    .set_required_features(features)
                                                    .set_surface(m_surface)
                                                    .select()
                                                    .value();

    vkb::DeviceBuilder vkbDeviceBuilder{vkbPhysicalDevice};
    vkb::Device vkbDevice = vkbDeviceBuilder.build().value();

    m_logger(Logger::INFO) << "Found physical device: " << vkbDevice.physical_device.name << '\n';

    VktCache::vkDevice = vkbDevice.device;// Caching device to be used globablly
    m_physicalDevice = vkbPhysicalDevice.physical_device;


    m_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    m_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = VktCache::vkDevice;
    allocatorInfo.instance = VktCache::vkInstance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    vmaCreateAllocator(&allocatorInfo, &VktCache::vmaAllocator);// Caching allocator to be used globally

    m_logger(Logger::INFO) << "Finished creating vmaAllocator\n";

    m_coreDeletionQueue.pushDeletable(DeletableType::VMA_ALLOCATOR, VktCache::vmaAllocator);
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_DEBUG_UTILS_MESSENGER, vkbInstance.debug_messenger);

    m_logger(Logger::INFO) << "Finished initializing Vulkan device\n";
}

void VktCore::initSwapchain() {
    // TODO better way for this. Grab size from monitor size or something larger than a m_window size.
    createSwapchain(m_windowExtent.width, m_windowExtent.height);

    const VkExtent3D drawImageExtent = {
            m_windowExtent.width,
            m_windowExtent.height,
            1};

    constexpr VkImageUsageFlags drawImageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                 VK_IMAGE_USAGE_STORAGE_BIT |
                                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    constexpr VkImageUsageFlags depthImageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    // Create draw image
    m_drawImage = VktImages::createDeviceMemory(drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageFlags).value();
    m_coreDeletionQueue.pushDeletable(DeletableType::TEC_RESOURCE_IMAGE, &m_drawImage);

    m_logger(Logger::INFO) << "Finished creating draw image with format: "
                           << string_VkFormat(VK_FORMAT_R16G16B16A16_SFLOAT)
                           << " and extent dimensions: "
                           << drawImageExtent.width << 'x' << drawImageExtent.height << '\n';

    // Create depth image
    m_depthImage = VktImages::createDeviceMemory(drawImageExtent, VK_FORMAT_D32_SFLOAT, depthImageFlags).value();
    m_coreDeletionQueue.pushDeletable(DeletableType::TEC_RESOURCE_IMAGE, &m_depthImage);

    m_logger(Logger::INFO) << "Finished creating depth image with format: "
                           << string_VkFormat(VK_FORMAT_D32_SFLOAT)
                           << " and extent dimensions: "
                           << drawImageExtent.width << 'x' << drawImageExtent.height << '\n';
}

void VktCore::initCommands() {
    VkCommandPoolCreateInfo commandPoolCreateInfo = VktStructs::commandPoolCreateInfo(m_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for(uint8_t i = 0; i < FRAMES_OVERLAP; i++) {
        VK_CHECK(vkCreateCommandPool(VktCache::vkDevice,
                                     &commandPoolCreateInfo,
                                     nullptr,
                                     &m_frames[i].commandPool))

        VkCommandBufferAllocateInfo cmdAllocInfo = VktStructs::commandBufferAllocateInfo(m_frames[i].commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(VktCache::vkDevice,
                                          &cmdAllocInfo,
                                          &m_frames[i].mainCommandBuffer))
    }
}

void VktCore::initSyncStructs() {
    VkFenceCreateInfo fenceCreateInfo = VktStructs::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = VktStructs::semaphoreCreateInfo();

    for(uint8_t i = 0; i < FRAMES_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(VktCache::vkDevice, &fenceCreateInfo, nullptr, &m_frames[i].renderFence))
        VK_CHECK(vkCreateSemaphore(VktCache::vkDevice, &semaphoreCreateInfo, nullptr, &m_frames[i].swapchainSemaphore))
        VK_CHECK(vkCreateSemaphore(VktCache::vkDevice, &semaphoreCreateInfo, nullptr, &m_frames[i].renderSemaphore))
    }
}

void VktCore::createSwapchain(uint32_t width, uint32_t height) {
    m_logger(Logger::INFO) << "Creating swapchain of dimensions: " << width << 'x' << height << '\n';

    vkb::SwapchainBuilder vkbSwapchainBuilder(m_physicalDevice, VktCache::vkDevice, m_surface);
    m_swapchainImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

    vkb::Swapchain vkbSwapchain = vkbSwapchainBuilder
                                          .set_desired_format(VkSurfaceFormatKHR{
                                                  .format = m_swapchainImageFormat,
                                                  .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                          .set_desired_extent(width, height)
                                          .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                          .build()
                                          .value();

    m_swapchainExtent = vkbSwapchain.extent;
    m_swapchain = vkbSwapchain.swapchain;
    m_swapchainImages = vkbSwapchain.get_images().value();
    m_swapchainImageViews = vkbSwapchain.get_image_views().value();
    m_swapchainImageFormat = vkbSwapchain.image_format;// Builder might choose different format
    m_logger(Logger::INFO) << "Finished building swapchain with format: " << string_VkFormat(vkbSwapchain.image_format) << '\n';
}

void VktCore::destroySwapchain() {
    vkDestroySwapchainKHR(VktCache::vkDevice, m_swapchain, nullptr);

    for(auto &imageView: m_swapchainImageViews) {
        vkDestroyImageView(VktCache::vkDevice, imageView, nullptr);
    }
}

VktTypes::FrameData &VktCore::getCurrentFrame() {
    return m_frames[m_frameNumber % FRAMES_OVERLAP];
}

void VktCore::draw() {
    // Updates relevant scene rendering data buffers
    updateScene();

    VK_CHECK(vkWaitForFences(VktCache::vkDevice, 1, &getCurrentFrame().renderFence, true, 1000000000))
    VK_CHECK(vkResetFences(VktCache::vkDevice, 1, &getCurrentFrame().renderFence))
    getCurrentFrame().deletionQueue.flush();
    getCurrentFrame().descriptors.clearPools();

    uint32_t swapchainIndex;
    VkResult error = vkAcquireNextImageKHR(VktCache::vkDevice,
                                           m_swapchain,
                                           1000000000,
                                           getCurrentFrame().swapchainSemaphore,
                                           nullptr,
                                           &swapchainIndex);
    if(error == VK_ERROR_OUT_OF_DATE_KHR) {
        m_resizeSwapchain = true;
        return;
    } else if(error) {
        throw vulkanException("Failed to acquire next image from swapchain, error code ", string_VkResult(error));
    }

    VkCommandBuffer cmd = getCurrentFrame().mainCommandBuffer;

    VK_CHECK(vkResetCommandBuffer(cmd, 0))
    VkCommandBufferBeginInfo cmdBeginInfo = VktStructs::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo))

    VktCache::drawExtent.width = static_cast<uint32_t>(static_cast<float>(std::min(m_drawImage.extent.width, m_swapchainExtent.width)) * m_renderScale);
    VktCache::drawExtent.height = static_cast<uint32_t>(static_cast<float>(std::min(m_drawImage.extent.height, m_swapchainExtent.height)) * m_renderScale);

    // Transition color and depth image into correct layouts
    VktUtils::transitionImage(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    VktUtils::transitionImage(cmd, m_depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo colorAttachment = VktStructs::attachmentInfo(m_drawImage.view, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingAttachmentInfo depthAttachment = VktStructs::depthAttachmentInfo(m_depthImage.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderingInfo = VktStructs::renderingInfo(VktCache::drawExtent, &colorAttachment, &depthAttachment);

    m_stats.drawCallCount = 0;
    m_stats.trigDrawCount = 0;
    auto startTime = std::chrono::system_clock::now();

    vkCmdBeginRendering(cmd, &renderingInfo);

    // Fetch scene uniform buffer allocated memory
    VktTypes::Resources::Buffer &gpuSceneDataBuffer = getCurrentFrame().sceneUniformBuffer;

    // Copy scene data to buffer
    memcpy(gpuSceneDataBuffer.info.pMappedData, &m_sceneData, sizeof(VktTypes::GPU::SceneData));

    VkDescriptorSet sceneDescriptorSet = getCurrentFrame().descriptors.allocate(VktCache::getLayout(VktCache::Layout::SCENE));

    // Write scene data to uniforms
    {
        DescriptorWriter writer;
        writer.writeBuffer(0, gpuSceneDataBuffer.buffer,
                           sizeof(VktTypes::GPU::SceneData),
                           0,
                           VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        m_skybox.writeIBLSets(writer);
        /*
        writer.writeImage(1, m_skyboxIBLDiffuse.view,
                          VktCache::getSampler(VktCache::Sampler::LINEAR),
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImage(2, m_skyboxIBLSpecular.view,
                          VktCache::getSampler(VktCache::Sampler::LINEAR),
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImage(3, m_skyboxBRDF.view,
                          VktCache::getSampler(VktCache::Sampler::LINEAR),
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);*/
        writer.updateSet(sceneDescriptorSet);
    }

    VktSkybox::draw(cmd, sceneDescriptorSet);
    //drawSkybox(cmd, sceneDescriptorSet);
    drawGeometry(cmd, sceneDescriptorSet);
    if(m_debugConf.enableDebugPipeline) {
        drawDebug(cmd, sceneDescriptorSet);
    }

    vkCmdEndRendering(cmd);

    auto endTime = std::chrono::system_clock::now();
    auto drawDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    m_stats.meshDrawTime = static_cast<float>(drawDuration.count()) / 1000.0f;

    m_mainDrawContext.opaqueSurfaces.clear();
    m_mainDrawContext.transparentSurfaces.clear();

    // Transition draw image and swap chain into correct transfer layouts
    VktUtils::transitionImage(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VktUtils::transitionImage(cmd, m_swapchainImages[swapchainIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy main image into swapchain
    VktUtils::copyImgToImg(cmd, m_drawImage.image, m_swapchainImages[swapchainIndex], VktCache::drawExtent, m_swapchainExtent);

    VktUtils::transitionImage(cmd, m_swapchainImages[swapchainIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Draw ImGui
    drawImGui(cmd, m_swapchainImageViews[swapchainIndex]);

    // Transition swapchain image to present layout
    VktUtils::transitionImage(cmd, m_swapchainImages[swapchainIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd))

    VkCommandBufferSubmitInfo cmdInfo = VktStructs::commandBufferSubmitInfo(cmd);

    VkSemaphoreSubmitInfo waitInfo = VktStructs::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame().swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = VktStructs::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().renderSemaphore);

    VkSubmitInfo2 submit = VktStructs::submitInfo(&cmdInfo, &signalInfo, &waitInfo);

    VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submit, getCurrentFrame().renderFence))

    VkPresentInfoKHR presentInfo{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, .pNext = nullptr};
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainIndex;

    error = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
    if(error == VK_ERROR_OUT_OF_DATE_KHR) {
        m_resizeSwapchain = true;
    } else if(error) {
        throw vulkanException("Failed to queue swapchain image for present, error code ", string_VkResult(error));
    }

    m_frameNumber++;
}

void VktCore::drawGeometry(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptorSet) {

    auto draw = [&](const VktTypes::RenderObject &renderObject) {
        m_stats.drawCallCount++;
        m_stats.trigDrawCount += renderObject.indexCount / 3;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderObject.material->pipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                renderObject.material->pipeline->layout,
                                0, 1,
                                &sceneDescriptorSet, 0,
                                nullptr);

        VkViewport viewport = VktStructs::viewport(VktCache::drawExtent);
        VkRect2D scissors = VktStructs::scissors(VktCache::drawExtent);
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

        vkCmdDrawIndexed(cmd, renderObject.indexCount, 1, renderObject.firstIndex, 0, 0);
    };

    for(const VktTypes::RenderObject &renderObject: m_mainDrawContext.opaqueSurfaces) {
        draw(renderObject);
    }

    for(const VktTypes::RenderObject &renderObject: m_mainDrawContext.transparentSurfaces) {
        draw(renderObject);
    }
}

void VktCore::drawDebug(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptorSet) {
    auto draw = [&](const VktTypes::RenderObject &renderObject) {
        m_stats.drawCallCount++;
        m_stats.trigDrawCount += renderObject.indexCount / 3;

        if(renderObject.isSkinned) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_normalsDebugSkinnedPipeline.pipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_normalsDebugSkinnedPipeline.layout,
                                    0, 1,
                                    &sceneDescriptorSet, 0,
                                    nullptr);
        } else {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_normalsDebugStaticPipeline.pipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_normalsDebugStaticPipeline.layout,
                                    0, 1,
                                    &sceneDescriptorSet, 0,
                                    nullptr);
        }

        VkViewport viewport = VktStructs::viewport(VktCache::drawExtent);
        VkRect2D scissors = VktStructs::scissors(VktCache::drawExtent);
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissors);

        vkCmdBindIndexBuffer(cmd, renderObject.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        if(renderObject.isSkinned) {
            VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Skinned> pushConstants;
            pushConstants.vertexBuffer = renderObject.vertexBufferAddress;
            pushConstants.jointsBuffer = renderObject.jointsBufferAddress;
            pushConstants.worldMatrix = renderObject.transform;
            vkCmdPushConstants(cmd, m_normalsDebugSkinnedPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0,
                               sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Skinned>), &pushConstants);
        } else {
            VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static> pushConstants;
            pushConstants.vertexBuffer = renderObject.vertexBufferAddress;
            pushConstants.worldMatrix = renderObject.transform;
            vkCmdPushConstants(cmd, m_normalsDebugStaticPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, 0,
                               sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>), &pushConstants);
        }

        vkCmdDrawIndexed(cmd, renderObject.indexCount, 1, renderObject.firstIndex, 0, 0);
    };

    for(const VktTypes::RenderObject &renderObject: m_mainDrawContext.opaqueSurfaces) {
        draw(renderObject);
    }

    for(const VktTypes::RenderObject &renderObject: m_mainDrawContext.transparentSurfaces) {
        draw(renderObject);
    }
}

void VktCore::initDescriptors() {
    std::vector<DescriptorAllocatorDynamic::PoolSizeRatio> sizes = {
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}};

    VktCache::descriptorAllocator.initPool(5, sizes);
    //VktCache::descriptorAllocator.initPool(m_device, 5, sizes);

    // Build draw image descriptor layout
    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        VktCache::storeLayout(VktCache::Layout::DRAW_IMAGE, builder.build(VK_SHADER_STAGE_COMPUTE_BIT));
    }

    // Allocate draw image
    m_drawImageDescriptors = VktCache::descriptorAllocator.allocate(VktCache::getLayout(VktCache::Layout::DRAW_IMAGE));

    // Write draw image
    {
        DescriptorWriter writer;
        writer.writeImage(0, m_drawImage.view,
                          VK_NULL_HANDLE,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(m_drawImageDescriptors);
    }

    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);        // SceneData
        builder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// IBLCube
        builder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// Specular cube
        builder.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);// BRDF texture
        VktCache::storeLayout(VktCache::Layout::SCENE, builder.build(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_GEOMETRY_BIT));
    }

    for(uint8_t i = 0; i < FRAMES_OVERLAP; i++) {
        std::vector<DescriptorAllocatorDynamic::PoolSizeRatio> frameSizes = {
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4}};

        m_frames[i].descriptors = DescriptorAllocatorDynamic{};
        m_frames[i].descriptors.initPool(1000, frameSizes);

        m_frames[i].sceneUniformBuffer = VktBuffers::create(sizeof(VktTypes::GPU::SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        m_coreDeletionQueue.pushDeletable(DeletableType::TEC_RESOURCE_BUFFER, &m_frames[i].sceneUniformBuffer);
        m_coreDeletionQueue.pushDeletable(DeletableType::TEC_DESCRIPTOR_ALLOCATOR_DYNAMIC, &m_frames[i].descriptors);
    }
}

void VktCore::initPipelines() {
    initMaterialPipelines();
    initGeometryPipeline();
    m_logger(Logger::INFO) << "Finished initializing graphics pipelines\n";
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

        VktCache::storeLayout(VktCache::Layout::MAT_METAL_ROUGHNESS, layoutBuilder.build(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
    }

    auto layouts = VktCache::getLayouts(VktCache::Layout::SCENE, VktCache::Layout::MAT_METAL_ROUGHNESS);

    // Create pipeline layout with provided descriptors and push constants
    VkPipelineLayoutCreateInfo meshLayoutInfo = VktStructs::pipelineLayoutCreateInfo(layouts, matrixRange);

    VkPipelineLayout staticLayout;
    VK_CHECK(vkCreatePipelineLayout(VktCache::vkDevice,
                                    &meshLayoutInfo,
                                    nullptr,
                                    &staticLayout))
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE_LAYOUT, staticLayout);

    meshLayoutInfo.pPushConstantRanges = &skinnedMatrixRange;

    VkPipelineLayout skinnedLayout;
    VK_CHECK(vkCreatePipelineLayout(VktCache::vkDevice,
                                    &meshLayoutInfo,
                                    nullptr,
                                    &skinnedLayout))
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE_LAYOUT, skinnedLayout);

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
    pipelineBuilder.setColorAttachmentFormat(m_drawImage.format);
    pipelineBuilder.setDepthFormat(m_depthImage.format);
    pipelineBuilder.setPipelineLayout(staticLayout);

    metalRoughMaterial.opaquePipeline.pipeline = pipelineBuilder.buildPipeline();
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, metalRoughMaterial.opaquePipeline.pipeline);

    // Switch from opaque to transparent
    pipelineBuilder.enableBlendingAdditive();
    pipelineBuilder.enableDepthTest(false, VK_COMPARE_OP_LESS);

    metalRoughMaterial.transparentPipeline.pipeline = pipelineBuilder.buildPipeline();
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, metalRoughMaterial.transparentPipeline.pipeline);

    // Switch from static vertex shaders to skinned with joint matrices
    pipelineBuilder.setVertexShader("shaders/mesh_skin.vert.spv");
    pipelineBuilder.setPipelineLayout(skinnedLayout);

    // Maintain the pipeline order
    pipelineBuilder.disableBlending();
    pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_LESS);

    metalRoughMaterial.skinnedOpaquePipeline.pipeline = pipelineBuilder.buildPipeline();
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, metalRoughMaterial.skinnedOpaquePipeline.pipeline);

    pipelineBuilder.enableBlendingAdditive();
    pipelineBuilder.enableDepthTest(false, VK_COMPARE_OP_LESS);

    metalRoughMaterial.skinnedTransparentPipeline.pipeline = pipelineBuilder.buildPipeline();
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, metalRoughMaterial.skinnedTransparentPipeline.pipeline);
}

void VktCore::initGeometryPipeline() {
    VkPushConstantRange matrixRange{};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Static>);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;

    VkPushConstantRange skinnedMatrixRange{};
    skinnedMatrixRange.offset = 0;
    skinnedMatrixRange.size = sizeof(VktTypes::GPU::DrawPushConstants<VktTypes::GPU::Skinned>);
    skinnedMatrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;

    const auto layouts = VktCache::getLayouts(VktCache::Layout::SCENE);

    // Create pipeline layout with provided descriptors and push constants
    VkPipelineLayoutCreateInfo meshLayoutInfo = VktStructs::pipelineLayoutCreateInfo(layouts, matrixRange);

    VkPipelineLayout staticLayout;
    VK_CHECK(vkCreatePipelineLayout(VktCache::vkDevice,
                                    &meshLayoutInfo,
                                    nullptr,
                                    &staticLayout))
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE_LAYOUT, staticLayout);

    meshLayoutInfo.pPushConstantRanges = &skinnedMatrixRange;

    VkPipelineLayout skinnedLayout;
    VK_CHECK(vkCreatePipelineLayout(VktCache::vkDevice,
                                    &meshLayoutInfo,
                                    nullptr,
                                    &skinnedLayout))
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE_LAYOUT, skinnedLayout);

    m_normalsDebugStaticPipeline.layout = staticLayout;
    m_normalsDebugSkinnedPipeline.layout = skinnedLayout;

    VktPipelineBuilder pipelineBuilder;
    pipelineBuilder.setVertexShader("shaders/debug/normal.vert.spv");
    pipelineBuilder.setFragmentShader("shaders/debug/normal.frag.spv");
    pipelineBuilder.setGeometryShader("shaders/debug/normal.geom.spv");
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.setMultisamplingNone();
    pipelineBuilder.disableBlending();
    pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
    pipelineBuilder.setColorAttachmentFormat(m_drawImage.format);
    pipelineBuilder.setDepthFormat(m_depthImage.format);
    pipelineBuilder.setPipelineLayout(staticLayout);

    m_normalsDebugStaticPipeline.pipeline = pipelineBuilder.buildPipeline();
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, m_normalsDebugStaticPipeline.pipeline);

    pipelineBuilder.setVertexShader("shaders/debug/normal_skin.vert.spv");
    pipelineBuilder.setGeometryShader("shaders/debug/normal_skin.geom.spv");
    pipelineBuilder.setPipelineLayout(skinnedLayout);

    m_normalsDebugSkinnedPipeline.pipeline = pipelineBuilder.buildPipeline();
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, m_normalsDebugSkinnedPipeline.pipeline);
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
    VK_CHECK(vkCreateDescriptorPool(VktCache::vkDevice, &poolInfo, nullptr, &imguiPool))

    ImGui::CreateContext();
    m_window->initImGuiVulkan();

    // Enable keyboard controls
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = VktCache::vkInstance;
    initInfo.PhysicalDevice = m_physicalDevice;
    initInfo.Device = VktCache::vkDevice;
    initInfo.Queue = m_graphicsQueue;
    initInfo.QueueFamily = m_graphicsQueueFamily;
    initInfo.DescriptorPool = imguiPool;
    initInfo.MinImageCount = 3;
    initInfo.ImageCount = 3;
    initInfo.UseDynamicRendering = true;
    initInfo.ColorAttachmentFormat = m_swapchainImageFormat;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo, VK_NULL_HANDLE);
    ImGui_ImplVulkan_CreateFontsTexture();

    m_coreDeletionQueue.pushDeletable(DeletableType::VK_DESCRIPTOR_POOL, imguiPool);
}

void VktCore::drawImGui(VkCommandBuffer cmd, VkImageView targetView) {
    VkRenderingAttachmentInfo colorAttachment = VktStructs::attachmentInfo(targetView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingInfo renderInfo = VktStructs::renderingInfo(m_swapchainExtent, &colorAttachment, nullptr);

    vkCmdBeginRendering(cmd, &renderInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRendering(cmd);
}

bool VktCore::shouldClose() {
    return m_window->shouldClose();
}

void VktCore::run() {

    auto startTime = std::chrono::system_clock::now();

    if(m_resizeSwapchain) {
        resizeSwapchain();
    }

    runImGui();

    draw();

    auto endTime = std::chrono::system_clock::now();
    auto drawDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    m_stats.frametime = static_cast<float>(drawDuration.count()) / 1000.0f;
}

void VktCore::runImGui() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if(ImGui::Begin("Scene")) {
        ImGui::InputFloat3("Ambient color: ", (float *) &m_sceneData.ambientColor);
        ImGui::InputFloat3("Sunlight direction: ", (float *) &m_sceneData.sunlightDirection);
        ImGui::InputFloat3("Sunlight color: ", (float *) &m_sceneData.sunlightColor);
        ImGui::InputFloat3("Camera position: ", (float *) &m_sceneData.cameraPosition);
        ImGui::InputFloat3("Camera direction: ", (float *) &m_sceneData.cameraDirection);
        ImGui::InputFloat("Time: ", &m_sceneData.time);
        ImGui::End();
    }

    if(ImGui::Begin("Performance")) {
        uint32_t frames = 1000 / m_stats.frametime;
        ImGui::Text("Frames %u", frames);
        ImGui::Text("Frametime %f ms", m_stats.frametime);
        ImGui::Text("Drawtime %f ms", m_stats.meshDrawTime);
        ImGui::Text("Update time %f ms", m_stats.sceneUpdateTime);
        ImGui::Text("Triangles %u", m_stats.trigDrawCount);
        ImGui::Text("Draws: %u", m_stats.drawCallCount);
        ImGui::End();
    }

    if(ImGui::Begin("Models")) {
        for(auto &[mID, object]: loadedObjects) {
            if(ImGui::TreeNode(object.name.c_str())) {
                ImGui::Text("ID: %u", mID);
                ImGui::Text("Name: %s", object.name.c_str());
                if(ImGui::TreeNode("Transformation")) {

                    // Change position
                    glm::vec3 pos = object.model.transformation.getTranslation();
                    if(ImGui::DragFloat3("Pos", (float *) &(pos))) {
                        object.model.transformation.setTranslation(pos.x, pos.y, pos.z);
                    }

                    // Change rotation
                    glm::vec3 rotation = object.model.transformation.getRotation();
                    if(ImGui::DragFloat3("Rotation", (float *) &(rotation))) {
                        object.model.transformation.setRotation(rotation.x, rotation.y, rotation.z);
                    }

                    // Change scale
                    float scale = object.model.transformation.getScale();
                    if(ImGui::DragFloat("Scale", &scale)) {
                        object.model.transformation.setScale(scale);
                    }
                    ImGui::TreePop();
                }

                if(object.model.isSkinned() && ImGui::TreeNode("Animation")) {
                    static std::size_t currentAnimation = object.model.currentAnimation();
                    if(ImGui::BeginListBox("##animation_list", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing()))) {
                        uint32_t animCount = object.model.animationCount();
                        for(std::size_t animID = 0; animID < animCount; animID++) {
                            const bool isActive = (currentAnimation == animID);
                            if(ImGui::Selectable(object.model.animationName(animID).data(), isActive)) {
                                object.model.setAnimation(animID);
                                currentAnimation = animID;
                            }
                            if(isActive) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndListBox();
                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
        ImGui::End();
    }

    if(ImGui::Begin("Debug")) {
        ImGui::Checkbox("Debug pipeline", &m_debugConf.enableDebugPipeline);

        ImGui::End();
    }

    ImGui::Render();
}

template VktTypes::GPU::MeshBuffers VktCore::uploadMesh<VktTypes::GPU::Skinned>(std::span<uint32_t> indices, std::span<VktTypes::GPU::Vertex<VktTypes::GPU::Skinned>> vertices);
template VktTypes::GPU::MeshBuffers VktCore::uploadMesh<VktTypes::GPU::Static>(std::span<uint32_t> indices, std::span<VktTypes::GPU::Vertex<VktTypes::GPU::Static>> vertices);

template<bool S>
VktTypes::GPU::MeshBuffers VktCore::uploadMesh(std::span<uint32_t> indices, std::span<VktTypes::GPU::Vertex<S>> vertices) {
    const size_t vertexBufferSize = vertices.size() * sizeof(VktTypes::GPU::Vertex<S>);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    VktTypes::GPU::MeshBuffers newSurface{};
    newSurface.vertexBuffer = VktBuffers::create(vertexBufferSize,
                                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                 VMA_MEMORY_USAGE_GPU_ONLY);

    VkBufferDeviceAddressInfo deviceAddressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .pNext = nullptr};
    deviceAddressInfo.buffer = newSurface.vertexBuffer.buffer;
    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(VktCache::vkDevice, &deviceAddressInfo);

    newSurface.indexBuffer = VktBuffers::create(indexBufferSize,
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

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

        VkBufferCopy indexCopy{};
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.dstOffset = 0;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
    });
    VktBuffers::destroy(staging);

    return newSurface;
}


VktTypes::GPU::JointsBuffers VktCore::uploadJoints(const std::span<glm::mat4> &jointMatrices) {
    const size_t jointsBufferSize = jointMatrices.size() * sizeof(glm::mat4);

    VktTypes::GPU::JointsBuffers newJoints{};
    newJoints.jointsBuffer = VktBuffers::create(jointsBufferSize,
                                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                VMA_MEMORY_USAGE_CPU_TO_GPU);

    VkBufferDeviceAddressInfo deviceAddressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .pNext = nullptr};
    deviceAddressInfo.buffer = newJoints.jointsBuffer.buffer;
    newJoints.jointsBufferAddress = vkGetBufferDeviceAddress(VktCache::vkDevice, &deviceAddressInfo);

    memcpy(newJoints.jointsBuffer.info.pMappedData, jointMatrices.data(), jointsBufferSize);

    return newJoints;
}

void VktCore::initDefaultData() {
    std::array<u_char, 4> white = {0xFF,0xFF,0xFF,0xFF};
    std::array<u_char, 4> black = {0x00,0x00,0x00,0x00};
    std::array<u_char, 4> gray = {0xAA,0xAA,0xAA,0xFF};

    m_whiteImage = VktImages::createDeviceMemory(VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT).value();
    m_blackImage = VktImages::createDeviceMemory(VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT).value();
    m_greyImage = VktImages::createDeviceMemory(VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT).value();
    VktImages::copyFromRaw(m_whiteImage, 4, reinterpret_cast<char*>(white.data()));
    VktImages::copyFromRaw(m_blackImage, 4, reinterpret_cast<char*>(black.data()));
    VktImages::copyFromRaw(m_greyImage, 4, reinterpret_cast<char*>(gray.data()));

    uint32_t pink = 0xFF00DC;
    std::array<uint32_t, 16 * 16> pixels{};
    for(uint32_t x = 0; x < 16; x++) {
        for(uint32_t y = 0; y < 16; y++) {
            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? pink : 0;
        }
    }
    m_errorCheckboardImage = VktImages::createDeviceMemory(VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT).value();
    VktImages::copyFromRaw(m_errorCheckboardImage, 16*16*4, reinterpret_cast<char*>(pixels.data()));

    VkSamplerCreateInfo samplerInfo{.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};

    VkSampler tmpSamplerHandle;

    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(VktCache::vkDevice, &samplerInfo, nullptr, &tmpSamplerHandle);
    VktCache::storeSampler(VktCache::Sampler::NEAREST, tmpSamplerHandle);
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_SAMPLER, tmpSamplerHandle);

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(VktCache::vkDevice, &samplerInfo, nullptr, &tmpSamplerHandle);
    VktCache::storeSampler(VktCache::Sampler::LINEAR, tmpSamplerHandle);
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_SAMPLER, tmpSamplerHandle);

    std::string tmp_cubemapDir = "terrain/skyboxtex/sea-panorama";
    m_skybox.load(tmp_cubemapDir.c_str());
    m_skybox.writeColorSet();
}

VkBool32 VktCore::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
                                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
    auto severityStr = vkb::to_string_message_severity(severity);
    auto typeStr = vkb::to_string_message_type(type);
    fprintf(stderr, "[%s: %s] %s\n", severityStr, typeStr, pCallbackData->pMessage);
    return VK_FALSE;
}

void VktCore::resizeSwapchain() {
    vkDeviceWaitIdle(VktCache::vkDevice);
    destroySwapchain();

    Utils::WindowDimension dimensions = m_window->getSize();
    m_windowExtent.width = dimensions.width;
    m_windowExtent.height = dimensions.height;

    createSwapchain(m_windowExtent.width, m_windowExtent.height);
    m_resizeSwapchain = false;
}

void VktCore::setWindow(Window *window) {
    assert(window != nullptr);
    m_window = window;

    Utils::WindowDimension winDimensions = m_window->getSize();
    m_windowExtent.width = winDimensions.width;
    m_windowExtent.height = winDimensions.height;
}

void VktCore::updateScene() {
    double currTime = glfwGetTime();
    static double prevTime = currTime;

    m_mainDrawContext.opaqueSurfaces.clear();
    m_sceneData.view = m_viewMatrix;
    m_sceneData.proj = m_projMatrix;

    m_sceneData.proj[1][1] *= -1;
    m_sceneData.viewproj = m_sceneData.proj * m_sceneData.view;
    m_sceneData.ambientColor = glm::vec3(0.1f);
    m_sceneData.sunlightColor = glm::vec3(1.0f);
    glm::vec4 sunPos = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) * glm::rotate(glm::identity<glm::mat4>(), static_cast<float>(glm::radians(currTime * 50.f)), glm::vec3(0.0f, 1.0f, 0.0f));
    m_sceneData.sunlightDirection = glm::vec3(sunPos - glm::vec4(0.0f));
    m_sceneData.cameraPosition = cameraPosition;
    m_sceneData.cameraDirection = cameraDirection;
    m_sceneData.time = currTime;

    currTime = glfwGetTime();
    double delta = currTime - prevTime;
    prevTime = currTime;
    for(auto &[oID, object]: loadedObjects) {
        if(object.model.isSkinned() && object.model.currentAnimation() != ModelTypes::NULL_ID) {
            object.model.updateAnimationTime(static_cast<float>(delta));
            object.model.updateJoints();
        }
        object.model.gatherDrawContext(m_mainDrawContext);
    }
}

void VktCore::setViewMatrix(const glm::mat4 &viewMatrix) {
    m_viewMatrix = viewMatrix;
}

void VktCore::setProjMatrix(const glm::mat4 &projMatrix) {
    m_projMatrix = projMatrix;
}

/**
 * Initialize first object identifiers to 0.
 */
VktCore::objectID_t VktCore::EngineObject::lastID = 0;

VktCore::EngineObject *VktCore::createObject(const std::filesystem::path &filePath, const std::string &name) {
    // Find free identifier
    while(loadedObjects.contains(EngineObject::lastID)) EngineObject::lastID++;
    objectID_t freeID = EngineObject::lastID++;

    // Create object
    loadedObjects[freeID] = EngineObject{
            .objectID = freeID,
            .name = name,
            .model = Model(filePath),
    };

    // Upload default position
    if(loadedObjects[freeID].model.isSkinned()) {
        loadedObjects[freeID].model.updateJoints();
    }

    // Return handle
    m_logger(Logger::INFO) << "Created an object " << name << " with ID " << freeID << '\n';
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

    matData.materialSet = descriptorAllocator.allocate(VktCache::getLayout(VktCache::Layout::MAT_METAL_ROUGHNESS));

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