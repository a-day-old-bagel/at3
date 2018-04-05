
#include <iostream>
#include "graphicsBackend.h"

namespace at3 {
  namespace graphicsBackend {
    namespace vulkan {
      bool init() {
        try {
          vkbe = std::make_unique<VulkanBackend>(sdl2::window);
        } catch (const std::runtime_error& e) {
          std::cerr << e.what() << std::endl;
          return false;
        }
        return true;
      }
      void swap() {
        try {
          vkbe->step();
        } catch (const std::runtime_error& e) {
          std::cerr << e.what() << std::endl;
        }
      }
      void clear() {

      }
      std::unique_ptr<VulkanBackend> vkbe;
//      PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
//      PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
//      VkDebugReportCallbackEXT msg_callback;
//      PFN_vkDebugReportMessageEXT DebugReportMessage;
    }
  }
}
