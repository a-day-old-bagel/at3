
#pragma once

#include <btBulletDynamicsCommon.h>
#include <LinearMath/btIDebugDraw.h>
#include "ezecs.hpp"
#include "sceneObject.hpp"
#include "interface.hpp"
#include "topics.hpp"
#include "vkc.hpp"

using namespace ezecs;

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

//      std::unique_ptr<rtu::Delegate<void(btDynamicsWorld *world, btScalar timeStep)>> onAfterSimulationStepCallback;

      std::shared_ptr<vkc::VulkanContext<EntityComponentSystemInterface>> vulkan;
      void setVulkanContext(void *vkc);

    public:
      std::vector<compMask> requiredComponents = {
              PHYSICS,
              PHYSICS | PYRAMIDCONTROLS,
              PHYSICS | TRACKCONTROLS,
              PHYSICS | WALKCONTROLS,
      };
      explicit PhysicsSystem(State* state);
      ~PhysicsSystem() override;
      bool onInit();
      void onTick(float dt);
      void deInit();
      bool onDiscover(const entityId& id);
      bool onForget(const entityId& id);
      bool onDiscoverWalkControls(const entityId& id);
      bool onDiscoverPyramidControls(const entityId& id);
      bool onDiscoverTrackControls(const entityId& id);
      bool onForgetTrackControls(const entityId& id);
//      void setOnAfterSimulationStep(void* dlgtPtr);
//      void onAfterSimulationStep(btDynamicsWorld *world, btScalar timeStep);
      void setDebugDrawer();
      btCollisionWorld::ClosestRayResultCallback rayTest(const btVector3 &start, const btVector3 &end);
      rayFuncType getRayFunc();
      void toggleDebugDraw(void* nothing);
      // TODO: add removal logic for vehicles and wheels
  };
}
