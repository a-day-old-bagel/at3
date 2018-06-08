
#include "vkcMaterial.hpp"

// TODO: get rid of stb
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include <stb/stb_image.h>

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

  void createBasicMaterial(
      unsigned char *vertData, unsigned int vertLen, unsigned char *fragData, unsigned int fragLen, VkcCommon &ctxt,
      VkcMaterialCreateInfo &createInfo, const VkSpecializationInfo* specializationInfo) {

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    VkPipelineShaderStageCreateInfo vertexStageInfo {};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStageInfo.pName = "main";
    createShaderModule(vertexStageInfo.module, vertData, vertLen, ctxt);
    shaderStages.push_back(vertexStageInfo);

    VkPipelineShaderStageCreateInfo fragmentStageInfo {};
    fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStageInfo.pName = "main";
    fragmentStageInfo.pSpecializationInfo = specializationInfo;
    createShaderModule(fragmentStageInfo.module, fragData, fragLen, ctxt);
    shaderStages.push_back(fragmentStageInfo);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(createInfo.descSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = createInfo.descSetLayouts.data();

    pipelineLayoutInfo.pPushConstantRanges = createInfo.pcRanges.data();
    pipelineLayoutInfo.pushConstantRangeCount = (uint32_t) createInfo.pcRanges.size();

    VkResult res = vkCreatePipelineLayout(ctxt.device, &pipelineLayoutInfo, nullptr, createInfo.outPipelineLayout);
    AT3_ASSERT(res == VK_SUCCESS, "Error creating pipeline layout");

    const VertexRenderData *vertexLayout = vertexRenderData();

    VkVertexInputBindingDescription bindingDescription {};
    bindingDescription.binding = 0;
    bindingDescription.stride = vertexLayout->vertexSize;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = vertexLayout->attrCount;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = &vertexLayout->attrDescriptions[0];

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport vp {};
    vp.x = 0;
    vp.y = 0;
    vp.width = static_cast<float>(ctxt.swapChain.extent.width);
    vp.height = static_cast<float>(ctxt.swapChain.extent.height);
    vp.minDepth = 0.f;
    vp.maxDepth = 1.f;

    VkRect2D scissor {};
    scissor.extent.width = ctxt.swapChain.extent.width;
    scissor.extent.height = ctxt.swapChain.extent.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pScissors = &scissor;
    viewportState.scissorCount = 1;
    viewportState.pViewports = &vp;
    viewportState.viewportCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer {};
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

    VkPipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE; //can be used for bitwise blending
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
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
    pipelineInfo.layout = *createInfo.outPipelineLayout;
    pipelineInfo.renderPass = createInfo.renderPass;
    pipelineInfo.pDepthStencilState = &depthStencil;

    pipelineInfo.subpass = 0;

    //can use this to create new pipelines by deriving from old ones
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    res = vkCreateGraphicsPipelines(ctxt.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, createInfo.outPipeline);
    AT3_ASSERT(res == VK_SUCCESS, "Error creating graphics pipeline");

    vkDestroyShaderModule(ctxt.device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(ctxt.device, shaderStages[1].module, nullptr);

  }

  void loadUBOTestMaterial(VkcCommon &ctxt, uint32_t texArrayLen) {

    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

    VkDescriptorSetLayoutBinding uboBinding {};
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboBinding.binding = 0;
    uboBinding.descriptorCount = 1;
    layoutBindings.push_back(uboBinding);

    VkDescriptorSetLayoutBinding textureArrayBinding {};
    textureArrayBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureArrayBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    textureArrayBinding.binding = 1;
    textureArrayBinding.descriptorCount = texArrayLen;
    layoutBindings.push_back(textureArrayBinding);

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutInfo.pBindings = layoutBindings.data();

    VkResult res = vkCreateDescriptorSetLayout(ctxt.device, &layoutInfo, nullptr, &ctxt.matData.descSetLayout);
    AT3_ASSERT(res == VK_SUCCESS, "Error creating desc set layout");

    VkcMaterialCreateInfo createInfo = {};
    createInfo.renderPass = ctxt.renderData.mainRenderPass;
    createInfo.outPipeline = &ctxt.matData.graphicsPipeline;
    createInfo.outPipelineLayout = &ctxt.matData.pipelineLayout;

    VkPushConstantRange pcRange = {};
    pcRange.offset = 0;
    pcRange.size = sizeof(MeshInstanceIndices::rawType);
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    createInfo.pcRanges.push_back(pcRange);

    createInfo.descSetLayouts.push_back(ctxt.matData.descSetLayout);


    // Specialization constants
    struct SpecializationData {
      uint32_t textureArrayLength = 1;
    } specializationData;
    specializationData.textureArrayLength = texArrayLen;

    std::vector<VkSpecializationMapEntry> specializationMapEntries;

    VkSpecializationMapEntry textureArrayLengthEntry {};
    textureArrayLengthEntry.constantID = 0;
    textureArrayLengthEntry.size = sizeof(specializationData.textureArrayLength);
    textureArrayLengthEntry.offset = static_cast<uint32_t>(offsetof(SpecializationData, textureArrayLength));
    specializationMapEntries.push_back(textureArrayLengthEntry);

    VkSpecializationInfo specializationInfo{};
    specializationInfo.dataSize = sizeof(specializationData);
    specializationInfo.mapEntryCount = static_cast<uint32_t>(specializationMapEntries.size());
    specializationInfo.pMapEntries = specializationMapEntries.data();
    specializationInfo.pData = &specializationData;


//    createBasicMaterial(ubo_array_vert_spv, ubo_array_vert_spv_len,
//                        static_sun_frag_spv, static_sun_frag_spv_len, ctxt, createInfo, &specializationInfo);
    createBasicMaterial(meshDefault_vert_spv, meshDefault_vert_spv_len,
                        meshDefault_frag_spv, meshDefault_frag_spv_len,
                        ctxt, createInfo, &specializationInfo);
  }
}
