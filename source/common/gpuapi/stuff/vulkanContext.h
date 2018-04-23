
#pragma once

#include <unordered_map>
#include <string>
#include <experimental/filesystem>

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

namespace fs = std::experimental::filesystem;

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
      std::unique_ptr<DataStore> uboData;

      std::unordered_map<std::string, std::vector<MeshAsset<EcsInterface>>> meshAssets;

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
      void registerMeshInstance(const std::string &meshFileName, const typename EcsInterface::EcsId id);

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

    // Open every mesh in the mesh directory
    // TODO: Handle the multiple-objects-in-one-file case (true -> false)?
    for (auto & path : fs::directory_iterator("./assets/models/")) {
      if (fs::is_directory(path)) { continue; }
      printf("\n%s -> %s\n", path.path().filename().c_str(), path.path().c_str()); //fs::absolute(path).c_str());
      meshAssets.emplace(path.path().filename(),
                         loadMesh<EcsInterface>(path.path().c_str(), true, guts));
    }

    printf("Num meshes: %lu\n", meshAssets.size());
    uboData = std::make_unique<DataStore>(guts);

    // Init the vulkan swapchain, pipeline, etc.

    initRendering(guts, (uint32_t)meshAssets.size());
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
    render<EcsInterface>(guts, uboData.get(), currentWvMat, meshAssets, ecs);
  }

  template <typename EcsInterface>
  void VulkanContext<EcsInterface>::reInitRendering() {
    vkDeviceWaitIdle(guts.device);
    cleanupRendering();
    getWindowSize(guts);
    createSwapchainForSurface(guts);
    initRendering(guts, (uint32_t)meshAssets.size());
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

  template<typename EcsInterface>
  void VulkanContext<EcsInterface>::registerMeshInstance(const std::string &meshFileName,
                                                         const typename EcsInterface::EcsId id) {
    for (auto &mesh : meshAssets.at(meshFileName)) {
      MeshInstance<EcsInterface> instance;
      DataStore::AcquireStatus didAcquire = uboData->acquire(instance.uboIdx);
      checkf(didAcquire != DataStore::AcquireStatus::FAILURE, "Error acquiring ubo index");
      if (didAcquire == DataStore::AcquireStatus::NEWPAGE) {
        updateDescriptorSets(guts, uboData.get());
      }
      instance.id = id;
      mesh.instances.push_back(instance);
    }
  }
}
