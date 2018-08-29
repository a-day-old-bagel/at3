
#include <global/settings.hpp>
#include "controls.hpp"
#include "cylinderMath.hpp"

#include "pyramid.hpp" // just for testing using pyramids instead of balls
#include "duneBuggy.hpp" // just for testing using pyramids instead of balls

using namespace rtu::topics;

namespace at3 {

  ControlSystem::ControlSystem(State *state) : System(state) {
    name = "Control System";
    // Static subscriptions will only apply to the first instance of this class created. But usually only one exists.
    RTU_STATIC_SUB(setEcsInterfaceSub, "set_ecs_interface", ControlSystem::setEcsInterface, this);
  }
  ControlSystem::~ControlSystem() {
    currentCtrlKeys.reset();
    currentCtrlMouse.reset();
  }

  bool ControlSystem::onInit() {
    registries[0].discoverHandler = RTU_MTHD_DLGT(&ControlSystem::onDiscoverMouseControls, this);
    RTU_STATIC_SUB(switchToMouseCtrlSub, "switch_to_mouse_controls", ControlSystem::switchToMouseCtrl, this);
    RTU_STATIC_SUB(switchToWalkCtrlSub, "switch_to_walking_controls", ControlSystem::switchToWalkCtrl, this);
    RTU_STATIC_SUB(switchToPyramidCtrlSub, "switch_to_pyramid_controls", ControlSystem::switchToPyramidCtrl, this);
    RTU_STATIC_SUB(switchToTrackCtrlSub, "switch_to_track_controls", ControlSystem::switchToTrackCtrl, this);
    RTU_STATIC_SUB(switchToFreeCtrlSub, "switch_to_free_controls", ControlSystem::switchToFreeCtrl, this);
    return true;
  }

  void ControlSystem::onTick(float dt) {
    for (auto id : (registries[0].ids)) { // Mouse controls updated at graphics framerate
      MouseControls *mouseControls;
      state->getMouseControls(id, &mouseControls);
      Placement *placement;
      state->getPlacement(id, &placement);

      glm::vec3 pos = placement->getTranslation(true);
      mouseControls->lastHorizCtrlRot = getCylStandingRot(pos, 0, mouseControls->yaw);
      mouseControls->lastCtrlRot = glm::mat4(getCylStandingRot(pos, mouseControls->pitch, mouseControls->yaw));
      glm::mat4 rot = mouseControls->lastCtrlRot;

      rot[3][0] = placement->mat[3][0];
      rot[3][1] = placement->mat[3][1];
      rot[3][2] = placement->mat[3][2];
      placement->mat = rot;
    }
    for (auto id: (registries[4].ids)) { // Free Controls position updated at graphics framerate
      FreeControls *freeControls;
      state->getFreeControls(id, &freeControls);
      MouseControls *mouseControls;
      state->getMouseControls(freeControls->mouseCtrlId, &mouseControls);
      if (length(freeControls->control) > 0.0f) {
        Placement *placement;
        state->getPlacement(id, &placement);

        glm::vec3 movement = (FreeControls::speedFactor * powf(10.f, freeControls->x10) * dt) * glm::normalize(
            mouseControls->lastCtrlRot * freeControls->control);

        placement->mat[3][0] += movement.x;
        placement->mat[3][1] += movement.y;
        placement->mat[3][2] += movement.z;
      }
    }
  }

