
#include "ecsSystem_physics.h"
#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <BulletCollision/CollisionShapes/btTriangleShape.h>
//#include <BulletDynamics/Character/btKinematicCharacterController.h>

#include "debug.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "IncompatibleTypes"
#pragma ide diagnostic ignored "TemplateArgumentsIssues"
using namespace ezecs;

namespace at3 {

  // A custom bullet collision callback to prevent collisions with backfaces (created for use with terrain).
  static bool myCustomMaterialCombinerCallback
      (
      btManifoldPoint& cp,
      const btCollisionObjectWrapper* colObj0Wrap,
      int partId0,
      int index0,
      const btCollisionObjectWrapper* colObj1Wrap,
      int partId1,
      int index1
      )
  {
    // one-sided triangles
    if (colObj1Wrap->getCollisionShape()->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE)
    {
      auto triShape = static_cast<const btTriangleShape*>( colObj1Wrap->getCollisionShape() );
      const btVector3* v = triShape->m_vertices1;
      btVector3 faceNormalLs = btCross(v[2] - v[0], v[1] - v[0]);
      faceNormalLs.normalize();
      btVector3 faceNormalWs = colObj1Wrap->getWorldTransform().getBasis() * faceNormalLs;
      float nDotF = btDot( faceNormalWs, cp.m_normalWorldOnB );
      if ( nDotF <= 0.0f )
      {
        // flip the contact normal to be aligned with the face normal
        cp.m_normalWorldOnB += -2.0f * nDotF * faceNormalWs;
      }
    }
    //this return value is currently ignored, but to be on the safe side: return false if you don't calculate friction
    return false;
  }

  PhysicsSystem::PhysicsSystem(State *state) : System(state),
      debugDrawToggleSub("key_down_f3", RTU_MTHD_DLGT(&PhysicsSystem::toggleDebugDraw, this))
  {
    name = "Physics System";
  }
  PhysicsSystem::~PhysicsSystem() {
    deInit();
  }

