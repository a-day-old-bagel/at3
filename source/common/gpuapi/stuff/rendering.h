#pragma once

#include <cstdint>
#include <unordered_map>
#include "vkh.h"
#include "vkh_mesh.h"
#include "dataStore.h"
#include "topics.hpp"

#if USE_AT3_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/glm.hpp>

void initRendering(at3::VkhContext &, uint32_t num);

void updateDescriptorSets(at3::VkhContext &ctxt, at3::DataStore *dataStore);

template <typename EcsInterface>
void render(at3::VkhContext &ctxt, at3::DataStore* dataStore, const glm::mat4 &wvMat,
            const std::unordered_map<std::string, std::vector<at3::MeshAsset<EcsInterface>>> &meshAssets,
            EcsInterface *ecs){

  glm::mat4 proj = glm::perspective(glm::radians(60.f), ctxt.windowWidth / (float) ctxt.windowHeight, 0.1f, 10000.f);
  // reverse the y
  proj[1][1] *= -1;

#if !COPY_ON_MAIN_COMMANDBUFFER
//  ubo_store::updateBuffers(wvMat, proj, nullptr, ctxt);
  dataStore->updateBuffers(wvMat, proj, nullptr, ctxt, ecs, meshAssets);
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
  dataStore->updateBuffers(view, proj, &ctxt.renderData.commandBuffers[imageIndex], ctxt);
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

  for (auto pair : meshAssets) {
    for (auto mesh : pair.second) {
      for (auto instance : mesh.instances) {
        glm::uint32 uboSlot = instance.uboIdx >> 16;
        glm::uint32 uboPage = instance.uboIdx & 0xFFFF;

        if (currentlyBound != uboPage) {
          vkCmdBindDescriptorSets(ctxt.renderData.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  ctxt.matData.pipelineLayout, 0, 1, &ctxt.matData.descSets[uboPage], 0, nullptr);
          currentlyBound = uboPage;
        }

        vkCmdPushConstants(
            ctxt.renderData.commandBuffers[imageIndex],
            ctxt.matData.pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(glm::uint32),
            (void *) &uboSlot);

        VkBuffer vertexBuffers[] = {mesh.buffer};
        VkDeviceSize vertexOffsets[] = {0};
        vkCmdBindVertexBuffers(ctxt.renderData.commandBuffers[imageIndex], 0, 1, vertexBuffers, vertexOffsets);
        vkCmdBindIndexBuffer(ctxt.renderData.commandBuffers[imageIndex], mesh.buffer, mesh.iOffset,
                             VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(ctxt.renderData.commandBuffers[imageIndex], static_cast<uint32_t>(mesh.iCount), 1, 0, 0,
                         0);
      }
    }
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

#if PRINT_VK_FRAMETIMES
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

  // A "firstFrame" bool is easier than setting up more synchronization objects.
  if (ctxt.renderData.firstFrame[imageIndex]) {
    ctxt.renderData.firstFrame[imageIndex] = false;
  } else {
    vkGetQueryPoolResults(ctxt.device, ctxt.queryPool, 1, 1, sizeof(uint32_t), &end, 0, VK_QUERY_RESULT_WAIT_BIT);
    vkGetQueryPoolResults(ctxt.device, ctxt.queryPool, 0, 1, sizeof(uint32_t), &begin, 0, VK_QUERY_RESULT_WAIT_BIT);
  }
  uint32_t diff = end - begin;
  totalTime += (diff) / (float)1e6;
#endif

  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
    rtu::topics::publish("window_resized");
  } else {
    checkf(res == VK_SUCCESS, "failed to present swap chain image!");
  }

}
