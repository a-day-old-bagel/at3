
#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>

#include "settings.h"
#include "topics.hpp"
#include "ezecs.hpp"
#include "dualityInterface.h"
#include "game.h"

#include "ecsSystem_animation.h"
#include "ecsSystem_controls.h"
#include "ecsSystem_physics.h"

#include "basicWalker.h"
#include "duneBuggy.h"
#include "pyramid.h"
#include "debugStuff.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

using namespace at3;
using namespace ezecs;
using namespace rtu::topics;

class CruisinGame : public Game<DualityInterface, CruisinGame> {

    ControlSystem     mControlSystem;
    AnimationSystem   mAnimationSystem;
    PhysicsSystem     mPhysicsSystem;

    std::unique_ptr<Subscription> key1Sub, key2Sub, key3Sub;

    std::shared_ptr<SkyBox_>         mpSkybox;
    std::shared_ptr<TerrainObject_>  mpTerrain;
    std::unique_ptr<BasicWalker>     mpPlayer;
    std::unique_ptr<Pyramid>         mpPyramid;
    std::unique_ptr<DuneBuggy>       mpDuneBuggy;
    std::shared_ptr<DebugStuff>      mpDebugStuff;

  public:

    CruisinGame()
        : mControlSystem(&mState),
          mAnimationSystem(&mState),
          mPhysicsSystem(&mState) { }

    ~CruisinGame() {
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
      glm::mat4 ident;

      // a terrain
      TerrainObject_::initTextures();
      mpTerrain = std::make_shared<TerrainObject_> (ident, -5000.f, 5000.f, -5000.f, 5000.f, -300, 100);
      this->mScene.addObject(mpTerrain);

      // the player
      glm::mat4 playerMat = glm::translate(ident, { 0.f, -100.f, 0.f});
      mpPlayer = std::make_unique<BasicWalker> (mState, mScene, playerMat);

      // a buggy
      glm::mat4 buggyMat = glm::translate(ident, { 0.f, -290.f, 0.f });
      mpDuneBuggy = std::make_unique<DuneBuggy> (mState, mScene, buggyMat);

      // a flying pyramid
      glm::mat4 pyramidMat = glm::translate(ident, { 0.f, 0.f, 5.f });
      mpPyramid = std::make_unique<Pyramid> (mState, mScene, pyramidMat);

      // a skybox-like background
      mpSkybox = std::make_shared<SkyBox_> ( );
      this->mScene.addObject(mpSkybox);
      mpSkybox->useCubeMap("assets/cubeMaps/sea.png");

      // Set up control switching, start as player entity.
      mpPlayer->makeActiveControl(nullptr);
      key1Sub =
          std::make_unique<Subscription>("key_down_1", RTU_MTHD_DLGT(&BasicWalker::makeActiveControl, mpPlayer.get()));
      key2Sub =
          std::make_unique<Subscription>("key_down_2", RTU_MTHD_DLGT(&DuneBuggy::makeActiveControl, mpDuneBuggy.get()));
      key3Sub =
          std::make_unique<Subscription>("key_down_3", RTU_MTHD_DLGT(&Pyramid::makeActiveControl, mpPyramid.get()));

      // some debug-draw features
      mpDebugStuff = std::make_shared<DebugStuff> (mScene, &mPhysicsSystem);

      // test out some music
      // mpDebugStuff->queueMusic();

      return true;
    }

    int32_t customSetting = 1337;
    virtual void registerCustomSettings() {
      settings::addCustom("cruisin_customSetting_i", &customSetting);
    }

    void onTick(float dt) {
      mControlSystem.tick(dt);
      mpPyramid->resizeFire();
      mPhysicsSystem.tick(dt);
      mAnimationSystem.tick(dt);
    }
};

int main(int argc, char **argv) {

  CruisinGame game;

  std::cout << std::endl << "Game is initializing..." << std::endl;
  if ( ! game.init("cruisin", "at3_cruisin_settings.ini")) {
    return -1;
  }

  std::cout << std::endl << "Game has started." << std::endl;
  while (!game.isQuit()) {
    game.tick();
  }

  std::cout << std::endl << "Game has finished." << std::endl;
  return 0;
}

#pragma clang diagnostic pop
