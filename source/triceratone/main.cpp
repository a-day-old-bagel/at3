
#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>
#include <functional>

#include "configuration.h"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/gtc/matrix_transform.hpp>

#include "settings.h"
#include "topics.hpp"
#include "ezecs.hpp"
#include "ecsInterface.h"
#include "game.h"

#include "ecsSystem_animation.h"
#include "ecsSystem_controls.h"
#include "ecsSystem_physics.h"

#include "basicWalker.h"
#include "basicWalkerVk.h"
#include "duneBuggy.h"
#include "duneBuggyVk.h"
#include "pyramid.h"
#include "pyramidVk.h"
#include "debugStuff.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

using namespace at3;
using namespace ezecs;
using namespace rtu::topics;

class Triceratone : public Game<EntityComponentSystemInterface, Triceratone> {

    ControlSystem     mControlSystem;
    AnimationSystem   mAnimationSystem;
    PhysicsSystem     mPhysicsSystem;

    std::shared_ptr<SceneObject_> mpFreeCam;
    std::shared_ptr<PerspectiveCamera_> mpCamera;
    std::shared_ptr<MeshObjectVk_> mpTestObj0;
    std::shared_ptr<MeshObjectVk_> mpTestObj1;
    std::unique_ptr<PyramidVk> mpPyramidVk;
    std::unique_ptr<DuneBuggyVk> mpDuneBuggyVk;
    std::unique_ptr<BasicWalkerVk> mpPlayerVk;

    std::unique_ptr<Subscription> keyFSub, key0Sub, key1Sub, key2Sub, key3Sub;

    std::shared_ptr<SkyBox_>         mpSkybox;
    std::shared_ptr<TerrainObject_>  mpTerrain;
    std::unique_ptr<BasicWalker>     mpPlayer;
    std::unique_ptr<Pyramid>         mpPyramid;
    std::unique_ptr<DuneBuggy>       mpDuneBuggy;
    std::shared_ptr<DebugStuff>      mpDebugStuff;


  public:

    Triceratone()
        : mControlSystem(&mState),
          mAnimationSystem(&mState),
          mPhysicsSystem(&mState) { }

    ~Triceratone() {
      mScene.clear();
    }

