
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
    entityId ctrlId;
    entityId camId;
    entityId gimbalId;
    std::shared_ptr<Object> ctrl;
    std::shared_ptr<Object> cam;
    std::shared_ptr<Object> camGimbal;

    std::unique_ptr<Pyramid> pyramid;
    std::unique_ptr<DuneBuggy> duneBuggy;
    std::unique_ptr<Walker> walker;
};

class Triceratone : public Game<EntityComponentSystemInterface, Triceratone> {

    AnimationSystem   animationSystem;
    ControlSystem     controlSystem;
    NetSyncSystem     netSyncSystem;
    PhysicsSystem     physicsSystem;

    std::shared_ptr<Object> terrainArk;

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
      entityId arkId;
      state.createEntity(&arkId);
      state.add_Placement(arkId, arkMat);
      vulkan->registerMeshInstance(arkId, "terrainArk", "cliff1024_01");
      terrainArk = std::make_shared<Object>(arkId);
      TriangleMeshInfo info = {
          vulkan->getMeshStoredVertices("terrainArk"),
          vulkan->getMeshStoredIndices("terrainArk"),
          vulkan->getMeshStoredVertexStride(),
      };
      state.add_Physics(arkId, 0, &info, Physics::STATIC_MESH);
      scene.addObject(terrainArk);

      // the player avatars
      for (int i = 0; i < 2; ++i) {
        players.emplace_back();

        // the free cameras
        state.createEntity(&players.back().camId);
        float back = 0.f;
        float tilt = 0.f;
        glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, 0.f, back}), tilt , glm::vec3(1.0f, 0.0f, 0.0f));
        state.add_Placement(players.back().camId, camMat);
        state.add_Perspective(players.back().camId, settings::graphics::fovy, 0.1f, 10000.f);
        players.back().cam = std::make_shared<Object>(players.back().camId);

        state.createEntity(&players.back().gimbalId);
        state.add_Placement(players.back().gimbalId, ident);
        Placement *placement;
        state.get_Placement(players.back().gimbalId, &placement);
        placement->forceLocalRotationAndScale = true;
        state.add_MouseControls(players.back().gimbalId, settings::controls::mouseInvertX, settings::controls::mouseInvertY);
        players.back().camGimbal = std::make_shared<Object>(players.back().gimbalId);

        glm::mat4 start = glm::translate(ident, {0, -790, -120});
        state.createEntity(&players.back().ctrlId);
        state.add_Placement(players.back().ctrlId, start);
        state.add_FreeControls(players.back().ctrlId, players.back().gimbalId);
        players.back().ctrl = std::make_shared<Object>(players.back().ctrlId);

        players.back().camGimbal->addChild(players.back().cam);
        players.back().ctrl->addChild(players.back().camGimbal);
        scene.addObject(players.back().ctrl);

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

      // TODO: Not working IN WINDOWS BUT ONLY SOMETIMES for some reason? Undefined behavior?
      for (int i = 0; i < 2; ++i) {
        players[i].pyramid->resizeFire();
      }

      netSyncSystem.tick(dt);
      controlSystem.tick(dt);
      physicsSystem.tick(dt);
      animationSystem.tick(dt);
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
