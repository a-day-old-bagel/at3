
#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>
#include <functional>

#include "definitions.hpp"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "settings.hpp"
#include "topics.hpp"
#include "ezecs.hpp"
#include "ecsInterface.hpp"
#include "game.hpp"

#include "ecsSystem_animation.hpp"
#include "ecsSystem_controls.hpp"
#include "ecsSystem_physics.hpp"

#include "cylinderMath.h"

#include "walker.hpp"
#include "duneBuggy.hpp"
#include "pyramid.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

using namespace at3;
using namespace ezecs;
using namespace rtu::topics;

class Triceratone : public Game<EntityComponentSystemInterface, Triceratone> {

    ControlSystem     controlSystem;
    AnimationSystem   animationSystem;
    PhysicsSystem     physicsSystem;

    std::shared_ptr<Object> freeCam;
    std::shared_ptr<ThirdPersonCamera> camera;
    std::shared_ptr<Mesh> terrainArk;
    std::unique_ptr<Pyramid> pyramid;
    std::unique_ptr<DuneBuggy> duneBuggy;
    std::unique_ptr<Walker> player;

    std::unique_ptr<Subscription> keyRSub, key0Sub, key1Sub, key2Sub, key3Sub;

  public:

    Triceratone()
        : controlSystem(&state),
          animationSystem(&state),
          physicsSystem(&state) { }

    ~Triceratone() {
      scene.clear();
    }

    bool onInit() {

      // Initialize the systems
      bool initSuccess = true;
      initSuccess &= controlSystem.init();
      initSuccess &= physicsSystem.init();
      initSuccess &= animationSystem.init();
      assert(initSuccess);

      // an identity matrix
      glm::mat4 ident(1.f);


      glm::mat4 start = glm::translate(ident, {0, -790, -120});
      freeCam = std::make_shared<Object>();
      state.add_Placement(freeCam->getId(), start);
      state.add_FreeControls(freeCam->getId());
      camera = std::make_shared<ThirdPersonCamera>(0.f, 0.f);
      camera->anchorTo(freeCam);
      scene.addObject(freeCam);
      makeFreeCamActiveControl();


      glm::mat4 arkMat = glm::scale(ident, glm::vec3(100.f, 100.f, 100.f));
      terrainArk = std::make_shared<Mesh>(vulkan.get(), "terrainArk", "cliff1024_01", arkMat);
      TriangleMeshInfo info = {
          vulkan->getMeshStoredVertices("terrainArk"),
          vulkan->getMeshStoredIndices("terrainArk"),
          vulkan->getMeshStoredVertexStride(),
      };
      state.add_Physics(terrainArk->getId(), 0, &info, Physics::STATIC_MESH);
      scene.addObject(terrainArk);

      glm::vec3 playerPos = glm::vec3(10, -790, -100);
      glm::mat4 playerMat = glm::translate(ident, playerPos);
      player = std::make_unique<Walker>(state, vulkan.get(), scene, playerMat);

      glm::vec3 buggyPos = glm::vec3(0, -790, -100);
      glm::mat4 buggyMat = glm::translate(ident, buggyPos);
      buggyMat *= glm::mat4(getCylStandingRot(buggyPos, (float)M_PI * -0.5f, 0));
      duneBuggy = std::make_unique<DuneBuggy>(state, vulkan.get(), scene, buggyMat);

      glm::vec3 pyramidPos = glm::vec3(-10, -790, -100);
      glm::mat4 pyramidMat = glm::translate(ident, pyramidPos);
      pyramidMat *= glm::mat4(getCylStandingRot(pyramidPos, (float)M_PI * -0.5f, 0));
      pyramid = std::make_unique<Pyramid>(state, vulkan.get(), scene, pyramidMat);


      key0Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_0", Triceratone::makeFreeCamActiveControl, this);
      key1Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_1", Walker::makeActiveControl, player.get());
      key2Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_2", DuneBuggy::makeActiveControl, duneBuggy.get());
      key3Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_3", Pyramid::makeActiveControl, pyramid.get());
      keyRSub = RTU_MAKE_SUB_UNIQUEPTR("key_down_r", Pyramid::spawnSphere, pyramid.get());


      return true;
    }

    int32_t customSetting = 1337;
    virtual void registerCustomSettings() {
      settings::addCustom("triceratone_customSetting_i", &customSetting);
    }

    void onTick(float dt) {

      // TODO: Not working IN WINDOWS DEBUG BUILD ONLY for some reason?
      pyramid->resizeFire();

      controlSystem.tick(dt);
      physicsSystem.tick(dt);
      animationSystem.tick(dt);
    }

    void makeFreeCamActiveControl() {
      publish<std::shared_ptr<PerspectiveCamera>>("set_primary_camera", camera->actual);
      publish<entityId>("switch_to_free_controls", freeCam->getId());
      publish<entityId>("switch_to_mouse_controls", camera->gimbal->getId());
    }
};

int main(int argc, char **argv) {

  Triceratone game;

  std::cout << std::endl << "AT3 is initializing..." << std::endl;
  if ( ! game.init("triceratone", "at3_triceratone_settings.ini")) {
    return -1;
  }

  std::cout << std::endl << "AT3 has started." << std::endl;
  while ( !game.getIsQuit()) {
    game.tick();
  }

  std::cout << std::endl << "AT3 has finished." << std::endl;
  return 0;
}

#pragma clang diagnostic pop
