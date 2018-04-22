
#include "vulkanContext.h"

namespace at3 {

  VulkanContextCreateInfo VulkanContextCreateInfo::defaults() {
    VulkanContextCreateInfo info;
    info.appName = "at3";
    info.window = nullptr;
    info.types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    info.types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    info.types.push_back(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    info.types.push_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    info.types.push_back(VK_DESCRIPTOR_TYPE_SAMPLER);
    info.typeCounts.push_back(512);
    info.typeCounts.push_back(512);
    info.typeCounts.push_back(512);
    info.typeCounts.push_back(512);
    info.typeCounts.push_back(512);
    info.typeCounts.push_back(1);
    return info;
  }
}
