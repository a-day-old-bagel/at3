
#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>
#include <functional>

#include "math.hpp"
#include "settings.hpp"
#include "topics.hpp"
#include "ezecs.hpp"
#include "interface.hpp"
#include "game.hpp"

#include "scene.hpp"
#include "controls.hpp"
#include "network.hpp"
#include "physics.hpp"

#include "cylinderMath.hpp"

#include "walker.hpp"
#include "duneBuggy.hpp"
#include "pyramid.hpp"

#include "server.hpp"
#include "client.hpp"

using namespace at3;
using namespace ezecs;
using namespace rtu::topics;

//struct PlayerAvatarSet{
//    entityId ctrlId = 0;
//    entityId camId = 0;
//    entityId gimbalId = 0;
//
//    std::unique_ptr<Pyramid> pyramid;
//    std::unique_ptr<DuneBuggy> duneBuggy;
//    std::unique_ptr<Walker> walker;
//};

class Triceratone : public Game<EntityComponentSystemInterface, Triceratone> {

    ControlSystem controlSystem;
    NetworkSystem networkSystem;
    PhysicsSystem physicsSystem;
    SceneSystem   sceneSystem;

//    std::vector<PlayerAvatarSet> players;

    std::unique_ptr<Subscription> playerIdAssignmentSub, key1Sub, key2Sub, key3Sub, key4Sub;

    entityId playerId = 0;

//    PlayerAvatarSet & player() {
//      return network->getRole() == settings::network::SERVER ? players.at(0) : players.at(1);
//    }

  public:

    Triceratone() : controlSystem(&state), networkSystem(&state), physicsSystem(&state), sceneSystem(&state) { }

    bool onInit() {

      // Initialize the systems
      bool initSuccess = true;
      initSuccess &= controlSystem.init();
      initSuccess &= networkSystem.init();
      initSuccess &= physicsSystem.init();
      initSuccess &= sceneSystem.init();
      AT3_ASSERT(initSuccess, "System initialization failed!\n");

      // Make sure static transform functions are always registered in the same order, regardless of access order.
      registerStaticTrasformFunctions();

      // A client should not be directly creating any entities. The game world only needs to be created by non-clients.
      if (network->getRole() != settings::network::CLIENT) {
        // Create the map
        ecs->openEntityRequest();
        ecs->requestPlacement(glm::mat4(1.f));
        ecs->requestMesh("terrainArk", "cliff1024_01");
        ecs->requestStaticMeshPhysics("terrainArk");
        ecs->requestSceneNode(0);
        ecs->closeEntityRequest();
        // Create a player
        playerId = ecs->createManualPlayer();
      } else {
        // Set up the callback for player ID assignment
        playerIdAssignmentSub = RTU_MAKE_SUB_UNIQUEPTR("set_player_id", Triceratone::assignPlayerId, this);
      }

      // A client needs to wait for a player ID to be assigned. For non-clients, playerId is already non-zero.
      while ( ! playerId) {
        network->tick();
        networkSystem.tick(0);
        SDL_Delay(50);
      }

      // Start the player off in the freeCam avatar
      switchToFreeControl();

      // Set up the controls for switching between player avatars
      key1Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_1", Triceratone::switchToFreeControl, this);
      key2Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_2", Triceratone::switchToWalkControl, this);
      key3Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_3", Triceratone::switchToPyramidControl, this);
      key4Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_4", Triceratone::switchToBuggyControl, this);


//      // an identity matrix
//      glm::mat4 ident(1.f);
//
//      // the ark (the cylinder)
//      entityId arkId;
//      state.createEntity(&arkId);
//      state.addPlacement(arkId, ident);
//      state.addMesh(arkId, "terrainArk", "cliff1024_01");
//      state.addPhysics(arkId, 0, std::make_shared<std::string>("terrainArk"), Physics::STATIC_MESH);
//      state.addSceneNode(arkId, 0);
//
//      // the player avatars
//      for (int i = 0; i < 2; ++i) {
//        players.emplace_back();
//
//        // the free cameras
//        state.createEntity(&players.back().camId);
//        float back = 0.f;
//        float tilt = 0.f;
//        glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, 0.f, back}), tilt , glm::vec3(1.0f, 0.0f, 0.0f));
//        state.addPlacement(players.back().camId, camMat);
//        state.addCamera(players.back().camId, settings::graphics::fovy, 0.1f, 10000.f);
//
//        state.createEntity(&players.back().gimbalId);
//        state.addPlacement(players.back().gimbalId, ident);
//        Placement *placement;
//        state.getPlacement(players.back().gimbalId, &placement);
//        placement->forceLocalRotationAndScale = true;
//        state.addMouseControls(
//            players.back().gimbalId, settings::controls::mouseInvertX, settings::controls::mouseInvertY);
//
//        glm::mat4 start = glm::translate(ident, {0, -790, -120});
//        state.createEntity(&players.back().ctrlId);
//        state.addPlacement(players.back().ctrlId, start);
//        state.addFreeControls(players.back().ctrlId, players.back().gimbalId);
//
//        state.addSceneNode(players.back().ctrlId, 0);
//        state.addSceneNode(players.back().gimbalId, players.back().ctrlId);
//        state.addSceneNode(players.back().camId, players.back().gimbalId);
//
//        // the human
//        glm::vec3 walkerPos = glm::vec3(10, -790, -100 + i * 10);
//        glm::mat4 walkerMat = glm::translate(ident, walkerPos);
//        players.back().walker = std::make_unique<Walker>(state, walkerMat);
//
//        // the car
//        glm::vec3 buggyPos = glm::vec3(0, -790, -100 + i * 10);
//        glm::mat4 buggyMat = glm::translate(ident, buggyPos);
//        buggyMat *= glm::mat4(getCylStandingRot(buggyPos, (float) M_PI * -0.5f, 0));
//        players.back().duneBuggy = std::make_unique<DuneBuggy>(state, buggyMat);
//
//        // the flying illuminati pyramid
//        glm::vec3 pyramidPos = glm::vec3(-10, -790, -100 + i * 10);
//        glm::mat4 pyramidMat = glm::translate(ident, pyramidPos);
//        pyramidMat *= glm::mat4(getCylStandingRot(pyramidPos, (float) M_PI * -0.5f, 0));
//        players.back().pyramid = std::make_unique<Pyramid>(state, ecs, pyramidMat);
//      }
//      makeFreeCamActiveControl();
//
//      // the event subscriptions
//      key0Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_0", Triceratone::makeFreeCamActiveControl, this);
//      key1Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_1", Walker::makeActiveControl, player().walker.get());
//      key2Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_2", DuneBuggy::makeActiveControl, player().duneBuggy.get());
//      key3Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_3", Pyramid::makeActiveControl, player().pyramid.get());

      return initSuccess;
    }

