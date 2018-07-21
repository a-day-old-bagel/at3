
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

#include "ecsSystem_animation.hpp"
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
    std::shared_ptr<Object> freeCam;
    std::shared_ptr<ThirdPersonCamera> camera;
    std::unique_ptr<Pyramid> pyramid;
    std::unique_ptr<DuneBuggy> duneBuggy;
    std::unique_ptr<Walker> walker;
};

class Triceratone : public Game<EntityComponentSystemInterface, Triceratone> {

    AnimationSystem   animationSystem;
    ControlSystem     controlSystem;
    NetSyncSystem     netSyncSystem;
    PhysicsSystem     physicsSystem;

    std::shared_ptr<Mesh> terrainArk;
    std::vector<PlayerAvatarSet> players;

    std::unique_ptr<Subscription> lClickSub, rClickSub, key0Sub, key1Sub, key2Sub, key3Sub;

    PlayerAvatarSet & player() {
      return network->getRole() == settings::network::SERVER ? players.at(0) : players.at(1);
    }

  public:

    Triceratone() : animationSystem(&state), controlSystem(&state), netSyncSystem(&state), physicsSystem(&state) { }
    ~Triceratone() override {
      scene.clear();
    }

    bool onInit() {

      // Initialize the systems
      bool initSuccess = true;
      initSuccess &= animationSystem.init();
      initSuccess &= controlSystem.init();
      initSuccess &= netSyncSystem.init();
      initSuccess &= physicsSystem.init();
      assert(initSuccess);

      // an identity matrix
      glm::mat4 ident(1.f);


      // the ark (the cylinder)
      glm::mat4 arkMat = glm::scale(ident, glm::vec3(100.f, 100.f, 100.f));
      terrainArk = std::make_shared<Mesh>(vulkan.get(), "terrainArk", "cliff1024_01", arkMat);
      TriangleMeshInfo info = {
          vulkan->getMeshStoredVertices("terrainArk"),
          vulkan->getMeshStoredIndices("terrainArk"),
          vulkan->getMeshStoredVertexStride(),
      };
      state.add_Physics(terrainArk->getId(), 0, &info, Physics::STATIC_MESH);
      scene.addObject(terrainArk);


      // the player avatars
      for (int i = 0; i < 2; ++i) {
        players.emplace_back();

        // the free cameras
        glm::mat4 start = glm::translate(ident, {0, -790, -120});
        players.back().freeCam = std::make_shared<Object>();
        state.add_Placement(players.back().freeCam->getId(), start);
        players.back().camera = std::make_shared<ThirdPersonCamera>(0.f, 0.f);
        state.add_FreeControls(players.back().freeCam->getId(), players.back().camera->gimbal->getId());
        players.back().camera->anchorTo(players.back().freeCam);
        scene.addObject(players.back().freeCam);

        // the human
        glm::vec3 walkerPos = glm::vec3(10, -790, -100 + i * 10);
        glm::mat4 walkerMat = glm::translate(ident, walkerPos);
        players.back().walker = std::make_unique<Walker>(state, vulkan.get(), scene, walkerMat);

        // the car
        glm::vec3 buggyPos = glm::vec3(0, -790, -100 + i * 10);
        glm::mat4 buggyMat = glm::translate(ident, buggyPos);
        buggyMat *= glm::mat4(getCylStandingRot(buggyPos, (float) M_PI * -0.5f, 0));
        players.back().duneBuggy = std::make_unique<DuneBuggy>(state, vulkan.get(), scene, buggyMat);

        // the flying illuminati pyramid
        glm::vec3 pyramidPos = glm::vec3(-10, -790, -100 + i * 10);
        glm::mat4 pyramidMat = glm::translate(ident, pyramidPos);
        pyramidMat *= glm::mat4(getCylStandingRot(pyramidPos, (float) M_PI * -0.5f, 0));
        players.back().pyramid = std::make_unique<Pyramid>(state, vulkan.get(), scene, pyramidMat);
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

      // TODO: Not working IN WINDOWS DEBUG BUILD ONLY for some reason?
      for (int i = 0; i < 2; ++i) {
        players[i].pyramid->resizeFire();
      }

      netSyncSystem.tick(dt);
      controlSystem.tick(dt);
      physicsSystem.tick(dt);
      animationSystem.tick(dt);
    }

    void makeFreeCamActiveControl() {
      publish<std::shared_ptr<PerspectiveCamera>>("set_primary_camera", player().camera->actual);
      publish<entityId>("switch_to_free_controls", player().freeCam->getId());
      publish<entityId>("switch_to_mouse_controls", player().camera->gimbal->getId());
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
