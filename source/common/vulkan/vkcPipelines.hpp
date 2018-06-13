#pragma once

#include "configuration.hpp"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#  define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>

#include "vkcTypes.hpp"

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

  struct VkcShaderSourceInfo {
    unsigned char *data = nullptr;
    unsigned int length = 0;
  };
  struct VkcPipelineCreateInfo {
    VkcCommon *ctxt;
    VkRenderPass renderPass;
    std::vector<VkDescriptorSetLayoutCreateInfo> descSetLayoutInfos;
    std::vector<VkPushConstantRange> pcRanges;
    VkSpecializationInfo specializationInfo;
    VkcShaderSourceInfo
        vertCode,
        tescCode,
        teseCode,
        geomCode,
        fragCode;
  };

  const VertexRenderData *vertexRenderData();
  void setVertexRenderData(VertexRenderData *renderData);

  class VkcPipelineRepository {

      struct VkcPipeline {
        std::vector<VkDescriptorSet> descSets;
        std::vector<VkDescriptorSetLayout> descSetLayouts;
        VkPipelineLayout layout;
        VkPipeline handle;
      };
      std::vector<VkcPipeline> pipelines;

      void createPipeline(VkcPipelineCreateInfo &info);
      void createStandardMeshPipeline(VkcCommon &ctxt, uint32_t texArrayLen);
      void createStaticHeightmapTerrainPipeline(VkcCommon &ctxt);

    public:
      VkcPipelineRepository(VkcCommon &ctxt, uint32_t numTextures2D);
      VkcPipeline &at(uint32_t index);
  };

}
