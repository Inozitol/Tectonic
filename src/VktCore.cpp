
#include "engine/vulkan/VktCore.h"
#define VMA_IMPLEMENTATION
#include "extern/vma/vk_mem_alloc.h"

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
    initDefaultData();

    m_isInitialized = true;
}

void VktCore::clean() {
    if(m_isInitialized){
        vkDeviceWaitIdle(m_device);

        for(auto& [k,object] : loadedObjects){
            for(auto& buffer : object.skins){
                destroyBuffer(buffer.jointsBuffer);
            }
        }

        for(auto& [k,model] : loadedModels){
            delete(model.resources);
        }

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_coreDeletionQueue.flush();
        m_globDescriptorAllocator.destroyPool(m_device);

        for(uint8_t i = 0; i < FRAMES_OVERLAP; i++){
            vkDestroyCommandPool(m_device, m_frames[i].commandPool, nullptr);
            vkDestroyFence(m_device, m_frames[i].renderFence, nullptr);
            vkDestroySemaphore(m_device, m_frames[i].renderSemaphore, nullptr);
            vkDestroySemaphore(m_device, m_frames[i].swapchainSemaphore, nullptr);
        }

        destroySwapchain();

        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyDevice(m_device, nullptr);

        vkDestroyInstance(m_instance, nullptr);

        m_isInitialized = false;
    }
}

VktCore::~VktCore(){
    clean();
}

void VktCore::initVulkan() {
    vkb::InstanceBuilder vkBuilder;
    auto instance = vkBuilder.set_app_name("Tectonic")
            .request_validation_layers(true)
            .set_debug_callback(VktCore::debugCallback)
            .require_api_version(1,3,0)
            .build();

    m_logger(Logger::INFO) << "Finished building vkbInstance\n";

    vkb::Instance vkbInstance = instance.value();

    m_instance = vkbInstance.instance;
    m_surface = m_window->createWindowSurface(m_instance);

    VkPhysicalDeviceVulkan13Features features13{};

    m_logger(Logger::DEBUG) << "Enabling dynamicRendering feature\n";
    features13.dynamicRendering = true;

    m_logger(Logger::DEBUG) << "Enabling synchronization2 feature\n";
    features13.synchronization2 = true;

    m_logger(Logger::DEBUG) << "Enabling maintenance4 feature\n";
    features13.maintenance4 = true;     // TODO remove this, used to keep performance warning quiet


    VkPhysicalDeviceVulkan12Features features12{};

    m_logger(Logger::DEBUG) << "Enabling bufferDeviceAddress feature\n";
    features12.bufferDeviceAddress = true;

    m_logger(Logger::DEBUG) << "Enabling descriptorIndexing feature\n";
    features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector vkbSelector{vkbInstance};
    vkb::PhysicalDevice vkbPhysicalDevice = vkbSelector
            .set_minimum_version(1,3)
            .set_required_features_13(features13)
            .set_required_features_12(features12)
            .set_surface(m_surface)
            .select()
            .value();

    vkb::DeviceBuilder vkbDeviceBuilder{vkbPhysicalDevice};
    vkb::Device vkbDevice = vkbDeviceBuilder.build().value();

    m_logger(Logger::INFO) << "Found physical device: " << vkbDevice.physical_device.name << '\n';

    m_device = vkbDevice.device;
    m_physicalDevice = vkbPhysicalDevice.physical_device;

    m_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    m_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = m_device;
    allocatorInfo.instance = m_instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &m_vmaAllocator);

    m_logger(Logger::INFO) << "Finished creating vmaAllocator\n";

    m_coreDeletionQueue.setInstance(m_instance);
    m_coreDeletionQueue.setDevice(m_device);
    m_coreDeletionQueue.setVmaAllocator(m_vmaAllocator);
    for(uint8_t i = 0; i < FRAMES_OVERLAP; i++) {
        m_frames[i].deletionQueue.setInstance(m_instance);
        m_frames[i].deletionQueue.setDevice(m_device);
        m_frames[i].deletionQueue.setVmaAllocator(m_vmaAllocator);
    }

    m_coreDeletionQueue.pushDeletable(DeletableType::VMA_ALLOCATOR, m_vmaAllocator);
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_DEBUG_UTILS_MESSENGER, vkbInstance.debug_messenger);

    m_logger(Logger::INFO) << "Finished initializing Vulkan device\n";
}

