
#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>
#include <functional>

#include "math.hpp"
#include "settings.hpp"
#include "topics.hpp"
#include "ezecs.hpp"
#include "ecsInterface.hpp"
#include "game.hpp"

#include "ecsSystem_scene.hpp"
#include "ecsSystem_controls.hpp"
#include "ecsSystem_netSync.hpp"
#include "ecsSystem_physics.hpp"

#include "cylinderMath.hpp"

#include "walker.hpp"
#include "duneBuggy.hpp"
#include "pyramid.hpp"

#include "server.hpp"
#include "client.hpp"

using namespace at3;
using namespace ezecs;
using namespace rtu::topics;

struct PlayerAvatarSet{
    entityId ctrlId = 0;
    entityId camId = 0;
    entityId gimbalId = 0;

    std::unique_ptr<Pyramid> pyramid;
    std::unique_ptr<DuneBuggy> duneBuggy;
    std::unique_ptr<Walker> walker;
};

class Triceratone : public Game<EntityComponentSystemInterface, Triceratone> {

    ControlSystem controlSystem;
    NetSyncSystem netSyncSystem;
    PhysicsSystem physicsSystem;
    SceneSystem   sceneSystem;

    std::vector<PlayerAvatarSet> players;

    std::unique_ptr<Subscription> lClickSub, rClickSub, key0Sub, key1Sub, key2Sub, key3Sub;

    PlayerAvatarSet & player() {
      return network->getRole() == settings::network::SERVER ? players.at(0) : players.at(1);
    }

  public:

    Triceratone() : controlSystem(&state), netSyncSystem(&state), physicsSystem(&state), sceneSystem(&state) { }

