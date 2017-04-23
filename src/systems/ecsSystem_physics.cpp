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

using namespace ezecs;

namespace at3 {

  PhysicsSystem::PhysicsSystem(State *state) : System(state) {

  }

  bool PhysicsSystem::onInit() {
    registries[0].discoverHandler = DELEGATE(&PhysicsSystem::onDiscover, this);
    registries[0].forgetHandler = DELEGATE(&PhysicsSystem::onForget, this);
    registries[2].discoverHandler = DELEGATE(&PhysicsSystem::onDiscoverTerrain, this);

    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver();
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0.0f, 0.0f, -9.81f));



    
    // PREVENT BACKFACE COLLISIONS
    /*static bool myCustomMaterialCombinerCallback(
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
        btVector3 faceNormalLs = btCross(v[1] - v[0], v[2] - v[0]);
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

    void BulletPhysics::init()
    {
      // somewhere in your init code you need to setup the callback
      gContactAddedCallback = myCustomMaterialCombinerCallback;
    }*/
    // AND
    /*terrain->setCollisionFlags(_floor->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);*/




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
          btVector3(rotAxis.x, rotAxis.y, rotAxis.z) * tip * (controls->up.z < 0 ? -10.f : 10.f)
      );
    }
    dynamicsWorld->stepSimulation(dt); // time step (s), max sub-steps, sub-step length (s)
    if (debugDrawMode) { dynamicsWorld->debugDrawWorld(); }
    for (auto id : registries[0].ids) {
      Physics *physics;
      state->get_Physics(id, &physics);
      btTransform currentTrans;
      physics->rigidBody->getMotionState()->getWorldTransform(currentTrans);

      Placement *placement;
      state->get_Placement(id, &placement);
      glm::mat4 newTransform;
      currentTrans.getOpenGLMatrix((btScalar *) &newTransform);
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
      default:
        break;
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
    dynamicsWorld->removeRigidBody(physics->rigidBody);
    delete physics->rigidBody->getMotionState();
    delete physics->rigidBody;
    delete physics->shape;
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
    dynamicsWorld->addRigidBody(physics->rigidBody);

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
