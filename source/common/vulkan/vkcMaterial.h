#pragma once

#include "configuration.h"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#  define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>

#include "vkcTypes.h"

namespace at3 {

  struct VShaderInput {
    glm::mat4 model = glm::mat4(1.f);
    glm::mat4 normal = glm::mat4(1.f);
  };

  struct GlobalShaderData {
    AT3_ALIGNED_(16) glm::float32 time;
    AT3_ALIGNED_(16) glm::float32 lightIntensity;
    AT3_ALIGNED_(16) glm::vec2 resolution;
    AT3_ALIGNED_(16) glm::vec4 lightDir;
    AT3_ALIGNED_(16) glm::vec3 lightCol;
    AT3_ALIGNED_(16) glm::vec4 mouse;
    AT3_ALIGNED_(16) glm::vec4 worldSpaceCameraPos;
    AT3_ALIGNED_(16) glm::vec4 vpMatrix;
  };

  struct GlobalShaderDataStore {
    //storage for global shader data
    VkcAllocation mem;
    VkBuffer buffer;
    GlobalShaderData shaderData;
    uint32_t size;
    void *mappedMemory;

    VkDescriptorSetLayout layout;
    VkDescriptorSet descSet;
    VkSampler sampler;
  };

  struct VkcMaterialCreateInfo {
    VkRenderPass renderPass;
    std::vector<VkDescriptorSetLayout> descSetLayouts;
    VkPipelineLayout *outPipelineLayout;
    VkPipeline *outPipeline;
    std::vector<VkPushConstantRange> pcRanges;
  };

  const VertexRenderData *vertexRenderData();
  void setVertexRenderData(VertexRenderData *renderData);

  void createBasicMaterial(
      unsigned char *vertData, unsigned int vertLen, unsigned char *fragData, unsigned int fragLen, VkcCommon &ctxt,
      VkcMaterialCreateInfo &createInfo, const VkSpecializationInfo* specializationInfo = nullptr);

  void loadUBOTestMaterial(VkcCommon &ctxt, uint32_t texArrayLen);
}
