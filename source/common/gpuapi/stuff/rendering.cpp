#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "config.h"
#include "rendering.h"
#include "vkh_material.h"
#include "shader_inputs.h"
#include "vkh_types.h"
#include "topics.hpp"

#include <glm/gtx/transform.hpp>
#include <glm/glm.hpp>

// Include the shader codes
#include "ubo_array.vert.spv.c"
#include "dynamic_ubo.vert.spv.c"
#include "common_vert.vert.spv.c"
#include "random_frag.frag.spv.c"
#include "debug_normals.frag.spv.c"

void createMainRenderPass(vkh::VkhContext &ctxt);

void createDepthBuffer(vkh::VkhContext &ctxt);

void loadDebugMaterial(vkh::VkhContext &ctxt);

void loadUBOTestMaterial(vkh::VkhContext &ctxt, int num);

int bindDescriptorSets(vkh::VkhContext &ctxt, int curPage, int pageToBind, int slotToBind, VkCommandBuffer &cmd);

void initRendering(vkh::VkhContext &ctxt, uint32_t num) {
  createMainRenderPass(ctxt);
  createDepthBuffer(ctxt);

  vkh::createFrameBuffers(ctxt.renderData.frameBuffers, ctxt.swapChain, &ctxt.renderData.depthBuffer.view,
                          ctxt.renderData.mainRenderPass,
                          ctxt.device);

  uint32_t swapChainImageCount = static_cast<uint32_t>(ctxt.swapChain.imageViews.size());
  ctxt.renderData.commandBuffers.resize(swapChainImageCount);
  for (uint32_t i = 0; i < swapChainImageCount; ++i) {
    vkh::createCommandBuffer(ctxt.renderData.commandBuffers[i], ctxt.gfxCommandPool, ctxt.device);
  }

#if PUSH_TEST
  loadDebugMaterial(ctxt);
#else
  loadUBOTestMaterial(ctxt, num);
#endif
}

void loadUBOTestMaterial(vkh::VkhContext &ctxt, int num) {
  //create descriptor set layout
  VkDescriptorSetLayoutBinding layoutBinding;
  layoutBinding = vkh::descriptorSetLayoutBinding(data_store::getDescriptorType(), VK_SHADER_STAGE_VERTEX_BIT, 0, 1);

  VkDescriptorSetLayoutCreateInfo layoutInfo = vkh::descriptorSetLayoutCreateInfo(&layoutBinding, 1);

  VkResult res = vkCreateDescriptorSetLayout(ctxt.device, &layoutInfo, nullptr,
                                             &ctxt.matData.descSetLayout);
  checkf(res == VK_SUCCESS, "Error creating desc set layout");


  vkh::VkhMaterialCreateInfo createInfo = {};
  createInfo.renderPass = ctxt.renderData.mainRenderPass;
  createInfo.outPipeline = &ctxt.matData.graphicsPipeline;
  createInfo.outPipelineLayout = &ctxt.matData.pipelineLayout;

  createInfo.pushConstantStages = VK_SHADER_STAGE_VERTEX_BIT;
  createInfo.pushConstantRange = sizeof(uint32_t);
  createInfo.descSetLayouts.push_back(ctxt.matData.descSetLayout);

#if WITH_COMPLEX_SHADER
  vkh::createBasicMaterial(VERT_SHADER_NAME, VERT_SHADER_LEN, random_frag_frag_spv, random_frag_frag_spv_len, ctxt, createInfo);
#else
  vkh::createBasicMaterial(VERT_SHADER_NAME, VERT_SHADER_LEN, debug_normals_frag_spv, debug_normals_frag_spv_len, ctxt, createInfo);
#endif

  //allocate a descriptor set for each ubo transform array
  ctxt.matData.descSets.resize(data_store::getNumPages());

  for (uint32_t i = 0; i < data_store::getNumPages(); ++i) {
    VkDescriptorSetAllocateInfo allocInfo = vkh::descriptorSetAllocateInfo(&ctxt.matData.descSetLayout, 1,
                                                                           ctxt.descriptorPool);
    res = vkAllocateDescriptorSets(ctxt.device, &allocInfo, &ctxt.matData.descSets[i]);
    checkf(res == VK_SUCCESS, "Error allocating global descriptor set");

    ctxt.bufferInfo = {};
    ctxt.bufferInfo.buffer = data_store::getPage(i);
    ctxt.bufferInfo.offset = 0;
    ctxt.bufferInfo.range = VK_WHOLE_SIZE;

    ctxt.setWrite = {};
    ctxt.setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    ctxt.setWrite.dstBinding = 0;
    ctxt.setWrite.dstArrayElement = 0;
    ctxt.setWrite.descriptorType = data_store::getDescriptorType();
    ctxt.setWrite.descriptorCount = 1;
    ctxt.setWrite.dstSet = ctxt.matData.descSets[i];
    ctxt.setWrite.pBufferInfo = &ctxt.bufferInfo;
    ctxt.setWrite.pImageInfo = 0;

    vkUpdateDescriptorSets(ctxt.device, 1, &ctxt.setWrite, 0, nullptr);
  }
}

