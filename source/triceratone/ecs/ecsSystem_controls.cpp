
#include <global/settings.hpp>
#include "ecsSystem_controls.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"
#pragma ide diagnostic ignored "IncompatibleTypes"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"
#include "glm/gtx/matrix_decompose.hpp"

#include "cylinderMath.h"

using namespace rtu::topics;

namespace at3 {

  constexpr float halfPi = ((float)M_PI * 0.5f);
  constexpr float twoPi = ((float)M_PI * 2.f);

  #if !SDL_VERSION_ATLEAST(2, 0, 4)
  SDL_Window *grabbedWindow = nullptr;

  SDL_Window *SDL_GetGrabbedWindow() {
    return grabbedWindow;
  }
  #endif

  ControlSystem::ControlSystem(State *state)
      : System(state),
        updateWvMatSub("primary_cam_wv", RTU_MTHD_DLGT(&ControlSystem::setWorldView, this)),
        switchToWalkCtrlSub("switch_to_walking_controls", RTU_MTHD_DLGT(&ControlSystem::switchToWalkCtrl, this)),
        switchToPyramidCtrlSub("switch_to_pyramid_controls", RTU_MTHD_DLGT(&ControlSystem::switchToPyramidCtrl, this)),
        switchToTrackCtrlSub("switch_to_track_controls", RTU_MTHD_DLGT(&ControlSystem::switchToTrackCtrl, this)),
        switchToMouseCtrlSub("switch_to_mouse_controls", RTU_MTHD_DLGT(&ControlSystem::switchToMouseCtrl, this)),
        switchToFreeCtrlSub("switch_to_free_controls", RTU_MTHD_DLGT(&ControlSystem::switchToFreeCtrl, this))
  {
    name = "Control System";
  }
  ControlSystem::~ControlSystem() {
    currentCtrlKeys.reset();
    currentCtrlMouse.reset();
  }

  bool ControlSystem::onInit() {
    return true;
  }

  void ControlSystem::onTick(float dt) {

    if (currentCtrlMouse) {
      currentCtrlMouse->tick();
    }

    // renew look infos
    lookInfoIsFresh = false;

    for (auto id : (registries[0].ids)) { // Pyramid
      PyramidControls* pyramidControls;
      state->get_PyramidControls(id, &pyramidControls);
      Placement* placement;
      state->get_Placement(id, &placement);

      // provide the up vector
      pyramidControls->up = glm::mat3(placement->absMat) * glm::vec3(0.f, 0.f, 1.f);

      // zero control forces
      pyramidControls->force = glm::vec3(0, 0, 0);

      if (length(pyramidControls->accel) > 0.0f) {
        updateLookInfos();
        // handle turbo
        float turbo = 1.f;
        if (pyramidControls->turbo) {
          turbo = 2.f;
          pyramidControls->turbo = false;
        }
        // Rotate the movement axis to the correct orientation
        pyramidControls->force = glm::mat3 {
            PYR_SIDE_ACCEL, 0, 0,
            0, PYR_SIDE_ACCEL, 0,
            0, 0, PYR_UP_ACCEL
        } * glm::normalize(lastKnownCylCtrlRot * pyramidControls->accel) * turbo;
        pyramidControls->accel = glm::vec3(0, 0, 0);
      }
    }
    for (auto id : (registries[1].ids)) { // Track/buggy
      TrackControls* trackControls;
      state->get_TrackControls(id, &trackControls);

      if (length(trackControls->control) > 0.f) {
        // Calculate torque to apply
        trackControls->torque += TRACK_TORQUE * trackControls->control;

        trackControls->control = glm::vec2(0, 0);
      }
    }
    for (auto id : (registries[2].ids)) { // Player/Walking
      PlayerControls* playerControls;
      state->get_PlayerControls(id, &playerControls);
      Placement* placement;
      state->get_Placement(id, &placement);

      // provide the up vector
      playerControls->up = glm::quat_cast(placement->mat) * glm::vec3(0.f, 0.f, 1.f);

      // TODO: fixme
      // FIXME: rotation is a little off - try checking forward movement vector.

      if (length(playerControls->horizControl) > 0.0f) {
        updateLookInfos();
        // Rotate the movement axis to the correct orientation
        playerControls->forces = (playerControls->isRunning ? CHARA_RUN : CHARA_WALK) *
            glm::normalize(lastKnownCylCtrlRot * glm::vec3(playerControls->horizControl, 0.f));
        // Clear the input
        playerControls->horizControl = glm::vec2(0, 0);
      }
    }
    for (auto id: (registries[3].ids)) { // Free Control
      FreeControls *freeControls;
      state->get_FreeControls(id, &freeControls);

      if (length(freeControls->control) > 0.0f) {
        Placement *placement;
        state->get_Placement(id, &placement);

        // Rotate the movement axis to the correct orientation
        // TODO: Figure out math to avoid inverse; use lastKnownHorizCtrlRot, etc? Or better not?
        glm::vec3 movement = (float)(FREE_SPEED * pow(10.f, freeControls->x10) * dt) * glm::normalize(
            glm::inverse(glm::mat3(lastKnownWorldView)) * freeControls->control);

        placement->mat[3][0] += movement.x;
        placement->mat[3][1] += movement.y;
        placement->mat[3][2] += movement.z;

        freeControls->control = glm::vec3(0, 0, 0);
      }
      if (freeControls->x10) {
        freeControls->x10 = 0;
      }
    }
  }

