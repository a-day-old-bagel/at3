#pragma once

#include <cstring>
#include <sstream>
#include <algorithm>
#include "vkh_types.h"
#include "vkh.h"
#include "vkh_alloc.h"
#include "os_sdl2.h"
#include "config.h"

#include <SDLvulkan.h>
#include "settings.h"

namespace at3 {
  struct VkhContextCreateInfo {
      std::vector<VkDescriptorType> types;
      std::vector<uint32_t> typeCounts;
  };

  const uint32_t INVALID_QUEUE_FAMILY_IDX = -1;

  VkDebugReportCallbackEXT callback;

  VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
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

  void createInstance(VkhContext &ctxt, const char *appName) {
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

    checkf(foundAllLayers, "Not all required validation layers were found.");
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
      bool success = SDL_Vulkan_GetInstanceExtensions(ctxt.window, &sdlVkExtensionCount, NULL);
      checkf(success, "SDL_Vulkan_GetInstanceExtensions(): %s\n", SDL_GetError());

      sdlVkExtensions = (const char **) SDL_malloc(sizeof(const char *) * sdlVkExtensionCount);
      checkf(sdlVkExtensions, "Out of memory.\n");

      success = SDL_Vulkan_GetInstanceExtensions(ctxt.window, &sdlVkExtensionCount, sdlVkExtensions);
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

    checkf(allExtensionsFound, "Failed to find all required vulkan extensions");

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
    res = vkCreateInstance(&inst_info, NULL, &ctxt.instance);

    checkf(res != VK_ERROR_INCOMPATIBLE_DRIVER, "Cannot create VkInstance - driver incompatible");
    checkf(res == VK_SUCCESS, "Cannot create VkInstance - unknown error");


  }

  void createSurface(VkhContext &ctxt) {
#   if USE_CUSTOM_SDL_VULKAN
      bool success = SDL_CreateVulkanSurface(ctxt.window, ctxt.instance, &ctxt.surface.surface);
#   else
      bool success = SDL_Vulkan_CreateSurface(ctxt.window, ctxt.instance, &ctxt.surface.surface);
#   endif
      checkf(success, "SDL_Vulkan_CreateSurface(): %s\n", SDL_GetError());
      if (!success) {
        ctxt.surface.surface = VK_NULL_HANDLE;
      }
  }