  bool PhysicsSystem::onInit() {
    registries[0].discoverHandler = RTU_MTHD_DLGT(&PhysicsSystem::onDiscover, this);
    registries[0].forgetHandler = RTU_MTHD_DLGT(&PhysicsSystem::onForget, this);
    registries[2].discoverHandler = RTU_MTHD_DLGT(&PhysicsSystem::onDiscoverTerrain, this);
    registries[3].discoverHandler = RTU_MTHD_DLGT(&PhysicsSystem::onDiscoverTrackControls, this);
    registries[3].forgetHandler = RTU_MTHD_DLGT(&PhysicsSystem::onForgetTrackControls, this);

    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver();
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0.0f, 0.0f, -9.81f));
    vehicleRaycaster = new btDefaultVehicleRaycaster(dynamicsWorld);

    // prevent backface collisions with anything that uses the custom collision callback (like terrain)
    gContactAddedCallback = myCustomMaterialCombinerCallback;

    // set up ghost object collision detection
    dynamicsWorld->getPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

    if (settings::graphics::gpuApi == settings::graphics::VULKAN) {
      // Create a ground object
      planeShape = new btStaticPlaneShape(btVector3(0.f, 0.f, 1.f), 0.f);
      groundMotionState = new btDefaultMotionState(btTransform(
          btQuaternion(0.f, 0.f, 0.f, 1.f),
          btVector3(0.f, 0.f, 0.f)));
      btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, planeShape, btVector3(0, 0, 0));
      groundRigidBody = new btRigidBody(groundRigidBodyCI);
      groundRigidBody->setRestitution(0.5f);
      groundRigidBody->setFriction(1.f);
      dynamicsWorld->addRigidBody(groundRigidBody);
    }

    return true;
  }

  void PhysicsSystem::onTick(float dt) {

    // PyramidControls
    for (auto id : registries[1].ids) {
      PyramidControls *ctrls;
      state->get_PyramidControls(id, &ctrls);
      Physics *physics;
      state->get_Physics(id, &physics);
      physics->rigidBody->applyForce({ctrls->force.x, ctrls->force.y, ctrls->force.z},
                                     btVector3(ctrls->up.x, ctrls->up.y, ctrls->up.z) * 0.05f);

      // Custom constraint to keep the pyramid up FixMe: this sometimes gets stuck in a horizontal position?
      glm::vec3 up{0.f, 0.f, 1.f};
      float tip = glm::dot(ctrls->up, up);
      glm::vec3 rotAxis = glm::cross(ctrls->up, up);
      physics->rigidBody->applyTorque(
          btVector3(rotAxis.x, rotAxis.y, rotAxis.z) * tip * (ctrls->up.z < 0 ? -1000.f : 1000.f)
      );
    }

    // PlayerControls
    // TODO: create a kinematic body (0 mass, recalc inertia) to which to anchor
    //       the capsule when it lands from a height (point to point constraint, AKA ball joint, maybe)?
    for (auto id : registries[4].ids) {
      PlayerControls *ctrls;
      state->get_PlayerControls(id, &ctrls);
      Physics *physics;
      state->get_Physics(id, &physics);

      // Fire some rays straight down
      btTransform capsuleTrans;
      physics->rigidBody->getMotionState()->getWorldTransform(capsuleTrans);
      float rayLength = 2.f;//HUMAN_HEIGHT * 1.5f;
      float rayDistTravelledAvg = 0.f;
      btVector3 rayHitPointAvg = {0.f, 0.f, 0.f};
      btVector3 rayNormalAvg = {0.f, 0.f, 0.f};
      btVector3 groundVelocity;
      bool groundVelocityFound = false;
      bool anyRayHitGoodGround = false;
      btVector3 orig = capsuleTrans.getOrigin();
      std::vector<btVector3> rayStartLocations = {
#         if CHARA_USE_FOUR_RAYS
            {orig.x(), orig.y() + HUMAN_DEPTH * 0.5f, orig.z() - HUMAN_HEIGHT * 0.15f},
            {orig.x(), orig.y() - HUMAN_DEPTH * 0.5f, orig.z() - HUMAN_HEIGHT * 0.15f},
            {orig.x() + HUMAN_WIDTH * 0.5f, orig.y(), orig.z() - HUMAN_HEIGHT * 0.15f},
            {orig.x() - HUMAN_WIDTH * 0.5f, orig.y(), orig.z() - HUMAN_HEIGHT * 0.15f}
#         else
            {orig.x(), orig.y(), orig.z() - HUMAN_HEIGHT * 0.15f},
#         endif
      };
      for (auto rayStart : rayStartLocations) {
        btVector3 rayEnd = rayStart + btVector3(0.f, 0.f, -rayLength);
        btCollisionWorld::ClosestRayResultCallback groundSpringRayCallback(rayStart, rayEnd);
        dynamicsWorld->rayTest(rayStart, rayEnd, groundSpringRayCallback);
        rayDistTravelledAvg += (groundSpringRayCallback.m_hitPointWorld - rayStart).length();
        anyRayHitGoodGround |= (groundSpringRayCallback.hasHit() && groundSpringRayCallback.m_hitNormalWorld.z() > 0.4);
        rayHitPointAvg += groundSpringRayCallback.m_hitPointWorld;
        rayNormalAvg += groundSpringRayCallback.m_hitNormalWorld;
        if ( ! groundVelocityFound && groundSpringRayCallback.m_collisionObject) { // Use the results of the first ray
          groundVelocity = groundSpringRayCallback.m_collisionObject->getInterpolationLinearVelocity();
          groundVelocityFound = true;
        }
      }
      rayDistTravelledAvg /= rayStartLocations.size();
      rayHitPointAvg /= rayStartLocations.size();
      rayNormalAvg /= rayStartLocations.size();

      // some variables to use below
      float maxSpringForce = rayLength * 0.5f;
      float springForceLinear = maxSpringForce - rayDistTravelledAvg;
      bool isInJumpRange = springForceLinear > -rayLength * 0.2f;

      // store the linear spring force as offset from equilibrium
      ctrls->equilibriumOffset = springForceLinear;

      // Handle jumping
      float zVel = physics->rigidBody->getLinearVelocity().z();
      if (ctrls->jumpInProgress) {
        if (springForceLinear >= -0.1f && zVel < 0.f) {
          ctrls->jumpInProgress = false;
        }
      } else if (ctrls->jumpRequested) {
        physics->rigidBody->applyImpulse({0.f, 0.f, CHARA_JUMP}, {0.f, 0.f, 0.f});
        ctrls->jumpInProgress = true;
      }

      // damping variables to be applied to object
      float linDamp = 0.f;
      float angDamp = 0.f;
      float zFactor = 1.f;

      float springForceOverMax = springForceLinear / maxSpringForce; // How far from equilibrium (0 to 1, 1 is far)
      ctrls->isGrounded = anyRayHitGoodGround; // not grounded if high in the air or on steep slope
      ctrls->isGrounded &= fabs(springForceOverMax) < 1.f; // extra range checks
      ctrls->isGrounded &= fabs(springForceOverMax) > -1.f;
      ctrls->isGrounded &= (! ctrls->jumpInProgress); // not grounded if jumping

      if (ctrls->isGrounded) { // Movement along ground - apply controls and a "stick to ground" force
        auto sfomPow2 = (float)pow(springForceOverMax, 2);
        auto sfomPow4 = (float)pow(springForceOverMax, 4);
        auto sfom10Pow2 = (float)pow(springForceOverMax * 10.f, 2);
        float springForceMagnitude = (1.0f - sfomPow2) - std::max(0.f, 1.f - sfom10Pow2);
        springForceMagnitude *= CHARA_SPRING_FACTOR;
        float mvmntForceMagnitude = std::max(CHARA_MIDAIR_FACTOR, 1.f - sfomPow2);
        float dampingMagnitude = std::min(0.9999999f, 1.1f - sfomPow4);
        linDamp = dampingMagnitude;

        float springForceFinal = springForceMagnitude * ((0 < springForceLinear) - (springForceLinear < 0));

        // Add a strong damping and negation of falling motion if the character is moving down fast past equilibrium
        if (zVel < -0.2f && springForceLinear > 0.05f) {
          physics->rigidBody->setLinearVelocity(btVector3(
              physics->rigidBody->getLinearVelocity().x(),
              physics->rigidBody->getLinearVelocity().y(), 0));
          zFactor = 0.0f;
        }

        // If the character is on top of a moving object, match its velocity TODO: This is poop. Riding car is ghetto.
        float groundSpeedFactor = 1.f;
        if (groundVelocity.length()) {
          btVector3 groundForce = groundVelocity - physics->rigidBody->getLinearVelocity();
          physics->rigidBody->applyCentralImpulse(groundForce);
          linDamp = 0;
          groundSpeedFactor = 2.5f;
        }

        // modify walking force to be perpendicular to the ground normal so as not to point into or out of the ground.
        // this avoids "jiggling" as you walk up or down a hill, especially on slow hardware.
        float origForceMag = glm::length(ctrls->forces);
        ctrls->forces.z = -1 * ( ctrls->forces.x * rayNormalAvg.x() +
                                 ctrls->forces.y * rayNormalAvg.y() ) ;
        // re-normalize the new walking force vector to match the magnitude of the old one
        if ( glm::length(ctrls->forces) ) {
          ctrls->forces = origForceMag * glm::normalize(ctrls->forces);
        }

        physics->rigidBody->applyCentralForce({
            ctrls->forces.x * mvmntForceMagnitude * groundSpeedFactor,
            ctrls->forces.y * mvmntForceMagnitude * groundSpeedFactor,
            ctrls->forces.z * mvmntForceMagnitude * groundSpeedFactor + springForceFinal });

      } else { // movement while in air - apply greatly reduced controls
        physics->rigidBody->applyCentralForce({
            ctrls->forces.x * CHARA_MIDAIR_FACTOR,
            ctrls->forces.y * CHARA_MIDAIR_FACTOR,
            0.f});
      }

      physics->rigidBody->setDamping(linDamp, angDamp); // Set damping
      physics->rigidBody->setLinearFactor(btVector3(1, 1, zFactor));

      // zero controls
      ctrls->forces = glm::vec3();
      ctrls->jumpRequested = false;
      ctrls->isRunning = false;
    }

    // Step the world here
    dynamicsWorld->stepSimulation(dt); // time step (s), max sub-steps, sub-step length (s)
    if (debugDrawMode) { dynamicsWorld->debugDrawWorld(); }

    // TrackControls
    for (auto id : registries[3].ids) {
      TrackControls *trackControls;
      state->get_TrackControls(id, &trackControls);
      //Todo: put engine force application *before* the sim update, with wheel update after?

//      for (size_t i = 0; i < trackControls->wheels.size(); ++i) {
//        if (trackControls->wheels.at(i).leftOrRight < 0) {
//          trackControls->vehicle->applyEngineForce(trackControls->torque.x, trackControls->wheels.at(i).bulletWheelId);
//          trackControls->vehicle->setBrake(trackControls->brakes.x, trackControls->wheels.at(i).bulletWheelId);
//        } else if (trackControls->wheels.at(i).leftOrRight > 0) {
//          trackControls->vehicle->applyEngineForce(trackControls->torque.y, trackControls->wheels.at(i).bulletWheelId);
//          trackControls->vehicle->setBrake(trackControls->brakes.y, trackControls->wheels.at(i).bulletWheelId);
//        }
//      }

      if (trackControls->hasDriver) {
        for (size_t i = 0; i < trackControls->wheels.size(); ++i) {
          if (trackControls->wheels.at(i).leftOrRight < 0) {
            trackControls->vehicle->applyEngineForce(trackControls->torque.x,
                                                     trackControls->wheels.at(i).bulletWheelId);
            trackControls->vehicle->setBrake(trackControls->brakes.x, trackControls->wheels.at(i).bulletWheelId);
          } else if (trackControls->wheels.at(i).leftOrRight > 0) {
            trackControls->vehicle->applyEngineForce(trackControls->torque.y,
                                                     trackControls->wheels.at(i).bulletWheelId);
            trackControls->vehicle->setBrake(trackControls->brakes.y, trackControls->wheels.at(i).bulletWheelId);
          }
        }
      } else {
        for (size_t i = 0; i < trackControls->wheels.size(); ++i) {
          trackControls->vehicle->setBrake(TRACK_VACANT_BRAKE, trackControls->wheels.at(i).bulletWheelId);
        }
      }


      for (int i = 0; i < trackControls->vehicle->getNumWheels(); ++i) {
        trackControls->vehicle->updateWheelTransform(i, true);
      }
      // zero control forces
      trackControls->torque = glm::vec2();
      trackControls->brakes = glm::vec2();
    }

    // All Physics
    for (auto id : registries[0].ids) {
      Physics *physics;
      state->get_Physics(id, &physics);
      Placement *placement;
      state->get_Placement(id, &placement);
      btTransform transform;
      glm::mat4 newTransform;
      switch (physics->geom) {
        case Physics::WHEEL: {
          WheelInfo wi = *((WheelInfo*)physics->customData);
          TrackControls *trackControls;
          if (SUCCESS == state->get_TrackControls(wi.parentVehicle, &trackControls)) {
            transform = trackControls->vehicle->getWheelTransformWS(((WheelInfo*)physics->customData)->bulletWheelId);
          } else {
            // TODO: handle an orphan wheel
            // TODO: instead of deleting all wheels in onForgetTrackControls, do the following here:
            // replace the wheel's physics component with a normal physics component at the same location
          }
        } break;
        default: {
          physics->rigidBody->getMotionState()->getWorldTransform(transform);
        } break;
      }
      transform.getOpenGLMatrix((btScalar *) &newTransform);
      placement->mat = newTransform;
    }
  }

  void PhysicsSystem::deInit() {
	  // onForget will be called for each remaining id
    std::vector<entityId> ids = registries[1].ids;
    for (auto id : ids) {
      state->rem_PyramidControls((entityId) id);
    }
    ids = registries[4].ids;
    for (auto id : ids) {
      state->rem_PlayerControls((entityId) id);
    }
    ids = registries[0].ids;
    for (auto id : ids) {
      state->rem_Physics((entityId) id);
    }

    delete vehicleRaycaster;
    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;
  }

  bool PhysicsSystem::onDiscover(const entityId &id) {
    Placement *placement;
    state->get_Placement(id, &placement);
    Physics *physics;
    state->get_Physics(id, &physics);
    switch (physics->geom) {
      case Physics::SPHERE: {
        physics->shape = new btSphereShape(*((float *) physics->geomInitData));
      } break;
      case Physics::PLANE: {
        assert("missing plane collision implementation" == nullptr);
      } break;
      case Physics::BOX: {
        physics->shape = new btBoxShape(*((btVector3*) physics->geomInitData));
      } break;
      case Physics::MESH: {
        std::vector<float> *points = (std::vector<float> *) physics->geomInitData;
        physics->shape = new btConvexHullShape(points->data(), (int) points->size() / 3, 3 * sizeof(float));
      } break;
      case Physics::CHARA: {
        physics->shape = new btCapsuleShapeZ(HUMAN_WIDTH * 0.5f, HUMAN_HEIGHT * 0.33f);
      } break;
      case Physics::TERRAIN: {
        return false; // do not track, wait for terrain component to appear
      }
      case Physics::WHEEL: {
        WheelInitInfo initInfo = *((WheelInitInfo*)physics->geomInitData);
        TrackControls *trackControls;
        CompOpReturn status = state->get_TrackControls(initInfo.wi.parentVehicle, &trackControls);
        if (status != SUCCESS) {
          EZECS_CHECK_PRINT(EZECS_ERR_MSG(status, "Attempted to add wheel to nonexistent vehicle!\n"));
          return false;
        }
        printf("wheel add:\n%2.f %2.f %2.f\n%2.f %2.f %2.f\n%2.f %2.f %2.f\n",
               initInfo.connectionPoint.x(), initInfo.connectionPoint.y(), initInfo.connectionPoint.z(),
               initInfo.direction.x(), initInfo.direction.y(), initInfo.direction.z(),
               initInfo.axle.x(), initInfo.axle.y(), initInfo.axle.z()
        );
        trackControls->vehicle->addWheel(initInfo.connectionPoint, initInfo.direction, initInfo.axle,
                                         initInfo.suspensionRestLength, initInfo.wheelRadius,
                                         trackControls->tuning, initInfo.isFrontWheel);
        printf("wheel add finished\n");
        initInfo.wi.bulletWheelId = (int)trackControls->wheels.size();
        physics->customData = new WheelInfo(initInfo.wi);
        trackControls->wheels.push_back(initInfo.wi);
        trackControls->vehicle->resetSuspension();
        return true; // wheels do not need normal geometry/collisions (this is for the btRaycastVehicle), but do track.
      }
      default: break;
    }
    physics->geomInitData = nullptr;
    btTransform transform;
    transform.setFromOpenGLMatrix((btScalar *) &placement->mat);
    btDefaultMotionState *motionState = new btDefaultMotionState(transform);
    btVector3 inertia(0.f, 0.f, 0.f);
    physics->shape->calculateLocalInertia(physics->mass, inertia);
    btRigidBody::btRigidBodyConstructionInfo ci(physics->mass, motionState, physics->shape, inertia);
    physics->rigidBody = new btRigidBody(ci);
    switch (physics->geom) {
      case Physics::CHARA: {
        physics->rigidBody->setAngularFactor({0.f, 0.f, 0.f});
        physics->rigidBody->setRestitution(0.f);
        physics->rigidBody->setFriction(0.f);

        // the slow stuff (CCD)
        physics->rigidBody->setCcdMotionThreshold(1);
        physics->rigidBody->setCcdSweptSphereRadius(0.1f);
      } break;
      default: {
        physics->rigidBody->setRestitution(0.5f);
        physics->rigidBody->setFriction(1.f);
      } break;
    }
    physics->rigidBody->setUserIndex((int)id);
    dynamicsWorld->addRigidBody(physics->rigidBody);
    return true;
  }

  bool PhysicsSystem::onForget(const entityId &id) {
    Physics *physics;
    state->get_Physics(id, &physics);
    switch (physics->geom) {
      case Physics::WHEEL: {
        // TODO: delete wheel from vehicle somehow? also remove from trackControl's vector?
        /*WheelInfo *wheelInfo = ((WheelInfo*)physics->customData);
        TrackControls *trackControls;
        CompOpReturn status = state->get_TrackControls(wheelInfo->parentVehicle, &trackControls);
        if (status != SUCCESS) {
          EZECS_CHECK_PRINT(EZECS_ERR_MSG(status, "Attempted to access wheel of nonexistent vehicle!\n"));
          return false;
        }
        btWheelInfo& wheelInfoBt = trackControls->vehicle->getWheelInfo(wheelInfo->bulletWheelId);*/
      } break;
      default: {
        dynamicsWorld->removeRigidBody(physics->rigidBody);
        delete physics->rigidBody->getMotionState();
        delete physics->rigidBody;
        delete physics->shape;
      } break;
    }
    return true;
  }

  bool PhysicsSystem::onDiscoverTerrain(const entityId &id) {
    Placement *placement;
    state->get_Placement(id, &placement);
    Physics *physics;
    state->get_Physics(id, &physics);
    Terrain *terrain;
    state->get_Terrain(id, &terrain);

    physics->shape = new btHeightfieldTerrainShape(
        (int)terrain->resX,         // int heightStickWidth  (stupid name - I think it just means width
        (int)terrain->resY,         // int heightStickLength (sutpid name - I think it just means height
        terrain->heights->data(),   // const void *heightfieldData
        0,                          // btScalar heightScale (doesn't matter for float data)
        terrain->minZ,              // btScalar minHeight
        terrain->maxZ,              // btScalar maxHeight
        2,                          // int upAxis
        PHY_FLOAT,                  // PHY_ScalarType heightDataType
        false                       // bool flipQuadEdges
    );

    physics->shape->setLocalScaling({
        terrain->sclX / (float)terrain->resX,
        terrain->sclY / (float)terrain->resY,
        terrain->sclZ });
    btTransform transform;
    transform.setFromOpenGLMatrix((btScalar *) &placement->mat);
    auto *terrainMotionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo terrainRBCI(0, terrainMotionState, physics->shape, btVector3(0, 0, 0));
    physics->rigidBody = new btRigidBody(terrainRBCI);
    physics->rigidBody->setRestitution(0.5f);
    physics->rigidBody->setFriction(1.f);
    physics->rigidBody->setCollisionFlags(physics->rigidBody->getCollisionFlags()
                                          | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK
                                          | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
    physics->rigidBody->setUserIndex(-(int)id); // negative numbers for static objects, let's say. TODO: follow-up
    dynamicsWorld->addRigidBody(physics->rigidBody);

    return true;
  }

  bool PhysicsSystem::onDiscoverTrackControls(const entityId &id) {
    // onDiscover is guaranteed to have been called already, since TrackControls requires Physics.
    Physics *physics;
    state->get_Physics(id, &physics);
    TrackControls *trackControls;
    state->get_TrackControls(id, &trackControls);
    trackControls->vehicle = new btRaycastVehicle(trackControls->tuning, physics->rigidBody, vehicleRaycaster);
    dynamicsWorld->addVehicle(trackControls->vehicle);
    trackControls->vehicle->setCoordinateSystem(0, 2, 1);
    return true;
  }
  bool PhysicsSystem::onForgetTrackControls(const entityId &id) {
    // fixme: investigate the order of deletion if both this and the normal onForget are called on a chassis
    TrackControls *trackControls;
    state->get_TrackControls(id, &trackControls);
    for (auto wheel : trackControls->wheels) {
      state->rem_Physics(wheel.myId);
    }
    dynamicsWorld->removeVehicle(trackControls->vehicle);
    delete trackControls->vehicle;
    return true;
  }

  void PhysicsSystem::setDebugDrawer(std::shared_ptr<BulletDebug_> debug) {
    dynamicsWorld->setDebugDrawer(debug.get());
  }

  btCollisionWorld::ClosestRayResultCallback PhysicsSystem::rayTest(const btVector3 &start, const btVector3 &end) {
    btCollisionWorld::ClosestRayResultCallback rayCallback(start, end);
    dynamicsWorld->rayTest(start, end, rayCallback);
    return rayCallback;
  }

  rayFuncType PhysicsSystem::getRayFunc() {
    return std::bind( &PhysicsSystem::rayTest, this, std::placeholders::_1, std::placeholders::_2 );
  }

  void PhysicsSystem::toggleDebugDraw(void* nothing) {
    debugDrawMode = !debugDrawMode;
  }
}

#pragma clang diagnostic pop
