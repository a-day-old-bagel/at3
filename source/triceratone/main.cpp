
#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>
#include <functional>

#include "configuration.hpp"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/gtc/matrix_transform.hpp>

#include "settings.hpp"
#include "topics.hpp"
#include "ezecs.hpp"
#include "ecsInterface.hpp"
#include "game.hpp"

#include "ecsSystem_animation.hpp"
#include "ecsSystem_controls.hpp"
#include "ecsSystem_physics.hpp"

#include "basicWalker.hpp"
#include "basicWalkerVk.hpp"
#include "duneBuggy.hpp"
#include "duneBuggyVk.hpp"
#include "pyramid.hpp"
#include "pyramidVk.hpp"
#include "debugStuff.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

using namespace at3;
using namespace ezecs;
using namespace rtu::topics;

class Triceratone : public Game<EntityComponentSystemInterface, Triceratone> {

    ControlSystem     controlSystem;
    AnimationSystem   animationSystem;
    PhysicsSystem     physicsSystem;

    std::shared_ptr<SceneObject_> freeCam;
    std::shared_ptr<PerspectiveCamera_> camera;
    std::shared_ptr<MeshObjectVk_> testObj0;
    std::shared_ptr<MeshObjectVk_> testObj1;
    std::unique_ptr<PyramidVk> pyramidVk;
    std::unique_ptr<DuneBuggyVk> duneBuggyVk;
    std::unique_ptr<BasicWalkerVk> playerVk;

    std::unique_ptr<Subscription> keyFSub, key0Sub, key1Sub, key2Sub, key3Sub;

    std::shared_ptr<SkyBox_>         skybox;
    std::shared_ptr<TerrainObject_>  terrain;
    std::unique_ptr<BasicWalker>     player;
    std::unique_ptr<Pyramid>         pyramid;
    std::unique_ptr<DuneBuggy>       duneBuggy;
    std::shared_ptr<DebugStuff>      debugStuff;


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





      glm::mat4 start = glm::rotate(glm::translate(ident, {0.f, 0.f, 5.f}), 0.f, {1.0f, 0.0f, 0.0f});
      freeCam = std::make_shared<SceneObject_>();
      state.add_Placement(freeCam->getId(), start);
      state.add_FreeControls(freeCam->getId());
      camera = std::make_shared<PerspectiveCamera_> (ident);
      state.add_MouseControls(camera->getId(), false, false);
      freeCam->addChild(camera);
      scene.addObject(freeCam);
      makeFreeCamActiveControl();






      if (settings::graphics::gpuApi == settings::graphics::VULKAN) {
        testObj0 = std::make_shared<MeshObjectVk_>(vulkan.get(), "pyramid_bottom", start);
        testObj1 = std::make_shared<MeshObjectVk_>(vulkan.get(), "pyramid_top", start);
        testObj0->addChild(testObj1);
        scene.addObject(testObj0);

        glm::mat4 playerMat = glm::translate(ident, {0.f, -10.f, 0.f});
        playerVk = std::make_unique<BasicWalkerVk>(state, vulkan.get(), scene, playerMat);

        glm::mat4 buggyMat = glm::translate(ident, {0.f, 10.f, 0.f});
        duneBuggyVk = std::make_unique<DuneBuggyVk>(state, vulkan.get(), scene, buggyMat);

        glm::mat4 pyramidMat = glm::translate(ident, {0.f, 0.f, 0.f});
        pyramidVk = std::make_unique<PyramidVk>(state, vulkan.get(), scene, pyramidMat);

        key0Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_0", Triceratone::makeFreeCamActiveControl, this);
        key1Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_1", BasicWalkerVk::makeActiveControl, playerVk.get());
        key2Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_2", DuneBuggyVk::makeActiveControl, duneBuggyVk.get());
        key3Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_3", PyramidVk::makeActiveControl, pyramidVk.get());
        keyFSub = RTU_MAKE_SUB_UNIQUEPTR("key_down_r", PyramidVk::spawnSphere, pyramidVk.get());
      }






      if (settings::graphics::gpuApi == settings::graphics::OPENGL_OPENCL) {

        // a terrain
        TerrainObject_::initTextures();
        terrain = std::make_shared<TerrainObject_>(ident, -5000.f, 5000.f, -5000.f, 5000.f, -10, 140);
        this->scene.addObject(terrain);

        // the player
        glm::mat4 playerMat = glm::translate(ident, {0.f, -10.f, 0.f});
        player = std::make_unique<BasicWalker>(state, scene, playerMat);

        // a buggy
        glm::mat4 buggyMat = glm::translate(ident, {0.f, 10.f, 0.f});
        duneBuggy = std::make_unique<DuneBuggy>(state, scene, buggyMat);

        // a flying pyramid
        glm::mat4 pyramidMat = glm::translate(ident, {0.f, 0.f, 0.f});
        pyramid = std::make_unique<Pyramid>(state, scene, pyramidMat);

        // a skybox-like background
        skybox = std::make_shared<SkyBox_>();
        this->scene.addObject(skybox);
        skybox->useCubeMap("assets/cubeMaps/sea.png");

        // Set up control switching subscriptions
        key0Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_0", Triceratone::makeFreeCamActiveControl, this);
        key1Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_1", BasicWalker::makeActiveControl, player.get());
        key2Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_2", DuneBuggy::makeActiveControl, duneBuggy.get());
        key3Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_3", Pyramid::makeActiveControl, pyramid.get());

        // some debug-draw features
        debugStuff = std::make_shared<DebugStuff>(scene, &physicsSystem);

        // test out some music
        // mpDebugStuff->queueMusic();

      }

      return true;
    }

    int32_t customSetting = 1337;
    virtual void registerCustomSettings() {
      settings::addCustom("triceratone_customSetting_i", &customSetting);
    }

    void onTick(float dt) {
      controlSystem.tick(dt);

      if (settings::graphics::gpuApi == settings::graphics::OPENGL_OPENCL) {
        pyramid->resizeFire();
      }
      if (settings::graphics::gpuApi == settings::graphics::VULKAN) {
        // TODO: Not working for some reason
        pyramidVk->resizeFire();
      }

      physicsSystem.tick(dt);
      animationSystem.tick(dt);
    }

    void makeFreeCamActiveControl() {
      publish<std::shared_ptr<PerspectiveCamera_>>("set_primary_camera", camera);
      publish<entityId>("switch_to_free_controls", freeCam->getId());
      publish<entityId>("switch_to_mouse_controls", camera->getId());
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
