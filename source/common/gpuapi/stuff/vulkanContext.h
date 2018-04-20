
#pragma once

#include <string>
#include "vkh_types.h"
#include "vkh_mesh.h"
#include "topics.hpp"

namespace at3 {

  struct VulkanContextCreateInfo {
    std::string appName;
    SDL_Window *window;
    std::vector<VkDescriptorType> types;
    std::vector<uint32_t> typeCounts;

    static VulkanContextCreateInfo defaults();
  };

  class VulkanContext {

      VkhContext guts;
      std::vector<MeshAsset> testMesh;
      std::vector<uint32_t> uboIdx;
      glm::mat4 currentWvMat;
      std::unique_ptr<rtu::topics::Subscription> wvUpdate;

      void cleanupRendering();

    public:
      VulkanContext(VulkanContextCreateInfo info);
      virtual ~VulkanContext();
      void updateWvMat(void* data);
      void step();
      void reInitRendering();

  };
}