  void ControlSystem::onBeforePhysicsStep() {
    for (auto id : (registries[1].ids)) { // Pyramid Controls updated at physics framerate
      PyramidControls* pyramidControls;
      state->getPyramidControls(id, &pyramidControls);
      Placement* placement;
      state->getPlacement(id, &placement);
      MouseControls *mouseControls;
      state->getMouseControls(pyramidControls->mouseCtrlId, &mouseControls);

      // provide the up vector
      pyramidControls->up = glm::mat3(placement->absMat) * glm::vec3(0.f, 0.f, 1.f);

      // zero control forces
      pyramidControls->force = glm::vec3(0, 0, 0);

      // keep track of how much the engine is being "used" (just for looks right now, but
      // maybe later use to calculate fuel consumption)
      uint8_t level = 0;

      if (length(pyramidControls->accel) > 0.0f) {
        // if thrusting upwards at all (boost or not) add to level
        if (pyramidControls->accel.y > 0) { ++level; }
        // handle turbo
        float turbo = 1.f;
        if (pyramidControls->turbo) {
          // if boosting anywhere except down, add to level
          if (pyramidControls->accel.y >= 0) { ++level; }
          turbo = 2.f;
          pyramidControls->turbo = false;
        }
        // Rotate the movement axis to the correct orientation
        pyramidControls->force = glm::mat3 {
            PyramidControls::sideAccelFactor, 0, 0,
            0, PyramidControls::sideAccelFactor, 0,
            0, 0, PyramidControls::upAccelFactor
        } * glm::normalize(mouseControls->lastHorizCtrlRot * pyramidControls->accel) * turbo;
        // zero inputs, but not for networked inputs (this is an attempt to smooth out networked movement)
        if (currentCtrlKeys && id == currentCtrlKeys->getId()) {
          pyramidControls->accel = glm::vec3(0, 0, 0);
        }
      }

      // assign the engine utilization level
      pyramidControls->engineActivationLevel = level;

      // deal with ball spawning
      if (pyramidControls->drop || pyramidControls->shoot) {
        if (settings::network::role != settings::network::CLIENT) {
          Placement *source;
          state->getPlacement(id, &source);
          Physics *sourcePhysics;
          state->getPhysics(id, &sourcePhysics);
          glm::mat4 sourceMat = glm::translate(source->absMat, {0.f, 0.f, 3.f});
          btVector3 sourceVel = sourcePhysics->rigidBody->getLinearVelocity();
          Physics *ballPhysics = nullptr;

          pyramidControls->projectiles.push(0);
          uint32_t count = pyramidControls->shoot ? 1 : 4;  // 0 for shoot will crash
          for (uint32_t i = 0; i < count; ++i) {

            // SPAWN A BALL
//            ecs->openEntityRequest();
//            ecs->requestPlacement(sourceMat);
//            ecs->requestMesh("sphere", "");
//            std::shared_ptr<void> radius = std::make_shared<float>(1.f);
//            ecs->requestPhysics(5.f, radius, Physics::SPHERE);
//            ecs->requestNetworking();
//            ecs->requestSceneNode(0);
//            pyramidControls->projectiles.top() = ecs->closeEntityRequest();
//            state->getPhysics(pyramidControls->projectiles.top(), &ballPhysics);
//            ballPhysics->rigidBody->setLinearVelocity(sourceVel);

            // SPAWN A BUGGY
            entityId camId = DuneBuggy::create(*state, sourceMat);
            SceneNode *sceneNode;
            state->getSceneNode(camId, &sceneNode);
            entityId gimbalId = sceneNode->parentId;
            state->getSceneNode(gimbalId, &sceneNode);
            pyramidControls->projectiles.top() = sceneNode->parentId;
            state->getPhysics(pyramidControls->projectiles.top(), &ballPhysics);
            ballPhysics->rigidBody->setLinearVelocity(sourceVel);
            DuneBuggy::broadcastLatest(*state, *ecs);

          }
          if (pyramidControls->shoot) {
            if (pyramidControls->projectiles.top()) { // This just won't work on an unfulfilled request (which is another reason to get rid of them)
              glm::mat3 tiltRot = glm::rotate(.35f, glm::vec3(1.0f, 0.0f, 0.0f));
              glm::mat3 rot = getCylStandingRot(source->getTranslation(true), mouseControls->pitch, mouseControls->yaw);
              glm::vec3 shootDir = rot * tiltRot * glm::vec3(0, 0, -1);
//              btVector3 shot = btVector3(shootDir.x, shootDir.y, shootDir.z) * 1000.f; // FOR BALL
              btVector3 shot = btVector3(shootDir.x, shootDir.y, shootDir.z) * 4000.f; // FOR BUGGY
              ballPhysics->rigidBody->applyCentralImpulse(shot);
            }
          }
        }
        pyramidControls->shoot = false;
        pyramidControls->drop = false;
      }
      if (pyramidControls->pop) {
        if ( ! pyramidControls->projectiles.empty()) {
          ecs->requestEntityDeletion(pyramidControls->projectiles.top());
          pyramidControls->projectiles.pop();
        }
        pyramidControls->pop = false;
      }
    }
    for (auto id : (registries[2].ids)) { // Track/buggy Controls updated at physics framerate
      TrackControls* trackControls;
      state->getTrackControls(id, &trackControls);

      if (length(trackControls->control) > 0.f) {
        // Calculate torque to apply
        trackControls->torque += TrackControls::torqueFactor * trackControls->control;
        // zero inputs, but not for networked inputs (this is an attempt to smooth out networked movement)
        if (currentCtrlKeys && id == currentCtrlKeys->getId()) {
          trackControls->control = glm::vec2(0, 0);
        }
      }
    }
    for (auto id : (registries[3].ids)) { // Player/Walking Controls updated at physics framerate
      WalkControls* walkControls;
      state->getWalkControls(id, &walkControls);
      Placement* placement;
      state->getPlacement(id, &placement);
      MouseControls *mouseControls;
      state->getMouseControls(walkControls->mouseCtrlId, &mouseControls);

      // provide the up vector
      walkControls->up = glm::mat3(placement->absMat) * glm::vec3(0.f, 0.f, 1.f);

      // zero control forces
      walkControls->force = glm::vec3(0, 0, 0);

      if (length(walkControls->accel) > 0.0f) {
        // Rotate the movement axis to the correct orientation
        float speed = walkControls->isRunning ? WalkControls::runFactor : WalkControls::walkFactor;
        walkControls->force = glm::mat3 {
            speed, 0, 0,
            0, speed, 0,
            0, 0, speed
        } * glm::normalize(mouseControls->lastHorizCtrlRot * walkControls->accel);
        // zero inputs, but not for networked inputs (this is an attempt to smooth out networked movement)
        if (currentCtrlKeys && id == currentCtrlKeys->getId()) {
          walkControls->accel = glm::vec3(0, 0, 0);
        }
      }
    }
  }

