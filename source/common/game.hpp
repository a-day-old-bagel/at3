
#pragma once

#include <memory>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "sdlc.hpp"
#include "vkc.hpp"
#include "sceneTree.hpp"
#include "netInterface.hpp"

namespace at3 {

  template <typename EcsInterface> class SceneTree;

  template <typename EcsInterface, typename Derived>
  class Game {

    public:

      Game();
      virtual ~Game() = default;
      bool init(const char *appName, const char *settingsName);
      void tick();
      bool getIsQuit();

    protected:

      typename EcsInterface::State state;
      SceneTree<EcsInterface> scene;

      std::unique_ptr<SdlContext> sdlc;
      std::unique_ptr<vkc::VulkanContext<EcsInterface>> vulkan;
      std::shared_ptr<NetInterface> network;

    private:

      std::unique_ptr<EcsInterface> ecsInterface;
      typename EcsInterface::EcsId currentCameraId = 0;
      rtu::topics::Subscription switchToCamSub;
      rtu::topics::Subscription quitSub;
      std::string settingsFileName;
      bool isQuit = false;

      Derived &topLevel();
      void setCamera(void *camPtr);
      void deInit(void* nothing);

  };

  template<typename EcsInterface, typename Derived>
  Game<EcsInterface, Derived>::Game()
      : switchToCamSub("switch_to_camera", RTU_MTHD_DLGT(&Game::setCamera, this)),
        quitSub("quit", RTU_MTHD_DLGT(&Game::deInit, this))
        { }

  template <typename EcsInterface, typename Derived>
  Derived & Game<EcsInterface, Derived>::topLevel() {
    return *static_cast<Derived*>(this);
  }

  template <typename EcsInterface, typename Derived>
  bool Game<EcsInterface, Derived>::init(const char *appName, const char *settingsName) {

    ecsInterface = std::make_unique<EcsInterface>(&state);
    Object::linkEcs(*ecsInterface);

    settingsFileName = settingsName;
    topLevel().registerCustomSettings();
    settings::loadFromIni(settingsFileName.c_str());

    sdlc = std::make_unique<SdlContext>(appName);

    vkc::VulkanContextCreateInfo<EcsInterface> contextCreateInfo =
        vkc::VulkanContextCreateInfo<EcsInterface>::defaults();
    contextCreateInfo.window = sdlc->getWindow();
    contextCreateInfo.ecs = ecsInterface.get();
    vulkan = std::make_unique<vkc::VulkanContext<EcsInterface>>(contextCreateInfo);

    network = std::make_shared<NetInterface>();
    rtu::topics::publish<std::shared_ptr<NetInterface> &>("set_network_interface", network);

    return topLevel().onInit();
  }

  template <typename EcsInterface, typename Derived>
  bool Game<EcsInterface, Derived>::getIsQuit() {
    return isQuit;
  }

  template <typename EcsInterface, typename Derived>
  void Game<EcsInterface, Derived>::deInit(void* nothing) {
    isQuit = true;
    settings::saveToIni(settingsFileName.c_str());
  }

  /**
   * The main game loop
   * @tparam EcsInterface
   * @tparam Derived
   */
  template <typename EcsInterface, typename Derived>
  void Game<EcsInterface, Derived>::tick() {

    // Poll SDL events and publish the pertinent ones to the appropriate topics (keyboard, mouse, window events, etc).
    sdlc->publishEvents();

    // Call any network updates that need to happen (just listening for packets, probably).
    network->tick();

    // Call any implementation-defined routines that need to happen each frame (all game logic is in here).
    topLevel().onTick(sdlc->getDeltaTime());  // Give it a time delta to use in all systems for consistency

    // Cache the inheritance-aware world-space transforms of objects and have vulkan use those cached values to draw.
    if (currentCameraId) {  // Don't bother if there's no camera - it will crash.
      scene.updateAbsoluteTransformCaches();
      glm::mat4 viewMat = glm::inverse(ecsInterface->getAbsTransform(currentCameraId));
      rtu::topics::publish<glm::mat4>("primary_cam_wv", viewMat);
      vulkan->tick();
    }
  }

  template <typename EcsInterface, typename Derived>
  void Game<EcsInterface, Derived>::setCamera(void *camPtr) {
    currentCameraId = * (typename EcsInterface::EcsId *) camPtr;
  }
}
