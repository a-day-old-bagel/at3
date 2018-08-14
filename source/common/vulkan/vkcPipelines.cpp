
#include "vkcPipelines.hpp"

// Include the shader codes
#include "meshDefault.vert.spv.c"
#include "meshDefault.frag.spv.c"

#include "triangleDebug.vert.spv.c"
#include "triangleDebug.geom.spv.c"
#include "triangleDebug.frag.spv.c"

namespace at3::vkc {

  void createShaderModule(VkShaderModule &outModule, unsigned char *binaryData, size_t dataSize,
                          const Common &ctxt) {
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

  PipelineRepository::PipelineRepository(Common &ctxt, uint32_t numTextures2D) {
    std::vector<EMeshVertexAttribute> meshLayout;
    meshLayout.push_back(EMeshVertexAttribute::POSITION);
    meshLayout.push_back(EMeshVertexAttribute::UV0);
    meshLayout.push_back(EMeshVertexAttribute::NORMAL);
    setVertexAttributes(meshLayout);

    pipelines.resize(PIPELINE_COUNT);

    createRenderPass(ctxt);
    createStandardMeshPipeline(ctxt, numTextures2D);
    createTriangleDebugPipeline(ctxt, numTextures2D);
    createStaticHeightmapTerrainPipeline(ctxt);
  }

  void PipelineRepository::setVertexAttributes(std::vector<EMeshVertexAttribute> layout) {

    vertexAttributes = std::make_unique<VertexAttributes>();

    vertexAttributes->attrCount = (uint32_t)layout.size();
    vertexAttributes->attrDescriptions = (VkVertexInputAttributeDescription *) malloc(
        sizeof(VkVertexInputAttributeDescription) * vertexAttributes->attrCount);
    vertexAttributes->attributes = (EMeshVertexAttribute *) malloc(
        sizeof(EMeshVertexAttribute) * vertexAttributes->attrCount);
    memcpy(vertexAttributes->attributes, layout.data(), sizeof(EMeshVertexAttribute) * vertexAttributes->attrCount);

    uint32_t curOffset = 0;
    for (uint32_t i = 0; i < layout.size(); ++i) {
      switch (layout[i]) {
        case EMeshVertexAttribute::POSITION:
        case EMeshVertexAttribute::NORMAL:
        case EMeshVertexAttribute::TANGENT:
        case EMeshVertexAttribute::BITANGENT: {
          vertexAttributes->attrDescriptions[i] = {i, 0, VK_FORMAT_R32G32B32_SFLOAT, curOffset};
          curOffset += sizeof(glm::vec3);
        } break;
        case EMeshVertexAttribute::UV0:
        case EMeshVertexAttribute::UV1: {
          vertexAttributes->attrDescriptions[i] = {i, 0, VK_FORMAT_R32G32_SFLOAT, curOffset};
          curOffset += sizeof(glm::vec2);
        } break;
        case EMeshVertexAttribute::COLOR: {
          vertexAttributes->attrDescriptions[i] = {i, 0, VK_FORMAT_R32G32B32A32_SFLOAT, curOffset};
          curOffset += sizeof(glm::vec4);
        } break;
        default:
          AT3_ASSERT(0, "Invalid vertex attribute specified");
          break;
      }
    }
    vertexAttributes->vertexSize = curOffset;
  }






  void PipelineRepository::createRenderPass(Common &ctxt) {

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorRefs;
    VkAttachmentReference depthRef = {};
    {
      VkAttachmentDescription &colorAttachment = attachments.emplace_back();
      colorAttachment.format = ctxt.swapChain.imageFormat;
      colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
      colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

      uint32_t attachIdx = 0;
      for (; attachIdx < attachments.size(); ++attachIdx) {
        VkAttachmentReference &ref = colorRefs.emplace_back();
        ref.attachment = attachIdx;
        ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      }

      VkAttachmentDescription &depthAttachment = attachments.emplace_back();
      depthAttachment.format = VK_FORMAT_D32_SFLOAT;
      depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
      depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      depthRef.attachment = attachIdx;
      depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    std::vector<VkSubpassDescription> subpasses;
    {
      VkSubpassDescription &meshSubpass = subpasses.emplace_back();
      meshSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      meshSubpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
      meshSubpass.pColorAttachments = colorRefs.data();
      meshSubpass.pDepthStencilAttachment = &depthRef;

      VkSubpassDescription &normSubpass = subpasses.emplace_back();
      normSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      normSubpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
      normSubpass.pColorAttachments = colorRefs.data();
      normSubpass.pDepthStencilAttachment = &depthRef;
    }

    std::vector<VkSubpassDependency> dependencies;
    {
      VkSubpassDependency &depsExternTo0 = dependencies.emplace_back();
      depsExternTo0.srcSubpass = VK_SUBPASS_EXTERNAL;
      depsExternTo0.dstSubpass = 0;
      depsExternTo0.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      depsExternTo0.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      depsExternTo0.srcAccessMask = 0;
      depsExternTo0.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      depsExternTo0.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

      VkSubpassDependency &deps0to1 = dependencies.emplace_back();
      deps0to1.srcSubpass = 0;
      deps0to1.dstSubpass = 1;
      deps0to1.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      deps0to1.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      deps0to1.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      deps0to1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      deps0to1.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

      VkSubpassDependency &deps1toExtern = dependencies.emplace_back();
      deps1toExtern.srcSubpass = 0;
      deps1toExtern.dstSubpass = VK_SUBPASS_EXTERNAL;
      deps1toExtern.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      deps1toExtern.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      deps1toExtern.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      deps1toExtern.dstAccessMask = 0;
      deps1toExtern.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    }

    VkRenderPassCreateInfo renderPassInfo = {};
    {
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
      renderPassInfo.pAttachments = attachments.data();
      renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
      renderPassInfo.pSubpasses = subpasses.data();
      renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
      renderPassInfo.pDependencies = dependencies.data();
    }

    VkResult res = vkCreateRenderPass(ctxt.device, &renderPassInfo, nullptr, &mainRenderPass);
    AT3_ASSERT(res == VK_SUCCESS, "Error creating render pass");
  }



  void PipelineRepository::createPipelineLayout(PipelineCreateInfo &info) {

    // Get a reference to the pipeline to be filled
    Pipeline &pipeline = pipelines.at(info.index);

    // This will be used to hold the return values of Vulkan functions for error checking.
    VkResult res;

    { // Populate pipelineLayoutInfo and use it to create the pipeline layout
      VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
      // Create the descriptor set layouts
      for (auto layoutInfo : info.descSetLayoutInfos) {
        pipeline.descSetLayouts.emplace_back();
        res = vkCreateDescriptorSetLayout(
            info.ctxt->device, &layoutInfo, nullptr, &pipeline.descSetLayouts.back());
        AT3_ASSERT(res == VK_SUCCESS, "Error creating desc set layout");
      }

      // Include the descriptor set layouts in the pipeline layout creation info.
      pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(pipeline.descSetLayouts.size());
      pipelineLayoutInfo.pSetLayouts = pipeline.descSetLayouts.data();

      // Include the push constants in the pipeline layout creation info.
      pipelineLayoutInfo.pPushConstantRanges = info.pcRanges.data();
      pipelineLayoutInfo.pushConstantRangeCount = (uint32_t) info.pcRanges.size();

      // Create the pipeline layout.
      res = vkCreatePipelineLayout(info.ctxt->device, &pipelineLayoutInfo, nullptr, &pipeline.layout);
      AT3_ASSERT(res == VK_SUCCESS, "Error creating pipeline layout");
    }
  }

  void PipelineRepository::createPipeline(PipelineCreateInfo &info) {

    // Get a reference to the pipeline to be filled
    Pipeline &pipeline = pipelines.at(info.index);

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
        shaderStages.push_back(tescStageInfo);
      }
      // Tesselation evaluation shader. Optional, but must also come with tesc above
      if (info.teseCode.data) {
        VkPipelineShaderStageCreateInfo teseStageInfo{};
        teseStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        teseStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        teseStageInfo.pName = "main";
        createShaderModule(teseStageInfo.module, info.teseCode.data, info.teseCode.length, *info.ctxt);
        shaderStages.push_back(teseStageInfo);
      }
      // Geometry shader. Optional.
      if (info.geomCode.data) {
        VkPipelineShaderStageCreateInfo geomStageInfo{};
        geomStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        geomStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        geomStageInfo.pName = "main";
        createShaderModule(geomStageInfo.module, info.geomCode.data, info.geomCode.length, *info.ctxt);
        shaderStages.push_back(geomStageInfo);
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

    VkVertexInputBindingDescription bindingDescription {};
    {
      bindingDescription.binding = 0;
      bindingDescription.stride = vertexAttributes->vertexSize;
      bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    {
      vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
      vertexInputInfo.vertexBindingDescriptionCount = 1;
      vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributes->attrCount;
      vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
      vertexInputInfo.pVertexAttributeDescriptions = &vertexAttributes->attrDescriptions[0];
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
      pipelineInfo.pDynamicState = nullptr; // Enable dynamic states here, watch performance?
      pipelineInfo.layout = pipeline.layout;
      pipelineInfo.renderPass = info.renderPass;
      pipelineInfo.pDepthStencilState = &depthStencil;

      pipelineInfo.subpass = info.subpass;

      // Can use this to create new pipelines by deriving from old ones
      pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
      pipelineInfo.basePipelineIndex = -1;
    }

    // Create the pipeline on the gpu.
    res = vkCreateGraphicsPipelines(
        info.ctxt->device, VK_NULL_HANDLE, 1,&pipelineInfo, nullptr, &pipeline.handle);
    AT3_ASSERT(res == VK_SUCCESS, "Error creating graphics pipeline");

    // Cleanup the shader modules now that they've been copied to the gpu.
    for (auto stage : shaderStages) {
      vkDestroyShaderModule(info.ctxt->device, stage.module, nullptr);
    }

  }

  void PipelineRepository::createStandardMeshPipeline(Common &ctxt, uint32_t texArrayLen) {

    // This is populated and then passed to createPipeline.
    PipelineCreateInfo info {};

    { // pipeline type, context, and renderpass
      info.index = MESH;
      info.ctxt = &ctxt;
      info.renderPass = mainRenderPass;
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

    // If this is a re-initialization, the layouts will already exist and do not need to be recreated.
    if (!pipelines.at(info.index).layoutsExist) {
      createPipelineLayout(info);
      pipelines.at(info.index).layoutsExist = true;
    }

    // The pipeline itself will need to be recreated even for a re-initialization, since it's not dynamic.
    // An alternative would be to make the viewport and scissors dynamic, but that could hurt performance.
    // Benchmarking would need to be done to make sure, but for now I'm leaving them non-dynamic.
    // Another option would be to keep both a dynamic and non-dynamic version of each pipeline, so that the dynamic
    // one could be used while the static ones were being recreated.
    createPipeline(info);
  }



  void PipelineRepository::createDeferredGBufferPipeline(Common &ctxt, uint32_t texArrayLen) {

  }

  void PipelineRepository::createDeferredComposePipeline(Common &ctxt, uint32_t texArrayLen) {

  }



  void PipelineRepository::createTriangleDebugPipeline(Common &ctxt, uint32_t texArrayLen) {

    // This is populated and then passed to createPipeline.
    PipelineCreateInfo info {};

    { // pipeline type, context, and renderpass
      info.index = TRI_DEBUG;
      info.ctxt = &ctxt;
      info.renderPass = mainRenderPass;
      info.subpass = 1;
    }

    { // Shaders
      info.vertCode = {triangleDebug_vert_spv, triangleDebug_vert_spv_len};
      info.geomCode = {triangleDebug_geom_spv, triangleDebug_geom_spv_len};
      info.fragCode = {triangleDebug_frag_spv, triangleDebug_frag_spv_len};
    }

    // Descriptor set layout bindings
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    {
      VkDescriptorSetLayoutBinding uboBinding{};
      uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT /*| VK_SHADER_STAGE_GEOMETRY_BIT*/;
      uboBinding.binding = 0;
      uboBinding.descriptorCount = 1;
      layoutBindings.push_back(uboBinding);
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
      pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT /*| VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT*/;
      info.pcRanges.push_back(pcRange);
    }

    // If this is a re-initialization, the layouts will already exist and do not need to be recreated.
    if (!pipelines.at(info.index).layoutsExist) {
      createPipelineLayout(info);
      pipelines.at(info.index).layoutsExist = true;
    }

    // The pipeline itself will need to be recreated even for a re-initialization, since it's not dynamic.
    // An alternative would be to make the viewport and scissors dynamic, but that could hurt performance.
    // Benchmarking would need to be done to make sure, but for now I'm leaving them non-dynamic.
    // Another option would be to keep both a dynamic and non-dynamic version of each pipeline, so that the dynamic
    // one could be used while the static ones were being recreated.
    createPipeline(info);
  }









//  void PipelineRepository::setupDescriptorSetLayout(PipelineCreateInfo &info)
  /*{
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
    {
      VkDescriptorSetLayoutBinding setLayoutBinding{};
      setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      setLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      setLayoutBinding.binding = 0;
      setLayoutBinding.descriptorCount = 1;
      setLayoutBindings.push_back(setLayoutBinding);
    }

    VkDescriptorSetLayoutCreateInfo descriptorLayout {};
    {
      descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      descriptorLayout.pBindings = setLayoutBindings.data();
      descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    }



    VkResult res = vkCreateDescriptorSetLayout(info.ctxt->device, &descriptorLayout, nullptr, &descriptorSetLayouts.scene);

    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
        vks::initializers::pipelineLayoutCreateInfo(
            &descriptorSetLayouts.scene,
            1);

    // Offscreen (scene) rendering pipeline layout
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayouts.offscreen));
  }*/

//  void PipelineRepository::setupDescriptorSet()
  /*{
    std::vector<VkWriteDescriptorSet> writeDescriptorSets;

    VkDescriptorSetAllocateInfo allocInfo =
        vks::initializers::descriptorSetAllocateInfo(
            descriptorPool,
            &descriptorSetLayouts.scene,
            1);

    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.scene));
    writeDescriptorSets =
        {
            // Binding 0: Vertex shader uniform buffer
            vks::initializers::writeDescriptorSet(
                descriptorSets.scene,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                0,
                &uniformBuffers.GBuffer.descriptor)
        };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
  }*/

//  void PipelineRepository::preparePipelines(Common &ctxt, uint32_t texArrayLen)
  /*{
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
        vks::initializers::pipelineInputAssemblyStateCreateInfo(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            0,
            VK_FALSE);

    VkPipelineRasterizationStateCreateInfo rasterizationState =
        vks::initializers::pipelineRasterizationStateCreateInfo(
            VK_POLYGON_MODE_FILL,
            VK_CULL_MODE_BACK_BIT,
            VK_FRONT_FACE_CLOCKWISE,
            0);

    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vks::initializers::pipelineColorBlendAttachmentState(
            0xf,
            VK_FALSE);

    VkPipelineColorBlendStateCreateInfo colorBlendState =
        vks::initializers::pipelineColorBlendStateCreateInfo(
            1,
            &blendAttachmentState);

    VkPipelineDepthStencilStateCreateInfo depthStencilState =
        vks::initializers::pipelineDepthStencilStateCreateInfo(
            VK_TRUE,
            VK_TRUE,
            VK_COMPARE_OP_LESS_OR_EQUAL);

    VkPipelineViewportStateCreateInfo viewportState =
        vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

    VkPipelineMultisampleStateCreateInfo multisampleState =
        vks::initializers::pipelineMultisampleStateCreateInfo(
            VK_SAMPLE_COUNT_1_BIT,
            0);

    std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState =
        vks::initializers::pipelineDynamicStateCreateInfo(
            dynamicStateEnables.data(),
            static_cast<uint32_t>(dynamicStateEnables.size()),
            0);

    // Final fullscreen pass pipeline
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo =
        vks::initializers::pipelineCreateInfo(
            pipelineLayouts.offscreen,
            renderPass,
            0);

    pipelineCreateInfo.pVertexInputState = &vertices.inputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.subpass = 0;

    std::array<VkPipelineColorBlendAttachmentState, 4> blendAttachmentStates = {
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
    };

    colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
    colorBlendState.pAttachments = blendAttachmentStates.data();

    // Offscreen scene rendering pipeline
    shaderStages[0] = loadShader(getAssetPath() + "shaders/subpasses/gbuffer.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(getAssetPath() + "shaders/subpasses/gbuffer.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.offscreen));
  }*/

//  void PipelineRepository::setupRenderPass(Common &ctxt)
  /*{
//    createGBufferAttachments(); // Create attachments in vkc?

    std::array<VkAttachmentDescription, 5> attachments{};
    // Color attachment
//    attachments[0].format = swapChain.colorFormat;
    attachments[0].format = ctxt.swapChain.imageFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Deferred attachments
    // Position
//    attachments[1].format = this->attachments.position.format;
    attachments[1].format = VK_FORMAT_R16G16B16A16_SFLOAT;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Normals
//    attachments[2].format = this->attachments.normal.format;
    attachments[2].format = VK_FORMAT_R16G16B16A16_SFLOAT;
    attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Albedo
//    attachments[3].format = this->attachments.albedo.format;
    attachments[3].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Depth attachment
//    attachments[4].format = depthFormat;
    attachments[4].format = VK_FORMAT_D32_SFLOAT;
    attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[4].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Three subpasses
    std::array<VkSubpassDescription,3> subpassDescriptions{};

    // First subpass: Fill G-Buffer components
    // ----------------------------------------------------------------------------------------

    VkAttachmentReference colorReferences[4];
    colorReferences[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    colorReferences[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    colorReferences[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    colorReferences[3] = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depthReference = { 4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[0].colorAttachmentCount = 4;
    subpassDescriptions[0].pColorAttachments = colorReferences;
    subpassDescriptions[0].pDepthStencilAttachment = &depthReference;

    // Second subpass: Final composition (using G-Buffer components)
    // ----------------------------------------------------------------------------------------

    VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkAttachmentReference inputReferences[3];
    inputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    inputReferences[1] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    inputReferences[2] = { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    uint32_t preserveAttachmentIndex = 1;

    subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[1].colorAttachmentCount = 1;
    subpassDescriptions[1].pColorAttachments = &colorReference;
    subpassDescriptions[1].pDepthStencilAttachment = &depthReference;
    // Use the color attachments filled in the first pass as input attachments
    subpassDescriptions[1].inputAttachmentCount = 3;
    subpassDescriptions[1].pInputAttachments = inputReferences;

    // Third subpass: Forward transparency
    // ----------------------------------------------------------------------------------------
    colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    inputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    subpassDescriptions[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[2].colorAttachmentCount = 1;
    subpassDescriptions[2].pColorAttachments = &colorReference;
    subpassDescriptions[2].pDepthStencilAttachment = &depthReference;
    // Use the color/depth attachments filled in the first pass as input attachments
    subpassDescriptions[2].inputAttachmentCount = 1;
    subpassDescriptions[2].pInputAttachments = inputReferences;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 4> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // This dependency transitions the input attachment from color attachment to shader read
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = 1;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[2].srcSubpass = 1;
    dependencies[2].dstSubpass = 2;
    dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[3].srcSubpass = 0;
    dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[3].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[3].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
    renderPassInfo.pSubpasses = subpassDescriptions.data();
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

    // Create custom overlay render pass
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &uiRenderPass));
  }*/

//  void PipelineRepository::prepareCompositionPass(Common &ctxt)
  /*{
    // Descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
        {
            // Binding 0: Position input attachment
            vks::initializers::descriptorSetLayoutBinding(
                VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0),
            // Binding 1: Normal input attachment
            vks::initializers::descriptorSetLayoutBinding(
                VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                1),
            // Binding 2: Albedo input attachment
            vks::initializers::descriptorSetLayoutBinding(
                VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                2),
            // Binding 3: Light positions
            vks::initializers::descriptorSetLayoutBinding(
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                3),
        };

    VkDescriptorSetLayoutCreateInfo descriptorLayout =
        vks::initializers::descriptorSetLayoutCreateInfo(
            setLayoutBindings.data(),
            static_cast<uint32_t>(setLayoutBindings.size()));

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayouts.composition));

    // Pipeline layout
    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
        vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.composition, 1);

    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayouts.composition));

    // Descriptor sets
    VkDescriptorSetAllocateInfo allocInfo =
        vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.composition, 1);

    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.composition));

    // Image descriptors for the offscreen color attachments
    VkDescriptorImageInfo texDescriptorPosition =
        vks::initializers::descriptorImageInfo(
            VK_NULL_HANDLE,
            attachments.position.view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkDescriptorImageInfo texDescriptorNormal =
        vks::initializers::descriptorImageInfo(
            VK_NULL_HANDLE,
            attachments.normal.view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkDescriptorImageInfo texDescriptorAlbedo =
        vks::initializers::descriptorImageInfo(
            VK_NULL_HANDLE,
            attachments.albedo.view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        // Binding 0: Position texture target
        vks::initializers::writeDescriptorSet(
            descriptorSets.composition,
            VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            0,
            &texDescriptorPosition),
        // Binding 1: Normals texture target
        vks::initializers::writeDescriptorSet(
            descriptorSets.composition,
            VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            1,
            &texDescriptorNormal),
        // Binding 2: Albedo texture target
        vks::initializers::writeDescriptorSet(
            descriptorSets.composition,
            VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            2,
            &texDescriptorAlbedo),
        // Binding 4: Fragment shader lights
        vks::initializers::writeDescriptorSet(
            descriptorSets.composition,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            3,
            &uniformBuffers.lights.descriptor),
    };

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

    // Pipeline
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
        vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

    VkPipelineRasterizationStateCreateInfo rasterizationState =
        vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE, 0);

    VkPipelineColorBlendAttachmentState blendAttachmentState =
        vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);

    VkPipelineColorBlendStateCreateInfo colorBlendState =
        vks::initializers::pipelineColorBlendStateCreateInfo(1,	&blendAttachmentState);

    VkPipelineDepthStencilStateCreateInfo depthStencilState =
        vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

    VkPipelineViewportStateCreateInfo viewportState =
        vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

    VkPipelineMultisampleStateCreateInfo multisampleState =
        vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);

    std::vector<VkDynamicState> dynamicStateEnables = {	VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState =
        vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), static_cast<uint32_t>(dynamicStateEnables.size()), 0);

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    shaderStages[0] = loadShader(getAssetPath() + "shaders/subpasses/composition.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(getAssetPath() + "shaders/subpasses/composition.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    // Use specialization constants to pass number of lights to the shader
    VkSpecializationMapEntry specializationEntry{};
    specializationEntry.constantID = 0;
    specializationEntry.offset = 0;
    specializationEntry.size = sizeof(uint32_t);

    uint32_t specializationData = NUM_LIGHTS;

    VkSpecializationInfo specializationInfo;
    specializationInfo.mapEntryCount = 1;
    specializationInfo.pMapEntries = &specializationEntry;
    specializationInfo.dataSize = sizeof(specializationData);
    specializationInfo.pData = &specializationData;

    shaderStages[1].pSpecializationInfo = &specializationInfo;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo =
        vks::initializers::pipelineCreateInfo(pipelineLayouts.composition, renderPass, 0);

    VkPipelineVertexInputStateCreateInfo emptyInputState{};
    emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    pipelineCreateInfo.pVertexInputState = &emptyInputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    // Index of the subpass that this pipeline will be used in
    pipelineCreateInfo.subpass = 1;

    depthStencilState.depthWriteEnable = VK_FALSE;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.composition));

    // Transparent (forward) pipeline

    // Descriptor set layout
    setLayoutBindings = {
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
        vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
    };

    descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayouts.transparent));

    // Pipeline layout
    pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.transparent, 1);
    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayouts.transparent));

    // Descriptor sets
    allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.transparent, 1);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.transparent));

    writeDescriptorSets = {
        vks::initializers::writeDescriptorSet(descriptorSets.transparent, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.GBuffer.descriptor),
        vks::initializers::writeDescriptorSet(descriptorSets.transparent, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &texDescriptorPosition),
        vks::initializers::writeDescriptorSet(descriptorSets.transparent, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &textures.glass.descriptor),
    };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

    // Enable blending
    blendAttachmentState.blendEnable = VK_TRUE;
    blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    pipelineCreateInfo.pVertexInputState = &vertices.inputState;
    pipelineCreateInfo.layout = pipelineLayouts.transparent;
    pipelineCreateInfo.subpass = 2;

    shaderStages[0] = loadShader(getAssetPath() + "shaders/subpasses/transparent.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] = loadShader(getAssetPath() + "shaders/subpasses/transparent.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.transparent));
  }*/









  void PipelineRepository::createStaticHeightmapTerrainPipeline(Common &ctxt) {

  }

  void PipelineRepository::destroy(Common &ctxt) {
    for (auto pipeline : pipelines) {
      for (auto set : pipeline.descSets) {
        vkFreeDescriptorSets(ctxt.device, ctxt.descriptorPool, (uint32_t)pipeline.descSetLayouts.size(), &set);
      }
      vkDestroyPipeline(ctxt.device, pipeline.handle, nullptr);
      vkDestroyPipelineLayout(ctxt.device, pipeline.layout, nullptr);
      for (auto layout : pipeline.descSetLayouts) {
        vkDestroyDescriptorSetLayout(ctxt.device, layout, nullptr);
      }
      pipeline.descSetLayouts.clear();
      vkDestroyRenderPass(ctxt.device, mainRenderPass, nullptr);
    }
  }

  PipelineRepository::Pipeline &PipelineRepository::at(uint32_t index) {
    return pipelines.at(index);
  }

  void PipelineRepository::reinit(Common &ctxt, uint32_t numTextures2D) {
    createStandardMeshPipeline(ctxt, numTextures2D);
    createTriangleDebugPipeline(ctxt, numTextures2D);
    createStaticHeightmapTerrainPipeline(ctxt);
  }
  const VertexAttributes &PipelineRepository::getVertexAttributes() {
    return *vertexAttributes;
  }
}