  void createDebugCallback(VkhContext &ctxt) {
#ifndef NDEBUG
    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                       VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    createInfo.pfnCallback = debugCallback;

    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = VK_NULL_HANDLE;
    CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)
        vkGetInstanceProcAddr(ctxt.instance, "vkCreateDebugReportCallbackEXT");
    CreateDebugReportCallback(ctxt.instance, &createInfo, NULL, &callback);
#endif
  }

  void createPhysicalDevice(VkhContext &ctxt) {
    VkPhysicalDevice &outDevice = ctxt.gpu.device;

    uint32_t gpu_count;
    VkResult res = vkEnumeratePhysicalDevices(ctxt.instance, &gpu_count, NULL);

    // Allocate space and get the list of devices.
    std::vector<VkPhysicalDevice> gpus;
    gpus.resize(gpu_count);

    res = vkEnumeratePhysicalDevices(ctxt.instance, &gpu_count, gpus.data());
    checkf(res == VK_SUCCESS, "Could not enumerate physical devices");

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
        VkhSwapChainSupportInfo scSupport;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, ctxt.surface.surface, &scSupport.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, ctxt.surface.surface, &formatCount, nullptr);

        if (formatCount != 0) {
          scSupport.formats.resize(formatCount);
          vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, ctxt.surface.surface, &formatCount, scSupport.formats.data());
        } else {
          continue;
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, ctxt.surface.surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
          scSupport.presentModes.resize(presentModeCount);
          vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, ctxt.surface.surface, &presentModeCount,
                                                    scSupport.presentModes.data());
        }

        bool worksWithSurface = scSupport.formats.size() > 0 && scSupport.presentModes.size() > 0;

        if (score > curScore && supportsAllRequiredExtensions && worksWithSurface) {
          found = true;

          ctxt.gpu.device = gpu;
          ctxt.gpu.swapChainSupport = scSupport;
          curScore = score;
        }
      }
    }

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(ctxt.gpu.device, &deviceProperties);
    printf("Selected GPU: %s\n", deviceProperties.deviceName);

    checkf(found, "Could not find a gpu that matches our specifications");

    vkGetPhysicalDeviceFeatures(outDevice, &ctxt.gpu.features);
    vkGetPhysicalDeviceMemoryProperties(outDevice, &ctxt.gpu.memProps);
    vkGetPhysicalDeviceProperties(outDevice, &ctxt.gpu.deviceProps);
    printf("Max mem allocations: %i\n", ctxt.gpu.deviceProps.limits.maxMemoryAllocationCount);

    //get queue families while we're here
    vkGetPhysicalDeviceQueueFamilyProperties(outDevice, &ctxt.gpu.queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies;
    queueFamilies.resize(ctxt.gpu.queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(outDevice, &ctxt.gpu.queueFamilyCount, queueFamilies.data());

    std::vector<VkQueueFamilyProperties> queueVec;
    queueVec.resize(ctxt.gpu.queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(outDevice, &ctxt.gpu.queueFamilyCount, &queueVec[0]);


    // Iterate over each queue to learn whether it supports presenting:
    VkBool32 *pSupportsPresent = (VkBool32 *) malloc(ctxt.gpu.queueFamilyCount * sizeof(VkBool32));
    for (uint32_t i = 0; i < ctxt.gpu.queueFamilyCount; i++) {
      vkGetPhysicalDeviceSurfaceSupportKHR(outDevice, i, ctxt.surface.surface, &pSupportsPresent[i]);
    }


    ctxt.gpu.graphicsQueueFamilyIdx = INVALID_QUEUE_FAMILY_IDX;
    ctxt.gpu.transferQueueFamilyIdx = INVALID_QUEUE_FAMILY_IDX;
    ctxt.gpu.presentQueueFamilyIdx = INVALID_QUEUE_FAMILY_IDX;

    bool foundGfx = false;
    bool foundTransfer = false;
    bool foundPresent = false;

    for (uint32_t queueIdx = 0; queueIdx < queueFamilies.size(); ++queueIdx) {
      const auto &queueFamily = queueFamilies[queueIdx];
      const auto &queueFamily2 = queueVec[queueIdx];

      if (!foundGfx && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        ctxt.gpu.graphicsQueueFamilyIdx = queueIdx;
        foundGfx = true;
      }

      if (!foundTransfer && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
        ctxt.gpu.transferQueueFamilyIdx = queueIdx;
        foundTransfer = true;

      }

      if (!foundPresent && queueFamily.queueCount > 0 && pSupportsPresent[queueIdx]) {
        ctxt.gpu.presentQueueFamilyIdx = queueIdx;
        foundPresent = true;

      }

      if (foundGfx && foundTransfer && foundPresent) break;
    }

    checkf(foundGfx && foundPresent && foundTransfer, "Failed to find all required device queues");
  }

  void createLogicalDevice(VkhContext &ctxt) {
    const VkhPhysicalDevice &physDevice = ctxt.gpu;
    VkDevice &outDevice = ctxt.device;
    VkhDeviceQueues &outQueues = ctxt.deviceQueues;

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
    checkf(res == VK_SUCCESS, "Error creating logical device");

    vkGetDeviceQueue(outDevice, physDevice.graphicsQueueFamilyIdx, 0, &ctxt.deviceQueues.graphicsQueue);
    vkGetDeviceQueue(outDevice, physDevice.transferQueueFamilyIdx, 0, &ctxt.deviceQueues.transferQueue);
    vkGetDeviceQueue(outDevice, physDevice.presentQueueFamilyIdx, 0, &ctxt.deviceQueues.presentQueue);

  }

  VkExtent2D chooseSwapExtent(const VkhContext &ctxt) {
    VkSurfaceCapabilitiesKHR capabilities = ctxt.gpu.swapChainSupport.capabilities;
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    } else {
      VkExtent2D actualExtent = {
          static_cast<uint32_t>(ctxt.windowWidth),
          static_cast<uint32_t>(ctxt.windowHeight)
      };
      actualExtent.width = std::max(capabilities.minImageExtent.width,
                                    std::min(capabilities.maxImageExtent.width, actualExtent.width));
      actualExtent.height = std::max(capabilities.minImageExtent.height,
                                     std::min(capabilities.maxImageExtent.height, actualExtent.height));
      return actualExtent;
    }
  }

  void createSwapchainForSurface(VkhContext &ctxt) {
    //choose the surface format to use
    VkSurfaceFormatKHR desiredFormat;
    VkPresentModeKHR desiredPresentMode;
    VkExtent2D swapExtent;

    VkhSwapChain &outSwapChain = ctxt.swapChain;
    VkhPhysicalDevice &physDevice = ctxt.gpu;
    const VkDevice &lDevice = ctxt.device;
    const VkhSurface &surface = ctxt.surface;

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
    if (!at3::settings::graphics::vulkan::forceFifo) {
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
    swapExtent = chooseSwapExtent(ctxt);

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
    checkf(res == VK_SUCCESS, "Error creating Vulkan Swapchain");

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

  void createQueryPool(VkQueryPool &outPool, VkDevice &device, int queryCount) {
    VkQueryPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = queryCount;

    VkResult res = vkCreateQueryPool(device, &createInfo, nullptr, &outPool);
    checkf(res == VK_SUCCESS, "Error creating query pool");

  }

  void getWindowSize(VkhContext &ctxt) {
    int width, height;
    SDL_GetWindowSize(ctxt.window, &width, &height);
    if (width <= 0 || height <= 0) {
      return;
    }
    printf("Window size is %ux%u.\n", width, height);
    ctxt.windowWidth = width;
    ctxt.windowHeight = height;
  }

  void initContext(VkhContextCreateInfo &info, const char *appName, VkhContext &ctxt) {
    sdl::WindowCreateInfo sdlWindowCreateInfo;
    sdlWindowCreateInfo.width = ctxt.windowWidth;
    sdlWindowCreateInfo.height = ctxt.windowHeight;
    ctxt.window = sdl::init(sdlWindowCreateInfo);
    getWindowSize(ctxt); // in case we got the window size we deserve

    createInstance(ctxt, appName);
    createDebugCallback(ctxt);

    createSurface(ctxt);
    createPhysicalDevice(ctxt);
    createLogicalDevice(ctxt);

    at3::allocators::pool::activate(&ctxt);

    createSwapchainForSurface(ctxt); // window size is needed here

    createCommandPool(ctxt.gfxCommandPool, ctxt.device, ctxt.gpu, ctxt.gpu.graphicsQueueFamilyIdx);
    createCommandPool(ctxt.transferCommandPool, ctxt.device, ctxt.gpu, ctxt.gpu.transferQueueFamilyIdx);
    createCommandPool(ctxt.presentCommandPool, ctxt.device, ctxt.gpu, ctxt.gpu.presentQueueFamilyIdx);

    createDescriptorPool(ctxt.descriptorPool, ctxt.device, info.types, info.typeCounts);

    createVkSemaphore(ctxt.imageAvailableSemaphore, ctxt.device);
    createVkSemaphore(ctxt.renderFinishedSemaphore, ctxt.device);

    createQueryPool(ctxt.queryPool, ctxt.device, 10);

    ctxt.frameFences.resize(ctxt.swapChain.imageViews.size());

    for (uint32_t i = 0; i < ctxt.frameFences.size(); ++i) {
      createFence(ctxt.frameFences[i], ctxt.device);
    }
  }
}