    bool onInit() {

      // Initialize the systems
      bool initSuccess = true;
      initSuccess &= mControlSystem.init();
      initSuccess &= mPhysicsSystem.init();
      initSuccess &= mAnimationSystem.init();
      assert(initSuccess);

      // an identity matrix
      glm::mat4 ident(1.f);





      glm::mat4 start = glm::rotate(glm::translate(ident, {0.f, 0.f, 5.f}), 0.f, {1.0f, 0.0f, 0.0f});
      mpFreeCam = std::make_shared<SceneObject_>();
      mState.add_Placement(mpFreeCam->getId(), start);
      mState.add_FreeControls(mpFreeCam->getId());
      mpCamera = std::make_shared<PerspectiveCamera_> (ident);
      mState.add_MouseControls(mpCamera->getId(), false, false);
      mpFreeCam->addChild(mpCamera);
      mScene.addObject(mpFreeCam);
      makeFreeCamActiveControl();






      if (settings::graphics::gpuApi == settings::graphics::VULKAN) {
        mpTestObj0 = std::make_shared<MeshObjectVk_>(mVulkan.get(), "pyramid_bottom", start);
        mpTestObj1 = std::make_shared<MeshObjectVk_>(mVulkan.get(), "pyramid_top", start);
        mpTestObj0->addChild(mpTestObj1);
        mScene.addObject(mpTestObj0);

        glm::mat4 playerMat = glm::translate(ident, {0.f, -10.f, 0.f});
        mpPlayerVk = std::make_unique<BasicWalkerVk>(mState, mVulkan.get(), mScene, playerMat);

        glm::mat4 buggyMat = glm::translate(ident, {0.f, 10.f, 0.f});
        mpDuneBuggyVk = std::make_unique<DuneBuggyVk>(mState, mVulkan.get(), mScene, buggyMat);

        glm::mat4 pyramidMat = glm::translate(ident, {0.f, 0.f, 0.f});
        mpPyramidVk = std::make_unique<PyramidVk>(mState, mVulkan.get(), mScene, pyramidMat);

        key0Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_0", Triceratone::makeFreeCamActiveControl, this);
        key1Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_1", BasicWalkerVk::makeActiveControl, mpPlayerVk.get());
        key2Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_2", DuneBuggyVk::makeActiveControl, mpDuneBuggyVk.get());
        key3Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_3", PyramidVk::makeActiveControl, mpPyramidVk.get());
        keyFSub = RTU_MAKE_SUB_UNIQUEPTR("key_down_f", PyramidVk::spawnSphere, mpPyramidVk.get());
      }






      if (settings::graphics::gpuApi == settings::graphics::OPENGL_OPENCL) {

        // a terrain
        TerrainObject_::initTextures();
        mpTerrain = std::make_shared<TerrainObject_>(ident, -5000.f, 5000.f, -5000.f, 5000.f, -10, 140);
        this->mScene.addObject(mpTerrain);

        // the player
        glm::mat4 playerMat = glm::translate(ident, {0.f, -10.f, 0.f});
        mpPlayer = std::make_unique<BasicWalker>(mState, mScene, playerMat);

        // a buggy
        glm::mat4 buggyMat = glm::translate(ident, {0.f, 10.f, 0.f});
        mpDuneBuggy = std::make_unique<DuneBuggy>(mState, mScene, buggyMat);

        // a flying pyramid
        glm::mat4 pyramidMat = glm::translate(ident, {0.f, 0.f, 0.f});
        mpPyramid = std::make_unique<Pyramid>(mState, mScene, pyramidMat);

        // a skybox-like background
        mpSkybox = std::make_shared<SkyBox_>();
        this->mScene.addObject(mpSkybox);
        mpSkybox->useCubeMap("assets/cubeMaps/sea.png");

        // Set up control switching subscriptions
        key0Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_0", Triceratone::makeFreeCamActiveControl, this);
        key1Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_1", BasicWalker::makeActiveControl, mpPlayer.get());
        key2Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_2", DuneBuggy::makeActiveControl, mpDuneBuggy.get());
        key3Sub = RTU_MAKE_SUB_UNIQUEPTR("key_down_3", Pyramid::makeActiveControl, mpPyramid.get());

        // some debug-draw features
        mpDebugStuff = std::make_shared<DebugStuff>(mScene, &mPhysicsSystem);

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
      mControlSystem.tick(dt);

      if (settings::graphics::gpuApi == settings::graphics::OPENGL_OPENCL) {
        mpPyramid->resizeFire();
      }
      if (settings::graphics::gpuApi == settings::graphics::VULKAN) {
        // TODO: Not working for some reason
        mpPyramidVk->resizeFire();
      }

      mPhysicsSystem.tick(dt);
      mAnimationSystem.tick(dt);
    }

    void makeFreeCamActiveControl() {
      publish<std::shared_ptr<PerspectiveCamera_>>("set_primary_camera", mpCamera);
      publish<entityId>("switch_to_free_controls", mpFreeCam->getId());
      publish<entityId>("switch_to_mouse_controls", mpCamera->getId());
    }
};

int main(int argc, char **argv) {

  Triceratone game;

  std::cout << std::endl << "AT3 is initializing..." << std::endl;
  if ( ! game.init("triceratone", "at3_triceratone_settings.ini")) {
    return -1;
  }

  std::cout << std::endl << "AT3 has started." << std::endl;
  while ( ! game.isQuit()) {
    game.tick();
  }

  std::cout << std::endl << "AT3 has finished." << std::endl;
  return 0;
}

#pragma clang diagnostic pop
