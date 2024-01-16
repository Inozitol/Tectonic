#include "engine/vulkan/VulkanCore.h"

VulkanCore &VulkanCore::getInstance() {
    static VulkanCore instance;
    return instance;
}

VulkanCore::VulkanCore() {
    initGLFW();
    initVulkan();
    initSwapchain();
    initCommands();
    initSyncStructs();

    m_isInitialized = true;
}

VulkanCore::~VulkanCore(){
    if(m_isInitialized){
        vkDeviceWaitIdle(m_device);


        for(uint8_t i = 0; i < FRAMES_OVERLAP; i++){
            vkDestroyCommandPool(m_device, m_frames[i].commandPool, nullptr);
            vkDestroyFence(m_device, m_frames[i].renderFence, nullptr);
            vkDestroySemaphore(m_device, m_frames[i].renderSemaphore, nullptr);
            vkDestroySemaphore(m_device, m_frames[i].swapchainSemaphore, nullptr);
        }

        destroySwapchain();

        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyDevice(m_device, nullptr);

        vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
        vkDestroyInstance(m_instance, nullptr);

        // Window is destroyed with Window destructor
    }
}

void VulkanCore::initGLFW() {
    glfwSetErrorCallback(VulkanCore::glfwErrorCallback);

    if (!glfwInit()) {
        throw engineException("Engine couldn't initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_FALSE);

    m_window = std::make_unique<Window>("Tectonic");
    m_window->makeCurrentContext();
    Utils::Dimensions winDimensions = m_window->getSize();
    m_windowExtent.width = winDimensions.width;
    m_windowExtent.height = winDimensions.height;
}

void VulkanCore::glfwErrorCallback(int, const char *msg) {
    fprintf(stderr, "GLFW Error: %s\n", msg);
}

void VulkanCore::initVulkan() {
    vkb::InstanceBuilder vkBuilder;

    auto instance = vkBuilder.set_app_name("Tectonic")
            .request_validation_layers(true)
            .use_default_debug_messenger()
            .require_api_version(1,3,0)
            .build();

    vkb::Instance vkbInstance = instance.value();

    m_instance = vkbInstance.instance;
    m_debugMessenger = vkbInstance.debug_messenger;

    m_surface = m_window->createWindowSurface(m_instance);

    VkPhysicalDeviceVulkan13Features features13{};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress = true;
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

    m_device = vkbDevice.device;
    m_physicalDevice = vkbPhysicalDevice.physical_device;

    m_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    m_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanCore::initSwapchain() {
    createSwapchain(m_windowExtent.width, m_windowExtent.height);
}

void VulkanCore::initCommands() {
    VkCommandPoolCreateInfo commandPoolCreateInfo = VkStructs::commandPoolCreateInfo(m_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for(uint8_t i = 0; i < FRAMES_OVERLAP; i++){
        VK_CHECK(vkCreateCommandPool(m_device,
                                     &commandPoolCreateInfo,
                                     nullptr,
                                     &m_frames[i].commandPool));

        VkCommandBufferAllocateInfo cmdAllocInfo = VkStructs::commandBufferAllocateInfo(m_frames[i].commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(m_device,
                                          &cmdAllocInfo,
                                          &m_frames[i].mainCommandBuffer));
    }
}

void VulkanCore::initSyncStructs() {
    VkFenceCreateInfo fenceCreateInfo = VkStructs::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = VkStructs::semaphoreCreateInfo();

    for(uint8_t i = 0; i < FRAMES_OVERLAP; i++){
        VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_frames[i].renderFence));
        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_frames[i].renderSemaphore));
    }
}

void VulkanCore::createSwapchain(uint32_t width, uint32_t height) {
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
}

void VulkanCore::destroySwapchain() {
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    for(auto& imageView : m_swapchainImageViews){
        vkDestroyImageView(m_device, imageView, nullptr);
    }
}

VulkanCore::frameData &VulkanCore::getCurrentFrame() {
    return m_frames[m_frameNumber % FRAMES_OVERLAP];
}

void VulkanCore::draw() {
    VK_CHECK(vkWaitForFences(m_device, 1, &getCurrentFrame().renderFence, true, 1000000000));
    VK_CHECK(vkResetFences(m_device, 1, &getCurrentFrame().renderFence));

    uint32_t swapchainIndex;
    VK_CHECK(vkAcquireNextImageKHR(m_device,
                                   m_swapchain,
                                   1000000000,
                                   getCurrentFrame().swapchainSemaphore,
                                   nullptr,
                                   &swapchainIndex));

    VkCommandBuffer cmd = getCurrentFrame().mainCommandBuffer;

    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    VkCommandBufferBeginInfo cmdBeginInfo = VkStructs::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    VkUtils::transitionImage(cmd, m_swapchainImages[swapchainIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    VkClearColorValue clearValue;
    float flash = std::abs(std::sin(static_cast<float>(m_frameNumber) / 120.f));
    clearValue = {{0.0f, 0.0f, flash, 1.0f}};

    VkImageSubresourceRange clearRange = VkStructs::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(cmd, m_swapchainImages[swapchainIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
    VkUtils::transitionImage(cmd, m_swapchainImages[swapchainIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdInfo = VkStructs::commandBufferSubmitInfo(cmd);

    VkSemaphoreSubmitInfo waitInfo = VkStructs::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame().swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = VkStructs::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().renderSemaphore);

    VkSubmitInfo2 submit = VkStructs::submitInfo(&cmdInfo, &signalInfo, &waitInfo);

    VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submit, getCurrentFrame().renderFence));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainIndex;

    VK_CHECK(vkQueuePresentKHR(m_graphicsQueue, &presentInfo));

    m_frameNumber++;
}