void VktCore::initSwapchain() {
    // TODO better way for this. Grab size from monitor size or something larger than a m_window size.
    createSwapchain(m_windowExtent.width, m_windowExtent.height);

    VkExtent3D drawImageExtent = {
            m_windowExtent.width,
            m_windowExtent.height,
            1
    };

    m_drawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    m_drawImage.extent = drawImageExtent;

    VkImageUsageFlags drawImageFlags{};
    drawImageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      VK_IMAGE_USAGE_STORAGE_BIT |
                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimgInfo = VktStructs::imageCreateInfo(m_drawImage.format, drawImageFlags, drawImageExtent);

    VmaAllocationCreateInfo rImgAllocInfo{};
    rImgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    rImgAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vmaCreateImage(m_vmaAllocator, &rimgInfo, &rImgAllocInfo, &m_drawImage.image, &m_drawImage.allocation, nullptr);
    m_coreDeletionQueue.pushDeletable(DeletableType::VMA_IMAGE, m_drawImage.image, {m_drawImage.allocation});

    VkImageViewCreateInfo rviewInfo = VktStructs::imageViewCreateInfo(m_drawImage.format, m_drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(m_device, &rviewInfo, nullptr, &m_drawImage.view));
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_IMAGE_VIEW, m_drawImage.view);

    m_logger(Logger::INFO) << "Finished creating draw image with format: "
                                << string_VkFormat(VK_FORMAT_R16G16B16A16_SFLOAT)
                                << " and extent dimensions: "
                                << drawImageExtent.width << 'x' << drawImageExtent.height << '\n';

    m_depthImage.format = VK_FORMAT_D32_SFLOAT;
    m_depthImage.extent = drawImageExtent;
    VkImageUsageFlags depthImageUsages{};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageCreateInfo dImgInfo = VktStructs::imageCreateInfo(m_depthImage.format, depthImageUsages, m_depthImage.extent);
    vmaCreateImage(m_vmaAllocator, &dImgInfo, &rImgAllocInfo, &m_depthImage.image, &m_depthImage.allocation, nullptr);
    m_coreDeletionQueue.pushDeletable(DeletableType::VMA_IMAGE, m_depthImage.image, {m_depthImage.allocation});

    VkImageViewCreateInfo dViewInfo = VktStructs::imageViewCreateInfo(m_depthImage.format, m_depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);
    VK_CHECK(vkCreateImageView(m_device, &dViewInfo, nullptr, &m_depthImage.view));
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_IMAGE_VIEW, m_depthImage.view);

    m_logger(Logger::INFO) << "Finished creating depth image with format: "
                           << string_VkFormat(VK_FORMAT_D32_SFLOAT)
                           << " and extent dimensions: "
                           << drawImageExtent.width << 'x' << drawImageExtent.height << '\n';

}

void VktCore::initCommands() {
    VkCommandPoolCreateInfo commandPoolCreateInfo = VktStructs::commandPoolCreateInfo(m_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for(uint8_t i = 0; i < FRAMES_OVERLAP; i++){
        VK_CHECK(vkCreateCommandPool(m_device,
                                     &commandPoolCreateInfo,
                                     nullptr,
                                     &m_frames[i].commandPool));

        VkCommandBufferAllocateInfo cmdAllocInfo = VktStructs::commandBufferAllocateInfo(m_frames[i].commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(m_device,
                                          &cmdAllocInfo,
                                          &m_frames[i].mainCommandBuffer));
    }

    // Create command pool and buffer for immediate commands
    VK_CHECK(vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_immCommandPool));
    VkCommandBufferAllocateInfo cmdAllocInfo = VktStructs::commandBufferAllocateInfo(m_immCommandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_immCommandBuffer));
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_COMMAND_POOL, m_immCommandPool);
}

void VktCore::initSyncStructs() {
    VkFenceCreateInfo fenceCreateInfo = VktStructs::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = VktStructs::semaphoreCreateInfo();

    for(uint8_t i = 0; i < FRAMES_OVERLAP; i++){
        VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_frames[i].renderFence));
        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].renderSemaphore));
    }

    VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_immFence));

    m_coreDeletionQueue.pushDeletable(DeletableType::VK_FENCE, m_immFence);
}

void VktCore::createSwapchain(uint32_t width, uint32_t height) {
    m_logger(Logger::INFO) << "Creating swapchain of dimensions: " << width << 'x' << height << '\n';

    vkb::SwapchainBuilder vkbSwapchainBuilder(m_physicalDevice, m_device, m_surface);
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
    m_swapchainImageFormat = vkbSwapchain.image_format;     // Builder might choose different format
    m_logger(Logger::INFO) << "Finished building swapchain with format: " << string_VkFormat(vkbSwapchain.image_format) << '\n';

}

void VktCore::destroySwapchain() {
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    for(auto& imageView : m_swapchainImageViews){
        vkDestroyImageView(m_device, imageView, nullptr);
    }
}

VktTypes::FrameData &VktCore::getCurrentFrame() {
    return m_frames[m_frameNumber % FRAMES_OVERLAP];
}

