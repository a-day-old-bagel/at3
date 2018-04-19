#pragma once

#include <iostream>
#include <SDL.h>
#include <vulkan/vulkan.hpp>

#ifdef NDEBUG
static const bool useValidationLayers = false;
#else
static const bool useValidationLayers = true;
#endif

namespace at3 {

  struct VulkanContextCreateInfo {
    explicit VulkanContextCreateInfo(
        SDL_Window *window = nullptr,
        bool useValidation = useValidationLayers,
        std::vector<const char *> desiredLayers = { "VK_LAYER_LUNARG_standard_validation" } );

    SDL_Window *window;
    bool useValidation;
    std::vector<const char*> desiredLayers;
  };

  class VulkanContext {

      VulkanContextCreateInfo info;
      vk::Instance instance;
      VkDebugReportCallbackEXT callback;
      VkSurfaceKHR surface;

      void createInstance();
      void enableValidationLayers(vk::InstanceCreateInfo &instanceInfo);
      void enableExtensions(vk::InstanceCreateInfo &instanceInfo);
      void setupDebugCallback();
      void createSurface();
      static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
          VkDebugReportFlagsEXT flags,
          VkDebugReportObjectTypeEXT objType,
          uint64_t sourceObject,
          size_t location,
          int32_t messageCode,
          const char *layerPrefix,
          const char *message,
          void *userData);

    public:
      VulkanContext(VulkanContextCreateInfo &contextCreateInfo);
      ~VulkanContext();
  };
}