    void registerStaticTrasformFunctions() {
      Pyramid::getTopTransFuncDesc();
      Pyramid::getFireTransFuncDesc();
      DuneBuggy::getWheelTransFuncDesc();
      Walker::getBodyTransFuncDesc();
    }

    std::string exampleSetting = "1337_H4XX0R5";
    virtual void registerCustomSettings() {
      settings::addCustom("example_custom_setting_s", &exampleSetting);
    }

    void onTick(float dt) {
      // Order matters here
      networkSystem.tick(dt); // First receive and/or send any network updates about controls or physics state
      controlSystem.tick(dt); // Process all control signals, remote or otherwise
      physicsSystem.tick(dt); // Given these inputs, step the physics simulation
      sceneSystem.tick(dt); // Given the most recent physics and world states, traverse the scene to cache transforms
    }

    void assignPlayerId(void * playerId) {
      this->playerId = *(entityId*)playerId;
    }
    void switchToFreeControl() {
      Player *player;
      SceneNode *sceneNode;
      state.getPlayer(playerId, &player);
      publish<entityId>("switch_to_camera", player->free);
      state.getSceneNode(player->free, &sceneNode);
      publish<entityId>("switch_to_mouse_controls", sceneNode->parentId);
      state.getSceneNode(sceneNode->parentId, &sceneNode);
      publish<entityId>("switch_to_free_controls", sceneNode->parentId);
    }
    void switchToWalkControl() {

    }
    void switchToPyramidControl() {

    }
    void switchToBuggyControl() {

    }

//    void makeFreeCamActiveControl() {
//      publish<entityId>("switch_to_camera", player().camId);
//      publish<entityId>("switch_to_free_controls", player().ctrlId);
//      publish<entityId>("switch_to_mouse_controls", player().gimbalId);
//
////      if (network->getRole() == settings::network::CLIENT) {
//////        printf("\nIDS: %u %u %u\n", player().camId, player().ctrlId, player().gimbalId);
////        publish<entityId>("switch_to_camera", 22);
////        publish<entityId>("switch_to_free_controls", 24);
////        publish<entityId>("switch_to_mouse_controls", 23);
////      } else {
////        publish<entityId>("switch_to_camera", player().camId);
////        publish<entityId>("switch_to_free_controls", player().ctrlId);
////        publish<entityId>("switch_to_mouse_controls", player().gimbalId);
////      }
//    }
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