void VktCore::draw() {
    // Updates relevant scene rendering data buffers
    updateScene();

    VK_CHECK(vkWaitForFences(m_device, 1, &getCurrentFrame().renderFence, true, 1000000000));
    VK_CHECK(vkResetFences(m_device, 1, &getCurrentFrame().renderFence));
    getCurrentFrame().deletionQueue.flush();
    getCurrentFrame().descriptors.clearPools(m_device);

    uint32_t swapchainIndex;
    VkResult error = vkAcquireNextImageKHR(m_device,
                                            m_swapchain,
                                            1000000000,
                                            getCurrentFrame().swapchainSemaphore,
                                            nullptr,
                                            &swapchainIndex);
    if(error == VK_ERROR_OUT_OF_DATE_KHR){
        m_resizeSwapchain = true;
        return;
    }else if(error){
        throw vulkanException("Failed to acquire next image from swapchain, error code ", string_VkResult(error));
    }

    VkCommandBuffer cmd = getCurrentFrame().mainCommandBuffer;

    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    VkCommandBufferBeginInfo cmdBeginInfo = VktStructs::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    m_drawExtent.width  = static_cast<uint32_t>(static_cast<float>(std::min(m_drawImage.extent.width, m_swapchainExtent.width)) * m_renderScale);
    m_drawExtent.height = static_cast<uint32_t>(static_cast<float>(std::min(m_drawImage.extent.height, m_swapchainExtent.height)) * m_renderScale);

    // Transition main image into general layout
    VktUtils::transitionImage(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // Draw compute background
    drawBackground(cmd);

    // Transition depth image into optimal layout
    VktUtils::transitionImage(cmd, m_depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    drawGeometry(cmd);

    // Transition draw image and swap chain into correct transfer layouts
    VktUtils::transitionImage(cmd, m_drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VktUtils::transitionImage(cmd, m_swapchainImages[swapchainIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy main image into swapchain
    VktUtils::copyImgToImg(cmd, m_drawImage.image, m_swapchainImages[swapchainIndex], m_drawExtent, m_swapchainExtent);

    VktUtils::transitionImage(cmd, m_swapchainImages[swapchainIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Draw ImGui
    drawImGui(cmd, m_swapchainImageViews[swapchainIndex]);

    // Transition swapchain image to present layout
    VktUtils::transitionImage(cmd, m_swapchainImages[swapchainIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdInfo = VktStructs::commandBufferSubmitInfo(cmd);

    VkSemaphoreSubmitInfo waitInfo = VktStructs::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame().swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = VktStructs::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().renderSemaphore);

    VkSubmitInfo2 submit = VktStructs::submitInfo(&cmdInfo, &signalInfo, &waitInfo);

    VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submit, getCurrentFrame().renderFence));

    VkPresentInfoKHR presentInfo{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, .pNext = nullptr};
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainIndex;

    error = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
    if(error == VK_ERROR_OUT_OF_DATE_KHR){
        m_resizeSwapchain = true;
    }else if(error){
        throw vulkanException("Failed to queue swapchain image for present, error code ", string_VkResult(error));
    }

    m_frameNumber++;
}

void VktCore::drawBackground(VkCommandBuffer cmd) {
    VktTypes::ComputeEffect& effect = m_backgroundEffect.at(m_currentBackgroundEffect);

    // Draw gradient with compute shader
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_gradientPipelineLayout, 0, 1, &m_drawImageDescriptors, 0,nullptr);

    // Push constants
    vkCmdPushConstants(cmd, m_gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(VktTypes::ComputePushConstants), &effect.data);

    vkCmdDispatch(cmd, std::ceil(m_drawExtent.width/16.0), std::ceil(m_drawExtent.height/16.0),1);
}

void VktCore::drawGeometry(VkCommandBuffer cmd) {

    m_stats.drawCallCount = 0;
    m_stats.trigDrawCount = 0;
    auto startTime = std::chrono::system_clock::now();

    // Fetch scene uniform buffer allocated memory
    VktTypes::AllocatedBuffer& gpuSceneDataBuffer = getCurrentFrame().sceneUniformBuffer;

    // Copy scene data to buffer
    memcpy(reinterpret_cast<VktTypes::GPUSceneData*>(gpuSceneDataBuffer.info.pMappedData), &m_sceneData, sizeof(VktTypes::GPUSceneData));

    VkDescriptorSet globalDescriptor = getCurrentFrame().descriptors.allocate(m_device, m_gpuSceneDataDescriptorLayout);

    // Write scene data to uniforms
    {
        DescriptorWriter writer;
        writer.writeBuffer(0, gpuSceneDataBuffer.buffer,
                           sizeof(VktTypes::GPUSceneData),
                           0,
                           VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.updateSet(m_device, globalDescriptor);
    }

    VkRenderingAttachmentInfo colorAttachment   = VktStructs::attachmentInfo(m_drawImage.view, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingAttachmentInfo depthAttachment   = VktStructs::depthAttachmentInfo(m_depthImage.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderingInfo               = VktStructs::renderingInfo(m_drawExtent, &colorAttachment, &depthAttachment);
    vkCmdBeginRendering(cmd, &renderingInfo);

    auto draw = [&](const VktTypes::RenderObject& renderObject){
        m_stats.drawCallCount++;
        m_stats.trigDrawCount += renderObject.indexCount / 3;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderObject.material->pipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                renderObject.material->pipeline->layout,
                                0, 1,
                                &globalDescriptor, 0,
                                nullptr);

        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = static_cast<float>(m_drawExtent.width);
        viewport.height = static_cast<float>(m_drawExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissors{};
        scissors.offset.x = 0;
        scissors.offset.y = 0;
        scissors.extent.width = m_drawExtent.width;
        scissors.extent.height = m_drawExtent.height;

        vkCmdSetScissor(cmd, 0, 1, &scissors);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                renderObject.material->pipeline->layout,
                                1, 1,
                                &renderObject.material->materialSet, 0,
                                nullptr);

        vkCmdBindIndexBuffer(cmd, renderObject.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        VktTypes::GPUDrawPushConstants pushConstants;
        pushConstants.vertexBuffer = renderObject.vertexBufferAddress;
        pushConstants.jointsBuffer = renderObject.jointsBufferAddress;
        pushConstants.worldMatrix = renderObject.transform;
        vkCmdPushConstants(cmd, renderObject.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VktTypes::GPUDrawPushConstants), &pushConstants);

        vkCmdDrawIndexed(cmd, renderObject.indexCount, 1, renderObject.firstIndex, 0, 0);
    };

    for(const VktTypes::RenderObject& renderObject : m_mainDrawContext.opaqueSurfaces){
        draw(renderObject);
    }

    for(const VktTypes::RenderObject& renderObject : m_mainDrawContext.transparentSurfaces){
        draw(renderObject);
    }

    m_mainDrawContext.opaqueSurfaces.clear();
    m_mainDrawContext.transparentSurfaces.clear();

    vkCmdEndRendering(cmd);

    auto endTime = std::chrono::system_clock::now();
    auto drawDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    m_stats.meshDrawTime = static_cast<float>(drawDuration.count()) / 1000.0f;

}

void VktCore::initDescriptors() {
    std::vector<DescriptorAllocatorDynamic::PoolSizeRatio> sizes = {
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3}
    };

    m_globDescriptorAllocator.initPool(m_device, 10, sizes);

    // Build draw image descriptor layout
    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        m_drawImageDescriptorLayout = builder.build(m_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    // Allocate draw image
    m_drawImageDescriptors = m_globDescriptorAllocator.allocate(m_device, m_drawImageDescriptorLayout);

    // Write draw image
    {
        DescriptorWriter writer;
        writer.writeImage(0, m_drawImage.view,
                          VK_NULL_HANDLE,
                          VK_IMAGE_LAYOUT_GENERAL,
                          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(m_device, m_drawImageDescriptors);
    }

    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        m_gpuSceneDataDescriptorLayout = builder.build(m_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        m_singleImageDescriptorLayout = builder.build(m_device, VK_SHADER_STAGE_FRAGMENT_BIT);
    }


    for(uint8_t i = 0; i < FRAMES_OVERLAP; i++){
        std::vector<DescriptorAllocatorDynamic::PoolSizeRatio> frameSizes = {
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4}
        };

        m_frames[i].descriptors = DescriptorAllocatorDynamic{};
        m_frames[i].descriptors.initPool(m_device, 1000, frameSizes);

        m_frames[i].sceneUniformBuffer = createBuffer(sizeof(VktTypes::GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        VktTypes::GPUSceneData* sceneUniformData = reinterpret_cast<VktTypes::GPUSceneData*>(m_frames[i].sceneUniformBuffer.info.pMappedData);
        *sceneUniformData = m_sceneData;

        m_coreDeletionQueue.pushDeletable(DeletableType::VMA_BUFFER, m_frames[i].sceneUniformBuffer.buffer, {m_frames[i].sceneUniformBuffer.allocation});
        m_coreDeletionQueue.pushDeletable(DeletableType::TEC_DESCRIPTOR_ALLOCATOR_DYNAMIC, &m_frames[i].descriptors);
    }

    //m_coreDeletionQueue.pushDeletable(DeletableType::VK_DESCRIPTOR_POOL, m_globDescriptorAllocator.pool);
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_DESCRIPTOR_SET_LAYOUT, m_singleImageDescriptorLayout);
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_DESCRIPTOR_SET_LAYOUT, m_gpuSceneDataDescriptorLayout);
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_DESCRIPTOR_SET_LAYOUT, m_drawImageDescriptorLayout);
}

void VktCore::initPipelines() {
    initBackgroundPipelines();
    //initMeshPipeline();
    m_metalRoughMaterial.buildPipelines(this);
}

void VktCore::initBackgroundPipelines() {
    VkPipelineLayoutCreateInfo computeLayout{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .pNext = nullptr };
    computeLayout.pSetLayouts = &m_drawImageDescriptorLayout;
    computeLayout.setLayoutCount = 1;

    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(VktTypes::ComputePushConstants);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    computeLayout.pPushConstantRanges = &pushConstant;
    computeLayout.pushConstantRangeCount = 1;

    VK_CHECK(vkCreatePipelineLayout(m_device, &computeLayout, nullptr, &m_gradientPipelineLayout));
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE_LAYOUT, m_gradientPipelineLayout);

    VkShaderModule gradientShader = VktUtils::loadShaderModule("shaders/gradient_color.comp.spv", m_device);
    VkShaderModule skyShader = VktUtils::loadShaderModule("shaders/sky.comp.spv", m_device);
    VkShaderModule starNestShader = VktUtils::loadShaderModule("shaders/star_next.comp.spv", m_device);
    VkShaderModule oceanShader = VktUtils::loadShaderModule("shaders/ocean.comp.spv", m_device);

    VkPipelineShaderStageCreateInfo stageInfo = VktStructs::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, gradientShader);

    VkComputePipelineCreateInfo computePipelineCreateInfo{ .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, .pNext = nullptr };
    computePipelineCreateInfo.layout = m_gradientPipelineLayout;
    computePipelineCreateInfo.stage = stageInfo;

    VktTypes::ComputeEffect gradient{};
    gradient.layout = m_gradientPipelineLayout;
    gradient.name = "gradient";
    gradient.data = {};

    gradient.data.data1 = glm::vec4(1,0,0,1);
    gradient.data.data2 = glm::vec4(0,0,1,1);

    VK_CHECK(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.pipeline));

    computePipelineCreateInfo.stage.module = skyShader;

    VktTypes::ComputeEffect sky{};
    sky.layout = m_gradientPipelineLayout;
    sky.name = "sky";
    sky.data = {};

    sky.data.data1 = glm::vec4(0.1,0.2,0.4,0.97);

    VK_CHECK(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.pipeline));

    computePipelineCreateInfo.stage.module = starNestShader;

    VktTypes::ComputeEffect star{};
    star.layout = m_gradientPipelineLayout;
    star.name = "star";
    star.data = {};

    star.data.data1[0] = 0.0;

    VK_CHECK(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &star.pipeline));

    computePipelineCreateInfo.stage.module = oceanShader;

    VktTypes::ComputeEffect ocean{};
    ocean.layout = m_gradientPipelineLayout;
    ocean.name = "ocean";
    ocean.data = {};

    ocean.data.data1[0] = 0.0;

    VK_CHECK(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &ocean.pipeline));

    m_backgroundEffect.push_back(gradient);
    m_backgroundEffect.push_back(sky);
    m_backgroundEffect.push_back(star);
    m_backgroundEffect.push_back(ocean);

    vkDestroyShaderModule(m_device, gradientShader, nullptr);
    vkDestroyShaderModule(m_device, skyShader, nullptr);
    vkDestroyShaderModule(m_device, starNestShader, nullptr);
    vkDestroyShaderModule(m_device, oceanShader, nullptr);

    m_currentBackgroundEffect = 2;

    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, ocean.pipeline);
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, star.pipeline);
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, gradient.pipeline);
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, sky.pipeline);
}

