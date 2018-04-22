#pragma once

#include <cstdint>
#include <vector>
#include <SDLvulkan.h>
#include "vkh_types.h"

namespace at3 {

  struct VkhContextCreateInfo {
    std::vector<VkDescriptorType> types;
    std::vector<uint32_t> typeCounts;
  };

  VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugReportFlagsEXT flags,
      VkDebugReportObjectTypeEXT objType,
      uint64_t sourceObject,
      size_t location,
      int32_t messageCode,
      const char *layerPrefix,
      const char *message,
      void *userData);

  void createInstance(VkhContext &ctxt, const char *appName);

  void createSurface(VkhContext &ctxt);

  void createDebugCallback(VkhContext &ctxt);

  void createPhysicalDevice(VkhContext &ctxt);

  void createLogicalDevice(VkhContext &ctxt);

  VkExtent2D chooseSwapExtent(const VkhContext &ctxt);

  void createSwapchainForSurface(VkhContext &ctxt);

  void createQueryPool(VkQueryPool &outPool, VkDevice &device, int queryCount);

  void getWindowSize(VkhContext &ctxt);

  void initContext(VkhContextCreateInfo &info, const char *appName, VkhContext &ctxt);

}
