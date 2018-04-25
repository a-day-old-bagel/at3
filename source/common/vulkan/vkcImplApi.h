#pragma once

//#include <SDLvulkan.h>

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

  sub_wvUpdate = SUBSCRIBE_TOPIC("primary_cam_wv", updateWvMat);
  sub_windowResize = SUBSCRIBE_TOPIC("window_resized", reInitRendering);

  // Init the vulkan instance, device, pools, etc.

  common.window = info.window;
  getWindowSize();
  ecs = info.ecs;

  createInstance(info.appName.c_str());
  createDebugCallback();

  createSurface();
  createPhysicalDevice();
  createLogicalDevice();

  allocators::pool::activate(&common);

  createSwapchainForSurface(); // window size is needed here

  createCommandPool(common.gfxCommandPool, common.device, common.gpu, common.gpu.graphicsQueueFamilyIdx);
  createCommandPool(common.transferCommandPool, common.device, common.gpu, common.gpu.transferQueueFamilyIdx);
  createCommandPool(common.presentCommandPool, common.device, common.gpu, common.gpu.presentQueueFamilyIdx);

  createDescriptorPool(common.descriptorPool, common.device, info.types, info.typeCounts);

  createVkSemaphore(common.imageAvailableSemaphore, common.device);
  createVkSemaphore(common.renderFinishedSemaphore, common.device);

  createQueryPool(10);

  common.frameFences.resize(common.swapChain.imageViews.size());

  for (uint32_t i = 0; i < common.frameFences.size(); ++i) {
    createFence(common.frameFences[i], common.device);
  }

  // Init the ubo store and meshes

  std::vector<EMeshVertexAttribute> meshLayout;
  meshLayout.push_back(EMeshVertexAttribute::POSITION);
  meshLayout.push_back(EMeshVertexAttribute::UV0);
  meshLayout.push_back(EMeshVertexAttribute::NORMAL);
  setGlobalVertexLayout(meshLayout);

  // Open every mesh in the mesh directory
  // TODO: Handle the multiple-objects-in-one-file case (true -> false)?
  for (auto & path : fs::directory_iterator("./assets/models/")) {
    if (fs::is_directory(path)) { continue; }
    printf("\n%s -> %s\n", getFileNameOnly(path).c_str(), getFileNameRelative(path).c_str());
    meshResources.emplace(getFileNameOnly(path), loadMesh(getFileNameRelative(path).c_str(), true, common));
  }

  printf("Num meshes: %lu\n", meshResources.size());
  dataStore = std::make_unique<DataStore>(common);

  // Init the vulkan swapchain, pipeline, etc.

  initRendering(common, (uint32_t)meshResources.size());
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
  render(common, dataStore.get(), currentWvMat, meshResources, ecs);
}

template <typename EcsInterface>
void VulkanContext<EcsInterface>::reInitRendering() {
  vkDeviceWaitIdle(common.device);
  cleanupRendering();
  getWindowSize();
  createSwapchainForSurface();
  initRendering(common, (uint32_t)meshResources.size());
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::registerMeshInstance(const std::string &meshFileName,
                                                       const typename EcsInterface::EcsId id) {
  for (auto &mesh : meshResources.at(meshFileName)) {
    MeshInstance<EcsInterface> instance;
    DataStore::AcquireStatus didAcquire = dataStore->acquire(instance.uboIdx);
    AT3_ASSERT(didAcquire != DataStore::AcquireStatus::FAILURE, "Error acquiring ubo index");
    if (didAcquire == DataStore::AcquireStatus::NEWPAGE) {
      updateDescriptorSets(common, dataStore.get());
    }
    instance.id = id;
    mesh.instances.push_back(instance);
  }
}