void VktCore::immediateSubmit(std::function<void(VkCommandBuffer)> &&func) {
    VK_CHECK(vkResetFences(m_device, 1, &m_immFence));
    VK_CHECK(vkResetCommandBuffer(m_immCommandBuffer, 0));

    VkCommandBuffer cmd = m_immCommandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = VktStructs::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    func(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdInfo = VktStructs::commandBufferSubmitInfo(cmd);
    VkSubmitInfo2 submitInfo = VktStructs::submitInfo(&cmdInfo, nullptr, nullptr);

    VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submitInfo, m_immFence));
    VK_CHECK(vkWaitForFences(m_device, 1, &m_immFence, true, 9999999999));
}

void VktCore::initImGui() {

    // Overkill pool resources but only for dev so whatever
    VkDescriptorPoolSize poolSizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                                         {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                                         { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                                         { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                                         { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                                         { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                                         { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };
    VkDescriptorPoolCreateInfo poolInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .pNext = nullptr };
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkDescriptorPool imguiPool;
    VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &imguiPool));

    ImGui::CreateContext();
    m_window->initImGuiVulkan();

    // Enable keyboard controls
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = m_instance;
    initInfo.PhysicalDevice = m_physicalDevice;
    initInfo.Device = m_device;
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

    if(m_resizeSwapchain){
        resizeSwapchain();
    }

    runImGui();

    double currentTime = glfwGetTime();
    m_backgroundEffect.at(m_currentBackgroundEffect).data.data2[0] = static_cast<float>(m_drawExtent.width);
    m_backgroundEffect.at(m_currentBackgroundEffect).data.data2[1] = static_cast<float>(m_drawExtent.height);

    if(m_currentBackgroundEffect == 2 || m_currentBackgroundEffect == 3){
        m_backgroundEffect.at(m_currentBackgroundEffect).data.data1[0] = currentTime;
    }

    draw();

    auto endTime = std::chrono::system_clock::now();
    auto drawDuration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    m_stats.frametime = static_cast<float>(drawDuration.count()) / 1000.0f;
}

void VktCore::runImGui(){
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if(ImGui::Begin("Background")){
        VktTypes::ComputeEffect& selected = m_backgroundEffect.at(m_currentBackgroundEffect);

        ImGui::SliderFloat("Render scale", &m_renderScale, 0.3f, 1.0f);

        ImGui::Text("Selected effect: ", selected.name);

        ImGui::SliderInt("Effect index: ", &m_currentBackgroundEffect, 0, m_backgroundEffect.size()-1);

        ImGui::InputFloat4("data1", (float*)& selected.data.data1);
        ImGui::InputFloat4("data2", (float*)& selected.data.data2);
        ImGui::InputFloat4("data3", (float*)& selected.data.data3);
        ImGui::InputFloat4("data4", (float*)& selected.data.data4);

        ImGui::End();
    }

    if(ImGui::Begin("Performance")){
        uint32_t frames = 1000 / m_stats.frametime;
        ImGui::Text("Frames %u", frames);
        ImGui::Text("Frametime %f ms", m_stats.frametime);
        ImGui::Text("Drawtime %f ms", m_stats.meshDrawTime);
        ImGui::Text("Update time %f ms", m_stats.sceneUpdateTime);
        ImGui::Text("Triangles %u", m_stats.trigDrawCount);
        ImGui::Text("Draws: %u", m_stats.drawCallCount);
        ImGui::End();
    }

    if(ImGui::Begin("Models")){
        for(const auto& [mID,model] : loadedModels){
            if(ImGui::CollapsingHeader(model.name.c_str())){
                ImGui::Text("ID: %u", mID);
                ImGui::Text("Name: %s", model.name.c_str());
                for(const auto& [oID,object] : model.objects){
                    if(ImGui::TreeNode(std::to_string(oID).c_str())){

                        if(ImGui::TreeNode("Transformation")){
                            // Change position
                            glm::vec3 pos = object->transformation.getTranslation();
                            if(ImGui::DragFloat3("Pos",(float*)& (pos))){
                                object->transformation.setTranslation(pos.x, pos.y, pos.z);
                            }

                            // Change rotation
                            glm::vec3 rotation = object->transformation.getRotation();
                            if(ImGui::DragFloat3("Rotation",(float*)& (rotation))){
                                object->transformation.setRotation(rotation.x, rotation.y, rotation.z);
                            }

                            float scale = object->transformation.getScale();
                            if(ImGui::DragFloat("Scale", &scale)){
                                object->transformation.setScale(scale);
                            }
                            ImGui::TreePop();
                        }

                        if(ImGui::TreeNode("Animation")) {
                            static std::size_t currentAnimation = 0;
                            if(ImGui::BeginListBox("##animation_list", ImVec2(-FLT_MIN,5 * ImGui::GetTextLineHeightWithSpacing()))){
                                for(std::size_t animID = 0; animID < model.resources->animations.size(); animID++){
                                    const bool isActive = (currentAnimation == animID);
                                    VktTypes::Animation* animation = model.resources->animations[animID].get();
                                    if(ImGui::Selectable(animation->name.c_str(), isActive)) {
                                        object->activeAnimation = animation;
                                        animation->currTime = animation->start;
                                        currentAnimation = animID;
                                    }
                                    if(isActive){
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
            }
        }
        ImGui::End();
    }


    ImGui::Render();
}

VktTypes::AllocatedBuffer VktCore::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
    VktCore& vktCore = getInstance();

    VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .pNext = nullptr};
    bufferInfo.size = allocSize;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = memoryUsage;
    vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VktTypes::AllocatedBuffer newBuffer{};
    VK_CHECK(vmaCreateBuffer(vktCore.m_vmaAllocator,
                             &bufferInfo,
                             &vmaAllocInfo,
                             &newBuffer.buffer,
                             &newBuffer.allocation,
                             &newBuffer.info));
    return newBuffer;
}

void VktCore::destroyBuffer(const VktTypes::AllocatedBuffer &buffer) {
    VktCore& vktCore = getInstance();

    vmaDestroyBuffer(vktCore.m_vmaAllocator, buffer.buffer, buffer.allocation);
}

VktTypes::GPUMeshBuffers VktCore::uploadMesh(std::span<uint32_t> indices, std::span<VktTypes::Vertex> vertices) {
    VktCore& vktCore = VktCore::getInstance();

    const size_t vertexBufferSize = vertices.size() * sizeof(VktTypes::Vertex);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    VktTypes::GPUMeshBuffers newSurface{};
    newSurface.vertexBuffer = createBuffer(vertexBufferSize,
                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                           VMA_MEMORY_USAGE_GPU_ONLY);

    VkBufferDeviceAddressInfo deviceAddressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .pNext = nullptr};
    deviceAddressInfo.buffer = newSurface.vertexBuffer.buffer;
    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(VktCore::device(), &deviceAddressInfo);

    newSurface.indexBuffer = createBuffer(indexBufferSize,
                                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                          VMA_MEMORY_USAGE_GPU_ONLY);

    VktTypes::AllocatedBuffer staging = createBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data = staging.info.pMappedData;
    memcpy(data, vertices.data(), vertexBufferSize);
    memcpy((char*)data+vertexBufferSize, indices.data(), indexBufferSize);

    vktCore.immediateSubmit([&](VkCommandBuffer cmd){
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
    destroyBuffer(staging);

    return newSurface;
}

VktTypes::GPUJointsBuffers VktCore::uploadJoints(const std::span<glm::mat4>& jointMatrices) {

    const size_t jointsBufferSize = jointMatrices.size() * sizeof(glm::mat4);

    VktTypes::GPUJointsBuffers newJoints{};
    newJoints.jointsBuffer = createBuffer(jointsBufferSize,
                                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                          VMA_MEMORY_USAGE_CPU_TO_GPU);

    VkBufferDeviceAddressInfo deviceAddressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .pNext = nullptr};
    deviceAddressInfo.buffer = newJoints.jointsBuffer.buffer;
    newJoints.jointsBufferAddress = vkGetBufferDeviceAddress(VktCore::device(), &deviceAddressInfo);

    memcpy(newJoints.jointsBuffer.info.pMappedData, jointMatrices.data(), jointsBufferSize);

    return newJoints;
}

void VktCore::initMeshPipeline() {
    VkShaderModule vertShader = VktUtils::loadShaderModule("shaders/coloredTriangleMesh.vert.spv", m_device);
    VkShaderModule fragShader = VktUtils::loadShaderModule("shaders/texImage.frag.spv", m_device);

    VkPushConstantRange bufferRange{};
    bufferRange.offset = 0;
    bufferRange.size = sizeof(VktTypes::GPUDrawPushConstants);
    bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .pNext = nullptr};
    pipelineLayoutInfo.pPushConstantRanges = &bufferRange;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_singleImageDescriptorLayout;
    pipelineLayoutInfo.setLayoutCount = 1;

    VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_meshPipelineLayout));
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE_LAYOUT, m_meshPipelineLayout);

    VktPipelineBuilder pipelineBuilder;
    pipelineBuilder.setPipelineLayout(m_meshPipelineLayout);
    pipelineBuilder.setShaders(vertShader, fragShader);
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.setMultisamplingNone();
    pipelineBuilder.disableBlending();
    pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_LESS);
    pipelineBuilder.setColorAttachmentFormat(m_drawImage.format);
    pipelineBuilder.setDepthFormat(m_depthImage.format);

    m_meshPipeline = pipelineBuilder.buildPipeline(m_device);
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, m_meshPipeline);

    vkDestroyShaderModule(m_device, vertShader, nullptr);
    vkDestroyShaderModule(m_device, fragShader, nullptr);
}

