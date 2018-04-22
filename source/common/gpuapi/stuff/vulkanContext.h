
#pragma once

#include <unordered_map>

#include <string>
#include "vkh_types.h"
#include "vkh_mesh.h"
#include "topics.hpp"

#include "config.h"
#include "vkh_setup.h"
#include "rendering.h"
#include "dataStore.h"
#include "mesh_loading.h"
#include "vkh_alloc.h"

#define SUBSCRIBE_TOPIC(e,x) std::make_unique<rtu::topics::Subscription>(e, RTU_MTHD_DLGT(&VulkanContext::x, this));

namespace at3 {

  template <typename EcsInterface>
  struct VulkanContextCreateInfo {
    std::string appName;
    SDL_Window *window;
    EcsInterface *ecs;
    std::vector<VkDescriptorType> types;
    std::vector<uint32_t> typeCounts;

    static VulkanContextCreateInfo<EcsInterface> defaults();
  };

  struct MeshRepository {

  };

  template <typename EcsInterface>
  class VulkanContext {

      EcsInterface *ecs;
      VkhContext guts;
      std::vector<MeshAsset> testMeshes;
      std::vector<uint32_t> uboIdx;
      glm::mat4 currentWvMat;
      std::unique_ptr<rtu::topics::Subscription> wvUpdate;
      std::unique_ptr<rtu::topics::Subscription> windowResize;

      void cleanupRendering();

    public:
      VulkanContext(VulkanContextCreateInfo<EcsInterface> info);
      virtual ~VulkanContext();
      void updateWvMat(void* data);
      void step();
      void reInitRendering();

  };

  template <typename EcsInterface>
  VulkanContextCreateInfo<EcsInterface> VulkanContextCreateInfo<EcsInterface>::defaults() {
    VulkanContextCreateInfo<EcsInterface> info;
    info.appName = "at3";
    info.window = nullptr;
    info.ecs = nullptr;
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

  template <typename EcsInterface>
  VulkanContext<EcsInterface>::VulkanContext(VulkanContextCreateInfo<EcsInterface> info) {

    wvUpdate = SUBSCRIBE_TOPIC("primary_cam_wv", updateWvMat);
    windowResize = SUBSCRIBE_TOPIC("window_resized", reInitRendering);

    // Init the vulkan instance, device, pools, etc.

    guts.window = info.window;
    getWindowSize(guts);
    ecs = info.ecs;

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

    testMeshes = loadMesh("./assets/models/pyramid_bottom.dae", false, guts);

    auto top = loadMesh("./assets/models/pyramid_top.dae", false, guts);
    testMeshes.insert(testMeshes.end(), top.begin(), top.end());
    auto thrusters = loadMesh("./assets/models/pyramid_thrusters.dae", false, guts);
    testMeshes.insert(testMeshes.end(), thrusters.begin(), thrusters.end());
    auto thrusterFlames = loadMesh("./assets/models/pyramid_thruster_flames.dae", false, guts);
    testMeshes.insert(testMeshes.end(), thrusterFlames.begin(), thrusterFlames.end());



    uboIdx.resize(testMeshes.size());
    printf("Num meshes: %lu\n", testMeshes.size());
    ubo_store::init(guts);

    for (uint32_t i = 0; i < testMeshes.size(); ++i) {
      bool didAcquire = ubo_store::acquire(uboIdx[i]);
      checkf(didAcquire, "Error acquiring ubo index");
    }

    // Init the vulkan swapchain, pipeline, etc.

    initRendering(guts, (uint32_t)testMeshes.size());

  }

  template <typename EcsInterface>
  VulkanContext<EcsInterface>::~VulkanContext() {

  }

  template <typename EcsInterface>
  void VulkanContext<EcsInterface>::updateWvMat(void *data) {
    auto newMat = (glm::mat4*)data;
    currentWvMat = *newMat;
  }

  template <typename EcsInterface>
  void VulkanContext<EcsInterface>::step() {
    render(guts, currentWvMat, testMeshes, uboIdx);
  }

  template <typename EcsInterface>
  void VulkanContext<EcsInterface>::reInitRendering() {
    vkDeviceWaitIdle(guts.device);
    cleanupRendering();
    getWindowSize(guts);
    createSwapchainForSurface(guts);
    initRendering(guts, (uint32_t)testMeshes.size());
  }

  template <typename EcsInterface>
  void VulkanContext<EcsInterface>::cleanupRendering() {
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
