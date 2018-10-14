
#include "game.hpp"
#include "interface.hpp"

#include "scene.hpp"
#include "controls.hpp"
#include "network.hpp"
#include "physics.hpp"

#include "freeCam.hpp"
#include "duneBuggy.hpp"
#include "playerSet.hpp"
#include "pyramid.hpp"
#include "walker.hpp"

using namespace at3;
using namespace ezecs;
using namespace rtu::topics;

class Triceratone : public Game<EntityComponentSystemInterface, Triceratone> {

    ControlSystem controlSystem;
    NetworkSystem networkSystem;
    PhysicsSystem physicsSystem;
    SceneSystem   sceneSystem;

    entityId playerId = 0;
    uint8_t clientKeepTryingToConnect = 6;

  public:

    Triceratone() : controlSystem(&state), networkSystem(&state), physicsSystem(&state), sceneSystem(&state) { }

    void onInit() {
      // Prepare the systems to listen for new components
      initializeSystems();
      // Make sure static transform functions are always registered in the same order, regardless of access order.
      registerStaticTransformFunctions();
      // Set up the world, or wait for a server to do it
      prepareWorld();
      // A client needs to wait for a connection. For non-clients, this should exit immediately.
      waitForServerConnection();
      // Start the player off in the freeCam avatar
      switchToFreeControl();
      // Set up the high-level controls
      registerControlSubscriptions();
    }

    void initializeSystems() {
      bool systemInitSuccessful = true;
      systemInitSuccessful &= controlSystem.init();
      systemInitSuccessful &= networkSystem.init();
      systemInitSuccessful &= physicsSystem.init();
      systemInitSuccessful &= sceneSystem.init();
      if ( ! systemInitSuccessful) {
        fprintf(stderr, "Systems failed to initialize!\n");
      }
    }

    void registerStaticTransformFunctions() {
      Pyramid::getTopTransFuncDesc();
      Pyramid::getFireTransFuncDesc();
      DuneBuggy::getWheelTransFuncDesc();
      Walker::getBodyTransFuncDesc();
    }

    void prepareWorld() {
      // A client should not be directly creating any entities. The game world only needs to be created by non-clients.
      if (network->getRole() != settings::network::CLIENT) {
        // Create the map
        ecs->openEntityRequest();
        ecs->requestPlacement(glm::mat4(1.f));
        ecs->requestMesh("terrainArk", "cliff1024_01");
        ecs->requestStaticMeshPhysics("terrainArk");
        ecs->requestNetworking();
        ecs->requestSceneNode(0);
        ecs->closeEntityRequest();
        // Create a player
        playerId = PlayerSet::create(state, *ecs);
      } else {
        // Set up the callback for player ID assignment
        RTU_STATIC_SUB(assignPlayerIdSub, "set_player_id", Triceratone::assignPlayerId, this);
        RTU_STATIC_SUB(failedConnectSub, "client_could_not_connect", Triceratone::onFailedConnection, this);
      }
    }

    void waitForServerConnection() {
      while ( ! playerId ) {
        network->tick();
        networkSystem.tick(0);
        SDL_Delay(50);
        if (clientKeepTryingToConnect < 6) {  // Countdown to exit
          fprintf(stderr, "\n%u ...", clientKeepTryingToConnect--);
          SDL_Delay(1000);
          if ( ! clientKeepTryingToConnect) {
            exit(1);
          }
        }
      }
    }

    void registerControlSubscriptions() {
      RTU_STATIC_SUB(key1Sub, "key_down_1", Triceratone::switchToFreeControl, this);
      RTU_STATIC_SUB(key2Sub, "key_down_2", Triceratone::switchToWalkControl, this);
      RTU_STATIC_SUB(key3Sub, "key_down_3", Triceratone::switchToPyramidControl, this);
      RTU_STATIC_SUB(key4Sub, "key_down_4", Triceratone::switchToBuggyControl, this);
      RTU_STATIC_SUB(rewindPhysicsSub, "key_held_f4", NetworkSystem::includeInput, &networkSystem);
    }

    void onTick(float dt) {
      // Order matters here
      networkSystem.tick(dt); // First receive and/or send any network updates about controls or physics state
      controlSystem.tick(dt); // Process all non-physics-framerate-bound control signals, remote or otherwise
      physicsSystem.tick(dt); // Step the physics simulation if needed, possibly calling onBeforePhysicsStep first
      sceneSystem.tick(dt); // Given interpolated physics state and animations, traverse the scene to cache transforms
    }

    void onBeforePhysicsStep() {
      // Order matters here
      networkSystem.onBeforePhysicsStep(); // Record control states for synchronization and rewind/replay
      controlSystem.onBeforePhysicsStep(); // Process all physics-framerate-bound control signals, remote or otherwise
      physicsSystem.onBeforePhysicsStep(); // Apply those signals to the physics objects
    }

    void onAfterPhysicsStep() {
      networkSystem.onAfterPhysicsStep(); // Record the new physics state for synchronization and rewind/replay
    }

    std::string exampleSetting = "1337_H4XX0R5";
    virtual void registerCustomSettings() {
      settings::addCustom("example_custom_setting_s", &exampleSetting);
    }
    void assignPlayerId(void * playerId) {
      this->playerId = *(entityId*)playerId;
    }
    void onFailedConnection() {
      fprintf(stderr, "Aborting in ");
      --clientKeepTryingToConnect;
    }
    void switchToFreeControl() {
      Player *player;
      state.getPlayer(playerId, &player);
      FreeCam::switchTo(state, player->free);
    }
    void switchToWalkControl() {
      Player *player;
      state.getPlayer(playerId, &player);
      Walker::switchTo(state, player->walk);
    }
    void switchToPyramidControl() {
      Player *player;
      state.getPlayer(playerId, &player);
      Pyramid::switchTo(state, player->pyramid);
    }
    void switchToBuggyControl() {
      Player *player;
      state.getPlayer(playerId, &player);
      DuneBuggy::switchTo(state, player->track);
    }
};

int main(int argc, char **argv) {

  Triceratone game;

  std::cout << std::endl << "AT3 is initializing..." << std::endl;
  game.init("triceratone", "settings.ini");

  std::cout << std::endl << "AT3 has started." << std::endl;
  while ( ! game.getIsQuit()) {
    game.tick();
  }

  std::cout << std::endl << "AT3 has finished." << std::endl;
  return 0;
}
