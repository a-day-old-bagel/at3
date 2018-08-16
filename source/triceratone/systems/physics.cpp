
#include "physics.hpp"
#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <BulletCollision/CollisionShapes/btTriangleShape.h>

#include "math.hpp"
#include "cylinderMath.hpp"

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
    // one-sided triangles - This is used to allow for going back through the backside of terrain if you fall through.
    if (colObj1Wrap->getCollisionShape()->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE) {
      auto triShape = static_cast<const btTriangleShape*>( colObj1Wrap->getCollisionShape() );
      const btVector3* v = triShape->m_vertices1;
      btVector3 faceNormalLs = btCross(v[2] - v[0], v[1] - v[0]);
      faceNormalLs.normalize();
      btVector3 faceNormalWs = colObj1Wrap->getWorldTransform().getBasis() * faceNormalLs;
      float nDotF = btDot( faceNormalWs, cp.m_normalWorldOnB );
      if ( nDotF >= 0.0f ) { // switch this to switch forward/back faces
        // flip the contact normal to be aligned with the face normal
        cp.m_normalWorldOnB += -2.0f * nDotF * faceNormalWs;
      }
    }
    //this return value is currently ignored, but to be on the safe side: return false if you don't calculate friction
    return false;
  }

  PhysicsSystem::PhysicsSystem(State *state) : System(state),
      debugDrawToggleSub("key_down_f3", RTU_MTHD_DLGT(&PhysicsSystem::toggleDebugDraw, this)),
      setVulkanContextSub("set_vulkan_context", RTU_MTHD_DLGT(&PhysicsSystem::setVulkanContext, this))
  {
    name = "Physics System";
  }
  PhysicsSystem::~PhysicsSystem() {
    deInit();
  }

  void PhysicsSystem::setVulkanContext(void *vkc) {
    vulkan = *(std::shared_ptr<vkc::VulkanContext<EntityComponentSystemInterface>>*) vkc;
  }

  bool PhysicsSystem::onInit() {
    registries[0].discoverHandler = RTU_MTHD_DLGT(&PhysicsSystem::onDiscover, this);
    registries[0].forgetHandler = RTU_MTHD_DLGT(&PhysicsSystem::onForget, this);
    registries[1].discoverHandler = RTU_MTHD_DLGT(&PhysicsSystem::onDiscoverPyramidControls, this);
    registries[2].discoverHandler = RTU_MTHD_DLGT(&PhysicsSystem::onDiscoverTrackControls, this);
    registries[2].forgetHandler = RTU_MTHD_DLGT(&PhysicsSystem::onForgetTrackControls, this);

    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver();
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0.f, 0.f, 0.f));
    vehicleRaycaster = new btDefaultVehicleRaycaster(dynamicsWorld);

    // prevent backface collisions with anything that uses the custom collision callback (like terrain)
    gContactAddedCallback = myCustomMaterialCombinerCallback;

    // set up ghost object collision detection
    dynamicsWorld->getPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

    // TODO: somehow do this at the same time for a server and a client upon connect?