  bool ControlSystem::onDiscoverMouseControls(const entityId &id) {
    MouseControls *mouseControls;
    state->getMouseControls(id, &mouseControls);
    if (mouseControls->independent) {
      Placement *placement;
      state->getPlacement(id, &placement);
      placement->forceLocalRotationAndScale = true;
    }
    return true;
  }

  void ControlSystem::setEcsInterface(void *ecs) {
    this->ecs = *(std::shared_ptr<EntityComponentSystemInterface>*) ecs;
  }

  /*
   * This section deals with the current active control interface and defines actions for key
   * events for each type of control interface managed in ControlSystem.
   */

  EntityAssociatedERM::EntityAssociatedERM(State *state, const entityId id)
      : EventResponseMap(), state(state), id(id) { }

  entityId EntityAssociatedERM::getId() { return id; }

  class ActiveMouseControl : public EntityAssociatedERM {
      MouseControls *getComponent() {
        MouseControls *mouseControls = nullptr;
        state->getMouseControls(id, &mouseControls);
        assert(mouseControls);
        return mouseControls;
      }
      Placement *getPlacement() {
        Placement *placement = nullptr;
        state->getPlacement(id, &placement);
        assert(placement);
        return placement;
      }
      void mouseMove(void* sdlEvent) {
        auto *event = (SDL_Event*)sdlEvent;
        MouseControls *mouseControls = getComponent();
        Placement *placement = getPlacement();

        // Update pitch and yaw values
        float dx = (float)event->motion.xrel * (mouseControls->invertedX ? 1.f : -1.f) * settings::controls::mouseSpeed;
        float dy = (float)event->motion.yrel * (mouseControls->invertedY ? 1.f : -1.f) * settings::controls::mouseSpeed;
        mouseControls->yaw += dx;
        mouseControls->pitch += dy;
        // limited pitch value
        if (mouseControls->pitch > halfPi) {
          mouseControls->pitch = halfPi;
        } else if (mouseControls->pitch < -halfPi) {
          mouseControls->pitch = -halfPi;
        }
        // wrap-around yaw value
        if (mouseControls->yaw > twoPi) {
          mouseControls->yaw -= twoPi;
        } else if (mouseControls->yaw < -twoPi) {
          mouseControls->yaw += twoPi;
        }
      }
    public:
      ActiveMouseControl(State *state, const entityId id) : EntityAssociatedERM(state, id) {
        setAction("mouse_moved", RTU_MTHD_DLGT(&ActiveMouseControl::mouseMove, this));
      }
  };
  void ControlSystem::switchToMouseCtrl(void *id) {
    currentCtrlMouse = std::make_unique<ActiveMouseControl>(state, *(entityId*)id);
  }

  class ActiveWalkControl : public EntityAssociatedERM {
      WalkControls *getComponent() {
        WalkControls *walkControls = nullptr;
        state->getWalkControls(id, &walkControls);
        assert(walkControls);
        return walkControls;
      }
      void forwardOrBackward(float amount) { getComponent()->accel.z -= amount; }
      void rightOrLeft(float amount) { getComponent()->accel.x += amount; }
      void upOrDown(float amount) { getComponent()->accel.y += amount; }
      void run() { getComponent()->isRunning = true; }
      void jump() { getComponent()->jumpRequested = true; }

