
#include "config.h"

#include "rendering.h"
#include "vkh_material.h"
#include "shader_inputs.h"
#include "vkh_types.h"

// Include the shader codes
#include "ubo_array.vert.spv.c"
#include "dynamic_ubo.vert.spv.c"
#include "common_vert.vert.spv.c"
#include "random_frag.frag.spv.c"
#include "debug_normals.frag.spv.c"
#include "static_sun.frag.spv.c"

void createMainRenderPass(at3::VkhContext &ctxt);

void createDepthBuffer(at3::VkhContext &ctxt);

void loadDebugMaterial(at3::VkhContext &ctxt);

void loadUBOTestMaterial(at3::VkhContext &ctxt, int num);

void initRendering(at3::VkhContext &ctxt, uint32_t num) {
  createMainRenderPass(ctxt);
  createDepthBuffer(ctxt);

  at3::createFrameBuffers(ctxt.renderData.frameBuffers, ctxt.swapChain, &ctxt.renderData.depthBuffer.view,
                          ctxt.renderData.mainRenderPass,
                          ctxt.device);

  uint32_t swapChainImageCount = static_cast<uint32_t>(ctxt.swapChain.imageViews.size());
  ctxt.renderData.commandBuffers.resize(swapChainImageCount);
  for (uint32_t i = 0; i < swapChainImageCount; ++i) {
    at3::createCommandBuffer(ctxt.renderData.commandBuffers[i], ctxt.gfxCommandPool, ctxt.device);
  }
  loadUBOTestMaterial(ctxt, num);

  ctxt.renderData.firstFrame = std::vector<bool>(swapChainImageCount, true);
}

void updateDescriptorSets(at3::VkhContext &ctxt, at3::DataStore *dataStore) {

  size_t oldNumPages = ctxt.matData.descSets.size();
//  size_t newNumPages = ubo_store::getNumPages();
  size_t newNumPages = dataStore->getNumPages();

  ctxt.matData.descSets.resize(newNumPages);

  for (size_t i = oldNumPages; i < newNumPages; ++i) {
    VkDescriptorSetAllocateInfo allocInfo = at3::descriptorSetAllocateInfo(&ctxt.matData.descSetLayout, 1,
                                                                           ctxt.descriptorPool);
    VkResult res = vkAllocateDescriptorSets(ctxt.device, &allocInfo, &ctxt.matData.descSets[i]);
    checkf(res == VK_SUCCESS, "Error allocating global descriptor set");

    ctxt.bufferInfo = {};
    ctxt.bufferInfo.buffer = dataStore->getPage(i);
    ctxt.bufferInfo.offset = 0;
    ctxt.bufferInfo.range = VK_WHOLE_SIZE;

    ctxt.setWrite = {};
    ctxt.setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    ctxt.setWrite.dstBinding = 0;
    ctxt.setWrite.dstArrayElement = 0;
    ctxt.setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ctxt.setWrite.descriptorCount = 1;
    ctxt.setWrite.dstSet = ctxt.matData.descSets[i];
    ctxt.setWrite.pBufferInfo = &ctxt.bufferInfo;
    ctxt.setWrite.pImageInfo = 0;

    vkUpdateDescriptorSets(ctxt.device, 1, &ctxt.setWrite, 0, nullptr);

    printf("Descriptor set created for page: %lu\n", i);
  }
}

void loadUBOTestMaterial(at3::VkhContext &ctxt, int num) {
  //create descriptor set layout
  VkDescriptorSetLayoutBinding layoutBinding;
  layoutBinding = at3::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1);

  VkDescriptorSetLayoutCreateInfo layoutInfo = at3::descriptorSetLayoutCreateInfo(&layoutBinding, 1);

  VkResult res = vkCreateDescriptorSetLayout(ctxt.device, &layoutInfo, nullptr,
                                             &ctxt.matData.descSetLayout);
  checkf(res == VK_SUCCESS, "Error creating desc set layout");


  at3::VkhMaterialCreateInfo createInfo = {};
  createInfo.renderPass = ctxt.renderData.mainRenderPass;
  createInfo.outPipeline = &ctxt.matData.graphicsPipeline;
  createInfo.outPipelineLayout = &ctxt.matData.pipelineLayout;

  createInfo.pushConstantStages = VK_SHADER_STAGE_VERTEX_BIT;
  createInfo.pushConstantRange = sizeof(uint32_t);
  createInfo.descSetLayouts.push_back(ctxt.matData.descSetLayout);

  at3::createBasicMaterial(ubo_array_vert_spv, ubo_array_vert_spv_len,
                           static_sun_frag_spv, static_sun_frag_spv_len, ctxt, createInfo);

}

void loadDebugMaterial(at3::VkhContext &ctxt) {
  at3::VkhMaterialCreateInfo createInfo = {};
  createInfo.outPipeline = &ctxt.matData.graphicsPipeline;
  createInfo.outPipelineLayout = &ctxt.matData.pipelineLayout;
  createInfo.renderPass = ctxt.renderData.mainRenderPass;
  createInfo.pushConstantStages = VK_SHADER_STAGE_VERTEX_BIT;
  createInfo.pushConstantRange = sizeof(glm::mat4) * 2;
  at3::createBasicMaterial(common_vert_vert_spv, common_vert_vert_spv_len, debug_normals_frag_spv,
                           debug_normals_frag_spv_len,
                           ctxt, createInfo);
}

void createDepthBuffer(at3::VkhContext &ctxt) {
  at3::createImage(ctxt.renderData.depthBuffer.handle,
                   ctxt.swapChain.extent.width,
                   ctxt.swapChain.extent.height,
                   VK_FORMAT_D32_SFLOAT,
                   VK_IMAGE_TILING_OPTIMAL,
                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                   ctxt);

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(ctxt.device, ctxt.renderData.depthBuffer.handle, &memRequirements);

  at3::AllocationCreateInfo createInfo;
  createInfo.size = memRequirements.size;
  createInfo.memoryTypeIndex = at3::getMemoryType(ctxt.gpu.device, memRequirements.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  createInfo.usage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  at3::allocateDeviceMemory(ctxt.renderData.depthBuffer.imageMemory, createInfo, ctxt);

  vkBindImageMemory(ctxt.device, ctxt.renderData.depthBuffer.handle, ctxt.renderData.depthBuffer.imageMemory.handle,
                    ctxt.renderData.depthBuffer.imageMemory.offset);

  at3::createImageView(ctxt.renderData.depthBuffer.view, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, 1,
                       ctxt.renderData.depthBuffer.handle, ctxt.device);

  at3::transitionImageLayout(ctxt.renderData.depthBuffer.handle, VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, ctxt);
}

void createMainRenderPass(at3::VkhContext &ctxt) {
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = ctxt.swapChain.imageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depthAttachment = {};
  depthAttachment.format = VK_FORMAT_D32_SFLOAT;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


  std::vector<VkAttachmentDescription> renderPassAttachments;
  renderPassAttachments.push_back(colorAttachment);

  at3::createRenderPass(ctxt.renderData.mainRenderPass, renderPassAttachments, &depthAttachment, ctxt.device);

}