void VktCore::initDefaultData() {
    uint32_t white = 0xFFFFFFFF;
    m_whiteImage = createImage(reinterpret_cast<void*>(&white), VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    uint32_t black = 0x000000;
    m_blackImage = createImage(reinterpret_cast<void*>(&black), VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    uint32_t gray = 0xAAAAAAFF;
    m_whiteImage = createImage(reinterpret_cast<void*>(&gray), VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    uint32_t pink = 0xFF00DC;
    std::array<uint32_t, 16*16> pixels{};
    for(uint32_t x = 0; x < 16; x++){
        for(uint32_t y = 0; y < 16; y++){
            pixels[y*16+x] = ((x%2) ^ (y%2)) ? pink : black;
        }
    }
    m_errorCheckboardImage = createImage(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    VkSamplerCreateInfo samplerInfo{.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};

    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(m_device, &samplerInfo, nullptr, &m_defaultSamplerNearest);
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_SAMPLER, m_defaultSamplerNearest);

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(m_device, &samplerInfo, nullptr, &m_defaultSamplerLinear);
    m_coreDeletionQueue.pushDeletable(DeletableType::VK_SAMPLER, m_defaultSamplerLinear);
}

VkBool32 VktCore::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
                                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
    auto severityStr = vkb::to_string_message_severity(severity);
    auto typeStr = vkb::to_string_message_type(type);
    fprintf(stderr,"[%s: %s] %s\n", severityStr, typeStr, pCallbackData->pMessage);
    return VK_FALSE;
}

void VktCore::resizeSwapchain() {
    vkDeviceWaitIdle(m_device);
    destroySwapchain();

    Utils::WindowDimension dimensions = m_window->getSize();
    m_windowExtent.width = dimensions.width;
    m_windowExtent.height = dimensions.height;

    createSwapchain(m_windowExtent.width, m_windowExtent.height);
    m_resizeSwapchain = false;
}

VktTypes::AllocatedImage VktCore::createImage(VkExtent3D allocSize, VkFormat format, VkImageUsageFlags usage, bool mipMapped) {
    VktCore& vktCore = getInstance();

    VktTypes::AllocatedImage newImage{};
    newImage.format = format;
    newImage.extent = allocSize;

    VkImageCreateInfo imgInfo = VktStructs::imageCreateInfo(format, usage, allocSize);
    if(mipMapped){
        imgInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(allocSize.width, allocSize.height)))) + 1;
    }

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK(vmaCreateImage(vktCore.m_vmaAllocator, &imgInfo, &allocInfo, &newImage.image, &newImage.allocation,nullptr));

    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    if(format == VK_FORMAT_D32_SFLOAT){
        aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    VkImageViewCreateInfo viewInfo = VktStructs::imageViewCreateInfo(format, newImage.image, aspectFlags);
    viewInfo.subresourceRange.levelCount = imgInfo.mipLevels;

    VK_CHECK(vkCreateImageView(vktCore.m_device, &viewInfo, nullptr, &newImage.view));

    // Delete created resources on end of program
    // TODO Maybe not best place for this
    vktCore.m_coreDeletionQueue.pushDeletable(DeletableType::VK_IMAGE_VIEW, newImage.view);
    vktCore.m_coreDeletionQueue.pushDeletable(DeletableType::VMA_IMAGE, newImage.image, {newImage.allocation});
    return newImage;
}