      // These are used as actions for "key held" topic subscriptions
      void key_forward() { forwardOrBackward(1.f); }
      void key_backward() { forwardOrBackward(-1.f); }
      void key_right() { rightOrLeft(1.f); }
      void key_left() { rightOrLeft(-1.f); }
      void key_run() { run(); }
      void key_jump() { jump(); }
    public:
      ActiveWalkControl(State *state, const entityId id) : EntityAssociatedERM(state, id) {
        setAction("key_held_w", RTU_MTHD_DLGT(&ActiveWalkControl::key_forward, this));
        setAction("key_held_s", RTU_MTHD_DLGT(&ActiveWalkControl::key_backward, this));
        setAction("key_held_d", RTU_MTHD_DLGT(&ActiveWalkControl::key_right, this));
        setAction("key_held_a", RTU_MTHD_DLGT(&ActiveWalkControl::key_left, this));
        setAction("key_held_lshift", RTU_MTHD_DLGT(&ActiveWalkControl::key_run, this));
        setAction("key_held_space", RTU_MTHD_DLGT(&ActiveWalkControl::key_jump, this));
      }
  };
  void ControlSystem::switchToWalkCtrl(void *id) {
    currentCtrlKeys = std::make_unique<ActiveWalkControl>(state, *(entityId*)id);
  }

  class ActivePyramidControl : public EntityAssociatedERM {
      PyramidControls *getComponent() {
        PyramidControls *pyramidControls = nullptr;
        state->getPyramidControls(id, &pyramidControls);
        assert(pyramidControls);
        return pyramidControls;
      }
      void forwardOrBackward(float amount) { getComponent()->accel.z -= amount; }
      void rightOrLeft(float amount) { getComponent()->accel.x += amount; }
      void upOrDown(float amount) { getComponent()->accel.y += amount; }

      // These are used as actions for "key held" topic subscriptions
      void key_forward() { forwardOrBackward(1.f); }
      void key_backward() { forwardOrBackward(-1.f); }
      void key_right() { rightOrLeft(1.f); }
      void key_left() { rightOrLeft(-1.f); }
      void key_up() { upOrDown(1.f); }
      void key_down() { upOrDown(-1.f); }
      void turbo() { getComponent()->turbo = true; }
      void shoot() { getComponent()->shoot = true; }
      void drop() { getComponent()->drop = true; }
      void pop() { getComponent()->pop = true; }
    public:
      ActivePyramidControl(State *state, const entityId id) : EntityAssociatedERM(state, id) {
        setAction("key_held_w", RTU_MTHD_DLGT(&ActivePyramidControl::key_forward, this));
        setAction("key_held_s", RTU_MTHD_DLGT(&ActivePyramidControl::key_backward, this));
        setAction("key_held_d", RTU_MTHD_DLGT(&ActivePyramidControl::key_right, this));
        setAction("key_held_a", RTU_MTHD_DLGT(&ActivePyramidControl::key_left, this));
        setAction("key_held_lshift", RTU_MTHD_DLGT(&ActivePyramidControl::key_down, this));
        setAction("key_held_space", RTU_MTHD_DLGT(&ActivePyramidControl::key_up, this));
        setAction("key_held_f", RTU_MTHD_DLGT(&ActivePyramidControl::turbo, this));
        setAction("mouse_down_left", RTU_MTHD_DLGT(&ActivePyramidControl::shoot, this));
//        setAction("mouse_down_right", RTU_MTHD_DLGT(&ActivePyramidControl::drop, this));
        setAction("mouse_down_right", RTU_MTHD_DLGT(&ActivePyramidControl::pop, this));
      }
  };
  void ControlSystem::switchToPyramidCtrl(void *id) {
    currentCtrlKeys = std::make_unique<ActivePyramidControl>(state, *(entityId*)id);
  }

  class ActiveTrackControl : public EntityAssociatedERM {
      TrackControls *getComponent() {
        TrackControls *trackControls = nullptr;
        state->getTrackControls(id, &trackControls);
        return trackControls; // may return nullptr
      }
      Physics *getPhysics() {
        Physics *physics = nullptr;
        state->getPhysics(id, &physics);
        assert(physics);
        return physics;
      }
      Placement *getPlacement() {
        Placement *placement = nullptr;
        state->getPlacement(id, &placement);
        assert(placement);
        return placement;
      }
      void forwardOrBackward(float amount) {
        getComponent()->control += glm::vec2(amount, amount);
      }
      void rightOrLeft(float amount) {
        getComponent()->control += glm::vec2(amount * 2.f, -amount * 2.f);
      }
      void brakeRightOrLeft(float amount) {
        getComponent()->brakes += glm::vec2(std::max(0.f, -amount * 5.f), std::max(0.f, amount * 5.f));
      }

