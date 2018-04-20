
#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <cstdint>
#include "vkh.h"

namespace ubo_store
{
  void init(at3::VkhContext& ctxt);
  bool acquire(uint32_t& outIdx);
  uint32_t getNumPages();
  VkBuffer& getPage(uint32_t idx);
  at3::Allocation& getAlloc(uint32_t idx);
  void updateBuffers(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, VkCommandBuffer* commandBuffer, at3::VkhContext& ctxt);
}
