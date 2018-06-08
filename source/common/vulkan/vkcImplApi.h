#pragma once

template<typename EcsInterface>
VulkanContextCreateInfo <EcsInterface> VulkanContextCreateInfo<EcsInterface>::defaults() {
  VulkanContextCreateInfo<EcsInterface> info;
  info.appName = "at3";
  info.window = nullptr;
  info.ecs = nullptr;
  info.descriptorTypeCounts.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 512});
  info.descriptorTypeCounts.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});
  return info;
}

template<typename EcsInterface>
VulkanContext<EcsInterface>::VulkanContext(VulkanContextCreateInfo <EcsInterface> info) {

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

  createCommandPool(common.gfxCommandPool);
  createCommandPool(common.transferCommandPool);
  createCommandPool(common.presentCommandPool);

  createDescriptorPool(common.descriptorPool, info);

  createVkSemaphore(common.imageAvailableSemaphore);
  createVkSemaphore(common.renderFinishedSemaphore);

  createQueryPool(10);

  common.frameFences.resize(common.swapChain.imageViews.size());

  for (uint32_t i = 0; i < common.frameFences.size(); ++i) {
    createFence(common.frameFences[i]);
  }

  // Init the ubo store and meshes

  std::vector<EMeshVertexAttribute> meshLayout;
  meshLayout.push_back(EMeshVertexAttribute::POSITION);
  meshLayout.push_back(EMeshVertexAttribute::UV0);
  meshLayout.push_back(EMeshVertexAttribute::NORMAL);
  setGlobalVertexLayout(meshLayout);

  // Open every mesh in the mesh directory
  // TODO: Handle the multiple-objects-in-one-file case (true -> false)?
  for (auto &path : fs::directory_iterator("./assets/models/")) {
    if (fs::is_directory(path)) { continue; }
    printf("\n%s:\nLoading Mesh: %s\n", getFileNameOnly(path).c_str(), getFileNameRelative(path).c_str());
    meshResources.emplace(getFileNameOnly(path), loadMesh(getFileNameRelative(path).c_str(), true));
    fflush(stdout);
  }
  printf("\n");
  fflush(stdout);

//  // Open every texture in the texture directory
//  // TODO: Handle the multiple-objects-in-one-file case (true -> false)?
//  for (auto &path : fs::directory_iterator("./assets/textures/")) {
//    if (fs::is_directory(path) || getFileExtOnly(path) != "ktx") { continue; }
//    printf("\n%s:\nLoading Texture: %s\n", getFileNameOnly(path).c_str(), getFileNameRelative(path).c_str());
//    textureResources.emplace(getFileNameOnly(path));
//    textureResources.at(getFileNameOnly(path)).loadFromFile(
//        "assets/textures/pyramid_bottom.ktx", VK_FORMAT_R8G8B8A8_UNORM, this, common.deviceQueues.transferQueue);
//    fflush(stdout);
//  }
//  printf("\n");
//  fflush(stdout);


  VkcTextureOperationInfo texOpInfo {};
  texOpInfo.physicalDevice = common.gpu.device;
  texOpInfo.logicalDevice = common.device;
  texOpInfo.transferCommandPool = common.transferCommandPool;
  texOpInfo.transferQueue = common.deviceQueues.transferQueue;
  texOpInfo.physicalMemProps = common.gpu.memProps;
  texOpInfo.samplerAnisotropy = common.gpu.features.samplerAnisotropy;
  texOpInfo.maxSamplerAnisotropy = common.gpu.deviceProps.limits.maxSamplerAnisotropy;

  textures = std::make_unique<TextureRepository<EcsInterface>>("./assets/textures", texOpInfo);


//  testTex.loadFromFile("assets/textures/pyramid_bottom.ktx", VK_FORMAT_R8G8B8A8_UNORM, texOpInfo,
//                       common.deviceQueues.transferQueue);
//  testTex2.loadFromFile("assets/textures/pyramid_top.ktx", VK_FORMAT_R8G8B8A8_UNORM, texOpInfo,
//                       common.deviceQueues.transferQueue);

//  textureResources.emplace_back("assets/textures/pyramid_bottom.ktx", VK_FORMAT_R8G8B8A8_UNORM, texOpInfo,
//                       common.deviceQueues.transferQueue);

  dataStore = std::make_unique<UboPageMgr>(common);


//  initRendering((uint32_t) textureResources.size());
//  initRendering((uint32_t) meshResources.size());
//  initRendering(Texture2D<EcsInterface>::getDescriptorImageInfoArrayCount());
  initRendering(textures->getDescriptorImageInfoArrayCount());



//  initGlobalShaderData();


//  testTex.loadFromFile("assets/textures/pyramid_bottom.ktx", VK_FORMAT_R8G8B8A8_UNORM, this,
//                       common.deviceQueues.transferQueue);

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
  render(dataStore.get(), currentWvMat, meshResources, ecs);
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::reInitRendering() {
  vkDeviceWaitIdle(common.device);
  cleanupRendering();
  getWindowSize();
  createSwapchainForSurface();
  initRendering((uint32_t) meshResources.size());
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::registerMeshInstance(
    const typename EcsInterface::EcsId id, const std::string &meshFileName, const std::string &textureFileName) {
  for (auto &mesh : meshResources.at(meshFileName)) {
    MeshInstance<EcsInterface> instance;
    UboPageMgr::AcquireStatus didAcquire = dataStore->acquire(instance.indices);
    AT3_ASSERT(didAcquire != UboPageMgr::AcquireStatus::FAILURE, "Error acquiring ubo index");
    if (didAcquire == UboPageMgr::AcquireStatus::NEWPAGE) {
      updateDescriptorSets(dataStore.get());
    }
    instance.id = id;
//    if (textureFileName.length() && Texture2D<EcsInterface>::textureExists(textureFileName)) {
//      instance.indices.setTexture(Texture2D<EcsInterface>::getTextureArrayIndex(textureFileName));
    if (textureFileName.length() && textures->textureExists(textureFileName)) {
      instance.indices.setTexture(textures->getTextureArrayIndex(textureFileName));
    } else {
      instance.indices.setTexture(0u);  // The first texture will be used. Recommend using that slot for debug texture.
      printf("No texture applied to mesh: %s\n", meshFileName.c_str());
    }
    mesh.instances.push_back(instance);
  }
}