  void ControlSystem::updateLookInfos() {
    if (! lookInfoIsFresh) {
      Placement *placement;
      state->get_Placement(currentCtrlMouse->getId(), &placement);
      MouseControls *mouseControls;
      state->get_MouseControls(currentCtrlMouse->getId(), &mouseControls);
      lastKnownCylCtrlRot = getCylStandingRot(placement->getTranslation(true), 0.f, mouseControls->yaw);
    }
  }

  // used as a subscription action
  void ControlSystem::setWorldView(void *p_wv) {
    lastKnownWorldView = *((glm::mat4*)p_wv);
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
        state->get_MouseControls(id, &mouseControls);
        assert(mouseControls);
        return mouseControls;
      }
      Placement *getPlacement() {
        Placement *placement = nullptr;
        state->get_Placement(id, &placement);
        assert(placement);
        return placement;
      }
      void mouseMove(void* sdlEvent) {
        auto *event = (SDL_Event*)sdlEvent;
        MouseControls *mouseControls = getComponent();
        Placement *placement = getPlacement();

        // Update values used in tick()
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
      void tick() override {

        MouseControls *mouseControls = getComponent();
        Placement *placement = getPlacement();

        glm::vec3 pos = placement->getTranslation(true);
        glm::mat3 rot = getCylStandingRot(pos, mouseControls->pitch, mouseControls->yaw);

        rot[3][0] = placement->mat[3][0];
        rot[3][1] = placement->mat[3][1];
        rot[3][2] = placement->mat[3][2];
        placement->mat = rot;

      }
  };
  void ControlSystem::switchToMouseCtrl(void *id) {
    currentCtrlMouse = std::make_unique<ActiveMouseControl>(state, *(entityId*)id);
  }

  class ActiveWalkControl : public EntityAssociatedERM {
      PlayerControls *getComponent() {
        PlayerControls *playerControls = nullptr;
        state->get_PlayerControls(id, &playerControls);
        assert(playerControls);
        return playerControls;
      }
      void forwardOrBackward(float amount) { getComponent()->horizControl += glm::vec2(0.0f, amount); }
      void rightOrLeft(float amount) { getComponent()->horizControl += glm::vec2(amount, 0.f); }
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
      void tick() override {

      }
  };
  void ControlSystem::switchToWalkCtrl(void *id) {
    currentCtrlKeys = std::make_unique<ActiveWalkControl>(state, *(entityId*)id);
  }

  class ActivePyramidControl : public EntityAssociatedERM {
      PyramidControls *getComponent() {
        PyramidControls *pyramidControls = nullptr;
        state->get_PyramidControls(id, &pyramidControls);
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
    public:
      ActivePyramidControl(State *state, const entityId id) : EntityAssociatedERM(state, id) {
        setAction("key_held_w", RTU_MTHD_DLGT(&ActivePyramidControl::key_forward, this));
        setAction("key_held_s", RTU_MTHD_DLGT(&ActivePyramidControl::key_backward, this));
        setAction("key_held_d", RTU_MTHD_DLGT(&ActivePyramidControl::key_right, this));
        setAction("key_held_a", RTU_MTHD_DLGT(&ActivePyramidControl::key_left, this));
        setAction("key_held_lshift", RTU_MTHD_DLGT(&ActivePyramidControl::key_down, this));
        setAction("key_held_space", RTU_MTHD_DLGT(&ActivePyramidControl::key_up, this));
        setAction("key_held_f", RTU_MTHD_DLGT(&ActivePyramidControl::turbo, this));
      }
      void tick() override {

      }
  };
  void ControlSystem::switchToPyramidCtrl(void *id) {
    currentCtrlKeys = std::make_unique<ActivePyramidControl>(state, *(entityId*)id);
  }

  class ActiveTrackControl : public EntityAssociatedERM {
      TrackControls *getComponent() {
        TrackControls *trackControls = nullptr;
        state->get_TrackControls(id, &trackControls);
        return trackControls; // may return nullptr
      }
      Physics *getPhysics() {
        Physics *physics = nullptr;
        state->get_Physics(id, &physics);
        assert(physics);
        return physics;
      }
      Placement *getPlacement() {
        Placement *placement = nullptr;
        state->get_Placement(id, &placement);
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
      void flip() {
        glm::vec3 cen = glm::vec3(0, 0, 1);
        glm::vec3 dir = -getCylGrav(getPlacement()->getTranslation(true)) * 25.f;
        getPhysics()->rigidBody->applyImpulse({dir.x, dir.y, dir.z}, {cen.x, cen.y, cen.z});
      }

      // These are used as actions for "key held" topic subscriptions
      void key_forward() { forwardOrBackward(1.f); }
      void key_backward() { forwardOrBackward(-1.f); }
      void key_right() { rightOrLeft(1.f); }
      void key_left() { rightOrLeft(-1.f); }
      void key_brakeRight() { brakeRightOrLeft(1.f); }
      void key_brakeLeft() { brakeRightOrLeft(-1.f); }
      void key_flip() { flip(); }
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
          getComponent()->hasDriver = false;
        }
      }
      void tick() override {

      }
  };
  void ControlSystem::switchToTrackCtrl(void *id) {
    currentCtrlKeys = std::make_unique<ActiveTrackControl>(state, *(entityId*)id);
  }

  class ActiveFreeControl : public EntityAssociatedERM {
      FreeControls *getComponent() {
        FreeControls *freeControls = nullptr;
        state->get_FreeControls(id, &freeControls);
        assert(freeControls);
        return freeControls;
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
      void tick() override {

      }
  };
  void ControlSystem::switchToFreeCtrl(void *id) {
    currentCtrlKeys = std::make_unique<ActiveFreeControl>(state, *(entityId*)id);
  }
}

#pragma clang diagnostic pop
