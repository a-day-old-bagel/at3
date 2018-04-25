
#include "vkcMaterial.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include <stb/stb_image.h>

// Include the shader codes
#include "ubo_array.vert.spv.c"
#include "dynamic_ubo.vert.spv.c"
#include "common_vert.vert.spv.c"
#include "random_frag.frag.spv.c"
#include "debug_normals.frag.spv.c"
#include "static_sun.frag.spv.c"

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

  void createBasicMaterial(unsigned char *vertData, unsigned int vertLen, unsigned char *fragData, unsigned int fragLen,
                           VkcCommon &ctxt, VkhMaterialCreateInfo &createInfo) {

    VkPipelineShaderStageCreateInfo shaderStages[2];

    shaderStages[0] = shaderPipelineStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT);
    createShaderModule(shaderStages[0].module, vertData, vertLen, ctxt);

    shaderStages[1] = shaderPipelineStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT);
    createShaderModule(shaderStages[1].module, fragData, fragLen, ctxt);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = pipelineLayoutCreateInfo(createInfo.descSetLayouts.data(),
                                                                             createInfo.descSetLayouts.size());

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.offset = 0;
    pushConstantRange.size = createInfo.pushConstantRange;
    pushConstantRange.stageFlags = createInfo.pushConstantStages;

    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutInfo.pushConstantRangeCount = 1;

    VkResult res = vkCreatePipelineLayout(ctxt.device, &pipelineLayoutInfo, nullptr, createInfo.outPipelineLayout);
    AT3_ASSERT(res == VK_SUCCESS, "Error creating pipeline layout");

    const VertexRenderData *vertexLayout = vertexRenderData();

    VkVertexInputBindingDescription bindingDescription = vertexInputBindingDescription(0, vertexLayout->vertexSize,
                                                                                       VK_VERTEX_INPUT_RATE_VERTEX);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = pipelineVertexInputStateCreateInfo();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = vertexLayout->attrCount;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = &vertexLayout->attrDescriptions[0];

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = pipelineInputAssemblyStateCreateInfo(
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
    VkViewport vp = viewport(0, 0, static_cast<float>(ctxt.swapChain.extent.width),
                             static_cast<float>(ctxt.swapChain.extent.height), 0.0f, 1.0f);
    VkRect2D scissor = rect2D(0, 0, ctxt.swapChain.extent.width, ctxt.swapChain.extent.height);
    VkPipelineViewportStateCreateInfo viewportState = pipelineViewportStateCreateInfo(&vp, 1, &scissor, 1);

    VkPipelineRasterizationStateCreateInfo rasterizer = pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
    VkPipelineMultisampleStateCreateInfo multisampling = pipelineMultisampleStateCreateInfo();

    VkPipelineColorBlendAttachmentState colorBlendAttachment = pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        VK_FALSE);
    VkPipelineColorBlendStateCreateInfo colorBlending = pipelineColorBlendStateCreateInfo(colorBlendAttachment);

    VkPipelineDepthStencilStateCreateInfo depthStencil = pipelineDepthStencilStateCreateInfo(
        VK_TRUE,
        VK_TRUE,
        VK_COMPARE_OP_LESS_OR_EQUAL);

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
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

  void loadUBOTestMaterial(VkcCommon &ctxt, int num) {
    //create descriptor set layout
    VkDescriptorSetLayoutBinding layoutBinding;
    layoutBinding = descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1);

    VkDescriptorSetLayoutCreateInfo layoutInfo = descriptorSetLayoutCreateInfo(&layoutBinding, 1);

    VkResult res = vkCreateDescriptorSetLayout(ctxt.device, &layoutInfo, nullptr,
                                               &ctxt.matData.descSetLayout);
    AT3_ASSERT(res == VK_SUCCESS, "Error creating desc set layout");


    VkhMaterialCreateInfo createInfo = {};
    createInfo.renderPass = ctxt.renderData.mainRenderPass;
    createInfo.outPipeline = &ctxt.matData.graphicsPipeline;
    createInfo.outPipelineLayout = &ctxt.matData.pipelineLayout;

    createInfo.pushConstantStages = VK_SHADER_STAGE_VERTEX_BIT;
    createInfo.pushConstantRange = sizeof(uint32_t);
    createInfo.descSetLayouts.push_back(ctxt.matData.descSetLayout);

    createBasicMaterial(ubo_array_vert_spv, ubo_array_vert_spv_len,
                        static_sun_frag_spv, static_sun_frag_spv_len, ctxt, createInfo);
  }
}
