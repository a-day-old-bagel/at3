
#include "vkcPipelines.hpp"

// Include the shader codes
#include "meshDefault.vert.spv.c"
#include "meshDefault.frag.spv.c"

namespace at3 {

  VertexRenderData *_vkRenderData = nullptr;
  const VertexRenderData *vertexRenderData() {
    AT3_ASSERT(_vkRenderData,
             "Attempting to get global vertex layout, but it has not been set yet, call setGlobalVertexLayout first.");
    return _vkRenderData;
  }
  void setVertexRenderData(VertexRenderData *renderData) {
    AT3_ASSERT(_vkRenderData == nullptr, "Attempting to set global vertex layout, but this has already been set");
    _vkRenderData = renderData;
  }

  void createShaderModule(VkShaderModule &outModule, unsigned char *binaryData, size_t dataSize,
                          const VkcCommon &ctxt) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    //data for vulkan is stored in uint32_t  -  so we have to temporarily copy it to a container that respects that alignment

    std::vector<uint32_t> codeAligned;
    codeAligned.resize((uint32_t) (dataSize / sizeof(uint32_t) + 1));

    memcpy(&codeAligned[0], binaryData, dataSize);
    createInfo.pCode = &codeAligned[0];
    createInfo.codeSize = dataSize;

    AT3_ASSERT(dataSize % 4 == 0, "Invalid data size for .spv file -> are you sure that it compiled correctly?");