//    solver->reset();

    return true;
  }

  void PhysicsSystem::onTick(float dt) {

    // PyramidControls
    for (auto id : registries[1].ids) {
      Placement *placement;
      state->getPlacement(id, &placement);
      PyramidControls *ctrls;
      state->getPyramidControls(id, &ctrls);
      Physics *physics;
      state->getPhysics(id, &physics);
      physics->rigidBody->applyForce({ctrls->force.x, ctrls->force.y, ctrls->force.z},
                                     btVector3(ctrls->up.x, ctrls->up.y, ctrls->up.z) * 0.05f);

      // simplified gravity affecting pyramid, not accounting for tangential motion.
      glm::vec3 grav = getNaiveCylGrav(placement->getTranslation(true));

      // Custom constraint to keep the pyramid pointing in the correct "up" direction
      glm::vec3 up = -glm::normalize(grav);
      float tip = glm::dot(ctrls->up, up) + 1.f;  // magnitude greatest @ 180 degrees (good) (but why positive 1...?)
      glm::vec3 rotAxis = glm::cross(ctrls->up, up);  // magnitude greatest @ 90 degrees, weak @ 180 (not ideal)
      physics->rigidBody->applyTorque(
          btVector3(rotAxis.x, rotAxis.y, rotAxis.z) * tip * 1000.f
      );
    }

    // WalkControls
    // TODO: create a kinematic body (0 mass, recalc inertia) to which to anchor
    //       the body when it lands from a height (point to point constraint, AKA ball joint, maybe)?
    for (auto id : registries[3].ids) {
      Placement *placement;
      state->getPlacement(id, &placement);
      WalkControls *ctrls;
      state->getWalkControls(id, &ctrls);
      Physics *physics;
      state->getPhysics(id, &physics);

      // Fire some rays straight "down"

      float rayLength = 2.f;//HUMAN_HEIGHT * 1.5f;
      float rayDistTravelledAvg = 0.f;
      btVector3 rayHitPointAvg = {0.f, 0.f, 0.f};
      btVector3 rayNormalAvg = {0.f, 0.f, 0.f};
      btVector3 groundVelocity;
      bool groundVelocityFound = false;
      bool anyRayHitGoodGround = false;

      glm::vec3 pos = placement->getTranslation(true);
      btVector3 btPos = glmToBullet(pos);
      glm::vec3 btVel = bulletToGlm(physics->rigidBody->getLinearVelocity());
      glm::vec3 down = getNaiveCylGravDir(pos);
      btVector3 rayDirection = glmToBullet(down);

      std::vector<btVector3> rayStartLocations = {
          {btPos.x(), btPos.y(), btPos.z()},
      };

      for (auto rayStart : rayStartLocations) {
        btVector3 rayEnd = rayStart + rayDirection * rayLength;
        btCollisionWorld::ClosestRayResultCallback groundSpringRayCallback(rayStart, rayEnd);
        dynamicsWorld->rayTest(rayStart, rayEnd, groundSpringRayCallback);
        rayDistTravelledAvg += (groundSpringRayCallback.m_hitPointWorld - rayStart).length();
        anyRayHitGoodGround |= (groundSpringRayCallback.hasHit()); // TODO: add steepness check?
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
      float upSpeed = glm::dot(btVel, -down);
      if (ctrls->jumpInProgress) {
        if (springForceLinear >= -0.1f && upSpeed < 0.f) {
          ctrls->jumpInProgress = false;
        }
      } else if (ctrls->jumpRequested) {
        physics->rigidBody->applyCentralImpulse(-rayDirection * CHARA_JUMP);
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

//        // Add a strong damping and negation of falling motion if the character is moving down fast past equilibrium
////        if (zVel < -0.2f && springForceLinear > 0.05f) {
//        if (upSpeed < -0.2f && springForceLinear > 0.05f) {
//          physics->rigidBody->setLinearVelocity(btVector3(
//              physics->rigidBody->getLinearVelocity().x(),
//              physics->rigidBody->getLinearVelocity().y(), 0));
//          zFactor = 0.0f;
//        }

//        // If the character is on top of a moving object, match its velocity TODO: This is poop. Riding car is ghetto.
//        float groundSpeedFactor = 1.f;
//        if (groundVelocity.length()) {
//          btVector3 groundForce = groundVelocity - physics->rigidBody->getLinearVelocity();
//          physics->rigidBody->applyCentralImpulse(groundForce);
//          linDamp = 0;
//          groundSpeedFactor = 2.5f;
//        }

//        // modify walking force to be perpendicular to the ground normal so as not to point into or out of the ground.
//        // this avoids "jiggling" as you walk up or down a hill, especially on slow hardware.
//        float origForceMag = glm::length(ctrls->force);
//        ctrls->force.z = -1 * ( ctrls->force.x * rayNormalAvg.x() +
//                                 ctrls->force.y * rayNormalAvg.y() ) ;
//        // re-normalize the new walking force vector to match the magnitude of the old one
//        if ( glm::length(ctrls->force) ) {
//          ctrls->force = origForceMag * glm::normalize(ctrls->force);
//        }

        btVector3 springForceFinalVector = springForceFinal * glmToBullet(-down);
        btVector3 finalForceVector = btVector3(
            ctrls->force.x * mvmntForceMagnitude,
            ctrls->force.y * mvmntForceMagnitude,
            ctrls->force.z * mvmntForceMagnitude) + springForceFinalVector;
        physics->rigidBody->applyCentralForce(finalForceVector);

      } else { // movement while in air - apply greatly reduced controls
        physics->rigidBody->applyCentralForce({
              ctrls->force.x * CHARA_MIDAIR_FACTOR,
              ctrls->force.y * CHARA_MIDAIR_FACTOR,
              ctrls->force.z * CHARA_MIDAIR_FACTOR});
      }

      physics->rigidBody->setDamping(linDamp, angDamp); // Set damping
      physics->rigidBody->setLinearFactor(btVector3(1, 1, zFactor));

      // zero controls
      ctrls->force = glm::vec3(0, 0, 0);
      ctrls->jumpRequested = false;
      ctrls->isRunning = false;
    }

    // Step the world here
    dynamicsWorld->stepSimulation(dt); // time step (s), max sub-steps, sub-step length (s)
    if (debugDrawMode) { dynamicsWorld->debugDrawWorld(); }

    // TrackControls
    for (auto id : registries[2].ids) {
      TrackControls *trackControls;
      state->getTrackControls(id, &trackControls);
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

      if (trackControls->flipRequested) {
        Physics *physics;
        state->getPhysics(id, &physics);
        Placement *placement;
        state->getPlacement(id, &placement);
        glm::vec3 cen = glm::vec3(0, 0, 1);
        glm::vec3 dir = -getNaiveCylGrav(placement->getTranslation(true)) * 25.f;
        physics->rigidBody->applyImpulse({dir.x, dir.y, dir.z}, {cen.x, cen.y, cen.z});
        trackControls->flipRequested = false;
      }

      for (int i = 0; i < trackControls->vehicle->getNumWheels(); ++i) {
        trackControls->vehicle->updateWheelTransform(i, true);
      }
      // zero control forces
      trackControls->torque = glm::vec2(0, 0);
      trackControls->brakes = glm::vec2(0, 0);
    }

    // All Physics
    for (auto id : registries[0].ids) {
      Physics *physics;
      state->getPhysics(id, &physics);
      Placement *placement;
      state->getPlacement(id, &placement);
      btTransform transform;
      switch (physics->useCase) {
        case Physics::WHEEL: {
          WheelInfo wi = *((WheelInfo*)physics->customData);
          TrackControls *trackControls;
          if (SUCCESS == state->getTrackControls(wi.parentVehicle, &trackControls)) {
            transform = trackControls->vehicle->getWheelTransformWS(((WheelInfo*)physics->customData)->bulletWheelId);
          } else {
            // TODO: handle an orphan wheel
            // TODO: instead of deleting all wheels in onForgetTrackControls, do the following here:
            // replace the wheel's physics component with a normal physics component at the same location
          }
        } break;
        default: {
          if ( ! physics->rigidBody->isActive()) { continue; }

          physics->rigidBody->getMotionState()->getWorldTransform(transform);

          btVector3 vel = physics->rigidBody->getLinearVelocity();
          glm::vec3 grav = getCylGrav(placement->getTranslation(true), glm::vec3(vel.x(), vel.y(), vel.z()));

          btVector3 btGrav = btVector3(grav.x, grav.y, grav.z);
          physics->rigidBody->setGravity(btGrav);

          // temporary hack to stop things from flying off to infinity if they escape the cylinder FIXME: hack
          glm::vec3 pos = placement->getTranslation(true);
          if (glm::length(glm::vec2(pos.x, pos.y)) > 810.f || pos.z < -2510.f) {
            btTransform writeTransform = physics->rigidBody->getCenterOfMassTransform();
            float xOffset = (rand() % 400) * .1f - 20.f;
            float yOffset = (rand() % 400) * .1f - 20.f;
            writeTransform.setOrigin(btVector3(xOffset, yOffset, 2200)); // appears near "top" end of cylinder
            physics->rigidBody->setCenterOfMassTransform(writeTransform);
            physics->rigidBody->setLinearVelocity(btVector3(0, 0, 0));
          }

        } break;
      }
      glm::mat4 newTransform(1.f);
      transform.getOpenGLMatrix((btScalar *) &newTransform);
      placement->mat = newTransform;
    }
  }

  void PhysicsSystem::deInit() {  // TODO: re-evaluate if everything is being deleted correctly
    std::vector<entityId> ids = registries[0].ids;
    for (auto id : ids) {
      state->remPhysics((entityId) id);
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
    state->getPlacement(id, &placement);
    Physics *physics;
    state->getPhysics(id, &physics);
    // TODO: Re-use shapes instead of making a new one for each instance (like spheres with the same radius)
    btCollisionShape* shape = nullptr;
    switch (physics->useCase) {
      case Physics::SPHERE: {
        shape = new btSphereShape(*((float *) physics->initData.get()));
      } break;
      case Physics::PLANE: {
        AT3_ASSERT(false, "missing plane collision implementation");
      } break;
      case Physics::BOX: {
        shape = new btBoxShape(*((btVector3*) physics->initData.get()));
      } break;
      case Physics::DYNAMIC_CONVEX_MESH: {
        auto *points = (std::vector<float> *) physics->initData.get();
        shape = new btConvexHullShape(points->data(), (int) points->size() / 3, 3 * sizeof(float));
      } break;
      case Physics::CHARA: {
//        shape = new btCapsuleShapeZ(HUMAN_WIDTH * 0.5f, HUMAN_HEIGHT * 0.33f);
        shape = new btSphereShape(HUMAN_WIDTH * 0.5f);
      } break;
      case Physics::STATIC_MESH: {
        physics->customData = new btTriangleMesh();
        auto *mesh = reinterpret_cast<btTriangleMesh*>(physics->customData);

        std::string terrainMeshName = *static_cast<std::string*>(physics->initData.get());
        std::vector<float> *verts = vulkan->getMeshStoredVertices(terrainMeshName);
        std::vector<uint32_t> *indices = vulkan->getMeshStoredIndices(terrainMeshName);
        uint32_t vertexStride = vulkan->getMeshStoredVertexStride();

        for (uint32_t i = 0; i < verts->size(); i += vertexStride / sizeof(float)) {
          glm::vec4 pos = placement->mat * glm::vec4((*verts)[i], (*verts)[i + 1], (*verts)[i + 2], 1.f);
          mesh->findOrAddVertex({pos.x, pos.y, pos.z}, false);
        }
        for (uint32_t i = 0; i < indices->size(); i += 3) {
          mesh->addTriangleIndices((*indices)[i], (*indices)[i + 1], (*indices)[i + 2]);
        }
        shape = new btBvhTriangleMeshShape(mesh, true);

        auto *motionState = new btDefaultMotionState();
        btRigidBody::btRigidBodyConstructionInfo rbci(0, motionState, shape);
        physics->rigidBody = new btRigidBody(rbci);
        physics->rigidBody->setRestitution(0.5f);
        physics->rigidBody->setFriction(1.f);
        physics->rigidBody->setCollisionFlags(physics->rigidBody->getCollisionFlags() |
                                              btCollisionObject::CF_STATIC_OBJECT |
                                              btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
        physics->rigidBody->setUserIndex(-(int)id); // negative of its ID signifies a static object
        dynamicsWorld->addRigidBody(physics->rigidBody);

      } return true; // Too different from dynamic objects to use the code below, but do track.
      case Physics::WHEEL: {
        WheelInitInfo initInfo = *((WheelInitInfo*)physics->initData.get());
        TrackControls *trackControls;
        CompOpReturn status = state->getTrackControls(initInfo.wi.parentVehicle, &trackControls);
        if (status != SUCCESS) {
          EZECS_CHECK_PRINT(EZECS_ERR_MSG(status, "Attempted to add wheel to nonexistent vehicle!\n"));
          return false;
        }
        trackControls->vehicle->addWheel(initInfo.connectionPoint, initInfo.direction, initInfo.axle,
                                         initInfo.suspensionRestLength, initInfo.wheelRadius,
                                         trackControls->tuning, initInfo.isFrontWheel);
        initInfo.wi.bulletWheelId = (int)trackControls->wheels.size();
        physics->customData = new WheelInfo(initInfo.wi);
        trackControls->wheels.push_back(initInfo.wi);
        trackControls->vehicle->resetSuspension();
        return true; // wheels do not need normal geometry/collisions (this is for the btRaycastVehicle), but do track.
      }
      default: break;
    }
    btTransform transform;
    transform.setFromOpenGLMatrix((btScalar *) &placement->mat);
    auto *motionState = new btDefaultMotionState(transform);
    btVector3 inertia(0.f, 0.f, 0.f);
    shape->calculateLocalInertia(physics->mass, inertia);
    btRigidBody::btRigidBodyConstructionInfo ci(physics->mass, motionState, shape, inertia);
    physics->rigidBody = new btRigidBody(ci);
    switch (physics->useCase) {
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
    state->getPhysics(id, &physics);
    if (physics->useCase == Physics::WHEEL) {
      // TODO: delete wheel from vehicle somehow? also remove from trackControl's vector?
      delete ((WheelInfo*)physics->customData);
      return true;
    }
    dynamicsWorld->removeRigidBody(physics->rigidBody);
    if (physics->useCase == Physics::STATIC_MESH) {
//      delete ((btTriangleMeshShape*)physics->rigidBody->getCollisionShape())->getMeshInterface();
      delete (btTriangleMesh*)physics->customData;
    }
    delete physics->rigidBody->getMotionState();
    delete physics->rigidBody->getCollisionShape();
    delete physics->rigidBody;
    return true;
  }

  bool PhysicsSystem::onDiscoverTrackControls(const entityId &id) {
    // TODO: probably just make all the wheels in here as local entities once local entities are a thing.
    // onDiscover is guaranteed to have been called already, since TrackControls requires Physics.
    Physics *physics;
    state->getPhysics(id, &physics);
    TrackControls *trackControls;
    state->getTrackControls(id, &trackControls);
    trackControls->vehicle = new btRaycastVehicle(trackControls->tuning, physics->rigidBody, vehicleRaycaster);
    dynamicsWorld->addVehicle(trackControls->vehicle);
    trackControls->vehicle->setCoordinateSystem(0, 2, 1);

    // Keep the vehicle from freezing in place
    physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
    // The slow stuff (CCD) to keep it from passing through things at high speed (doesn't help below threshold)
    physics->rigidBody->setCcdMotionThreshold(1);
    physics->rigidBody->setCcdSweptSphereRadius(0.2f);
    return true;
  }
  bool PhysicsSystem::onForgetTrackControls(const entityId &id) {
    // fixme: investigate the order of deletion if both this and the normal onForget are called on a chassis
    TrackControls *trackControls;
    state->getTrackControls(id, &trackControls);
    for (auto wheel : trackControls->wheels) {
      state->remPhysics(wheel.myId);
    }
    dynamicsWorld->removeVehicle(trackControls->vehicle);
    delete trackControls->vehicle;
    return true;
  }

  void PhysicsSystem::setDebugDrawer() {
//    dynamicsWorld->setDebugDrawer(thing);
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

  bool PhysicsSystem::onDiscoverPyramidControls(const entityId &id) {
    Physics *physics;
    state->getPhysics(id, &physics);
    // Keep the vehicle from freezing in place
    physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
    // Damp angular velocity to make it look like it's gyroscopically stabilized or something
    physics->rigidBody->setDamping(0.f, 0.8f);
    // The slow stuff (CCD) to keep it from passing through things at high speed (doesn't help below threshold)
    physics->rigidBody->setCcdMotionThreshold(1);
    physics->rigidBody->setCcdSweptSphereRadius(0.2f);
    return true;
  }
}