    bool onInit() {

      // Initialize the systems
      bool initSuccess = true;
      initSuccess &= controlSystem.init();
      initSuccess &= netSyncSystem.init();
      initSuccess &= physicsSystem.init();
      initSuccess &= sceneSystem.init();
      AT3_ASSERT(initSuccess, "System initialization failed!\n");

      // Make sure the transform functions are always registered in the same order, regardless of access order.
      Pyramid::getTopTransFuncDesc();
      Pyramid::getFireTransFuncDesc();
      DuneBuggy::getWheelTransFuncDesc();
      Walker::getBodyTransFuncDesc();

      // an identity matrix
      glm::mat4 ident(1.f);

      // the ark (the cylinder)
      glm::mat4 arkMat = glm::scale(ident, glm::vec3(100.f, 100.f, 100.f));
      entityId arkId;
      state.createEntity(&arkId);
      state.add_Placement(arkId, arkMat);
      state.add_Mesh(arkId, "terrainArk", "cliff1024_01");
      state.add_Physics(arkId, 0, std::make_shared<std::string>("terrainArk"), Physics::STATIC_MESH);
      state.add_SceneNode(arkId, 0);

//      ecs->openEntityRequest();
//      ecs->requestPlacement(glm::scale(ident, glm::vec3(100.f, 100.f, 100.f)));
//      ecs->requestMesh("terrainArk", "cliff1024_01");
//      std::shared_ptr<void> meshName = std::make_shared<std::string>("terrainArk");
//      ecs->requestPhysics(0, meshName, Physics::STATIC_MESH);
//      ecs->requestSceneNode(0);
//      ecs->closeEntityRequest();


//      if (network->getRole() == settings::network::CLIENT) {
//        ecs->openEntityRequest();
//        ecs->requestPlacement(glm::translate(ident, {0, -790, -120}));
//        ecs->requestMesh("humanBean", "");
//        std::shared_ptr<void> radius = std::make_shared<float>(1.f);
//        ecs->requestPhysics(1.f, radius, Physics::SPHERE);
//        ecs->requestSceneNode(0);
//        ecs->closeEntityRequest();
//      }



      // the player avatars
      for (int i = 0; i < 2; ++i) {
        players.emplace_back();

        // the free cameras
        state.createEntity(&players.back().camId);
        float back = 0.f;
        float tilt = 0.f;
        glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, 0.f, back}), tilt , glm::vec3(1.0f, 0.0f, 0.0f));
        state.add_Placement(players.back().camId, camMat);
        state.add_Camera(players.back().camId, settings::graphics::fovy, 0.1f, 10000.f);

        state.createEntity(&players.back().gimbalId);
        state.add_Placement(players.back().gimbalId, ident);
        Placement *placement;
        state.get_Placement(players.back().gimbalId, &placement);
        placement->forceLocalRotationAndScale = true;
        state.add_MouseControls(
            players.back().gimbalId, settings::controls::mouseInvertX, settings::controls::mouseInvertY);

        glm::mat4 start = glm::translate(ident, {0, -790, -120});
        state.createEntity(&players.back().ctrlId);
        state.add_Placement(players.back().ctrlId, start);
        state.add_FreeControls(players.back().ctrlId, players.back().gimbalId);

        state.add_SceneNode(players.back().ctrlId, 0);
        state.add_SceneNode(players.back().gimbalId, players.back().ctrlId);
        state.add_SceneNode(players.back().camId, players.back().gimbalId);

        // the human
        glm::vec3 walkerPos = glm::vec3(10, -790, -100 + i * 10);
        glm::mat4 walkerMat = glm::translate(ident, walkerPos);
        players.back().walker = std::make_unique<Walker>(state, walkerMat);

        // the car
        glm::vec3 buggyPos = glm::vec3(0, -790, -100 + i * 10);
        glm::mat4 buggyMat = glm::translate(ident, buggyPos);
        buggyMat *= glm::mat4(getCylStandingRot(buggyPos, (float) M_PI * -0.5f, 0));
        players.back().duneBuggy = std::make_unique<DuneBuggy>(state, buggyMat);

        // the flying illuminati pyramid
        glm::vec3 pyramidPos = glm::vec3(-10, -790, -100 + i * 10);
        glm::mat4 pyramidMat = glm::translate(ident, pyramidPos);
        pyramidMat *= glm::mat4(getCylStandingRot(pyramidPos, (float) M_PI * -0.5f, 0));
        players.back().pyramid = std::make_unique<Pyramid>(state, pyramidMat);
      }
      makeFreeCamActiveControl();

      // the event subscriptions
      key0Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_0", Triceratone::makeFreeCamActiveControl, this);
      key1Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_1", Walker::makeActiveControl, player().walker.get());
      key2Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_2", DuneBuggy::makeActiveControl, player().duneBuggy.get());
      key3Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_3", Pyramid::makeActiveControl, player().pyramid.get());
      lClickSub = RTU_MAKE_SUB_UNIQUEPTR("mouse_down_left", Pyramid::shootSphere, player().pyramid.get());
      rClickSub = RTU_MAKE_SUB_UNIQUEPTR("mouse_down_right", Pyramid::dropSphere, player().pyramid.get());

      return true;
    }

    std::string exampleSetting = "1337_H4XX0R5";
    virtual void registerCustomSettings() {
      settings::addCustom("example_custom_setting_s", &exampleSetting);
    }

    void onTick(float dt) {
      // Order matters here
      netSyncSystem.tick(dt); // First receive and/or send any network updates about controls or physics state
      controlSystem.tick(dt); // Process all control signals, remote or otherwise
      physicsSystem.tick(dt); // Given these inputs, step the physics simulation
      sceneSystem.tick(dt); // Given the most recent physics and world states, traverse the scene to cache transforms
    }

    void makeFreeCamActiveControl() {
      publish<entityId>("switch_to_camera", player().camId);
      publish<entityId>("switch_to_free_controls", player().ctrlId);
      publish<entityId>("switch_to_mouse_controls", player().gimbalId);
    }
};

int main(int argc, char **argv) {

  Triceratone game;

  std::cout << std::endl << "AT3 is initializing..." << std::endl;
  if ( ! game.init("triceratone", "at3_triceratone_settings.ini")) {
    return -1;
  }

  std::cout << std::endl << "AT3 has started." << std::endl;
  while ( ! game.getIsQuit()) {
    game.tick();
  }

  std::cout << std::endl << "AT3 has finished." << std::endl;
  return 0;
}