void loadDebugMaterial(vkh::VkhContext &ctxt) {
  vkh::VkhMaterialCreateInfo createInfo = {};
  createInfo.outPipeline = &ctxt.matData.graphicsPipeline;
  createInfo.outPipelineLayout = &ctxt.matData.pipelineLayout;
  createInfo.renderPass = ctxt.renderData.mainRenderPass;
  createInfo.pushConstantStages = VK_SHADER_STAGE_VERTEX_BIT;
  createInfo.pushConstantRange = sizeof(glm::mat4) * 2;

#if WITH_COMPLEX_SHADER
  vkh::createBasicMaterial(common_vert_vert_spv, common_vert_vert_spv_len, random_frag_frag_spv, random_frag_frag_spv_len,
                           ctxt, createInfo);
#else
  vkh::createBasicMaterial(common_vert_vert_spv, common_vert_vert_spv_len, debug_normals_frag_spv, debug_normals_frag_spv_len,
                           ctxt, createInfo);
#endif

}


void createDepthBuffer(vkh::VkhContext &ctxt) {
  vkh::createImage(ctxt.renderData.depthBuffer.handle,
                   ctxt.swapChain.extent.width,
                   ctxt.swapChain.extent.height,
                   VK_FORMAT_D32_SFLOAT,
                   VK_IMAGE_TILING_OPTIMAL,
                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                   ctxt);

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(ctxt.device, ctxt.renderData.depthBuffer.handle, &memRequirements);

  vkh::AllocationCreateInfo createInfo;
  createInfo.size = memRequirements.size;
  createInfo.memoryTypeIndex = vkh::getMemoryType(ctxt.gpu.device, memRequirements.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  createInfo.usage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  vkh::allocateDeviceMemory(ctxt.renderData.depthBuffer.imageMemory, createInfo, ctxt);

  vkBindImageMemory(ctxt.device, ctxt.renderData.depthBuffer.handle, ctxt.renderData.depthBuffer.imageMemory.handle,
                    ctxt.renderData.depthBuffer.imageMemory.offset);

  vkh::createImageView(ctxt.renderData.depthBuffer.view, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, 1,
                       ctxt.renderData.depthBuffer.handle, ctxt.device);

  vkh::transitionImageLayout(ctxt.renderData.depthBuffer.handle, VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, ctxt);
}

void createMainRenderPass(vkh::VkhContext &ctxt) {
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

  vkh::createRenderPass(ctxt.renderData.mainRenderPass, renderPassAttachments, &depthAttachment, ctxt.device);

}


void render(vkh::VkhContext &ctxt, Camera::Cam &cam, const std::vector<vkh::MeshAsset> &drawCalls,
            const std::vector<uint32_t> &uboIdx) {
  const glm::mat4 view = Camera::viewMatrix(cam);

  glm::mat4 p = glm::perspectiveRH(glm::radians(60.0f), ctxt.windowWidth / (float) ctxt.windowHeight, 0.1f, 10000.0f);
  //from https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
  //this flips the y coordinate back to positive == up, and readjusts depth range to match opengl
  glm::mat4 vulkanCorrection = glm::mat4(
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.5f, 0.0f,
      0.0f, 0.0f, 0.5f, 1.0f);

  static float rads = 0.001f;
  glm::mat4 proj = vulkanCorrection * p;

#if !COPY_ON_MAIN_COMMANDBUFFER
  data_store::updateBuffers(view, proj, nullptr, ctxt);
#endif

  VkResult res;

  //acquire an image from the swap chain
  uint32_t imageIndex;

  res = vkAcquireNextImageKHR(ctxt.device, ctxt.swapChain.swapChain, UINT64_MAX,
                              ctxt.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

  if (res == VK_ERROR_OUT_OF_DATE_KHR) {
    rtu::topics::publish("window_resized");
    return;
  } else {
    checkf(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR, "failed to acquire swap chain image!");
  }

  vkWaitForFences(ctxt.device, 1, &ctxt.frameFences[imageIndex], VK_FALSE, 5000000000);
  vkResetFences(ctxt.device, 1, &ctxt.frameFences[imageIndex]);

  //record drawing
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = nullptr; // Optional

  vkResetCommandBuffer(ctxt.renderData.commandBuffers[imageIndex], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  res = vkBeginCommandBuffer(ctxt.renderData.commandBuffers[imageIndex], &beginInfo);

#if COPY_ON_MAIN_COMMANDBUFFER
  data_store::updateBuffers(view, proj, &ctxt.renderData.commandBuffers[imageIndex], ctxt);
#endif


  vkCmdResetQueryPool(ctxt.renderData.commandBuffers[imageIndex], ctxt.queryPool, 0, 10);
  vkCmdWriteTimestamp(ctxt.renderData.commandBuffers[imageIndex], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                      ctxt.queryPool, 0);


  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = ctxt.renderData.mainRenderPass;
  renderPassInfo.framebuffer = ctxt.renderData.frameBuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = ctxt.swapChain.extent;

  VkClearValue clearColors[2];
  clearColors[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
  clearColors[1].depthStencil.depth = 1.0f;
  clearColors[1].depthStencil.stencil = 0;

  renderPassInfo.clearValueCount = static_cast<uint32_t>(2);
  renderPassInfo.pClearValues = &clearColors[0];

  vkCmdBeginRenderPass(ctxt.renderData.commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  int currentlyBound = -1;

  vkCmdBindPipeline(ctxt.renderData.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
                    ctxt.matData.graphicsPipeline);

  for (uint32_t i = 0; i < drawCalls.size(); ++i) {
#if UBO_TEST || SSBO_TEST
    glm::uint32 uboSlot = uboIdx[i] >> 3;
    glm::uint32 uboPage = uboIdx[i] & 0x7;

    currentlyBound = bindDescriptorSets(ctxt, currentlyBound, uboPage, uboSlot,
                                        ctxt.renderData.commandBuffers[imageIndex]);

    vkCmdPushConstants(
        ctxt.renderData.commandBuffers[imageIndex],
        ctxt.matData.pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(glm::uint32),
        (void *) &uboSlot);

#elif PUSH_TEST

    //0 is MVP, 1 is normal
    glm::mat4 frameData[2];

    frameData[0] = vulkanCorrection * p * view;
    frameData[1] = glm::transpose(glm::inverse(view));

    vkCmdPushConstants(
      ctxt.renderData.commandBuffers[imageIndex],
      ctxt.matData.pipelineLayout,
      VK_SHADER_STAGE_VERTEX_BIT,
      0,
      sizeof(glm::mat4)*2,
      (void*)&frameData);
#endif

    VkBuffer vertexBuffers[] = {drawCalls[i].buffer};
    VkDeviceSize vertexOffsets[] = {0};
    vkCmdBindVertexBuffers(ctxt.renderData.commandBuffers[imageIndex], 0, 1, vertexBuffers, vertexOffsets);
    vkCmdBindIndexBuffer(ctxt.renderData.commandBuffers[imageIndex], drawCalls[i].buffer, drawCalls[i].iOffset,
                         VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(ctxt.renderData.commandBuffers[imageIndex], static_cast<uint32_t>(drawCalls[i].iCount), 1, 0, 0,
                     0);
  }

  vkCmdEndRenderPass(ctxt.renderData.commandBuffers[imageIndex]);
  vkCmdWriteTimestamp(ctxt.renderData.commandBuffers[imageIndex], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                      ctxt.queryPool, 1);

  res = vkEndCommandBuffer(ctxt.renderData.commandBuffers[imageIndex]);
  checkf(res == VK_SUCCESS, "Error ending render pass");

  // submit

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  //wait on writing colours to the buffer until the semaphore says the buffer is available
  VkSemaphore waitSemaphores[] = {ctxt.imageAvailableSemaphore};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;

  VkSemaphore signalSemaphores[] = {ctxt.renderFinishedSemaphore};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;
  submitInfo.pCommandBuffers = &ctxt.renderData.commandBuffers[imageIndex];
  submitInfo.commandBufferCount = 1;

  res = vkQueueSubmit(ctxt.deviceQueues.graphicsQueue, 1, &submitInfo, ctxt.frameFences[imageIndex]);
  checkf(res == VK_SUCCESS, "Error submitting queue");

  //present

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {ctxt.swapChain.swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr; // Optional
  res = vkQueuePresentKHR(ctxt.deviceQueues.transferQueue, &presentInfo);

#if WITH_VK_TIMESTAMP
  //log performance data:

  uint32_t end = 0;
  uint32_t begin = 0;

  static int count = 0;
  static float totalTime = 0.0f;

  if (count++ > 1024)
  {
    printf("VK Render Time (avg of past 1024 frames): %f ms\n", totalTime / 1024.0f);
    count = 0;
    totalTime = 0;
  }
  float timestampFrequency = ctxt.gpu.deviceProps.limits.timestampPeriod;


  vkGetQueryPoolResults(ctxt.device, ctxt.queryPool, 1, 1, sizeof(uint32_t), &end, 0, VK_QUERY_RESULT_WAIT_BIT);
  vkGetQueryPoolResults(ctxt.device, ctxt.queryPool, 0, 1, sizeof(uint32_t), &begin, 0, VK_QUERY_RESULT_WAIT_BIT);
  uint32_t diff = end - begin;
  totalTime += (diff) / (float)1e6;
#endif

  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
    rtu::topics::publish("window_resized");
  } else {
    checkf(res == VK_SUCCESS , "failed to present swap chain image!");
  }

}

int bindDescriptorSets(vkh::VkhContext &ctxt, int currentlyBound, int page, int slot, VkCommandBuffer &cmd) {

  size_t uboAlignment = ctxt.gpu.deviceProps.limits.minUniformBufferOffsetAlignment;
  size_t dynamicAlignment = ((sizeof(VShaderInput) / uboAlignment) * uboAlignment) +
                            (((sizeof(VShaderInput) % uboAlignment) > 0 ? uboAlignment : 0));

  uint32_t offsetCount = DYNAMIC_UBO;
  uint32_t offset = offsetCount > 0 ? slot * dynamicAlignment : 0;

  if (currentlyBound != page) {
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctxt.matData.pipelineLayout, 0, 1,
                            &ctxt.matData.descSets[page], offsetCount, &offset);
  }
  return page;
}
