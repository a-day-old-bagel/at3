
#pragma once

#include <epoxy/gl.h>
#include <btBulletDynamicsCommon.h>
#include <LinearMath/btIDebugDraw.h>
#include "ezecs.hpp"
#include "sceneObject.hpp"
#include "bulletDebug.hpp"
#include "ecsInterface.hpp"
#include "topics.hpp"

using namespace ezecs;

// TODO: figure out how to use bullet to simulate objects without collision components
namespace at3 {

  typedef std::function<btCollisionWorld::ClosestRayResultCallback(btVector3 &, btVector3 &)> rayFuncType;

  /*
   * The physics system
   */
  class PhysicsSystem : public System<PhysicsSystem> {
      /* Global physics data structures */
      btDispatcher *dispatcher;
      btBroadphaseInterface *broadphase;
      btConstraintSolver *solver;
      btCollisionConfiguration *collisionConfiguration;
//      btDynamicsWorld *dynamicsWorld;
      btVehicleRaycaster * vehicleRaycaster;
      bool debugDrawMode = false;

      rtu::topics::Subscription debugDrawToggleSub;

      // Ground objects
      btCollisionShape* planeShape;
      btDefaultMotionState* groundMotionState;
      btRigidBody* groundRigidBody;

    public:

      btDynamicsWorld *dynamicsWorld; // TODO: put back in private

      std::vector<compMask> requiredComponents = {
              PHYSICS,
              PHYSICS | PYRAMIDCONTROLS,
              PHYSICS | TERRAIN,
              PHYSICS | TRACKCONTROLS,
              PHYSICS | PLAYERCONTROLS,
      };
      PhysicsSystem(State* state);
      ~PhysicsSystem();
      bool onInit();
      void onTick(float dt);
      void deInit();
      bool onDiscover(const entityId& id);
      bool onForget(const entityId& id);
      bool onDiscoverTerrain(const entityId& id);
      bool onDiscoverTrackControls(const entityId& id);
      bool onForgetTrackControls(const entityId& id);
      void setDebugDrawer(std::shared_ptr<BulletDebug_> debug);
      btCollisionWorld::ClosestRayResultCallback rayTest(const btVector3 &start, const btVector3 &end);
      rayFuncType getRayFunc();
      void toggleDebugDraw(void* nothing);
      // TODO: add removal logic for vehicles and wheels
  };
}
