#pragma once

template<typename EcsInterface>
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext<EcsInterface>::debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t sourceObject,
    size_t location,
    int32_t messageCode,
    const char *layerPrefix,
    const char *message,
    void *userData) {
  std::ostringstream out;
  out << "VULKAN ";
  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    out << "ERROR ";
  }
  if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
    out << "WARNING ";
  }
  if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
    out << "INFO ";
  }
  if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
    out << "PERFORMANCE NOTE ";
  }
  if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
    out << "DEBUG ";
  }
  out << "(Layer " << layerPrefix << ", Code " << messageCode << "): " << message << std::endl;
  fprintf(stderr, "%s", out.str().c_str());
  fflush(stderr);

  return (VkBool32) false;
}

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
VkExtent2D VulkanContext<EcsInterface>::chooseSwapExtent() {
  VkSurfaceCapabilitiesKHR capabilities = common.gpu.swapChainSupport.capabilities;
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    VkExtent2D actualExtent = {
        static_cast<uint32_t>(common.windowWidth),
        static_cast<uint32_t>(common.windowHeight)
    };
    actualExtent.width = std::max(capabilities.minImageExtent.width,
                                  std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height,
                                   std::min(capabilities.maxImageExtent.height, actualExtent.height));
    return actualExtent;
  }
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createSwapchainForSurface() {
  //choose the surface format to use
  VkSurfaceFormatKHR desiredFormat;
  VkPresentModeKHR desiredPresentMode;
  VkExtent2D swapExtent;

  VkcSwapChain &outSwapChain = common.swapChain;
  VkcPhysicalDevice &physDevice = common.gpu;
  const VkDevice &lDevice = common.device;
  const VkcSurface &surface = common.surface;

  bool foundFormat = false;

  //if there is no preferred format, the formats array only contains VK_FORMAT_UNDEFINED
  if (physDevice.swapChainSupport.formats.size() == 1 &&
      physDevice.swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED) {
    desiredFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    desiredFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
    foundFormat = true;
  }

  //otherwise we can't just choose any format we want, but still let's try to grab one that we know will work for us first
  if (!foundFormat) {
    for (uint32_t i = 0; i < physDevice.swapChainSupport.formats.size(); ++i) {
      const auto &availableFormat = physDevice.swapChainSupport.formats[i];

      if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
          availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        desiredFormat = availableFormat;
      }
    }
  }

  //if our preferred format isn't available, let's just grab the first available because yolo
  if (!foundFormat) {
    desiredFormat = physDevice.swapChainSupport.formats[0];
  }

  // Prefer triple buffering, then immediate mode. Allow forced FIFO if user wants VSYNC.
  desiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
  if (!settings::graphics::vulkan::forceFifo) {
    for (const auto &availablePresentMode : physDevice.swapChainSupport.presentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
        desiredPresentMode = availablePresentMode;
        printf("Using mailbox present mode (triple buffering).\n");
        break;
      } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        desiredPresentMode = availablePresentMode;
      }
    }
  }
  if (desiredPresentMode == VK_PRESENT_MODE_FIFO_KHR) {
    printf("Using FIFO present mode (double buffering).\n");
  } else if (desiredPresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
    printf("Using IMMEDIATE present mode.\n");
  }

  //update physdevice for new surface size
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice.device, surface.surface,
                                            &physDevice.swapChainSupport.capabilities);

  //swap extent is the resolution of the swapchain
  swapExtent = chooseSwapExtent();

  //need 1 more than minimum image count for triple buffering
  uint32_t imageCount = physDevice.swapChainSupport.capabilities.minImageCount + 1;
  if (physDevice.swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > physDevice.swapChainSupport.capabilities.maxImageCount) {
    imageCount = physDevice.swapChainSupport.capabilities.maxImageCount;
  }

  //now that everything is set up, we need to actually create the swap chain
  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface.surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = desiredFormat.format;
  createInfo.imageColorSpace = desiredFormat.colorSpace;
  createInfo.imageExtent = swapExtent;
  createInfo.imageArrayLayers = 1; //always 1 unless a stereoscopic app

  //here, we're rendering directly to the swap chain, but if we were using post processing, this might be VK_IMAGE_USAGE_TRANSFER_DST_BIT
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


  if (physDevice.graphicsQueueFamilyIdx != physDevice.presentQueueFamilyIdx) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    uint32_t queueFamilyIndices[] = {physDevice.graphicsQueueFamilyIdx, physDevice.presentQueueFamilyIdx};
    createInfo.pQueueFamilyIndices = queueFamilyIndices;

  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // Optional
    createInfo.pQueueFamilyIndices = nullptr; // Optional
  }

  createInfo.preTransform = physDevice.swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = desiredPresentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.pNext = NULL;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  VkResult res = vkCreateSwapchainKHR(lDevice, &createInfo, nullptr, &outSwapChain.swapChain);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating Vulkan Swapchain");

  //get images for swap chain
  vkGetSwapchainImagesKHR(lDevice, outSwapChain.swapChain, &imageCount, nullptr);
  outSwapChain.imageHandles.resize(imageCount);
  outSwapChain.imageViews.resize(imageCount);

  vkGetSwapchainImagesKHR(lDevice, outSwapChain.swapChain, &imageCount, &outSwapChain.imageHandles[0]);

  outSwapChain.imageFormat = desiredFormat.format;
  outSwapChain.extent = swapExtent;

  //create image views
  for (uint32_t i = 0; i < outSwapChain.imageHandles.size(); i++) {
    createImageView(outSwapChain.imageViews[i], outSwapChain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1,
                    outSwapChain.imageHandles[i], lDevice);
  }
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
void VulkanContext<EcsInterface>::getWindowSize() {
  int width, height;
  SDL_GetWindowSize(common.window, &width, &height);
  if (width <= 0 || height <= 0) {
    return;
  }
  printf("Window size is %ux%u.\n", width, height);
  common.windowWidth = width;
  common.windowHeight = height;
}


template<typename EcsInterface>
void VulkanContext<EcsInterface>::createDescriptorPool(VkDescriptorPool &outPool, const VkDevice &device,
                                                       std::vector<VkDescriptorType> &descriptorTypes,
                                                       std::vector<uint32_t> &maxDescriptors) {
  std::vector<VkDescriptorPoolSize> poolSizes;
  poolSizes.reserve(descriptorTypes.size());
  uint32_t summedDescCount = 0;

  for (uint32_t i = 0; i < descriptorTypes.size(); ++i) {
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = descriptorTypes[i];
    poolSize.descriptorCount = maxDescriptors[i];
    poolSizes.push_back(poolSize);

    summedDescCount += poolSize.descriptorCount;
  }

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = &poolSizes[0];
  poolInfo.maxSets = summedDescCount;

  VkResult res = vkCreateDescriptorPool(device, &poolInfo, nullptr, &outPool);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating descriptor pool");
}

