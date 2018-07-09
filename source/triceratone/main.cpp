
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
    std::shared_ptr<MeshObjectVk_> terrainArk;
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


    std::unique_ptr<btTriangleMesh> arkMesh;
    std::unique_ptr<btCollisionShape> arkShape;
    std::unique_ptr<btRigidBody> arkRigidBody;
    std::unique_ptr<btDefaultMotionState> arkState;

  public:

    Triceratone()
        : controlSystem(&state),
          animationSystem(&state),
          physicsSystem(&state) { }

    ~Triceratone() {
      printf("Triceratone is being deconstructed.\n");
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


        glm::vec3 arkScale = {10.f, 10.f, 10.f};
        glm::mat4 arkMat = glm::scale(glm::rotate(ident, (float)M_PI * .5f, {1.f, 0.f, 0.f}), arkScale);
        terrainArk = std::make_shared<MeshObjectVk_>(vulkan.get(), "terrainArk", "cliff1024_01", arkMat);
        arkMesh = std::make_unique<btTriangleMesh>();

        std::vector<float> &verts = *(vulkan->getMeshStoredVertices("terrainArk"));
        std::vector<uint32_t> &indices = *(vulkan->getMeshStoredIndices("terrainArk"));

        for (uint32_t i = 0; i < verts.size(); i += vulkan->getMeshStoredVertexStride() / sizeof(float)) {
          glm::vec4 pos = arkMat * glm::vec4(verts[i], verts[i + 1], verts[i + 2], 1.f);
          arkMesh->findOrAddVertex({pos.x, pos.y, pos.z}, false);
        }
        for (uint32_t i = 0; i < indices.size(); i += 3) {
          arkMesh->addTriangleIndices(indices[i], indices[i + 1], indices[i + 2]);
        }
        arkShape = std::make_unique<btBvhTriangleMeshShape>(arkMesh.get(), true);

        arkState = std::make_unique<btDefaultMotionState>();
        btRigidBody::btRigidBodyConstructionInfo terrainRBCI(0, arkState.get(), arkShape.get());
        arkRigidBody = std::make_unique<btRigidBody>(terrainRBCI);
        arkRigidBody->setRestitution(0.5f);
        arkRigidBody->setFriction(1.f);
        arkRigidBody->setCollisionFlags(arkRigidBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
        arkRigidBody->setUserIndex(-(int)terrainArk->getId()); // negative of its ID signifies a static object
        physicsSystem.dynamicsWorld->addRigidBody(arkRigidBody.get());



////        arkShape->setLocalScaling({arkScale.x, arkScale.y, arkScale.z});
////        arkState = std::make_unique<btDefaultMotionState>(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
//
//        arkState = std::make_unique<btDefaultMotionState>(btTransform(btMatrix3x3(1, 0, 0, 0, 1, 0, 0, 0, 1)));
//
////        btTransform transform;
////        transform.setFromOpenGLMatrix((btScalar *) &arkMat);
////        arkState = std::make_unique<btDefaultMotionState>(transform);
//
//        btRigidBody::btRigidBodyConstructionInfo terrainRBCI(0, arkState.get(), arkShape.get(), btVector3(0, 0, 0));
//        arkRigidBody = std::make_unique<btRigidBody>(terrainRBCI);
//        arkRigidBody->setRestitution(0.5f);
//        arkRigidBody->setFriction(1.f);
////        arkRigidBody->setCollisionFlags(arkRigidBody->getCollisionFlags()
////                                        | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK
////                                        /*| btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT*/);
//        arkRigidBody->setUserIndex(-(int)terrainArk->getId()); // negative number for static object
//        physicsSystem.dynamicsWorld->addRigidBody(arkRigidBody.get());






        scene.addObject(terrainArk);

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