VktTypes::AllocatedImage VktCore::createImage(void *data, VkExtent3D allocSize, VkFormat format, VkImageUsageFlags usage, bool mipMapped) {
    VktCore& vktCore = getInstance();

    size_t dataSize = allocSize.width * allocSize.height * allocSize.depth * 4;
    VktTypes::AllocatedBuffer stagingBuffer = createBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    // Copying data to staging buffer
    memcpy(stagingBuffer.info.pMappedData, data, dataSize);

    VktTypes::AllocatedImage newImage = createImage(allocSize, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipMapped);

    // Transfer data to image through staging buffer
    vktCore.immediateSubmit([&](VkCommandBuffer cmd){
        VktUtils::transitionImage(cmd, newImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = allocSize;

        vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
        VktUtils::transitionImage(cmd, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });
    destroyBuffer(stagingBuffer);

    return newImage;
}

void VktCore::destroyImage(const VktTypes::AllocatedImage &img){
    VktCore& vktCore = getInstance();

    vkDestroyImageView(vktCore.m_device, img.view, nullptr);
    vmaDestroyImage(vktCore.m_vmaAllocator, img.image, img.allocation);
}

void VktCore::setWindow(Window *window) {
    assert(window != nullptr);
    m_window = window;

    Utils::WindowDimension winDimensions = m_window->getSize();
    m_windowExtent.width = winDimensions.width;
    m_windowExtent.height = winDimensions.height;
}

void VktCore::updateScene() {
    m_mainDrawContext.opaqueSurfaces.clear();
    m_sceneData.view = m_viewMatrix;
    m_sceneData.proj = m_projMatrix;

    m_sceneData.proj[1][1] *= -1;
    m_sceneData.viewproj = m_sceneData.proj * m_sceneData.view;
    m_sceneData.ambientColor = glm::vec4(0.1f);
    m_sceneData.sunlightColor = glm::vec4(1.0f);
    m_sceneData.sunlightDirection = glm::vec4(0.0f, 1.0f, 0.5f, 1.0f);

    double currTime = glfwGetTime();
    static double prevTime = currTime;

    currTime = glfwGetTime();
    double delta = currTime - prevTime;
    prevTime = currTime;
    for(auto& [k,object] : loadedObjects){
        if(object.activeAnimation){
            VktTypes::Animation* animation = object.activeAnimation;
            object.modelHandle->resources->updateAnimation(animation, object.skins, static_cast<float>(delta));
        }
    }

    for(const auto& [oID, object] : loadedObjects){
        object.modelHandle->resources->gatherContext(object.transformation.getMatrix(), object.skins, m_mainDrawContext);
    }
}

void VktCore::setViewMatrix(const glm::mat4 &viewMatrix) {
    m_viewMatrix = viewMatrix;
}

void VktCore::setProjMatrix(const glm::mat4& projMatrix){
    m_projMatrix = projMatrix;
}

VkDevice VktCore::device() {
    VktCore& vktCore = getInstance();
    if(vktCore.m_device){
        return vktCore.m_device;
    }
    return nullptr;
}

/**
 * Initialize first model and object identifiers to 0.
 */
VktCore::modelID_t  VktCore::EngineModel::lastID = 0;
VktCore::objectID_t  VktCore::EngineObject::lastID = 0;

VktCore::EngineModel* VktCore::createModel(VktModelResources* resources, const std::string& name) {
    assert(resources);

    // Find free identifier
    while(loadedModels.contains(EngineModel::lastID)) EngineModel::lastID++;
    modelID_t freeID = EngineModel::lastID++;

    // Create model
    loadedModels[freeID] = {
            .modelID = freeID,
            .resources = resources,
            .name = name
    };

    // Return handle
    m_logger(Logger::INFO) << "Created a model at ID " << freeID << '\n';
    return &loadedModels[freeID];
}

VktCore::EngineObject* VktCore::createObject(EngineModel* model) {
    assert(model);

    // Find free identifier
    while(loadedObjects.contains(EngineObject::lastID)) EngineObject::lastID++;
    objectID_t freeID = EngineObject::lastID++;

    // Create object
    loadedObjects[freeID] = EngineObject{
            .objectID = freeID,
            .modelHandle = model,
            .activeAnimation = model->resources->animations[0].get()
            // Skins are auto-constructed
    };

    // Copy skins for per-object animation
    loadedObjects[freeID].skins.reserve(model->resources->skins.size());
    for(const auto& skin : model->resources->skins){
        // Create and store new buffer with joints matrices
        // TODO Destroy buffer
        loadedObjects[freeID].skins.emplace_back(uploadJoints(skin->inverseBindMatrices));
    }

    model->objects[freeID] = &loadedObjects[freeID];

    // Return handle
    m_logger(Logger::INFO) << "Created an object with ID " << freeID << " from model at ID " << model->modelID << '\n';
    return &loadedObjects[freeID];
}

void VktCore::GLTFMetallicRoughness::buildPipelines(VktCore *core) {
    // Load shaders
    VkShaderModule fragShader;
    VkShaderModule vertShader;
    fragShader = VktUtils::loadShaderModule("shaders/mesh.frag.spv", core->m_device);
    vertShader = VktUtils::loadShaderModule("shaders/mesh.vert.spv", core->m_device);

    // Create vertex shader push constants range
    VkPushConstantRange matrixRange{};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(VktTypes::GPUDrawPushConstants);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    {
        DescriptorLayoutBuilder layoutBuilder;
        // TODO Create enums/defs/constexpr for binding numbers
        layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        materialLayout = layoutBuilder.build(core->m_device,
                                             VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        core->m_coreDeletionQueue.pushDeletable(DeletableType::VK_DESCRIPTOR_SET_LAYOUT, materialLayout);
    }

    {
        DescriptorLayoutBuilder layoutBuilder;
        layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

        jointsLayout = layoutBuilder.build(core->m_device,VK_SHADER_STAGE_VERTEX_BIT);
        core->m_coreDeletionQueue.pushDeletable(DeletableType::VK_DESCRIPTOR_SET_LAYOUT, jointsLayout);
    }

    VkDescriptorSetLayout layouts[] = {core->m_gpuSceneDataDescriptorLayout, materialLayout, jointsLayout};

    // Create pipeline layout with provided descriptors and push constants
    VkPipelineLayoutCreateInfo meshLayoutInfo = VktStructs::pipelineLayoutCreateInfo();
    meshLayoutInfo.setLayoutCount = 3;
    meshLayoutInfo.pSetLayouts = layouts;
    meshLayoutInfo.pPushConstantRanges = &matrixRange;
    meshLayoutInfo.pushConstantRangeCount = 1;

    VkPipelineLayout newLayout;
    VK_CHECK(vkCreatePipelineLayout(core->m_device,
                                    &meshLayoutInfo,
                                    nullptr,
                                    &newLayout));
    core->m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE_LAYOUT, newLayout);

    // Set the pipeline layout for both opaque and transparent material pipeline
    opaquePipeline.layout = newLayout;
    transparentPipeline.layout = newLayout;

    // Build two pipelines fo opaque and transparent material rendering
    VktPipelineBuilder pipelineBuilder;
    pipelineBuilder.setShaders(vertShader, fragShader);
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.setMultisamplingNone();
    pipelineBuilder.disableBlending();
    pipelineBuilder.enableDepthTest(true, VK_COMPARE_OP_LESS);
    pipelineBuilder.setColorAttachmentFormat(core->m_drawImage.format);
    pipelineBuilder.setDepthFormat(core->m_depthImage.format);
    pipelineBuilder.setPipelineLayout(newLayout);

    opaquePipeline.pipeline = pipelineBuilder.buildPipeline(core->m_device);
    core->m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, opaquePipeline.pipeline);

    // Switch from opaque to transparent
    pipelineBuilder.enableBlendingAdditive();
    pipelineBuilder.enableDepthTest(false, VK_COMPARE_OP_LESS);

    transparentPipeline.pipeline = pipelineBuilder.buildPipeline(core->m_device);
    core->m_coreDeletionQueue.pushDeletable(DeletableType::VK_PIPELINE, transparentPipeline.pipeline);

    // Destroy unnecessary shader modules
    vkDestroyShaderModule(core->m_device, fragShader, nullptr);
    vkDestroyShaderModule(core->m_device, vertShader, nullptr);
}

void VktCore::GLTFMetallicRoughness::clearResources(VkDevice device) {

}

VktTypes::MaterialInstance VktCore::GLTFMetallicRoughness::writeMaterial(VkDevice device, VktTypes::MaterialPass pass,
                                                                         const VktCore::GLTFMetallicRoughness::MaterialResources &resources,
                                                                         DescriptorAllocatorDynamic &descriptorAllocator) {
    VktTypes::MaterialInstance matData{};
    matData.passType = pass;
    if(pass == VktTypes::MaterialPass::OPAQUE){
        matData.pipeline = &opaquePipeline;
    }else if(pass == VktTypes::MaterialPass::TRANSPARENT){
        matData.pipeline = &transparentPipeline;
    }

    matData.materialSet = descriptorAllocator.allocate(device, materialLayout);

    writer.clear();
    writer.writeBuffer(0,resources.dataBuffer,
                       sizeof(MaterialConstants),
                       resources.dataBufferOffset,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.writeImage(1, resources.colorImage.view,
                      resources.colorSampler,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.writeImage(2, resources.metalRoughImage.view,
                      resources.metalRoughSampler,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(device, matData.materialSet);

    return matData;
}

