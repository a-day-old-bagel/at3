#pragma once

#include "configuration.hpp"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#  define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>

#include "vkcTypes.hpp"

namespace at3::vkc {

  struct VShaderInput {
    glm::mat4 vp = glm::mat4(1.f);
    glm::mat4 m = glm::mat4(1.f);
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
    Allocation mem;
    VkBuffer buffer;
    GlobalShaderData shaderData;
    uint32_t size;
    void *mappedMemory;

    VkDescriptorSetLayout layout;
    VkDescriptorSet descSet;
    VkSampler sampler;
  };

  struct ShaderSourceInfo {
    unsigned char *data = nullptr;
    unsigned int length = 0;
  };

  enum StandardPipeline {
    MESH = 0,
    NORMAL,
    SKYBOX,
    HEIGHT_TERRAIN,
    PIPELINE_COUNT,
    INVALID_PIPELINE
  };

  struct PipelineCreateInfo {
    StandardPipeline index = INVALID_PIPELINE;
    Common *ctxt = nullptr;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t subpass = 0;
    std::vector<VkDescriptorSetLayoutCreateInfo> descSetLayoutInfos;
    std::vector<VkPushConstantRange> pcRanges;
    VkSpecializationInfo specializationInfo;
    ShaderSourceInfo
        vertCode,
        tescCode,
        teseCode,
        geomCode,
        fragCode;
  };

  class PipelineRepository {

      struct Pipeline {
        std::vector<VkDescriptorSet> descSets;
        std::vector<VkDescriptorSetLayout> descSetLayouts;
        VkPipelineLayout layout;
        VkPipeline handle;
        bool layoutsExist = false;
      };
      std::vector<Pipeline> pipelines;
      std::unique_ptr<VertexAttributes> vertexAttributes;

      void setVertexAttributes(std::vector<EMeshVertexAttribute> layout);

      void createRenderPass(Common &ctxt);

      void createPipelineLayout(PipelineCreateInfo &info);
      void createPipeline(PipelineCreateInfo &info);

      void createStandardMeshPipeline(Common &ctxt, uint32_t texArrayLen);

      void createDeferredGBufferPipeline(Common &ctxt, uint32_t texArrayLen);
      void createDeferredComposePipeline(Common &ctxt, uint32_t texArrayLen);

      void createNormalDebugPipeline(Common &ctxt, uint32_t texArrayLen);
      void createStaticHeightmapTerrainPipeline(Common &ctxt);

      void destroy(Common &ctxt);



      void setupDescriptorSetLayout(PipelineCreateInfo &info);
      void setupDescriptorSet();
      void preparePipelines(Common &ctxt, uint32_t texArrayLen);
      void setupRenderPass(Common &ctxt);
      void prepareCompositionPass(Common &ctxt);



    public:

      VkRenderPass mainRenderPass;

      PipelineRepository(Common &ctxt, uint32_t numTextures2D);
      const VertexAttributes &getVertexAttributes();
      Pipeline &at(uint32_t index);
      void reinit(Common &ctxt, uint32_t numTextures2D);
  };

}
