/*
 * Copyright (c) 2016 Galen Cochrane
 * Galen Cochrane <galencochrane@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "ecsSystem_physics.h"
#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <BulletCollision/CollisionShapes/btTriangleShape.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "IncompatibleTypes"
#pragma ide diagnostic ignored "TemplateArgumentsIssues"
using namespace ezecs;

namespace at3 {

  // A custom bullet collision callback to prevent collisions with backfaces (created for use with terrain).
  static bool myCustomMaterialCombinerCallback(
      btManifoldPoint& cp,
      const btCollisionObjectWrapper* colObj0Wrap,
      int partId0,
      int index0,
      const btCollisionObjectWrapper* colObj1Wrap,
      int partId1,
      int index1
  ) {
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

  PhysicsSystem::PhysicsSystem(State *state) : System(state) {

  }

  bool PhysicsSystem::onInit() {
    registries[0].discoverHandler = DELEGATE(&PhysicsSystem::onDiscover, this);
    registries[0].forgetHandler = DELEGATE(&PhysicsSystem::onForget, this);
    registries[2].discoverHandler = DELEGATE(&PhysicsSystem::onDiscoverTerrain, this);
    registries[3].discoverHandler = DELEGATE(&PhysicsSystem::onDiscoverTrackControls, this);
    registries[4].forgetHandler = DELEGATE(&PhysicsSystem::onForgetTrackControls, this);

    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver();
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0.0f, 0.0f, -9.81f));
    vehicleRaycaster = new btDefaultVehicleRaycaster(dynamicsWorld);

    // prevent backface collisions with anything that uses the custom collision callback (like terrain)
    gContactAddedCallback = myCustomMaterialCombinerCallback;

    return true;
  }

  void PhysicsSystem::onTick(float dt) {
    for (auto id : registries[1].ids) {
      PyramidControls *controls;
      state->get_PyramidControls(id, &controls);
      Physics *physics;
      state->get_Physics(id, &physics);
      physics->rigidBody->applyImpulse({controls->force.x, controls->force.y, controls->force.z},
                                       btVector3(controls->up.x, controls->up.y, controls->up.z) * 0.05f);

      // Custom constraint to keep the pyramid up FixMe: this sometimes gets stuck in a horizontal position?
      glm::vec3 up{0.f, 0.f, 1.f};
      float tip = glm::dot(controls->up, up);
      glm::vec3 rotAxis = glm::cross(controls->up, up);
      physics->rigidBody->applyTorque(
          btVector3(rotAxis.x, rotAxis.y, rotAxis.z) * tip * (controls->up.z < 0 ? -1000.f : 1000.f)
      );
    }

    dynamicsWorld->stepSimulation(dt); // time step (s), max sub-steps, sub-step length (s)
    if (debugDrawMode) { dynamicsWorld->debugDrawWorld(); }

    for (auto id : registries[3].ids) {
      TrackControls *trackControls;
      state->get_TrackControls(id, &trackControls);
      //Todo: put engine force application *before* the sim update, with wheel update after?
      for (int i = 0; i < trackControls->wheels.size(); ++i) {
        if (trackControls->wheels.at(i).leftOrRight < 0) {
          trackControls->vehicle->applyEngineForce(trackControls->torque.x, trackControls->wheels.at(i).bulletWheelId);
        } else if (trackControls->wheels.at(i).leftOrRight > 0) {
          trackControls->vehicle->applyEngineForce(trackControls->torque.y, trackControls->wheels.at(i).bulletWheelId);
        }
      }
      for (int i = 0; i < trackControls->vehicle->getNumWheels(); ++i) {
        trackControls->vehicle->updateWheelTransform(i, true);
      }
    }

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
	  // onForget will be called for each id
    std::vector<entityId> ids = registries[1].ids;
    for (auto id : ids) {
      state->rem_PyramidControls((entityId) id);
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
      case Physics::SPHERE:
        physics->shape = new btSphereShape(*((float *) physics->geomInitData));
        break;
      case Physics::PLANE:
        assert("missing plane collision implementation" == nullptr);
        break;
      case Physics::MESH: {
        std::vector<float> *points = (std::vector<float> *) physics->geomInitData;
        physics->shape = new btConvexHullShape(points->data(), (int) points->size() / 3, 3 * sizeof(float));
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
    physics->geomInitData = nullptr;
    btTransform transform;
    transform.setFromOpenGLMatrix((btScalar *) &placement->mat);
    btDefaultMotionState *motionState = new btDefaultMotionState(transform);
    btVector3 inertia(0.f, 0.f, 0.f);
    physics->shape->calculateLocalInertia(physics->mass, inertia);
    btRigidBody::btRigidBodyConstructionInfo ci(physics->mass, motionState, physics->shape, inertia);
    physics->rigidBody = new btRigidBody(ci);
    physics->rigidBody->setRestitution(0.5f);
    physics->rigidBody->setFriction(1.f);
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
    btDefaultMotionState *terrainMotionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo terrainRBCI(0, terrainMotionState, physics->shape, btVector3(0, 0, 0));
    physics->rigidBody = new btRigidBody(terrainRBCI);
    physics->rigidBody->setRestitution(0.5f);
    physics->rigidBody->setFriction(1.f);
    physics->rigidBody->setCollisionFlags(physics->rigidBody->getCollisionFlags()
                                          | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
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

  bool PhysicsSystem::handleEvent(SDL_Event& event) {
    switch (event.type) {
      case SDL_KEYDOWN:
        switch (event.key.keysym.scancode) {
          case SDL_SCANCODE_R: {
            debugDrawMode = !debugDrawMode;
            break;
          }
          default:
            return false; // could not handle it here
        } break;
      default:
        return false; // could not handle it here
    }
    return true; // handled it here
  }

  void PhysicsSystem::setDebugDrawer(std::shared_ptr<BulletDebug> debug) {
    dynamicsWorld->setDebugDrawer(debug.get());
  }
}

#pragma clang diagnostic pop