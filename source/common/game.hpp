
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

      std::unique_ptr<vkc::VulkanContext<EcsInterface>> vulkan;
      std::shared_ptr<NetInterface> network;

    private:

      std::unique_ptr<SdlContext> sdlc;
      std::shared_ptr<EcsInterface> ecs;
      typename EcsInterface::EcsId currentCameraId = 0;
      rtu::topics::Subscription switchToCamSub;
      rtu::topics::Subscription quitSub;
      std::string settingsFileName;
      bool isQuit = false;

      Derived &game();
      void setCamera(void *camPtr);
      void deInit(void* nothing);

  };

  template<typename EcsInterface, typename Derived>
  Game<EcsInterface, Derived>::Game()
      : switchToCamSub("switch_to_camera", RTU_MTHD_DLGT(&Game::setCamera, this)),
        quitSub("quit", RTU_MTHD_DLGT(&Game::deInit, this))
        { }

  template <typename EcsInterface, typename Derived>
  Derived & Game<EcsInterface, Derived>::game() {
    return *static_cast<Derived*>(this);
  }

  /**
   * The main game initialization process
   * @tparam EcsInterface
   * @tparam Derived
   * @param appName
   * @param settingsName
   * @return true if successful, false otherwise
   */
  template <typename EcsInterface, typename Derived>
  bool Game<EcsInterface, Derived>::init(const char *appName, const char *settingsName) {

    // Read the settings from the ini file, including settings defined in the inheriting class (top level)
    settingsFileName = settingsName;
    game().registerCustomSettings();
    settings::loadFromIni(settingsFileName.c_str());

    // Initialize and publicize the ECS interface object
    ecs = std::make_shared<EcsInterface>(&state);
    rtu::topics::publish<std::shared_ptr<EcsInterface> &>("set_ecs_interface", ecs);

    // Initialize and publicize the network interface object
    network = std::make_shared<NetInterface>();
    rtu::topics::publish<std::shared_ptr<NetInterface> &>("set_network_interface", network);

    // Initialize SDL2, creating the window
    sdlc = std::make_unique<SdlContext>(appName);

    // Initialize Vulkan, giving it the SDL2 window and ECS interface to use
    vkc::VulkanContextCreateInfo<EcsInterface> contextCreateInfo =
        vkc::VulkanContextCreateInfo<EcsInterface>::defaults();
    contextCreateInfo.window = sdlc->getWindow();
    contextCreateInfo.ecs = ecs.get();
    vulkan = std::make_unique<vkc::VulkanContext<EcsInterface>>(contextCreateInfo);

    // Call any implementation-defined initialization routines that need to happen (all system initializations are here)
    return game().onInit();
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
    game().onTick(sdlc->getDeltaTime());  // Give it a time delta to use in all systems for consistency

    // Get the view matrix and use it to render with Vulkan.
    if (currentCameraId) {  // Don't bother if there's no camera - it will crash.
      glm::mat4 viewMat = glm::inverse(ecs->getAbsTransform(currentCameraId));
      // TODO: make this a normal call to the VulkanContext instead of a topic - only VulkanContext subscribes anyway.
      rtu::topics::publish<glm::mat4>("primary_cam_wv", viewMat);
      vulkan->tick();
    }
  }

  template <typename EcsInterface, typename Derived>
  void Game<EcsInterface, Derived>::setCamera(void *camPtr) {
    currentCameraId = * (typename EcsInterface::EcsId *) camPtr;
  }
}
