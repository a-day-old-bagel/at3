
#include <iostream>
#include "graphicsBackend.h"

namespace at3 {
  namespace graphicsBackend {
    namespace vulkan {
      bool init() {
        try {
//          vkbe = std::make_unique<VulkanBackend>(sdl2::window);

          VulkanContextCreateInfo contextCreateInfo = VulkanContextCreateInfo::defaults();
          contextCreateInfo.window = sdl2::window;
          vkc = std::make_unique<VulkanContext>(contextCreateInfo);

        } catch (const std::runtime_error& e) {
          std::cerr << e.what() << std::endl;
          return false;
        }
        return true;
      }
      void swap() {
        try {
//          vkbe->step();
          vkc->step();
        } catch (const std::runtime_error& e) {
          std::cerr << e.what() << std::endl;
        }
      }
      void clear() {

      }
      void handleWindowSizeChange() {
        vkc->reInitRendering();
      }
      std::unique_ptr<VulkanBackend> vkbe;
      std::unique_ptr<VulkanContext> vkc;
    }
  }
}
