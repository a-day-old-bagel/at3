#include "vulkanContext.h"
#include <utility>
#include <SDLvulkan.h>

#define ERRLOG std::cerr
#define STDLOG std::cout

namespace at3 {

  VulkanContextCreateInfo::VulkanContextCreateInfo(
      SDL_Window *window,
      bool useValidation,
      std::vector<const char *> desiredLayers) :

      window(window), useValidation(useValidation), desiredLayers(std::move(desiredLayers))
  { }

  VulkanContext::VulkanContext(VulkanContextCreateInfo &contextCreateInfo) : info(contextCreateInfo) {

  }

  VulkanContext::~VulkanContext() {

  }

  void VulkanContext::createInstance() {
    vk::ApplicationInfo appInfo(
        "Triceratone", VK_MAKE_VERSION(0, 1, 0),
        "AT3", VK_MAKE_VERSION(0, 1, 0),
        VK_API_VERSION_1_1);

    vk::InstanceCreateInfo instanceInfo;
    instanceInfo.setPApplicationInfo(&appInfo);

    if (useValidationLayers) {
      enableValidationLayers(instanceInfo);
    }
    enableExtensions(instanceInfo);

    instance = vk::createInstance(instanceInfo);
  }

  void VulkanContext::enableValidationLayers(vk::InstanceCreateInfo &instanceInfo) {
    uint32_t layerCount;
    vk::enumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<vk::LayerProperties> availableLayers(layerCount);
    vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    std::vector<const char*> enabledLayers;
    STDLOG << "Using Vulkan validation layers: " << std::endl;
    for (auto layer : info.desiredLayers) {
      for (const auto &layerProperties : availableLayers) {
        if (strcmp(layer, layerProperties.layerName) == 0) {
          STDLOG << "\t" << layer << std::endl;
          enabledLayers.push_back(layer);
        }
      }
    }
    if (enabledLayers.size() != info.desiredLayers.size()) {
      ERRLOG << "Couldn't enable all the desired validation layers!";
    }
    instanceInfo.setEnabledLayerCount((uint32_t)enabledLayers.size());
    instanceInfo.setPpEnabledLayerNames(enabledLayers.data());
  }

  void VulkanContext::enableExtensions(vk::InstanceCreateInfo &instanceInfo) {
    std::vector<const char *> extensions;
    uint32_t extensionCount = 0;
    const char *extensionNames[64];
    if (info.useValidation) {
      extensionNames[extensionCount++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    }
    extensionNames[extensionCount++] = VK_KHR_SURFACE_EXTENSION_NAME;
    unsigned c = 64 - extensionCount;
    if (!SDL_GetVulkanInstanceExtensions(&c, &extensionNames[extensionCount])) {
      ERRLOG << "SDL_GetVulkanInstanceExtensions failed: " << SDL_GetError() << std::endl;
    }
    extensionCount += c;
    STDLOG << "Using Vulkan extensions: " << std::endl;
    for (unsigned int i = 0; i < extensionCount; i++) {
      extensions.push_back(extensionNames[i]);
      STDLOG << "\t" << extensionNames[i] << std::endl;
    }
    instanceInfo.setEnabledExtensionCount((uint32_t)extensions.size());
    instanceInfo.setPpEnabledExtensionNames(extensions.data());
  }

  void VulkanContext::setupDebugCallback() {
    if (info.useValidation) {
      auto vkCreateDebugReportCallbackEXT =
          (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

      if (vkCreateDebugReportCallbackEXT) {

        vk::DebugReportCallbackCreateInfoEXT debugReportInfo;
        debugReportInfo.setFlags(
            vk::DebugReportFlagBitsEXT::eError |
            vk::DebugReportFlagBitsEXT::eWarning |
            vk::DebugReportFlagBitsEXT::ePerformanceWarning |
            vk::DebugReportFlagBitsEXT::eDebug |
            vk::DebugReportFlagBitsEXT::eInformation );
        debugReportInfo.setPfnCallback(debugCallback);

        VkResult res = vkCreateDebugReportCallbackEXT(
            instance, reinterpret_cast<const VkDebugReportCallbackCreateInfoEXT*>(&debugReportInfo),
            nullptr, &callback);

        if (res != VK_SUCCESS) { ERRLOG << "Failed to set up Vulkan debug report callback!" << std::endl; }
      }
    }
  }

  void VulkanContext::createSurface() {
    if (!SDL_CreateVulkanSurface(info.window, instance, &surface)) {
      throw std::runtime_error("failed to create window surface!");
    }
  }

  VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debugCallback(
      VkDebugReportFlagsEXT flags,
      VkDebugReportObjectTypeEXT objType,
      uint64_t sourceObject,
      size_t location,
      int32_t messageCode,
      const char *layerPrefix,
      const char *message,
      void *userData) {

    ERRLOG << "VULKAN ";
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
      ERRLOG << "ERROR ";
    }
    if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
      ERRLOG << "WARNING ";
    }
    if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
      ERRLOG << "INFO ";
    }
    if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
      ERRLOG << "PERFORMANCE NOTE ";
    }
    if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
      ERRLOG << "DEBUG ";
    }
    ERRLOG << "(Layer " << layerPrefix << ", Code " << messageCode << "): " << message << std::endl;

    return (VkBool32) false;
  }
}
