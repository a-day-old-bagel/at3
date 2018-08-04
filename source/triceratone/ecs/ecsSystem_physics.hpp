
#pragma once

#include <btBulletDynamicsCommon.h>
#include <LinearMath/btIDebugDraw.h>
#include "ezecs.hpp"
#include "sceneObject.hpp"
#include "ecsInterface.hpp"
#include "topics.hpp"
#include "vkc.hpp"

using namespace ezecs;

// TODO: figure out how to use bullet to simulate objects without collision components
namespace at3 {

  typedef std::function<btCollisionWorld::ClosestRayResultCallback(btVector3 &, btVector3 &)> rayFuncType;

  class PhysicsSystem : public System<PhysicsSystem> {
      /* Global physics data structures */
      btDispatcher *dispatcher;
      btBroadphaseInterface *broadphase;
      btConstraintSolver *solver;
      btCollisionConfiguration *collisionConfiguration;
      btDynamicsWorld *dynamicsWorld;
      btVehicleRaycaster * vehicleRaycaster;
      bool debugDrawMode = false;

      std::shared_ptr<vkc::VulkanContext<EntityComponentSystemInterface>> vulkan;
      rtu::topics::Subscription setVulkanContextSub;
      void setVulkanContext(void *vkc);

      rtu::topics::Subscription debugDrawToggleSub;

    public:
      std::vector<compMask> requiredComponents = {
              PHYSICS,
              PHYSICS | PYRAMIDCONTROLS,
              PHYSICS | TRACKCONTROLS,
              PHYSICS | PLAYERCONTROLS,
      };
      PhysicsSystem(State* state);
      ~PhysicsSystem() override;
      bool onInit();
      void onTick(float dt);
      void deInit();
      bool onDiscover(const entityId& id);
      bool onForget(const entityId& id);
      bool onDiscoverTrackControls(const entityId& id);
      bool onForgetTrackControls(const entityId& id);
      void setDebugDrawer();
      btCollisionWorld::ClosestRayResultCallback rayTest(const btVector3 &start, const btVector3 &end);
      rayFuncType getRayFunc();
      void toggleDebugDraw(void* nothing);
      // TODO: add removal logic for vehicles and wheels
  };
}