template<typename EcsInterface>
void
VulkanContext<EcsInterface>::createImageView(VkImageView &outView, VkFormat imageFormat, VkImageAspectFlags aspectMask,
                                             uint32_t mipCount, const VkImage &imageHdl, const VkDevice &device) {
  VkImageViewCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image = imageHdl;
  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format = imageFormat;

  createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  createInfo.subresourceRange.aspectMask = aspectMask;
  createInfo.subresourceRange.baseMipLevel = 0;
  createInfo.subresourceRange.levelCount = mipCount;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount = 1;


  VkResult res = vkCreateImageView(device, &createInfo, nullptr, &outView);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating Image View");

}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createVkSemaphore(VkSemaphore &outSemaphore, const VkDevice &device) {
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkResult res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &outSemaphore);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating vk semaphore");
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createFence(VkFence &outFence, VkDevice &device) {
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.pNext = nullptr;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  VkResult vk_res = vkCreateFence(device, &fenceInfo, nullptr, &outFence);
  AT3_ASSERT(vk_res == VK_SUCCESS, "Error creating vk fence");
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createCommandPool(VkCommandPool &outPool, const VkDevice &lDevice,
                                                    const VkcPhysicalDevice &physDevice, uint32_t queueFamilyIdx) {
  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIdx;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

  VkResult res = vkCreateCommandPool(lDevice, &poolInfo, nullptr, &outPool);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating command pool");
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::freeDeviceMemory(VkcAllocation &mem) {
  //this is sorta weird
  mem.context->allocator.free(mem);
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createRenderPass(VkRenderPass &outPass,
                                                   std::vector<VkAttachmentDescription> &colorAttachments,
                                                   VkAttachmentDescription *depthAttachment, const VkDevice &device) {
  std::vector<VkAttachmentReference> attachRefs;

  std::vector<VkAttachmentDescription> allAttachments;
  allAttachments = colorAttachments;

  uint32_t attachIdx = 0;
  while (attachIdx < colorAttachments.size()) {
    VkAttachmentReference ref = {0};
    ref.attachment = attachIdx++;
    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachRefs.push_back(ref);
  }

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
  subpass.pColorAttachments = &attachRefs[0];

  VkAttachmentReference depthRef = {0};

  if (depthAttachment) {
    depthRef.attachment = attachIdx;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    subpass.pDepthStencilAttachment = &depthRef;
    allAttachments.push_back(*depthAttachment);
  }


  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(allAttachments.size());
  renderPassInfo.pAttachments = &allAttachments[0];
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  //we need a subpass dependency for transitioning the image to the right format, because by default, vulkan
  //will try to do that before we have acquired an image from our fb
  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL; //External means outside of the render pipeline, in srcPass, it means before the render pipeline
  dependency.dstSubpass = 0; //must be higher than srcSubpass
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  //add the dependency to the renderpassinfo
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;


  VkResult res = vkCreateRenderPass(device, &renderPassInfo, nullptr, &outPass);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating render pass");
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createCommandBuffer(VkCommandBuffer &outBuffer, VkCommandPool &pool,
                                                      const VkDevice &lDevice) {
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = pool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkResult res = vkAllocateCommandBuffers(lDevice, &allocInfo, &outBuffer);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating command buffer");
}

template<typename EcsInterface>
uint32_t VulkanContext<EcsInterface>::getMemoryType(const VkPhysicalDevice &device, uint32_t memoryTypeBitsRequirement,
                                                    VkMemoryPropertyFlags requiredProperties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

  //The VkPhysicalDeviceMemoryProperties structure has two arraysL memoryTypes and memoryHeaps.
  //Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM
  //for when VRAM runs out.The different types of memory exist within these heaps.Right now
  //we'll only concern ourselves with the type of memory and not the heap it comes from,
  //but you can imagine that this can affect performance.

  for (uint32_t memoryIndex = 0; memoryIndex < memProperties.memoryTypeCount; memoryIndex++) {
    const uint32_t memoryTypeBits = (1 << memoryIndex);
    const bool isRequiredMemoryType = memoryTypeBitsRequirement & memoryTypeBits;

    const VkMemoryPropertyFlags properties = memProperties.memoryTypes[memoryIndex].propertyFlags;
    const bool hasRequiredProperties = (properties & requiredProperties) == requiredProperties;

    if (isRequiredMemoryType && hasRequiredProperties) {
      return static_cast<int32_t>(memoryIndex);
    }

  }

  AT3_ASSERT(0, "Could not find a valid memory type for requiredProperties");
  return 0;
}

template<typename EcsInterface>
void
VulkanContext<EcsInterface>::createFrameBuffers(std::vector<VkFramebuffer> &outBuffers, const VkcSwapChain &swapChain,
                                                const VkImageView *depthBufferView, const VkRenderPass &renderPass,
                                                const VkDevice &device) {
  outBuffers.resize(swapChain.imageViews.size());

  for (uint32_t i = 0; i < outBuffers.size(); i++) {
    std::vector<VkImageView> attachments;
    attachments.push_back(swapChain.imageViews[i]);

    if (depthBufferView) {
      attachments.push_back(*depthBufferView);
    }

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = &attachments[0];
    framebufferInfo.width = swapChain.extent.width;
    framebufferInfo.height = swapChain.extent.height;
    framebufferInfo.layers = 1;

    VkResult r = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &outBuffers[i]);
    AT3_ASSERT(r == VK_SUCCESS, "Error creating framebuffer");
  }
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::allocateDeviceMemory(VkcAllocation &outMem, VkcAllocationCreateInfo info,
                                                       VkcCommon &ctxt) {
  ctxt.allocator.alloc(outMem, info);
}

template<typename EcsInterface>
VkcCommandBuffer VulkanContext<EcsInterface>::beginScratchCommandBuffer(VkcCmdPoolType type, VkcCommon &ctxt) {
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  if (type == VkcCmdPoolType::Graphics) {
    allocInfo.commandPool = ctxt.gfxCommandPool;
  } else if (type == VkcCmdPoolType::Transfer) {
    allocInfo.commandPool = ctxt.transferCommandPool;
  } else {
    allocInfo.commandPool = ctxt.presentCommandPool;
  }

  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(ctxt.device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkcCommandBuffer outBuf;
  outBuf.buffer = commandBuffer;
  outBuf.owningPool = type;
  outBuf.context = &ctxt;

  return outBuf;

}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::submitScratchCommandBuffer(VkcCommandBuffer &commandBuffer) {
  vkEndCommandBuffer(commandBuffer.buffer);

  AT3_ASSERT(commandBuffer.context, "Attempting to submit a scratch command buffer that does not have a valid context");
  VkcCommon &ctxt = *commandBuffer.context;

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer.buffer;

  VkQueue queue;
  VkCommandPool pool;
  if (commandBuffer.owningPool == VkcCmdPoolType::Graphics) {
    queue = ctxt.deviceQueues.graphicsQueue;
    pool = ctxt.gfxCommandPool;

  } else if (commandBuffer.owningPool == VkcCmdPoolType::Transfer) {
    queue = ctxt.deviceQueues.transferQueue;
    pool = ctxt.transferCommandPool;
  } else {
    queue = ctxt.deviceQueues.presentQueue;
    pool = ctxt.presentCommandPool;
  }


  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(ctxt.device, pool, 1, &commandBuffer.buffer);
}


template<typename EcsInterface>
void
VulkanContext<EcsInterface>::copyBuffer(VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize size, uint32_t srcOffset,
                                        uint32_t dstOffset, VkcCommandBuffer &buffer) {
  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset = srcOffset; // Optional
  copyRegion.dstOffset = dstOffset; // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(buffer.buffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

template<typename EcsInterface>
void
VulkanContext<EcsInterface>::copyBuffer(VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize size, uint32_t srcOffset,
                                        uint32_t dstOffset, VkCommandBuffer &buffer) {
  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset = srcOffset; // Optional
  copyRegion.dstOffset = dstOffset; // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(buffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

template<typename EcsInterface>
void
VulkanContext<EcsInterface>::copyBuffer(VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize size, uint32_t srcOffset,
                                        uint32_t dstOffset, VkCommandBuffer *buffer, VkcCommon &ctxt) {
  if (!buffer) {
    VkcCommandBuffer scratch = beginScratchCommandBuffer(VkcCmdPoolType::Transfer, ctxt);

    copyBuffer(srcBuffer, dstBuffer, size, srcOffset, dstOffset, scratch);

    submitScratchCommandBuffer(scratch);
  } else {
    copyBuffer(srcBuffer, dstBuffer, size, srcOffset, dstOffset, *buffer);
  }
}


//template<typename EcsInterface>
//void
//VulkanContext<EcsInterface>::createShaderModule(VkShaderModule &outModule, unsigned char *binaryData, size_t dataSize,
//                                                const VkcCommon &ctxt) {
//  VkShaderModuleCreateInfo createInfo = {};
//  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//
//  //data for vulkan is stored in uint32_t  -  so we have to temporarily copy it to a container that respects that alignment
//
//  std::vector<uint32_t> codeAligned;
//  codeAligned.resize((uint32_t) (dataSize / sizeof(uint32_t) + 1));
//
//  memcpy(&codeAligned[0], binaryData, dataSize);
//  createInfo.pCode = &codeAligned[0];
//  createInfo.codeSize = dataSize;
//
//  AT3_ASSERT(dataSize % 4 == 0, "Invalid data size for .spv file -> are you sure that it compiled correctly?");
//
//  VkResult res = vkCreateShaderModule(ctxt.device, &createInfo, nullptr, &outModule);
//  AT3_ASSERT(res == VK_SUCCESS, "Error creating shader module");
//
//}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createBuffer(VkBuffer &outBuffer, VkcAllocation &bufferMemory, VkDeviceSize size,
                                               VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                                               VkcCommon &ctxt) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;

  //concurrent so it can be used by the graphics and transfer queues

  std::vector<uint32_t> queues;
  queues.push_back(ctxt.gpu.graphicsQueueFamilyIdx);

  if (ctxt.gpu.graphicsQueueFamilyIdx != ctxt.gpu.transferQueueFamilyIdx) {
    queues.push_back(ctxt.gpu.transferQueueFamilyIdx);
    bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
  } else bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


  bufferInfo.pQueueFamilyIndices = &queues[0];
  bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(queues.size());

  VkResult res = vkCreateBuffer(ctxt.device, &bufferInfo, nullptr, &outBuffer);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating buffer");

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(ctxt.device, outBuffer, &memRequirements);

  VkcAllocationCreateInfo allocInfo = {};
  allocInfo.size = memRequirements.size;
  allocInfo.memoryTypeIndex = getMemoryType(ctxt.gpu.device, memRequirements.memoryTypeBits, properties);
  allocInfo.usage = properties;

  ctxt.allocator.alloc(bufferMemory, allocInfo);
  vkBindBufferMemory(ctxt.device, outBuffer, bufferMemory.handle, bufferMemory.offset);
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::copyDataToBuffer(VkBuffer *buffer, uint32_t dataSize, uint32_t dstOffset, char *data,
                                                   VkcCommon &ctxt) {
  VkBuffer stagingBuffer;
  VkcAllocation stagingMemory;

  createBuffer(stagingBuffer,
               stagingMemory,
               dataSize,
               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               ctxt);

  void *mappedStagingBuffer;
  vkMapMemory(ctxt.device, stagingMemory.handle, stagingMemory.offset, dataSize, 0, &mappedStagingBuffer);

  memset(mappedStagingBuffer, 0, dataSize);
  memcpy(mappedStagingBuffer, data, dataSize);

  vkUnmapMemory(ctxt.device, stagingMemory.handle);

  VkcCommandBuffer scratch = beginScratchCommandBuffer(VkcCmdPoolType::Transfer, ctxt);
  copyBuffer(stagingBuffer, *buffer, dataSize, 0, dstOffset, scratch);
  submitScratchCommandBuffer(scratch);

  vkDestroyBuffer(ctxt.device, stagingBuffer, 0);
  freeDeviceMemory(stagingMemory);

}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createImage(VkImage &outImage, uint32_t width, uint32_t height, VkFormat format,
                                              VkImageTiling tiling, VkImageUsageFlags usage, const VkcCommon &ctxt) {
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult res = vkCreateImage(ctxt.device, &imageInfo, nullptr, &outImage);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating vk image");
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,
                                                    VkcCommon &ctxt) {
  VkcCommandBuffer commandBuffer = beginScratchCommandBuffer(VkcCmdPoolType::Transfer, ctxt);

  VkBufferImageCopy region = {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};


  vkCmdCopyBufferToImage(
      commandBuffer.buffer,
      buffer,
      image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region
  );

  submitScratchCommandBuffer(commandBuffer);


}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
                                                        VkImageLayout newLayout, VkcCommon &ctxt) {
  VkcCommandBuffer commandBuffer = beginScratchCommandBuffer(VkcCmdPoolType::Graphics, ctxt);

  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;

  //these are used to transfer queue ownership, which we aren't doing
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

  } else {
    //Unsupported layout transition
    AT3_ASSERT(0, "Attempting an unsupported image layout transition");
  }

  vkCmdPipelineBarrier(
      commandBuffer.buffer,
      sourceStage, destinationStage,
      0,
      0, nullptr,
      0, nullptr,
      1, &barrier
  );


  submitScratchCommandBuffer(commandBuffer);

}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::allocMemoryForImage(VkcAllocation &outMem, const VkImage &image,
                                                      VkMemoryPropertyFlags properties, VkcCommon &ctxt) {
  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(ctxt.device, image, &memRequirements);

  VkcAllocationCreateInfo createInfo;
  createInfo.size = memRequirements.size;
  createInfo.memoryTypeIndex = getMemoryType(ctxt.gpu.device, memRequirements.memoryTypeBits, properties);
  createInfo.usage = properties;
  allocateDeviceMemory(outMem, createInfo, ctxt);
}


template<typename EcsInterface>
void VulkanContext<EcsInterface>::createBuffer(VkBuffer &outBuffer, VkcAllocation &bufferMemory, VkDeviceSize size,
                                               VkBufferUsageFlags usage,
                                               VkMemoryPropertyFlags properties, const VkcCommon &ctxt) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;

  //concurrent so it can be used by the graphics and transfer queues

  std::vector<uint32_t> queues;
  queues.push_back(ctxt.gpu.graphicsQueueFamilyIdx);

  if (ctxt.gpu.graphicsQueueFamilyIdx != ctxt.gpu.transferQueueFamilyIdx) {
    queues.push_back(ctxt.gpu.transferQueueFamilyIdx);
    bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
  } else bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  bufferInfo.pQueueFamilyIndices = &queues[0];
  bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(queues.size());

  VkResult res = vkCreateBuffer(ctxt.device, &bufferInfo, nullptr, &outBuffer);
  AT3_ASSERT(res == VK_SUCCESS, "Error creating Buffer");

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(ctxt.device, outBuffer, &memRequirements);

  VkcAllocationCreateInfo allocInfo = {};
  allocInfo.size = memRequirements.size;
  allocInfo.memoryTypeIndex = getMemoryType(ctxt.gpu.device, memRequirements.memoryTypeBits, properties);
  allocInfo.usage = properties;

  ctxt.allocator.alloc(bufferMemory, allocInfo);
  vkBindBufferMemory(ctxt.device, outBuffer, bufferMemory.handle, bufferMemory.offset);
}


template<typename EcsInterface>
void VulkanContext<EcsInterface>::initRendering(VkcCommon &ctxt, uint32_t num) {
  createMainRenderPass(ctxt);
  createDepthBuffer(ctxt);

  createFrameBuffers(ctxt.renderData.frameBuffers, ctxt.swapChain, &ctxt.renderData.depthBuffer.view,
                     ctxt.renderData.mainRenderPass,
                     ctxt.device);

  uint32_t swapChainImageCount = static_cast<uint32_t>(ctxt.swapChain.imageViews.size());
  ctxt.renderData.commandBuffers.resize(swapChainImageCount);
  for (uint32_t i = 0; i < swapChainImageCount; ++i) {
    createCommandBuffer(ctxt.renderData.commandBuffers[i], ctxt.gfxCommandPool, ctxt.device);
  }
  loadUBOTestMaterial(ctxt, num);

  ctxt.renderData.firstFrame = std::vector<bool>(swapChainImageCount, true);
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::updateDescriptorSets(VkcCommon &ctxt, DataStore *dataStore) {

  size_t oldNumPages = ctxt.matData.descSets.size();
  size_t newNumPages = dataStore->getNumPages();

  ctxt.matData.descSets.resize(newNumPages);

  for (size_t i = oldNumPages; i < newNumPages; ++i) {
    VkDescriptorSetAllocateInfo allocInfo = descriptorSetAllocateInfo(&ctxt.matData.descSetLayout, 1,
                                                                      ctxt.descriptorPool);
    VkResult res = vkAllocateDescriptorSets(ctxt.device, &allocInfo, &ctxt.matData.descSets[i]);
    AT3_ASSERT(res == VK_SUCCESS, "Error allocating global descriptor set");

    ctxt.bufferInfo = {};
    ctxt.bufferInfo.buffer = dataStore->getPage(i);
    ctxt.bufferInfo.offset = 0;
    ctxt.bufferInfo.range = VK_WHOLE_SIZE;

    ctxt.setWrite = {};
    ctxt.setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    ctxt.setWrite.dstBinding = 0;
    ctxt.setWrite.dstArrayElement = 0;
    ctxt.setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ctxt.setWrite.descriptorCount = 1;
    ctxt.setWrite.dstSet = ctxt.matData.descSets[i];
    ctxt.setWrite.pBufferInfo = &ctxt.bufferInfo;
    ctxt.setWrite.pImageInfo = 0;

    vkUpdateDescriptorSets(ctxt.device, 1, &ctxt.setWrite, 0, nullptr);
  }
}

//template <typename EcsInterface>
//void VulkanContext<EcsInterface>::loadUBOTestMaterial(VkcCommon &ctxt, int num) {
//  //create descriptor set layout
//  VkDescriptorSetLayoutBinding layoutBinding;
//  layoutBinding = descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1);
//
//  VkDescriptorSetLayoutCreateInfo layoutInfo = descriptorSetLayoutCreateInfo(&layoutBinding, 1);
//
//  VkResult res = vkCreateDescriptorSetLayout(ctxt.device, &layoutInfo, nullptr,
//                                             &ctxt.matData.descSetLayout);
//  AT3_ASSERT(res == VK_SUCCESS, "Error creating desc set layout");
//
//
//  VkhMaterialCreateInfo createInfo = {};
//  createInfo.renderPass = ctxt.renderData.mainRenderPass;
//  createInfo.outPipeline = &ctxt.matData.graphicsPipeline;
//  createInfo.outPipelineLayout = &ctxt.matData.pipelineLayout;
//
//  createInfo.pushConstantStages = VK_SHADER_STAGE_VERTEX_BIT;
//  createInfo.pushConstantRange = sizeof(uint32_t);
//  createInfo.descSetLayouts.push_back(ctxt.matData.descSetLayout);
//
//  createBasicMaterial(ubo_array_vert_spv, ubo_array_vert_spv_len,
//                           static_sun_frag_spv, static_sun_frag_spv_len, ctxt, createInfo);
//
//}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createDepthBuffer(VkcCommon &ctxt) {
  createImage(ctxt.renderData.depthBuffer.handle,
              ctxt.swapChain.extent.width,
              ctxt.swapChain.extent.height,
              VK_FORMAT_D32_SFLOAT,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
              ctxt);

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(ctxt.device, ctxt.renderData.depthBuffer.handle, &memRequirements);

  VkcAllocationCreateInfo createInfo;
  createInfo.size = memRequirements.size;
  createInfo.memoryTypeIndex = getMemoryType(ctxt.gpu.device, memRequirements.memoryTypeBits,
                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  createInfo.usage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  allocateDeviceMemory(ctxt.renderData.depthBuffer.imageMemory, createInfo, ctxt);

  vkBindImageMemory(ctxt.device, ctxt.renderData.depthBuffer.handle, ctxt.renderData.depthBuffer.imageMemory.handle,
                    ctxt.renderData.depthBuffer.imageMemory.offset);

  createImageView(ctxt.renderData.depthBuffer.view, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, 1,
                  ctxt.renderData.depthBuffer.handle, ctxt.device);

  transitionImageLayout(ctxt.renderData.depthBuffer.handle, VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, ctxt);
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::createMainRenderPass(VkcCommon &ctxt) {
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = ctxt.swapChain.imageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depthAttachment = {};
  depthAttachment.format = VK_FORMAT_D32_SFLOAT;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


  std::vector<VkAttachmentDescription> renderPassAttachments;
  renderPassAttachments.push_back(colorAttachment);

  createRenderPass(ctxt.renderData.mainRenderPass, renderPassAttachments, &depthAttachment, ctxt.device);

}


template<typename EcsInterface>
void VulkanContext<EcsInterface>::makeTexture(VkcTextureResource &outAsset, const char *filepath, VkcCommon &ctxt) {
  VkcTextureResource &t = outAsset;

  int texWidth, texHeight, texChannels;

  //STBI_rgb_alpha forces an alpha even if the image doesn't have one
  stbi_uc *pixels = stbi_load(filepath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

  AT3_ASSERT(pixels, "Could not load image");

  VkDeviceSize imageSize = texWidth * texHeight * 4;

  VkBuffer stagingBuffer;
  VkcAllocation stagingBufferMemory;

  createBuffer(stagingBuffer,
               stagingBufferMemory,
               imageSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               ctxt);

  void *data;
  vkMapMemory(ctxt.device, stagingBufferMemory.handle, stagingBufferMemory.offset, imageSize, 0, &data);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(ctxt.device, stagingBufferMemory.handle);

  stbi_image_free(pixels);

  t.width = texWidth;
  t.height = texHeight;
  t.numChannels = texChannels;
  t.format = VK_FORMAT_R8G8B8A8_UNORM;

  //VK image format must match buffer
  createImage(t.image,
              t.width, t.height,
              VK_FORMAT_R8G8B8A8_UNORM,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              ctxt);

  allocMemoryForImage(t.deviceMemory, t.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ctxt);
  vkBindImageMemory(ctxt.device, t.image, t.deviceMemory.handle, t.deviceMemory.offset);

  transitionImageLayout(t.image, t.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        ctxt);
  copyBufferToImage(stagingBuffer, t.image, t.width, t.height, ctxt);

  transitionImageLayout(t.image, t.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, ctxt);

  createImageView(t.view, t.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, t.image, ctxt.device);

  vkDestroyBuffer(ctxt.device, stagingBuffer, nullptr);
  freeDeviceMemory(stagingBufferMemory);
}

template<typename EcsInterface>
void VulkanContext<EcsInterface>::initGlobalShaderData(VkcCommon &ctxt) {
  static bool isInitialized = false;
  if (!isInitialized) {

    uint32_t structSize = static_cast<uint32_t>(sizeof(GlobalShaderData));
    size_t uboAlignment = ctxt.gpu.deviceProps.limits.minUniformBufferOffsetAlignment;

    globalData.size = (structSize / uboAlignment) * uboAlignment + ((structSize % uboAlignment) > 0 ? uboAlignment : 0);

    createBuffer(
        globalData.buffer,
        globalData.mem,
        globalData.size,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        ctxt);

    vkMapMemory(ctxt.device, globalData.mem.handle, globalData.mem.offset, globalData.size, 0,
                &globalData.mappedMemory);

    VkSamplerCreateInfo createInfo = samplerCreateInfo(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                                       VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f);
    VkResult res = vkCreateSampler(ctxt.device, &createInfo, 0, &globalData.sampler);


    AT3_ASSERT(res == VK_SUCCESS, "Error creating global sampler");

    isInitialized = true;

  }
}

//template<typename EcsInterface>
//void VulkanContext<EcsInterface>::createBasicMaterial(
//    unsigned char *vertData, unsigned int vertLen, unsigned char *fragData, unsigned int fragLen, VkcCommon &ctxt,
//    VkhMaterialCreateInfo &createInfo) {
//
//  VkPipelineShaderStageCreateInfo shaderStages[2];
//
//  shaderStages[0] = shaderPipelineStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT);
//  createShaderModule(shaderStages[0].module, vertData, vertLen, ctxt);
//
//  shaderStages[1] = shaderPipelineStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT);
//  createShaderModule(shaderStages[1].module, fragData, fragLen, ctxt);
//
//  VkPipelineLayoutCreateInfo pipelineLayoutInfo = pipelineLayoutCreateInfo(createInfo.descSetLayouts.data(),
//                                                                           createInfo.descSetLayouts.size());
//
//  VkPushConstantRange pushConstantRange = {};
//  pushConstantRange.offset = 0;
//  pushConstantRange.size = createInfo.pushConstantRange;
//  pushConstantRange.stageFlags = createInfo.pushConstantStages;
//
//  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
//  pipelineLayoutInfo.pushConstantRangeCount = 1;
//
//  VkResult res = vkCreatePipelineLayout(ctxt.device, &pipelineLayoutInfo, nullptr, createInfo.outPipelineLayout);
//  AT3_ASSERT(res == VK_SUCCESS, "Error creating pipeline layout");
//
//  const VertexRenderData *vertexLayout = vertexRenderData();
//
//  VkVertexInputBindingDescription bindingDescription = vertexInputBindingDescription(0, vertexLayout->vertexSize,
//                                                                                     VK_VERTEX_INPUT_RATE_VERTEX);
//
//  VkPipelineVertexInputStateCreateInfo vertexInputInfo = pipelineVertexInputStateCreateInfo();
//  vertexInputInfo.vertexBindingDescriptionCount = 1;
//  vertexInputInfo.vertexAttributeDescriptionCount = vertexLayout->attrCount;
//  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
//  vertexInputInfo.pVertexAttributeDescriptions = &vertexLayout->attrDescriptions[0];
//
//  VkPipelineInputAssemblyStateCreateInfo inputAssembly = pipelineInputAssemblyStateCreateInfo(
//      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
//  VkViewport vp = viewport(0, 0, static_cast<float>(ctxt.swapChain.extent.width),
//                           static_cast<float>(ctxt.swapChain.extent.height), 0.0f, 1.0f);
//  VkRect2D scissor = rect2D(0, 0, ctxt.swapChain.extent.width, ctxt.swapChain.extent.height);
//  VkPipelineViewportStateCreateInfo viewportState = pipelineViewportStateCreateInfo(&vp, 1, &scissor, 1);
//
//  VkPipelineRasterizationStateCreateInfo rasterizer = pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
//  VkPipelineMultisampleStateCreateInfo multisampling = pipelineMultisampleStateCreateInfo();
//
//  VkPipelineColorBlendAttachmentState colorBlendAttachment = pipelineColorBlendAttachmentState(
//      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
//      VK_FALSE);
//  VkPipelineColorBlendStateCreateInfo colorBlending = pipelineColorBlendStateCreateInfo(colorBlendAttachment);
//
//  VkPipelineDepthStencilStateCreateInfo depthStencil = pipelineDepthStencilStateCreateInfo(
//      VK_TRUE,
//      VK_TRUE,
//      VK_COMPARE_OP_LESS_OR_EQUAL);
//
//  VkGraphicsPipelineCreateInfo pipelineInfo = {};
//  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//  pipelineInfo.stageCount = 2;
//  pipelineInfo.pStages = shaderStages;
//  pipelineInfo.pVertexInputState = &vertexInputInfo;
//  pipelineInfo.pInputAssemblyState = &inputAssembly;
//  pipelineInfo.pViewportState = &viewportState;
//  pipelineInfo.pRasterizationState = &rasterizer;
//  pipelineInfo.pMultisampleState = &multisampling;
//  pipelineInfo.pColorBlendState = &colorBlending;
//  pipelineInfo.pDynamicState = nullptr; // Optional
//  pipelineInfo.layout = *createInfo.outPipelineLayout;
//  pipelineInfo.renderPass = createInfo.renderPass;
//  pipelineInfo.pDepthStencilState = &depthStencil;
//
//  pipelineInfo.subpass = 0;
//
//  //can use this to create new pipelines by deriving from old ones
//  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
//  pipelineInfo.basePipelineIndex = -1;
//
//  res = vkCreateGraphicsPipelines(ctxt.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, createInfo.outPipeline);
//  AT3_ASSERT(res == VK_SUCCESS, "Error creating graphics pipeline");
//
//  vkDestroyShaderModule(ctxt.device, shaderStages[0].module, nullptr);
//  vkDestroyShaderModule(ctxt.device, shaderStages[1].module, nullptr);
//
//}
//
//template<typename EcsInterface>
//void VulkanContext<EcsInterface>::loadUBOTestMaterial(VkcCommon &ctxt, int num) {
//  //create descriptor set layout
//  VkDescriptorSetLayoutBinding layoutBinding;
//  layoutBinding = descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1);
//
//  VkDescriptorSetLayoutCreateInfo layoutInfo = descriptorSetLayoutCreateInfo(&layoutBinding, 1);
//
//  VkResult res = vkCreateDescriptorSetLayout(ctxt.device, &layoutInfo, nullptr,
//                                             &ctxt.matData.descSetLayout);
//  AT3_ASSERT(res == VK_SUCCESS, "Error creating desc set layout");
//
//
//  VkhMaterialCreateInfo createInfo = {};
//  createInfo.renderPass = ctxt.renderData.mainRenderPass;
//  createInfo.outPipeline = &ctxt.matData.graphicsPipeline;
//  createInfo.outPipelineLayout = &ctxt.matData.pipelineLayout;
//
//  createInfo.pushConstantStages = VK_SHADER_STAGE_VERTEX_BIT;
//  createInfo.pushConstantRange = sizeof(uint32_t);
//  createInfo.descSetLayouts.push_back(ctxt.matData.descSetLayout);
//
//  createBasicMaterial(ubo_array_vert_spv, ubo_array_vert_spv_len,
//                      static_sun_frag_spv, static_sun_frag_spv_len, ctxt, createInfo);
//
//}

template<typename EcsInterface>
MeshResources<EcsInterface>
VulkanContext<EcsInterface>::loadMesh(const char *filepath, bool combineSubMeshes, VkcCommon &ctxt) {

  std::vector<MeshResource<EcsInterface>> outMeshes;

  const VertexRenderData *globalVertLayout = vertexRenderData();

  Assimp::Importer aiImporter;
  const struct aiScene *scene = NULL;

  struct aiLogStream stream;
  stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
  aiAttachLogStream(&stream);

  scene = aiImporter.ReadFile(filepath, MESH_FLAGS);

  const aiVector3D ZeroVector(0.0, 0.0, 0.0);
  const aiColor4D ZeroColor(0.0, 0.0, 0.0, 0.0);

  if (scene) {
    uint32_t floatsPerVert = globalVertLayout->vertexSize / sizeof(float);
    std::vector<float> vertexBuffer;
    std::vector<uint32_t> indexBuffer;
    uint32_t numVerts = 0;
    uint32_t numFaces = 0;

    outMeshes.resize(combineSubMeshes ? 1 : scene->mNumMeshes);

    for (uint32_t mIdx = 0; mIdx < scene->mNumMeshes; mIdx++) {
      if (!combineSubMeshes) {
        vertexBuffer.clear();
        indexBuffer.clear();
        numVerts = 0;
        numFaces = 0;
      }

      const aiMesh *mesh = scene->mMeshes[mIdx];

      for (uint32_t vIdx = 0; vIdx < mesh->mNumVertices; ++vIdx) {

        const aiVector3D *pos = &(mesh->mVertices[vIdx]);
        const aiVector3D *nrm = &(mesh->mNormals[vIdx]);
        const aiVector3D *uv0 = mesh->HasTextureCoords(0) ? &(mesh->mTextureCoords[0][vIdx]) : &ZeroVector;
        const aiVector3D *uv1 = mesh->HasTextureCoords(1) ? &(mesh->mTextureCoords[1][vIdx]) : &ZeroVector;
        const aiVector3D *tan = mesh->HasTangentsAndBitangents() ? &(mesh->mTangents[vIdx]) : &ZeroVector;
        const aiVector3D *biTan = mesh->HasTangentsAndBitangents() ? &(mesh->mBitangents[vIdx]) : &ZeroVector;
        const aiColor4D *col = mesh->HasVertexColors(0) ? &(mesh->mColors[0][vIdx]) : &ZeroColor;

        for (uint32_t lIdx = 0; lIdx < globalVertLayout->attrCount; ++lIdx) {
          EMeshVertexAttribute comp = globalVertLayout->attributes[lIdx];

          switch (comp) {
            case EMeshVertexAttribute::POSITION: {
              vertexBuffer.push_back(pos->x);
              vertexBuffer.push_back(pos->y);
              vertexBuffer.push_back(pos->z);
            };
              break;
            case EMeshVertexAttribute::NORMAL: {
              vertexBuffer.push_back(nrm->x);
              vertexBuffer.push_back(nrm->y);
              vertexBuffer.push_back(nrm->z);
            };
              break;
            case EMeshVertexAttribute::UV0: {
              vertexBuffer.push_back(uv0->x);
              vertexBuffer.push_back(uv0->y);
            };
              break;
            case EMeshVertexAttribute::UV1: {
              vertexBuffer.push_back(uv1->x);
              vertexBuffer.push_back(uv1->y);
            };
              break;
            case EMeshVertexAttribute::TANGENT: {
              vertexBuffer.push_back(tan->x);
              vertexBuffer.push_back(tan->y);
              vertexBuffer.push_back(tan->z);
            };
              break;
            case EMeshVertexAttribute::BITANGENT: {
              vertexBuffer.push_back(biTan->x);
              vertexBuffer.push_back(biTan->y);
              vertexBuffer.push_back(biTan->z);
            };
              break;

            case EMeshVertexAttribute::COLOR: {
              vertexBuffer.push_back(col->r);
              vertexBuffer.push_back(col->g);
              vertexBuffer.push_back(col->b);
              vertexBuffer.push_back(col->a);
            };
              break;
          }

        }
      }

      for (unsigned int fIdx = 0; fIdx < mesh->mNumFaces; fIdx++) {
        const aiFace &face = mesh->mFaces[fIdx];
        AT3_ASSERT(face.mNumIndices == 3, "unsupported number of indices in mesh face");

        indexBuffer.push_back(numVerts + face.mIndices[0]);
        indexBuffer.push_back(numVerts + face.mIndices[1]);
        indexBuffer.push_back(numVerts + face.mIndices[2]);
      }

      numVerts += mesh->mNumVertices;
      numFaces += mesh->mNumFaces;

      if (!combineSubMeshes) {
        make(outMeshes[mIdx], ctxt, vertexBuffer.data(), numVerts, indexBuffer.data(), indexBuffer.size());
      }
    }

    if (combineSubMeshes) {
      make(outMeshes[0], ctxt, vertexBuffer.data(), numVerts, indexBuffer.data(), indexBuffer.size());
    }
  }

  aiDetachAllLogStreams();

  return outMeshes;
}

//template<typename EcsInterface>
//const VertexRenderData *VulkanContext<EcsInterface>::vertexRenderData() {
//  AT3_ASSERT(_vkRenderData,
//             "Attempting to get global vertex layout, but it has not been set yet, call setGlobalVertexLayout first.");
//
//  return _vkRenderData;
//}

template<typename EcsInterface>
uint32_t VulkanContext<EcsInterface>::make(MeshResource <EcsInterface> &outAsset, VkcCommon &ctxt, float *vertices,
                                           uint32_t vertexCount,
                                           uint32_t *indices, uint32_t indexCount) {
  size_t vBufferSize = vertexRenderData()->vertexSize * vertexCount;
  size_t iBufferSize = sizeof(uint32_t) * indexCount;

  MeshResource<EcsInterface> &m = outAsset;
  m.iCount = indexCount;
  m.vCount = vertexCount;

  createBuffer(m.buffer,
               m.bufferMemory,
               vBufferSize + iBufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
               VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               ctxt
  );

  //transfer data to the above buffers
  VkBuffer stagingBuffer;
  VkcAllocation stagingMemory;

  createBuffer(stagingBuffer,
               stagingMemory,
               vBufferSize + iBufferSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               ctxt
  );

  void *data;

  vkMapMemory(ctxt.device, stagingMemory.handle, stagingMemory.offset, vBufferSize, 0, &data);
  memcpy(data, vertices, (size_t) vBufferSize);
  vkUnmapMemory(ctxt.device, stagingMemory.handle);

  vkMapMemory(ctxt.device, stagingMemory.handle, stagingMemory.offset + vBufferSize, iBufferSize, 0, &data);
  memcpy(data, indices, (size_t) iBufferSize);
  vkUnmapMemory(ctxt.device, stagingMemory.handle);

  //copy to device local here
  copyBuffer(stagingBuffer, m.buffer, vBufferSize + iBufferSize, 0, 0, nullptr, ctxt);
  freeDeviceMemory(stagingMemory);
  vkDestroyBuffer(ctxt.device, stagingBuffer, nullptr);

  m.vOffset = 0;
  m.iOffset = vBufferSize;

  return 0;

}

template<typename EcsInterface>
void
VulkanContext<EcsInterface>::quad(MeshResource <EcsInterface> &outAsset, VkcCommon &ctxt, float width, float height,
                                  float xOffset,
                                  float yOffset) {
  const VertexRenderData *vertexData = vertexRenderData();

  std::vector<float> verts;

  float wComp = width / 2.0f;
  float hComp = height / 2.0f;

  const glm::vec3 lbCorner = glm::vec3(-wComp + xOffset, -hComp + yOffset, 0.0f);
  const glm::vec3 ltCorner = glm::vec3(lbCorner.x, hComp + yOffset, 0.0f);
  const glm::vec3 rbCorner = glm::vec3(wComp + xOffset, lbCorner.y, 0.0f);
  const glm::vec3 rtCorner = glm::vec3(rbCorner.x, ltCorner.y, 0.0f);

  const glm::vec3 pos[4] = {rtCorner, ltCorner, lbCorner, rbCorner};
  const glm::vec2 uv[4] = {glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, 0.0f),
                           glm::vec2(1.0f, 0.0f)};


  static uint32_t indices[6] = {0, 2, 1, 2, 0, 3};
  uint32_t curIdx = 0;

  for (uint32_t i = 0; i < 4; ++i) {

    for (uint32_t j = 0; j < vertexData->attrCount; ++j) {
      EMeshVertexAttribute attrib = vertexData->attributes[j];

      switch (attrib) {
        case EMeshVertexAttribute::POSITION: {
          verts.push_back(pos[i].x);
          verts.push_back(pos[i].y);
          verts.push_back(pos[i].z);
        }
          break;
        case EMeshVertexAttribute::NORMAL:
        case EMeshVertexAttribute::TANGENT:
        case EMeshVertexAttribute::BITANGENT: {
          verts.push_back(0);
          verts.push_back(0);
          verts.push_back(0);
        }
          break;

        case EMeshVertexAttribute::UV0: {
          verts.push_back(uv[i].x);
          verts.push_back(uv[i].y);
        }
          break;
        case EMeshVertexAttribute::UV1: {
          verts.push_back(0);
          verts.push_back(0);
        }
          break;
        case EMeshVertexAttribute::COLOR: {
          verts.push_back(0);
          verts.push_back(0);
          verts.push_back(0);
          verts.push_back(0);

        }
          break;
        default:
          AT3_ASSERT(0, "Invalid vertex attribute specified");
          break;

      }
    }
  }
  make(outAsset, ctxt, verts.data(), 4, &indices[0], 6);
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


template<typename EcsInterface>
void VulkanContext<EcsInterface>::cleanupRendering() {
  vkDestroyImageView(common.device, common.renderData.depthBuffer.view, nullptr);
  vkDestroyImage(common.device, common.renderData.depthBuffer.handle, nullptr);
  allocators::pool::free(common.renderData.depthBuffer.imageMemory);

  for (size_t i = 0; i < common.renderData.frameBuffers.size(); i++) {
    vkDestroyFramebuffer(common.device, common.renderData.frameBuffers[i], nullptr);
  }

  vkFreeCommandBuffers(common.device, common.gfxCommandPool,
                       static_cast<uint32_t>(common.renderData.commandBuffers.size()),
                       common.renderData.commandBuffers.data());

  vkDestroyPipeline(common.device, common.matData.graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(common.device, common.matData.pipelineLayout, nullptr);
  vkDestroyRenderPass(common.device, common.renderData.mainRenderPass, nullptr);

  for (size_t i = 0; i < common.swapChain.imageViews.size(); i++) {
    vkDestroyImageView(common.device, common.swapChain.imageViews[i], nullptr);
  }

  vkDestroySwapchainKHR(common.device, common.swapChain.swapChain, nullptr);
}


template<typename EcsInterface>
void VulkanContext<EcsInterface>::render(
    VkcCommon &ctxt, DataStore *dataStore, const glm::mat4 &wvMat, const MeshRepository<EcsInterface> &meshAssets,
    EcsInterface *ecs) {

  glm::mat4 proj = glm::perspective(glm::radians(60.f), ctxt.windowWidth / (float) ctxt.windowHeight, 0.1f, 10000.f);
  // reverse the y
  proj[1][1] *= -1;

#if !COPY_ON_MAIN_COMMANDBUFFER
//  ubo_store::updateBuffers(wvMat, proj, nullptr, ctxt);
  dataStore->updateBuffers(wvMat, proj, nullptr, ctxt, ecs, meshAssets);
#endif

  VkResult res;

  //acquire an image from the swap chain
  uint32_t imageIndex;

  res = vkAcquireNextImageKHR(ctxt.device, ctxt.swapChain.swapChain, UINT64_MAX,
                              ctxt.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

  if (res == VK_ERROR_OUT_OF_DATE_KHR) {
    rtu::topics::publish("window_resized");
    return;
  } else {
    AT3_ASSERT(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR, "failed to acquire swap chain image!");
  }

  vkWaitForFences(ctxt.device, 1, &ctxt.frameFences[imageIndex], VK_FALSE, 5000000000);
  vkResetFences(ctxt.device, 1, &ctxt.frameFences[imageIndex]);

  //record drawing
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = nullptr; // Optional

  vkResetCommandBuffer(ctxt.renderData.commandBuffers[imageIndex], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  res = vkBeginCommandBuffer(ctxt.renderData.commandBuffers[imageIndex], &beginInfo);

#if COPY_ON_MAIN_COMMANDBUFFER
  dataStore->updateBuffers(view, proj, &ctxt.renderData.commandBuffers[imageIndex], ctxt);
#endif


  vkCmdResetQueryPool(ctxt.renderData.commandBuffers[imageIndex], ctxt.queryPool, 0, 10);
  vkCmdWriteTimestamp(ctxt.renderData.commandBuffers[imageIndex], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                      ctxt.queryPool, 0);


  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = ctxt.renderData.mainRenderPass;
  renderPassInfo.framebuffer = ctxt.renderData.frameBuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = ctxt.swapChain.extent;

  VkClearValue clearColors[2];
  clearColors[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
  clearColors[1].depthStencil.depth = 1.0f;
  clearColors[1].depthStencil.stencil = 0;

  renderPassInfo.clearValueCount = static_cast<uint32_t>(2);
  renderPassInfo.pClearValues = &clearColors[0];

  vkCmdBeginRenderPass(ctxt.renderData.commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  int currentlyBound = -1;

  vkCmdBindPipeline(ctxt.renderData.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
                    ctxt.matData.graphicsPipeline);

  for (auto pair : meshAssets) {
    for (auto mesh : pair.second) {
      for (auto instance : mesh.instances) {
        glm::uint32 uboSlot = instance.uboIdx >> 16;
        glm::uint32 uboPage = instance.uboIdx & 0xFFFF;

        if (currentlyBound != uboPage) {
          vkCmdBindDescriptorSets(ctxt.renderData.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  ctxt.matData.pipelineLayout, 0, 1, &ctxt.matData.descSets[uboPage], 0, nullptr);
          currentlyBound = uboPage;
        }

        vkCmdPushConstants(
            ctxt.renderData.commandBuffers[imageIndex],
            ctxt.matData.pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(glm::uint32),
            (void *) &uboSlot);

        VkBuffer vertexBuffers[] = {mesh.buffer};
        VkDeviceSize vertexOffsets[] = {0};
        vkCmdBindVertexBuffers(ctxt.renderData.commandBuffers[imageIndex], 0, 1, vertexBuffers, vertexOffsets);
        vkCmdBindIndexBuffer(ctxt.renderData.commandBuffers[imageIndex], mesh.buffer, mesh.iOffset,
                             VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(ctxt.renderData.commandBuffers[imageIndex], static_cast<uint32_t>(mesh.iCount), 1, 0, 0,
                         0);
      }
    }
  }

  vkCmdEndRenderPass(ctxt.renderData.commandBuffers[imageIndex]);
  vkCmdWriteTimestamp(ctxt.renderData.commandBuffers[imageIndex], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                      ctxt.queryPool, 1);

  res = vkEndCommandBuffer(ctxt.renderData.commandBuffers[imageIndex]);
  AT3_ASSERT(res == VK_SUCCESS, "Error ending render pass");

  // submit

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  //wait on writing colours to the buffer until the semaphore says the buffer is available
  VkSemaphore waitSemaphores[] = {ctxt.imageAvailableSemaphore};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;

  VkSemaphore signalSemaphores[] = {ctxt.renderFinishedSemaphore};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;
  submitInfo.pCommandBuffers = &ctxt.renderData.commandBuffers[imageIndex];
  submitInfo.commandBufferCount = 1;

  res = vkQueueSubmit(ctxt.deviceQueues.graphicsQueue, 1, &submitInfo, ctxt.frameFences[imageIndex]);
  AT3_ASSERT(res == VK_SUCCESS, "Error submitting queue");

  //present

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {ctxt.swapChain.swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr; // Optional
  res = vkQueuePresentKHR(ctxt.deviceQueues.transferQueue, &presentInfo);

#if PRINT_VK_FRAMETIMES
  //log performance data:

  uint32_t end = 0;
  uint32_t begin = 0;

  static int count = 0;
  static float totalTime = 0.0f;

  if (count++ > 1024)
  {
    printf("VK Render Time (avg of past 1024 frames): %f ms\n", totalTime / 1024.0f);
    count = 0;
    totalTime = 0;
  }
  float timestampFrequency = ctxt.gpu.deviceProps.limits.timestampPeriod;

  // A "firstFrame" bool is easier than setting up more synchronization objects.
  if (ctxt.renderData.firstFrame[imageIndex]) {
    ctxt.renderData.firstFrame[imageIndex] = false;
  } else {
    vkGetQueryPoolResults(ctxt.device, ctxt.queryPool, 1, 1, sizeof(uint32_t), &end, 0, VK_QUERY_RESULT_WAIT_BIT);
    vkGetQueryPoolResults(ctxt.device, ctxt.queryPool, 0, 1, sizeof(uint32_t), &begin, 0, VK_QUERY_RESULT_WAIT_BIT);
  }
  uint32_t diff = end - begin;
  totalTime += (diff) / (float)1e6;
#endif

  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
    rtu::topics::publish("window_resized");
  } else {
    AT3_ASSERT(res == VK_SUCCESS, "failed to present swap chain image!");
  }
}
