
#include "vulkanContext.h"

#include "config.h"
#include "vkh_setup.h"
#include "rendering.h"
#include "dataStore.h"
#include "mesh_loading.h"

#define SUBSCRIBE_TOPIC(e,x) std::make_unique<rtu::topics::Subscription>(e, RTU_MTHD_DLGT(&VulkanContext::x, this));

namespace at3 {

  VulkanContextCreateInfo VulkanContextCreateInfo::defaults() {
    VulkanContextCreateInfo info;
    info.appName = "at3";
    info.window = nullptr;
    info.types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    info.types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    info.types.push_back(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    info.types.push_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    info.types.push_back(VK_DESCRIPTOR_TYPE_SAMPLER);
    info.typeCounts.push_back(512);
    info.typeCounts.push_back(512);
    info.typeCounts.push_back(512);
    info.typeCounts.push_back(512);
    info.typeCounts.push_back(512);
    info.typeCounts.push_back(1);
    return info;
  }

  VulkanContext::VulkanContext(VulkanContextCreateInfo info) {

    wvUpdate = SUBSCRIBE_TOPIC("primary_cam_wv", updateWvMat);
    windowResize = SUBSCRIBE_TOPIC("window_resized", reInitRendering);

    // Init the vulkan instance, device, pools, etc.

    guts.window = info.window;
    getWindowSize(guts);

    createInstance(guts, info.appName.c_str());
    createDebugCallback(guts);

    createSurface(guts);
    createPhysicalDevice(guts);
    createLogicalDevice(guts);

    allocators::pool::activate(&guts);

    createSwapchainForSurface(guts); // window size is needed here

    createCommandPool(guts.gfxCommandPool, guts.device, guts.gpu, guts.gpu.graphicsQueueFamilyIdx);
    createCommandPool(guts.transferCommandPool, guts.device, guts.gpu, guts.gpu.transferQueueFamilyIdx);
    createCommandPool(guts.presentCommandPool, guts.device, guts.gpu, guts.gpu.presentQueueFamilyIdx);

    createDescriptorPool(guts.descriptorPool, guts.device, info.types, info.typeCounts);

    createVkSemaphore(guts.imageAvailableSemaphore, guts.device);
    createVkSemaphore(guts.renderFinishedSemaphore, guts.device);

    createQueryPool(guts.queryPool, guts.device, 10);

    guts.frameFences.resize(guts.swapChain.imageViews.size());

    for (uint32_t i = 0; i < guts.frameFences.size(); ++i) {
      createFence(guts.frameFences[i], guts.device);
    }

    // Init the ubo store and meshes

    std::vector<EMeshVertexAttribute> meshLayout;
    meshLayout.push_back(EMeshVertexAttribute::POSITION);
    meshLayout.push_back(EMeshVertexAttribute::UV0);
    meshLayout.push_back(EMeshVertexAttribute::NORMAL);
    Mesh::setGlobalVertexLayout(meshLayout);



//    testMesh = loadMesh("./meshes/sponza.obj", false, guts);
//    testMesh = loadMesh("./assets/models/ArmyPilot/ArmyPilot.obj", false, guts);

    testMesh = loadMesh("./assets/models/pyramid_bottom.dae", false, guts);

    auto top = loadMesh("./assets/models/pyramid_top.dae", false, guts);
    testMesh.insert(testMesh.end(), top.begin(), top.end());
    auto thrusters = loadMesh("./assets/models/pyramid_thrusters.dae", false, guts);
    testMesh.insert(testMesh.end(), thrusters.begin(), thrusters.end());
    auto thrusterFlames = loadMesh("./assets/models/pyramid_thruster_flames.dae", false, guts);
    testMesh.insert(testMesh.end(), thrusterFlames.begin(), thrusterFlames.end());



    uboIdx.resize(testMesh.size());
    printf("Num meshes: %lu\n", testMesh.size());
    ubo_store::init(guts);

    for (uint32_t i = 0; i < testMesh.size(); ++i) {
      bool didAcquire = ubo_store::acquire(uboIdx[i]);
      checkf(didAcquire, "Error acquiring ubo index");
    }

    // Init the vulkan swapchain, pipeline, etc.

    initRendering(guts, (uint32_t)testMesh.size());

  }

  VulkanContext::~VulkanContext() {

  }

  void VulkanContext::updateWvMat(void *data) {
    auto newMat = (glm::mat4*)data;
    currentWvMat = *newMat;
  }

  void VulkanContext::step() {
    render(guts, currentWvMat, testMesh, uboIdx);
  }

  void VulkanContext::reInitRendering() {
    vkDeviceWaitIdle(guts.device);
    cleanupRendering();
    getWindowSize(guts);
    createSwapchainForSurface(guts);
    initRendering(guts, (uint32_t)testMesh.size());
  }

  void VulkanContext::cleanupRendering() {
    vkDestroyImageView(guts.device, guts.renderData.depthBuffer.view, nullptr);
    vkDestroyImage(guts.device, guts.renderData.depthBuffer.handle, nullptr);
    allocators::pool::free(guts.renderData.depthBuffer.imageMemory);

    for (size_t i = 0; i < guts.renderData.frameBuffers.size(); i++) {
      vkDestroyFramebuffer(guts.device, guts.renderData.frameBuffers[i], nullptr);
    }

    vkFreeCommandBuffers(guts.device, guts.gfxCommandPool,
                         static_cast<uint32_t>(guts.renderData.commandBuffers.size()),
                         guts.renderData.commandBuffers.data());

    vkDestroyPipeline(guts.device, guts.matData.graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(guts.device, guts.matData.pipelineLayout, nullptr);
    vkDestroyRenderPass(guts.device, guts.renderData.mainRenderPass, nullptr);

    for (size_t i = 0; i < guts.swapChain.imageViews.size(); i++) {
      vkDestroyImageView(guts.device, guts.swapChain.imageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(guts.device, guts.swapChain.swapChain, nullptr);
  }

}
