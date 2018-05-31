#pragma once

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createInstance(const char *appName) {
  VkApplicationInfo app_info;

  //stype and pnext are the same usage as below
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pNext = NULL;
  app_info.pApplicationName = appName;
  app_info.applicationVersion = 1;
  app_info.pEngineName = appName;
  app_info.engineVersion = 1;
  app_info.apiVersion = VK_API_VERSION_1_0;

  std::vector<const char *> validationLayers;
  std::vector<bool> layersAvailable;

#ifndef NDEBUG
  printf("Starting up with validation layers enabled: \n");

  const char *layerNames = "VK_LAYER_LUNARG_standard_validation";

  validationLayers.push_back(layerNames);
  printf("\t* Looking for: %s\n", layerNames);

  layersAvailable.push_back(false);

  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers;
  availableLayers.resize(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (uint32_t i = 0; i < validationLayers.size(); ++i) {
    for (uint32_t j = 0; j < availableLayers.size(); ++j) {
      if (strcmp(validationLayers[i], availableLayers[j].layerName) == 0) {
        printf("\t* Found layer: %s\n", validationLayers[i]);
        layersAvailable[i] = true;
      }
    }
  }

  bool foundAllLayers = true;
  for (uint32_t i = 0; i < layersAvailable.size(); ++i) {
    foundAllLayers &= layersAvailable[i];
  }

  AT3_ASSERT(foundAllLayers, "Not all required validation layers were found.");
#endif

  //Get available extensions:
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> extensions;
  extensions.resize(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

  std::vector<const char *> requiredExtensions;
  std::vector<bool> extensionsPresent;


#   if USE_CUSTOM_SDL_VULKAN
  requiredExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  extensionsPresent.push_back(false);

  uint32_t extCount = 0;
  const char *extensionNames[64];
  unsigned c = 64 - extCount;
  if (!SDL_GetVulkanInstanceExtensions(&c, &extensionNames[extCount])) {
    std::string error = std::string("SDL_GetVulkanInstanceExtensions failed: ") +
                        std::string(SDL_GetError());
    throw std::runtime_error(error.c_str());
  }
  extCount += c;

  for (unsigned int i = 0; i < extCount; i++) {
    requiredExtensions.push_back(extensionNames[i]);
    extensionsPresent.push_back(false);
  }
#   else
  const char **sdlVkExtensions = nullptr;
  uint32_t sdlVkExtensionCount = 0;

  // Query the extensions requested by SDL_vulkan
  bool success = SDL_Vulkan_GetInstanceExtensions(common.window, &sdlVkExtensionCount, NULL);
  checkf(success, "SDL_Vulkan_GetInstanceExtensions(): %s\n", SDL_GetError());

  sdlVkExtensions = (const char **) SDL_malloc(sizeof(const char *) * sdlVkExtensionCount);
  checkf(sdlVkExtensions, "Out of memory.\n");

  success = SDL_Vulkan_GetInstanceExtensions(common.window, &sdlVkExtensionCount, sdlVkExtensions);
  checkf(success, "SDL_Vulkan_GetInstanceExtensions(): %s\n", SDL_GetError());

  // require the extensions requested by SDL_vulkan
  for (uint32_t i = 0; i < sdlVkExtensionCount; ++i) {
    requiredExtensions.push_back(sdlVkExtensions[i]);
    extensionsPresent.push_back(false);
  }
  SDL_free((void *) sdlVkExtensions);
#   endif


#ifndef NDEBUG // TODO: Migrate to using debug_utils extension instead of deprecated debug_report
  requiredExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  extensionsPresent.push_back(false);
#endif

  printf("Available Vulkan extensions (*) and enabled extensions (=>): \n");

  for (uint32_t i = 0; i < extensions.size(); ++i) {
    auto &prop = extensions[i];
    bool inUse = false;
    for (uint32_t i = 0; i < requiredExtensions.size(); i++) {
      if (strcmp(prop.extensionName, requiredExtensions[i]) == 0) {
        inUse = true;
        extensionsPresent[i] = true;
      }
    }
    printf("\t%s %s\n", inUse ? "=>" : "* ", prop.extensionName);
  }

  bool allExtensionsFound = true;
  for (uint32_t i = 0; i < extensionsPresent.size(); i++) {
    allExtensionsFound &= extensionsPresent[i];
  }

  AT3_ASSERT(allExtensionsFound, "Failed to find all required vulkan extensions");

  //create instance with all extensions

  VkInstanceCreateInfo inst_info;

  //useful for driver validation and when passing as void*
  inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

  //used to pass extension info when stype is extension defined
  inst_info.pNext = NULL;
  inst_info.flags = 0;
  inst_info.pApplicationInfo = &app_info;
  inst_info.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
  inst_info.ppEnabledExtensionNames = requiredExtensions.data();

  //validation layers / other layers
  inst_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
  inst_info.ppEnabledLayerNames = validationLayers.data();

  VkResult res;
  res = vkCreateInstance(&inst_info, NULL, &common.instance);

  AT3_ASSERT(res != VK_ERROR_INCOMPATIBLE_DRIVER, "Cannot create VkInstance - driver incompatible");
  AT3_ASSERT(res == VK_SUCCESS, "Cannot create VkInstance - unknown error");


}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createDebugCallback() {
#ifndef NDEBUG
  VkDebugReportCallbackCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                     VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
  createInfo.pfnCallback = debugCallback;

  PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = VK_NULL_HANDLE;
  CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)
      vkGetInstanceProcAddr(common.instance, "vkCreateDebugReportCallbackEXT");
  CreateDebugReportCallback(common.instance, &createInfo, NULL, &callback);
#endif
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createSurface() {
#   if USE_CUSTOM_SDL_VULKAN
  bool success = SDL_CreateVulkanSurface(common.window, common.instance, &common.surface.surface);
#   else
  bool success = SDL_Vulkan_CreateSurface(common.window, common.instance, &common.surface.surface);
#   endif
  AT3_ASSERT(success, "SDL_Vulkan_CreateSurface(): %s\n", SDL_GetError());
  if (!success) {
    common.surface.surface = VK_NULL_HANDLE;
  }
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createPhysicalDevice() {
  VkPhysicalDevice &outDevice = common.gpu.device;

  uint32_t gpu_count;
  VkResult res = vkEnumeratePhysicalDevices(common.instance, &gpu_count, NULL);

  // Allocate space and get the list of devices.
  std::vector<VkPhysicalDevice> gpus;
  gpus.resize(gpu_count);

  res = vkEnumeratePhysicalDevices(common.instance, &gpu_count, gpus.data());
  AT3_ASSERT(res == VK_SUCCESS, "Could not enumerate physical devices");

  bool found = false;
  int curScore = 0;

  std::vector<const char *> deviceExtensions;
  deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  for (uint32_t i = 0; i < gpus.size(); ++i) {
    const auto &gpu = gpus[i];

    auto props = VkPhysicalDeviceProperties();
    vkGetPhysicalDeviceProperties(gpu, &props);

    if (props.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      VkPhysicalDeviceProperties deviceProperties;
      VkPhysicalDeviceFeatures deviceFeatures;
      vkGetPhysicalDeviceProperties(gpu, &deviceProperties);
      vkGetPhysicalDeviceFeatures(gpu, &deviceFeatures);

      int score = 1000;
      score += props.limits.maxImageDimension2D;
      score += props.limits.maxFragmentInputComponents;
      score += deviceFeatures.geometryShader ? 1000 : 0;
      score += deviceFeatures.tessellationShader ? 1000 : 0;

      if (!deviceFeatures.shaderSampledImageArrayDynamicIndexing) {
        continue;
      }

      //make sure the device supports presenting

      uint32_t extensionCount;
      vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, nullptr);

      std::vector<VkExtensionProperties> availableExtensions;
      availableExtensions.resize(extensionCount);
      vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, availableExtensions.data());

      uint32_t foundExtensionCount = 0;
      for (uint32_t extIdx = 0; extIdx < availableExtensions.size(); ++extIdx) {
        for (uint32_t reqIdx = 0; reqIdx < deviceExtensions.size(); ++reqIdx) {
          if (strcmp(availableExtensions[extIdx].extensionName, deviceExtensions[reqIdx]) == 0) {
            foundExtensionCount++;
          }
        }
      }

      bool supportsAllRequiredExtensions = deviceExtensions.size() == foundExtensionCount;
      if (!supportsAllRequiredExtensions) {
        continue;
      }

      //make sure the device supports at least one valid image format for our surface
      VkcSwapChainSupportInfo scSupport;
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, common.surface.surface, &scSupport.capabilities);

      uint32_t formatCount;
      vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, common.surface.surface, &formatCount, nullptr);

      if (formatCount != 0) {
        scSupport.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, common.surface.surface, &formatCount, scSupport.formats.data());
      } else {
        continue;
      }

      uint32_t presentModeCount;
      vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, common.surface.surface, &presentModeCount, nullptr);

      if (presentModeCount != 0) {
        scSupport.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, common.surface.surface, &presentModeCount,
                                                  scSupport.presentModes.data());
      }

      bool worksWithSurface = scSupport.formats.size() > 0 && scSupport.presentModes.size() > 0;

      if (score > curScore && supportsAllRequiredExtensions && worksWithSurface) {
        found = true;

        common.gpu.device = gpu;
        common.gpu.swapChainSupport = scSupport;
        curScore = score;
      }
    }
  }

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(common.gpu.device, &deviceProperties);
  printf("Selected GPU: %s\n", deviceProperties.deviceName);

  AT3_ASSERT(found, "Could not find a gpu that matches our specifications");

  vkGetPhysicalDeviceFeatures(outDevice, &common.gpu.features);
  vkGetPhysicalDeviceMemoryProperties(outDevice, &common.gpu.memProps);
  vkGetPhysicalDeviceProperties(outDevice, &common.gpu.deviceProps);
  printf("Max mem allocations: %i\n", common.gpu.deviceProps.limits.maxMemoryAllocationCount);

  //get queue families while we're here
  vkGetPhysicalDeviceQueueFamilyProperties(outDevice, &common.gpu.queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies;
  queueFamilies.resize(common.gpu.queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(outDevice, &common.gpu.queueFamilyCount, queueFamilies.data());

  std::vector<VkQueueFamilyProperties> queueVec;
  queueVec.resize(common.gpu.queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(outDevice, &common.gpu.queueFamilyCount, &queueVec[0]);


  // Iterate over each queue to learn whether it supports presenting:
  VkBool32 *pSupportsPresent = (VkBool32 *) malloc(common.gpu.queueFamilyCount * sizeof(VkBool32));
  for (uint32_t i = 0; i < common.gpu.queueFamilyCount; i++) {
    vkGetPhysicalDeviceSurfaceSupportKHR(outDevice, i, common.surface.surface, &pSupportsPresent[i]);
  }


  common.gpu.graphicsQueueFamilyIdx = INVALID_QUEUE_FAMILY_IDX;
  common.gpu.transferQueueFamilyIdx = INVALID_QUEUE_FAMILY_IDX;
  common.gpu.presentQueueFamilyIdx = INVALID_QUEUE_FAMILY_IDX;

  bool foundGfx = false;
  bool foundTransfer = false;
  bool foundPresent = false;

  for (uint32_t queueIdx = 0; queueIdx < queueFamilies.size(); ++queueIdx) {
    const auto &queueFamily = queueFamilies[queueIdx];
    const auto &queueFamily2 = queueVec[queueIdx];

    if (!foundGfx && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      common.gpu.graphicsQueueFamilyIdx = queueIdx;
      foundGfx = true;
    }

    if (!foundTransfer && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
      common.gpu.transferQueueFamilyIdx = queueIdx;
      foundTransfer = true;

    }

    if (!foundPresent && queueFamily.queueCount > 0 && pSupportsPresent[queueIdx]) {
      common.gpu.presentQueueFamilyIdx = queueIdx;
      foundPresent = true;

    }

    if (foundGfx && foundTransfer && foundPresent) break;
  }

  AT3_ASSERT(foundGfx && foundPresent && foundTransfer, "Failed to find all required device queues");
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createLogicalDevice() {
  const VkcPhysicalDevice &physDevice = common.gpu;
  VkDevice &outDevice = common.device;
  VkcDeviceQueues &outQueues = common.deviceQueues;

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::vector<uint32_t> uniqueQueueFamilies;

  uniqueQueueFamilies.push_back(physDevice.graphicsQueueFamilyIdx);
  if (physDevice.transferQueueFamilyIdx != physDevice.graphicsQueueFamilyIdx) {
    uniqueQueueFamilies.push_back(physDevice.transferQueueFamilyIdx);
  }

  if (physDevice.presentQueueFamilyIdx != physDevice.graphicsQueueFamilyIdx &&
      physDevice.presentQueueFamilyIdx != physDevice.transferQueueFamilyIdx) {
    uniqueQueueFamilies.push_back(physDevice.presentQueueFamilyIdx);
  }

  for (uint32_t familyIdx = 0; familyIdx < uniqueQueueFamilies.size(); ++familyIdx) {
    uint32_t queueFamily = uniqueQueueFamilies[familyIdx];

    VkDeviceQueueCreateInfo queueCreateInfo = {};

    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;

    float queuePriority[] = {1.0f};
    queueCreateInfo.pQueuePriorities = queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);

  }

  // we don't need anything fancy right now, but this is where you require things
  // like geo shader support

  std::vector<const char *> deviceExtensions;
  deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  std::vector<const char *> validationLayers;

#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
  validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");

  //don't do anything else here because we already know the validation layer is available,
  //else we would have asserted earlier
#endif

  if (enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  VkResult res = vkCreateDevice(physDevice.device, &createInfo, nullptr, &outDevice);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating logical device");

  vkGetDeviceQueue(outDevice, physDevice.graphicsQueueFamilyIdx, 0, &common.deviceQueues.graphicsQueue);
  vkGetDeviceQueue(outDevice, physDevice.transferQueueFamilyIdx, 0, &common.deviceQueues.transferQueue);
  vkGetDeviceQueue(outDevice, physDevice.presentQueueFamilyIdx, 0, &common.deviceQueues.presentQueue);

}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createQueryPool(int queryCount) {
  VkQueryPoolCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  createInfo.pNext = nullptr;
  createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
  createInfo.queryCount = queryCount;

  VkResult res = vkCreateQueryPool(common.device, &createInfo, nullptr, &common.queryPool);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating query pool");

}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createDescriptorPool(
    VkDescriptorPool &outPool, VulkanContextCreateInfo<EcsInterface> &info) {
  uint32_t summedDescCount = 0;
  for (auto type : info.descriptorTypeCounts) {
    summedDescCount += type.descriptorCount;
  }

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(info.descriptorTypeCounts.size());
  poolInfo.pPoolSizes = info.descriptorTypeCounts.data();
  poolInfo.maxSets = summedDescCount;

  VkResult res = vkCreateDescriptorPool(common.device, &poolInfo, nullptr, &outPool);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating descriptor pool");
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createVkSemaphore(VkSemaphore &outSemaphore) {
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkResult res = vkCreateSemaphore(common.device, &semaphoreInfo, nullptr, &outSemaphore);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating vk semaphore");
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createFence(VkFence &outFence) {
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.pNext = nullptr;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  VkResult vk_res = vkCreateFence(common.device, &fenceInfo, nullptr, &outFence);
  AT3_ASSERT(vk_res == VK_SUCCESS, "Error creating vk fence");
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createCommandPool(VkCommandPool &outPool) {
  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = common.gpu.graphicsQueueFamilyIdx;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

  VkResult res = vkCreateCommandPool(common.device, &poolInfo, nullptr, &outPool);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating command pool");
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::initGlobalShaderData() {
  static bool isInitialized = false;
  if (!isInitialized) {

    uint32_t structSize = static_cast<uint32_t>(sizeof(GlobalShaderData));
    size_t uboAlignment = common.gpu.deviceProps.limits.minUniformBufferOffsetAlignment;

    globalData.size = (uint32_t)
        ((structSize / uboAlignment) * uboAlignment + ((structSize % uboAlignment) > 0 ? uboAlignment : 0));

    createBuffer(globalData.buffer, globalData.mem, globalData.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkMapMemory(common.device, globalData.mem.handle, globalData.mem.offset, globalData.size, 0,
                &globalData.mappedMemory);

    VkSamplerCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;

    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    createInfo.anisotropyEnable = VK_FALSE;
    createInfo.maxAnisotropy = 0;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;

    //mostly for pcf?
    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = 0.0f;

    VkResult res = vkCreateSampler(common.device, &createInfo, 0, &globalData.sampler);

    AT3_ASSERT(res == VK_SUCCESS, "Error creating global sampler");

    isInitialized = true;
  }
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::setGlobalVertexLayout(std::vector<EMeshVertexAttribute> layout) {

  VertexRenderData *renderData;
  renderData = (VertexRenderData *) malloc(sizeof(VertexRenderData));

  renderData->attrCount = layout.size();
  renderData->attrDescriptions = (VkVertexInputAttributeDescription *) malloc(
      sizeof(VkVertexInputAttributeDescription) * renderData->attrCount);
  renderData->attributes = (EMeshVertexAttribute *) malloc(
      sizeof(EMeshVertexAttribute) * renderData->attrCount);
  memcpy(renderData->attributes, layout.data(), sizeof(EMeshVertexAttribute) * renderData->attrCount);

  uint32_t curOffset = 0;
  for (uint32_t i = 0; i < layout.size(); ++i) {
    switch (layout[i]) {
      case EMeshVertexAttribute::POSITION:
      case EMeshVertexAttribute::NORMAL:
      case EMeshVertexAttribute::TANGENT:
      case EMeshVertexAttribute::BITANGENT: {
        renderData->attrDescriptions[i] = {i, 0, VK_FORMAT_R32G32B32_SFLOAT, curOffset};
        curOffset += sizeof(glm::vec3);
      }
        break;

      case EMeshVertexAttribute::UV0:
      case EMeshVertexAttribute::UV1: {
        renderData->attrDescriptions[i] = {i, 0, VK_FORMAT_R32G32_SFLOAT, curOffset};
        curOffset += sizeof(glm::vec2);
      }
        break;
      case EMeshVertexAttribute::COLOR: {
        renderData->attrDescriptions[i] = {i, 0, VK_FORMAT_R32G32B32A32_SFLOAT, curOffset};
        curOffset += sizeof(glm::vec4);
      }
        break;
      default:
        AT3_ASSERT(0, "Invalid vertex attribute specified");
        break;
    }
  }
  renderData->vertexSize = curOffset;
  setVertexRenderData(renderData);
}
