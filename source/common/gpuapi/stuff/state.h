#pragma once

#include <SDLvulkan.h>
#include <SDL_events.h>
#include "vkh_alloc.h"
#include "vkh_setup.h"
#include "camera.h"
#include "rendering.h"
#include "topics.hpp"
#include "keyInput.hpp"
#include "timing.h"

#define SUBSCRIBE_EVENT(e,x) std::make_unique<rtu::topics::Subscription>(e, RTU_MTHD_DLGT(&VkbbState::x, this));

class VkbbState {

    std::unique_ptr<rtu::topics::Subscription> quit, mouseMove, keyW, keyA, keyS, keyD, keyF, windowResize;
    float cameraSpeed = 2.f;
    float mouseSpeed = 0.005f;

  public:

    at3::VkhContext context;
    std::vector<at3::MeshAsset> testMesh;
    std::vector<uint32_t> uboIdx;
    Camera::Cam worldCamera;
    bool running = true;
    float leftRight = 0.f;
    float forwardBack = 0.f;

    VkbbState() {
      context.windowWidth = SCREEN_W;
      context.windowHeight = SCREEN_H;

      quit = SUBSCRIBE_EVENT("quit", onQuit);
      mouseMove = SUBSCRIBE_EVENT("mouse_moved", onMMove);
      keyW = SUBSCRIBE_EVENT("key_held_w", onHeldW);
      keyA = SUBSCRIBE_EVENT("key_held_a", onHeldA);
      keyS = SUBSCRIBE_EVENT("key_held_s", onHeldS);
      keyD = SUBSCRIBE_EVENT("key_held_d", onHeldD);
      keyF = SUBSCRIBE_EVENT("key_down_f", onDownF);
      windowResize = SUBSCRIBE_EVENT("window_resized", reInitRendering);

      at3::VkhContextCreateInfo ctxtInfo = {};
      ctxtInfo.types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
      ctxtInfo.types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
      ctxtInfo.types.push_back(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
      ctxtInfo.types.push_back(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
      ctxtInfo.types.push_back(VK_DESCRIPTOR_TYPE_SAMPLER);
      ctxtInfo.typeCounts.push_back(512);
      ctxtInfo.typeCounts.push_back(512);
      ctxtInfo.typeCounts.push_back(512);
      ctxtInfo.typeCounts.push_back(512);
      ctxtInfo.typeCounts.push_back(512);
      ctxtInfo.typeCounts.push_back(1);
      initContext(ctxtInfo, "Uniform Buffer Array Demo", context);

      Camera::init(worldCamera);
    }

    void onQuit() { running = false; }

    void onMMove(void *eventPtr) {
      auto event = (SDL_Event *) eventPtr;
      Camera::rotate(worldCamera, glm::vec3(0.0f, 1.0f, 0.0f), (float) event->motion.xrel * -mouseSpeed);
      Camera::rotate(worldCamera, Camera::localRight(worldCamera), (float) event->motion.yrel * -mouseSpeed);
    }

    void onHeldW() { forwardBack += 1.f; }

    void onHeldS() { forwardBack -= 1.f; }

    void onHeldA() { leftRight += 1.f; }

    void onHeldD() { leftRight -= 1.f; }

    void onDownF() { printf("WINDOW SIZE %ux%u\n", context.windowWidth, context.windowHeight); }

    void tick(double dt) {
      glm::vec3 translation =
          (Camera::localForward(worldCamera) * forwardBack) + (Camera::localRight(worldCamera) * leftRight);
      Camera::translate(worldCamera, translation * cameraSpeed * (float) dt);
      forwardBack = 0.f;
      leftRight = 0.f;
    }

    void reInitRendering() {
      vkDeviceWaitIdle(context.device);
      cleanupRendering();
      at3::getWindowSize(context);
      at3::createSwapchainForSurface(context);
      initRendering(context, (uint32_t)testMesh.size());
    }

    void cleanupRendering() {
      vkDestroyImageView(context.device, context.renderData.depthBuffer.view, nullptr);
      vkDestroyImage(context.device, context.renderData.depthBuffer.handle, nullptr);
      at3::allocators::pool::free(context.renderData.depthBuffer.imageMemory);

      for (size_t i = 0; i < context.renderData.frameBuffers.size(); i++) {
        vkDestroyFramebuffer(context.device, context.renderData.frameBuffers[i], nullptr);
      }

      vkFreeCommandBuffers(context.device, context.gfxCommandPool,
                           static_cast<uint32_t>(context.renderData.commandBuffers.size()),
                           context.renderData.commandBuffers.data());

      vkDestroyPipeline(context.device, context.matData.graphicsPipeline, nullptr);
      vkDestroyPipelineLayout(context.device, context.matData.pipelineLayout, nullptr);
      vkDestroyRenderPass(context.device, context.renderData.mainRenderPass, nullptr);

      for (size_t i = 0; i < context.swapChain.imageViews.size(); i++) {
        vkDestroyImageView(context.device, context.swapChain.imageViews[i], nullptr);
      }

      vkDestroySwapchainKHR(context.device, context.swapChain.swapChain, nullptr);
    }
};

#undef SUBSCRIBE_EVENT