    VkResult res = vkCreateShaderModule(ctxt.device, &createInfo, nullptr, &outModule);
    AT3_ASSERT(res == VK_SUCCESS, "Error creating shader module");

  }


  VkcPipelineRepository::VkcPipelineRepository(VkcCommon &ctxt, uint32_t numTextures2D) {
    createStandardMeshPipeline(ctxt, numTextures2D);
    createStaticHeightmapTerrainPipeline(ctxt);
  }

  void VkcPipelineRepository::createPipeline(VkcPipelineCreateInfo &info) {

    // This will be used to hold the return values of Vulkan functions for error checking.
    VkResult res;

    // This will hold information about shader stages
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    { // populate shaderStages
      // Vertex shader is required. Will crash without.
      VkPipelineShaderStageCreateInfo vertStageInfo{};
      vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
      vertStageInfo.pName = "main";
      createShaderModule(vertStageInfo.module, info.vertCode.data, info.vertCode.length, *info.ctxt);
      shaderStages.push_back(vertStageInfo);
      // Tesselation control shader. Optional, but must also come with tese below
      if (info.tescCode.data) {
        VkPipelineShaderStageCreateInfo tescStageInfo{};
        tescStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        tescStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        tescStageInfo.pName = "main";
        createShaderModule(tescStageInfo.module, info.tescCode.data, info.tescCode.length, *info.ctxt);
      }
      // Tesselation evaluation shader. Optional, but must also come with tesc above
      if (info.teseCode.data) {
        VkPipelineShaderStageCreateInfo teseStageInfo{};
        teseStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        teseStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        teseStageInfo.pName = "main";
        createShaderModule(teseStageInfo.module, info.teseCode.data, info.teseCode.length, *info.ctxt);
      }
      // Geometry shader. Optional.
      if (info.geomCode.data) {
        VkPipelineShaderStageCreateInfo geomStageInfo{};
        geomStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        geomStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        geomStageInfo.pName = "main";
        createShaderModule(geomStageInfo.module, info.geomCode.data, info.geomCode.length, *info.ctxt);
      }
      // Fragment shader is required. Will crash without.
      VkPipelineShaderStageCreateInfo fragStageInfo{};
      fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      fragStageInfo.pName = "main";
      fragStageInfo.pSpecializationInfo = &info.specializationInfo;
      createShaderModule(fragStageInfo.module, info.fragCode.data, info.fragCode.length, *info.ctxt);
      shaderStages.push_back(fragStageInfo);
    } // shaderStages is now populated and final.

    // Add a pipeline object to pipelines. This will be filled in below.
    pipelines.emplace_back();

    // Pipeline layout creation info allocated
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    { // Populate pipelineLayoutInfo and use it to create the pipeline layout

      // Create the descriptor set layouts
      for (auto layoutInfo : info.descSetLayoutInfos) {
        pipelines.back().descSetLayouts.emplace_back();
        res = vkCreateDescriptorSetLayout(
            info.ctxt->device, &layoutInfo, nullptr, &pipelines.back().descSetLayouts.back());
        AT3_ASSERT(res == VK_SUCCESS, "Error creating desc set layout");
      }

      // Include the descriptor set layouts in the pipeline layout creation info.
      pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(pipelines.back().descSetLayouts.size());
      pipelineLayoutInfo.pSetLayouts = pipelines.back().descSetLayouts.data();

      // Include the push constants in the pipeline layout creation info.
      pipelineLayoutInfo.pPushConstantRanges = info.pcRanges.data();
      pipelineLayoutInfo.pushConstantRangeCount = (uint32_t) info.pcRanges.size();

      // Create the pipeline layout.
      res = vkCreatePipelineLayout(info.ctxt->device, &pipelineLayoutInfo, nullptr, &pipelines.back().layout);
      AT3_ASSERT(res == VK_SUCCESS, "Error creating pipeline layout");
    }

    // TODO: this is stupid. Make a pipeline repository like the texture repository and have this value in there.
    const VertexRenderData *vertexLayout = vertexRenderData();

    VkVertexInputBindingDescription bindingDescription {};
    {
      bindingDescription.binding = 0;
      bindingDescription.stride = vertexLayout->vertexSize;
      bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    {
      vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
      vertexInputInfo.vertexBindingDescriptionCount = 1;
      vertexInputInfo.vertexAttributeDescriptionCount = vertexLayout->attrCount;
      vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
      vertexInputInfo.pVertexAttributeDescriptions = &vertexLayout->attrDescriptions[0];
    }

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    {
      inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
      inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      inputAssembly.primitiveRestartEnable = VK_FALSE;
    }

    VkViewport vp {};
    {
      vp.x = 0;
      vp.y = 0;
      vp.width = static_cast<float>(info.ctxt->swapChain.extent.width);
      vp.height = static_cast<float>(info.ctxt->swapChain.extent.height);
      vp.minDepth = 0.f;
      vp.maxDepth = 1.f;
    }

    VkRect2D scissor {};
    {
      scissor.extent.width = info.ctxt->swapChain.extent.width;
      scissor.extent.height = info.ctxt->swapChain.extent.height;
      scissor.offset.x = 0;
      scissor.offset.y = 0;
    }

    VkPipelineViewportStateCreateInfo viewportState = {};
    {
      viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
      viewportState.pScissors = &scissor;
      viewportState.scissorCount = 1;
      viewportState.pViewports = &vp;
      viewportState.viewportCount = 1;
    }

    VkPipelineRasterizationStateCreateInfo rasterizer {};
    {
      rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
      rasterizer.depthClampEnable = VK_FALSE; //if values are outside depth, they are clamped to min/max
      rasterizer.rasterizerDiscardEnable = VK_FALSE; //this disables output to the fb
      rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
      rasterizer.lineWidth = 1.0f;
      rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
      rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
      rasterizer.depthBiasEnable = VK_FALSE;
      rasterizer.depthBiasConstantFactor = 0.0f; // Optional
      rasterizer.depthBiasClamp = 0.0f; // Optional
      rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
    }

    VkPipelineMultisampleStateCreateInfo multisampling {};
    {
      multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
      multisampling.sampleShadingEnable = VK_FALSE;
      multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
      multisampling.minSampleShading = 1.0f; // Optional
      multisampling.pSampleMask = nullptr; // Optional
      multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
      multisampling.alphaToOneEnable = VK_FALSE; // Optional
    }

    VkPipelineColorBlendAttachmentState colorBlendAttachment {};
    {
      colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      colorBlendAttachment.blendEnable = VK_FALSE;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending {};
    {
      colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      colorBlending.logicOpEnable = VK_FALSE; //can be used for bitwise blending
      colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
      colorBlending.attachmentCount = 1;
      colorBlending.pAttachments = &colorBlendAttachment;
      colorBlending.blendConstants[0] = 0.0f; // Optional
      colorBlending.blendConstants[1] = 0.0f; // Optional
      colorBlending.blendConstants[2] = 0.0f; // Optional
      colorBlending.blendConstants[3] = 0.0f; // Optional
    }

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    {
      depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
      depthStencil.depthTestEnable = VK_TRUE;
      depthStencil.depthWriteEnable = VK_TRUE;
      depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    {
      pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
      pipelineInfo.pStages = shaderStages.data();
      pipelineInfo.pVertexInputState = &vertexInputInfo;
      pipelineInfo.pInputAssemblyState = &inputAssembly;
      pipelineInfo.pViewportState = &viewportState;
      pipelineInfo.pRasterizationState = &rasterizer;
      pipelineInfo.pMultisampleState = &multisampling;
      pipelineInfo.pColorBlendState = &colorBlending;
      pipelineInfo.pDynamicState = nullptr; // Optional
      pipelineInfo.layout = pipelines.back().layout;
      pipelineInfo.renderPass = info.renderPass;
      pipelineInfo.pDepthStencilState = &depthStencil;

      pipelineInfo.subpass = 0;

      // Can use this to create new pipelines by deriving from old ones
      pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
      pipelineInfo.basePipelineIndex = -1;
    }

    // Create the pipeline on the gpu.
    res = vkCreateGraphicsPipelines(
        info.ctxt->device, VK_NULL_HANDLE, 1,&pipelineInfo, nullptr, &pipelines.back().handle);
    AT3_ASSERT(res == VK_SUCCESS, "Error creating graphics pipeline");

    // Cleanup the shader modules now that they've been copied to the gpu.
    for (auto stage : shaderStages) {
      vkDestroyShaderModule(info.ctxt->device, stage.module, nullptr);
    }

  }

  void VkcPipelineRepository::createStandardMeshPipeline(VkcCommon &ctxt, uint32_t texArrayLen) {

    // This is populated and then passed to createPipeline.
    VkcPipelineCreateInfo info {};

    { // Context and renderpass
      info.ctxt = &ctxt;
      info.renderPass = ctxt.renderData.mainRenderPass;
    }

    { // Shaders
      info.vertCode = {meshDefault_vert_spv, meshDefault_vert_spv_len};
      info.fragCode = {meshDefault_frag_spv, meshDefault_frag_spv_len};
    }

    // Descriptor set layout bindings
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    {
      VkDescriptorSetLayoutBinding uboBinding{};
      uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      uboBinding.binding = 0;
      uboBinding.descriptorCount = 1;
      layoutBindings.push_back(uboBinding);

      VkDescriptorSetLayoutBinding textureArrayBinding{};
      textureArrayBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      textureArrayBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      textureArrayBinding.binding = 1;
      textureArrayBinding.descriptorCount = texArrayLen;
      layoutBindings.push_back(textureArrayBinding);
    }

    // Layout creation info
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    {
      layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
      layoutInfo.pBindings = layoutBindings.data();
      info.descSetLayoutInfos.push_back(layoutInfo);
    }

    // Push constants
    VkPushConstantRange pcRange = {};
    {
      pcRange.offset = 0;
      pcRange.size = sizeof(MeshInstanceIndices::rawType);
      pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
      info.pcRanges.push_back(pcRange);
    }

    // Specialization constants
    struct SpecializationData {
      uint32_t textureArrayLength = 1;
    } specializationData;
    std::vector<VkSpecializationMapEntry> specializationMapEntries;
    VkSpecializationMapEntry textureArrayLengthEntry{};
    {
      specializationData.textureArrayLength = texArrayLen;
      textureArrayLengthEntry.constantID = 0;
      textureArrayLengthEntry.size = sizeof(specializationData.textureArrayLength);
      textureArrayLengthEntry.offset = static_cast<uint32_t>(offsetof(SpecializationData, textureArrayLength));
      specializationMapEntries.push_back(textureArrayLengthEntry);
      info.specializationInfo.dataSize = sizeof(specializationData);
      info.specializationInfo.mapEntryCount = static_cast<uint32_t>(specializationMapEntries.size());
      info.specializationInfo.pMapEntries = specializationMapEntries.data();
      info.specializationInfo.pData = &specializationData;
    }

    createPipeline(info);
  }

  void VkcPipelineRepository::createStaticHeightmapTerrainPipeline(VkcCommon &ctxt) {

  }

  VkcPipelineRepository::VkcPipeline &VkcPipelineRepository::at(uint32_t index) {
    return pipelines.at(index);
  }
}
