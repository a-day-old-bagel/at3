#pragma once

template<typename EcsInterface>
VulkanContextCreateInfo <EcsInterface> VulkanContextCreateInfo<EcsInterface>::defaults() {
  VulkanContextCreateInfo<EcsInterface> info;
  info.appName = "at3";
  info.window = nullptr;
  info.ecs = nullptr;
  info.descriptorTypeCounts.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2048});
  info.descriptorTypeCounts.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096});
  return info;
}

template<typename EcsInterface>
VulkanContext<EcsInterface>::VulkanContext(VulkanContextCreateInfo <EcsInterface> info) {

  // Subscribe to view matrix updates and window resize events
  sub_wvUpdate = SUBSCRIBE_TOPIC("primary_cam_wv", updateWvMat);
  sub_windowResize = SUBSCRIBE_TOPIC("window_resized", reInitRendering);

  // Store the window and entity-component-system pointers.
  common.window = info.window;
  ecs = info.ecs;

  // Create the fundamental Vulkan backbone objects
  createInstance(info.appName.c_str());
  createDebugCallback();
  createSurface();  // The SDL_Vulkan surface
  createPhysicalDevice();
  createLogicalDevice();

  // Initialize the memory allocation pool
  pool::activate(&common);

  // Get the current window size and create a matching swap chain
  storeWindowSize();
  createSwapchainForSurface();

  // Create the command, descriptor, and query pools
  createCommandPool(common.gfxCommandPool);
  createCommandPool(common.transferCommandPool);
  createCommandPool(common.presentCommandPool);
  createDescriptorPool(common.descriptorPool, info);
  createQueryPool(10);

  // Create the synchronization structures
  createVkSemaphore(common.imageAvailableSemaphore);
  createVkSemaphore(common.renderFinishedSemaphore);
  common.frameFences.resize(common.swapChain.imageViews.size());
  for (uint32_t i = 0; i < common.frameFences.size(); ++i) {
    createFence(common.frameFences[i]);
  }

  // Load the textures into a repository
  // TODO: Synchronize texture loading (wait for it, ala loading screen). This might be causing the errors on fresh runs.
  TextureOperationInfo texOpInfo {};
  texOpInfo.physicalDevice = common.gpu.device;
  texOpInfo.logicalDevice = common.device;
  texOpInfo.transferCommandPool = common.transferCommandPool;
  texOpInfo.transferQueue = common.deviceQueues.transferQueue;
  texOpInfo.physicalMemProps = common.gpu.memProps;
  texOpInfo.samplerAnisotropy = common.gpu.features.samplerAnisotropy;
  texOpInfo.maxSamplerAnisotropy = common.gpu.deviceProps.limits.maxSamplerAnisotropy;
  textureRepo = std::make_unique<TextureRepository>("./assets/textures", texOpInfo);

  // Create the pipelines.
  // The number of textures that got loaded is needed for specialization constants.
  pipelineRepo = std::make_unique<PipelineRepository>(common, textureRepo->getDescriptorImageInfoArrayCount());

  // Load the meshes into a repository
  // TODO: put this crap in a proper repository like VkcTextureRepository does, do it when upgrading to gltf
  // TODO: Handle the multiple-objects-in-one-file case (true -> false)?
  for (auto &path : fs::recursive_directory_iterator("./assets/models")) {
    if (getFileExtOnly(path) == ".dae") {
      printf("\n%s:\nLoading Mesh: %s\n", getFileNameOnly(path).c_str(), getFileNameRelative(path).c_str());
      meshRepo.emplace(getFileNameOnly(path), loadMesh(getFileNameRelative(path).c_str(), true));
    }
  }
  printf("\n");

  // Create the paged UBO system for mesh instance data
  dataStore = std::make_unique<UboPageMgr>(common);

  // Create the frame and depth buffers, command buffers, and other things that depend on window size.
  // These will need to be recreated (by calling this function again) whenever the window size changes.
  createWindowSizeDependents();
}

template<typename EcsInterface>
VulkanContext<EcsInterface>::~VulkanContext() {

}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::updateWvMat(void *data) {
  auto newMat = (glm::mat4 *) data;
  currentWvMat = *newMat;
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::step() {
  render(dataStore.get(), currentWvMat, meshRepo, ecs);
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::reInitRendering() {
  printf("Re-initializing vulkan rendering pipeline\n");
  vkDeviceWaitIdle(common.device);
  destroyWindowSizeDependents();
  storeWindowSize();
  createSwapchainForSurface();
  createWindowSizeDependents();
  pipelineRepo->reinit(common, textureRepo->getDescriptorImageInfoArrayCount());
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::registerMeshInstance(
    const typename EcsInterface::EcsId id, const std::string &meshFileName, const std::string &textureFileName) {
  for (auto &mesh : meshRepo.at(meshFileName)) {
    MeshInstance<EcsInterface> instance;
    UboPageMgr::AcquireStatus didAcquire = dataStore->acquire(instance.indices);
    AT3_ASSERT(didAcquire != UboPageMgr::AcquireStatus::FAILURE, "Error acquiring ubo index");
    if (didAcquire == UboPageMgr::AcquireStatus::NEWPAGE) {
      updateDescriptorSets(dataStore.get());
    }
    instance.id = id;
    if (textureFileName.length() && textureRepo->textureExists(textureFileName)) {
      instance.indices.setTexture(textureRepo->getTextureArrayIndex(textureFileName));
    } else {
      instance.indices.setTexture(0u);  // The first texture will be used. Recommend using that slot for debug texture.
      printf("No texture applied to mesh: %s\n", meshFileName.c_str());
    }
    mesh.instances.push_back(instance);
  }
}
