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

namespace ld2016 {

  PhysicsSystem::PhysicsSystem(State *state) : System(state) {

  }
  bool PhysicsSystem::onInit() {
    registries[0].discoverHandler = DELEGATE(&PhysicsSystem::onDiscover, this);
    registries[0].forgetHandler = DELEGATE(&PhysicsSystem::onForget, this);

    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver();
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0.0f, 0.0f, -9.81f));
    planeShape = new btStaticPlaneShape(btVector3(0.f, 0.f, 1.f), 0.f);

    //region Create ground
    groundMotionState = new btDefaultMotionState(btTransform(
        btQuaternion(0.f, 0.f, 0.f, 1.f),
        btVector3(0.f, 0.f, 0.f)));
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, planeShape, btVector3(0, 0, 0));
    groundRigidBody = new btRigidBody(groundRigidBodyCI);
    groundRigidBody->setRestitution(0.5f);
    groundRigidBody->setFriction(1.f);
    dynamicsWorld->addRigidBody(groundRigidBody);
    //endregion
    return true;
  }
  void PhysicsSystem::onTick(float dt) {
    for (auto id : registries[1].ids) {
      WasdControls* wasdControls;
      state->get_WasdControls(id, &wasdControls);
      Physics* physics;
      state->get_Physics(id, &physics);
      physics->rigidBody->applyCentralImpulse({-wasdControls->accel.x, -wasdControls->accel.y, wasdControls->accel.z});
    }
    dynamicsWorld->stepSimulation(dt); // time step (s), max sub-steps, sub-step length (s)
    for (auto id : registries[0].ids) {
      Physics* physics;
      state->get_Physics(id, &physics);
      btTransform currentTrans;
      physics->rigidBody->getMotionState()->getWorldTransform(currentTrans);
      Position* position;
      state->get_Position(id, &position);
      btVector3 currentPos = currentTrans.getOrigin();
      position->vec = {currentPos.getX(), currentPos.getY(), currentPos.getZ()};

      Orientation* orientation;
      state->get_Orientation(id, &orientation);
      btQuaternion currentOr = currentTrans.getRotation();
      orientation->quat = {currentOr.getX(), currentOr.getY(), currentOr.getZ(), currentOr.getW()};
    }
  }
  void PhysicsSystem::deInit() {
    for (auto id : registries[0].ids) {
      state->deleteEntity(id); // onForget will be called for each id
    }
    //region Delete ground
    dynamicsWorld->removeRigidBody(groundRigidBody);
    delete groundRigidBody;
    delete groundMotionState;
    //endregion

    delete planeShape;
    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;
  }
  bool PhysicsSystem::onDiscover(const entityId &id) {
    Position* position;
    state->get_Position(id, &position);
    Orientation* orientation;
    state->get_Orientation(id, &orientation);
    Physics* physics;
    state->get_Physics(id, &physics);
    switch(physics->geom) {
      case Physics::SPHERE:
        physics->shape = new btSphereShape(*((float*)physics->geomInitData));
        break;
      case Physics::PLANE:
        assert("missing plane collision implementation" == nullptr);
        break;
      case Physics::MESH: {
          std::vector<float> *points = (std::vector<float> *) physics->geomInitData;
          physics->shape = new btConvexHullShape(points->data(), (int) points->size());
        }
        break;
      default:
        break;
    }
    physics->geomInitData = nullptr;
    btDefaultMotionState* motionState =
        new btDefaultMotionState(
            btTransform(btQuaternion(
                orientation->quat.x, orientation->quat.y, orientation->quat.z, orientation->quat.w),
            btVector3(position->vec.x, position->vec.y, position->vec.z)));
    btVector3 inertia(0.f, 0.f, 0.f);
    physics->shape->calculateLocalInertia(physics->mass, inertia);
    btRigidBody::btRigidBodyConstructionInfo ci(physics->mass, motionState, physics->shape, inertia);
    physics->rigidBody = new btRigidBody(ci);
    physics->rigidBody->setRestitution(0.8f);
    physics->rigidBody->setFriction(1.f);
    dynamicsWorld->addRigidBody(physics->rigidBody);
    return true;
  }
  bool PhysicsSystem::onForget(const entityId &id) {
    Physics* physics;
    state->get_Physics(id, &physics);
    dynamicsWorld->removeRigidBody(physics->rigidBody);
    delete physics->rigidBody->getMotionState();
    delete physics->rigidBody;
    delete physics->shape;
    return true;
  }
}