      // These are used as actions for "key held" topic subscriptions
      void key_forward() { forwardOrBackward(1.f); }
      void key_backward() { forwardOrBackward(-1.f); }
      void key_right() { rightOrLeft(1.f); }
      void key_left() { rightOrLeft(-1.f); }
      void key_brakeRight() { brakeRightOrLeft(1.f); }
      void key_brakeLeft() { brakeRightOrLeft(-1.f); }
      void key_flip() { getComponent()->flipRequested = true; }
    public:
      ActiveTrackControl(State *state, const entityId id) : EntityAssociatedERM(state, id) {
        setAction("key_held_w", RTU_MTHD_DLGT(&ActiveTrackControl::key_forward, this));
        setAction("key_held_s", RTU_MTHD_DLGT(&ActiveTrackControl::key_backward, this));
        setAction("key_held_d", RTU_MTHD_DLGT(&ActiveTrackControl::key_right, this));
        setAction("key_held_a", RTU_MTHD_DLGT(&ActiveTrackControl::key_left, this));
        setAction("key_held_e", RTU_MTHD_DLGT(&ActiveTrackControl::key_brakeRight, this));
        setAction("key_held_q", RTU_MTHD_DLGT(&ActiveTrackControl::key_brakeLeft, this));
        setAction("key_down_f", RTU_MTHD_DLGT(&ActiveTrackControl::key_flip, this));
        getComponent()->hasDriver = true;
      }
      ~ActiveTrackControl() override {
        TrackControls *controls = getComponent();
        if (controls) {
          getComponent()->hasDriver = false;  // FIXME: This doesn't sync over the network
        }
      }
  };
  void ControlSystem::switchToTrackCtrl(void *id) {
    currentCtrlKeys = std::make_unique<ActiveTrackControl>(state, *(entityId*)id);
  }

  class ActiveFreeControl : public EntityAssociatedERM {
      FreeControls *getComponent() {
        FreeControls *freeControls = nullptr;
        state->getFreeControls(id, &freeControls);
        assert(freeControls);
        return freeControls;
      }
      void clearInputs() {
        getComponent()->control = glm::vec3(0, 0, 0);
        getComponent()->x10 = 0;
      }
      void forwardOrBackward(float amount) { getComponent()->control += glm::vec3(0.f, 0.f, amount); }
      void rightOrLeft(float amount) { getComponent()->control += glm::vec3(amount, 0.f, 0.f); }
      void upOrDown(float amount) { getComponent()->control += glm::vec3(0.f, amount, 0.f); }
      void faster() { getComponent()->x10++; }

      // These are used as actions for "key held" topic subscriptions
      void key_forward() { forwardOrBackward(-1.f); }
      void key_backward() { forwardOrBackward(1.f); }
      void key_right() { rightOrLeft(1.f); }
      void key_left() { rightOrLeft(-1.f); }
      void key_up() { upOrDown(1.f); }
      void key_down() { upOrDown(-1.f); }
      void key_faster() { faster(); }
    public:
      ActiveFreeControl(State *state, const entityId id) : EntityAssociatedERM(state, id) {
        setAction("new_input_state_poll", RTU_MTHD_DLGT(&ActiveFreeControl::clearInputs, this));
        setAction("key_held_w", RTU_MTHD_DLGT(&ActiveFreeControl::key_forward, this));
        setAction("key_held_s", RTU_MTHD_DLGT(&ActiveFreeControl::key_backward, this));
        setAction("key_held_d", RTU_MTHD_DLGT(&ActiveFreeControl::key_right, this));
        setAction("key_held_a", RTU_MTHD_DLGT(&ActiveFreeControl::key_left, this));
        setAction("key_held_c", RTU_MTHD_DLGT(&ActiveFreeControl::key_down, this));
        setAction("key_held_space", RTU_MTHD_DLGT(&ActiveFreeControl::key_up, this));
        setAction("key_held_lshift", RTU_MTHD_DLGT(&ActiveFreeControl::key_faster, this));
        setAction("key_held_lalt", RTU_MTHD_DLGT(&ActiveFreeControl::key_faster, this));
        setAction("key_held_lctrl", RTU_MTHD_DLGT(&ActiveFreeControl::key_faster, this));
      }
  };
  void ControlSystem::switchToFreeCtrl(void *id) {
    currentCtrlKeys = std::make_unique<ActiveFreeControl>(state, *(entityId*)id);
  }
}