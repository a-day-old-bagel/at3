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
#include "ecsSystem_controls.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"
#include "glm/gtx/matrix_decompose.hpp"

#define MOUSE_SENSITIVITY 0.1f
#define PYR_SIDE_ACCEL 2500.f
#define PYR_UP_ACCEL 4000.f
#define TRACK_TORQUE 100.f
#define CHARA_WALK 100.f
#define CHARA_RUN 250.f

using namespace rtu::topics;

namespace at3 {

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
        switchToPyrmCtrlSub("switch_to_pyramid_controls", RTU_MTHD_DLGT(&ControlSystem::switchToPyrmCtrl, this)),
        switchToTrakCtrlSub("switch_to_track_controls", RTU_MTHD_DLGT(&ControlSystem::switchToTrakCtrl, this)),
        switchToMousCtrlSub("switch_to_mouse_controls", RTU_MTHD_DLGT(&ControlSystem::switchToMousCtrl, this))
  {
    name = "Control System";
  }

  bool ControlSystem::onInit() {
    return true;
  }

  void ControlSystem::onTick(float dt) {

    // renew look infos
    lookInfoIsFresh = false;

    for (auto id : (registries[0].ids)) { // Pyramid
      PyramidControls* pyramidControls;
      state->get_PyramidControls(id, &pyramidControls);
      Placement* placement;
      state->get_Placement(id, &placement);

      // provide the up vector
      pyramidControls->up = glm::quat_cast(placement->mat) * glm::vec3(0.f, 0.f, 1.f);

      if (length(pyramidControls->accel) > 0.0f) {
        updateLookInfos();
        // Rotate the movement axis to the correct orientation
        pyramidControls->force = glm::mat3 {
            PYR_SIDE_ACCEL * dt, 0, 0,
            0, PYR_SIDE_ACCEL * dt, 0,
            0, 0, PYR_UP_ACCEL * dt
        } * glm::normalize(lastKnownHorizCtrlRot * pyramidControls->accel);

        pyramidControls->accel = glm::vec3();
      }
    }
    for (auto id : (registries[1].ids)) { // Track/buggy
      TrackControls* trackControls;
      state->get_TrackControls(id, &trackControls);

      if (length(trackControls->control) > 0.f) {
        // Calculate torque to apply
        trackControls->torque += TRACK_TORQUE * trackControls->control;

        trackControls->control = glm::vec2();
      }
    }
    for (auto id : (registries[2].ids)) { // Player/Walking
      PlayerControls* playerControls;
      state->get_PlayerControls(id, &playerControls);
      Placement* placement;
      state->get_Placement(id, &placement);

      // provide the up vector
      playerControls->up = glm::quat_cast(placement->mat) * glm::vec3(0.f, 0.f, 1.f);

      if (length(playerControls->horizControl) > 0.0f) {
        updateLookInfos();
        // Rotate the movement axis to the correct orientation
        playerControls->horizForces = (playerControls->isRunning ? CHARA_RUN : CHARA_WALK) * dt *
            glm::normalize(lastKnownHorizCtrlRot * glm::vec3(playerControls->horizControl, 0.f));

        playerControls->horizControl = glm::vec2();
      }
    }
  }

  //TODO
  //TODO ALL EVENTS MOVED INTO GAME OBJECTS, USE TOPICS
  //TODO
  //TODO
  //TODO ALL EVENTS MOVED INTO GAME OBJECTS, USE TOPICS
  //TODO
  //TODO
  //TODO ALL EVENTS MOVED INTO GAME OBJECTS, USE TOPICS
  //TODO

  bool ControlSystem::handleEvent(SDL_Event &event) {
    switch (event.type) {
      case SDL_MOUSEBUTTONDOWN:
        // Make sure the window has grabbed the mouse cursor
        if (SDL_GetGrabbedWindow() == nullptr) {
          // Somehow obtain a pointer for the window
          SDL_Window *window = SDL_GetWindowFromID(event.button.windowID);
          if (window == nullptr)
            break;
          // Grab the mouse cursor
          SDL_SetWindowGrab(window, SDL_TRUE);
          #if (!SDL_VERSION_ATLEAST(2, 0, 4))
            grabbedWindow = window;
          #endif
          SDL_SetRelativeMouseMode(SDL_TRUE);
        }
        break;
      case SDL_MOUSEMOTION: {
        if (SDL_GetRelativeMouseMode()) {
          rtu::topics::publish("mouse_moved", (void *) &event);
        }
      }
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.scancode) {
          case SDL_SCANCODE_ESCAPE: {
            SDL_Window *window = SDL_GetGrabbedWindow();
            if (window == nullptr)
              break;
            // Release the mouse cursor from the window on escape pressed
            SDL_SetWindowGrab(window, SDL_FALSE);
            #if !SDL_VERSION_ATLEAST(2, 0, 4)
            grabbedWindow = nullptr;
            #endif
            SDL_SetRelativeMouseMode(SDL_FALSE);
          }
            break;
          default:
            return false; // could not handle it here
        }
        break;
      default:
        return false; // could not handle it here
    }
    return true; // handled it here
  }

  void ControlSystem::updateLookInfos() {
    if (! lookInfoIsFresh) {
      glm::quat quat = glm::quat_cast(lastKnownWorldView);
      lastKnownLookVec = quat * glm::vec3(0.f, 1.0, 0.f);
      glm::vec3 horizLookDir(lastKnownLookVec.x, 0.f, lastKnownLookVec.z);
      horizLookDir = glm::normalize(horizLookDir);
      float rotZ = acosf(glm::dot({0.f, 0.f, -1.f}, horizLookDir)) * (lastKnownLookVec.x < 0.f ? -1.f : 1.f);
      lastKnownHorizCtrlRot = glm::mat3(glm::rotate(rotZ, glm::vec3(0.f, 0.f, 1.f)));
      lookInfoIsFresh = true;
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

  class ActiveMousControl : public SwitchableControls {
      MouseControls *getComponent() {
        MouseControls *mouseControls = nullptr;
        assert(SUCCESS == state->get_MouseControls(id, &mouseControls));
        return mouseControls;
      }
      Placement *getPlacement() {
        Placement *placement = nullptr;
        assert(SUCCESS == state->get_Placement(id, &placement));
        return placement;
      }
      void mouseMove(void* sdlEvent) {
        SDL_Event *event = (SDL_Event*)sdlEvent;
        MouseControls *mouseControls = getComponent();
        Placement *placement = getPlacement();

        // Rotate object orientation according to the mouse motion
        float inversionValue = (mouseControls->invertedX ? 1.f : -1.f);
        placement->mat = glm::rotate(
            glm::mat4(),
            (float)event->motion.xrel * MOUSE_SENSITIVITY * ((float) M_PI / 180.0f) * inversionValue,
            { 0.0f, 0.0f, 1.0f }
        ) * placement->mat;
        inversionValue = (mouseControls->invertedY ? 1.f : -1.f);
        placement->mat = placement->mat * glm::rotate(
            glm::mat4(),
            (float)event->motion.yrel * MOUSE_SENSITIVITY * ((float) M_PI / 180.0f) * inversionValue,
            { 1.0f, 0.0f, 0.0f }
        );
      }
    public:
      ActiveMousControl(State *state, const entityId id) : SwitchableControls(state, id) {
        setAction("mouse_moved", RTU_MTHD_DLGT(&ActiveMousControl::mouseMove, this));
      }
  };
  void ControlSystem::switchToMousCtrl(void *id) {
    currentCtrlMous = std::make_unique<ActiveMousControl>(state, *(entityId*)id);
  }

  class ActiveWalkControl : public SwitchableControls {
      PlayerControls *getComponent() {
        PlayerControls *playerControls = nullptr;
        assert(SUCCESS == state->get_PlayerControls(id, &playerControls));
        return playerControls;
      }
      void forwardOrBackward(float amount) { getComponent()->horizControl += glm::vec2(0.0f, amount); }
      void rightOrLeft(float amount) { getComponent()->horizControl += glm::vec2(amount, 0.f); }
      void run() { getComponent()->isRunning = true; }
      void jump() { getComponent()->jumpRequested = true; }

      // These are used as actions for "key held" topic subscriptions
      void key_forward(void *nothing) { forwardOrBackward(1.f); }
      void key_backward(void *nothing) { forwardOrBackward(-1.f); }
      void key_right(void *nothing) { rightOrLeft(1.f); }
      void key_left(void *nothing) { rightOrLeft(-1.f); }
      void key_run(void *nothing) { run(); }
      void key_jump(void *nothing) { jump(); }
    public:
      ActiveWalkControl(State *state, const entityId id) : SwitchableControls(state, id) {
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

  class ActivePyrmControl : public SwitchableControls {
      PyramidControls *getComponent() {
        PyramidControls *pyramidControls = nullptr;
        assert(SUCCESS == state->get_PyramidControls(id, &pyramidControls));
        return pyramidControls;
      }
      void forwardOrBackward(float amount) { getComponent()->accel += glm::vec3(0.0f, amount, 0.f); }
      void rightOrLeft(float amount) { getComponent()->accel += glm::vec3(amount, 0.f, 0.f); }
      void upOrDown(float amount) { getComponent()->accel += glm::vec3(0.f, 0.f, amount); }

      // These are used as actions for "key held" topic subscriptions
      void key_forward(void *nothing) { forwardOrBackward(1.f); }
      void key_backward(void *nothing) { forwardOrBackward(-1.f); }
      void key_right(void *nothing) { rightOrLeft(1.f); }
      void key_left(void *nothing) { rightOrLeft(-1.f); }
      void key_up(void *nothing) { upOrDown(1.f); }
      void key_down(void *nothing) { upOrDown(-1.f); }
    public:
      ActivePyrmControl(State *state, const entityId id) : SwitchableControls(state, id) {
        setAction("key_held_w", RTU_MTHD_DLGT(&ActivePyrmControl::key_forward, this));
        setAction("key_held_s", RTU_MTHD_DLGT(&ActivePyrmControl::key_backward, this));
        setAction("key_held_d", RTU_MTHD_DLGT(&ActivePyrmControl::key_right, this));
        setAction("key_held_a", RTU_MTHD_DLGT(&ActivePyrmControl::key_left, this));
        setAction("key_held_lshift", RTU_MTHD_DLGT(&ActivePyrmControl::key_down, this));
        setAction("key_held_space", RTU_MTHD_DLGT(&ActivePyrmControl::key_up, this));
      }
  };
  void ControlSystem::switchToPyrmCtrl(void *id) {
    currentCtrlKeys = std::make_unique<ActivePyrmControl>(state, *(entityId*)id);
  }

  class ActiveTrakControl : public SwitchableControls {
      TrackControls *getComponent() {
        TrackControls *trackControls = nullptr;
        assert(SUCCESS == state->get_TrackControls(id, &trackControls));
        return trackControls;
      }
      Physics *getPhysics() {
        Physics *physics = nullptr;
        assert(SUCCESS == state->get_Physics(id, &physics));
        return physics;
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
        getPhysics()->rigidBody->applyImpulse({0.f, 0.f, 100.f}, {1.f, 0.f, 0.f});
      }

      // These are used as actions for "key held" topic subscriptions
      void key_forward(void *nothing) { forwardOrBackward(1.f); }
      void key_backward(void *nothing) { forwardOrBackward(-1.f); }
      void key_right(void *nothing) { rightOrLeft(1.f); }
      void key_left(void *nothing) { rightOrLeft(-1.f); }
      void key_brakeRight(void *nothing) { brakeRightOrLeft(1.f); }
      void key_brakeLeft(void *nothing) { brakeRightOrLeft(-1.f); }
      void key_flip(void *nothing) { flip(); }
    public:
      ActiveTrakControl(State *state, const entityId id) : SwitchableControls(state, id) {
        setAction("key_held_w", RTU_MTHD_DLGT(&ActiveTrakControl::key_forward, this));
        setAction("key_held_s", RTU_MTHD_DLGT(&ActiveTrakControl::key_backward, this));
        setAction("key_held_d", RTU_MTHD_DLGT(&ActiveTrakControl::key_right, this));
        setAction("key_held_a", RTU_MTHD_DLGT(&ActiveTrakControl::key_left, this));
        setAction("key_held_e", RTU_MTHD_DLGT(&ActiveTrakControl::key_brakeRight, this));
        setAction("key_held_q", RTU_MTHD_DLGT(&ActiveTrakControl::key_brakeLeft, this));
        setAction("key_down_f", RTU_MTHD_DLGT(&ActiveTrakControl::key_flip, this));
      }
  };
  void ControlSystem::switchToTrakCtrl(void *id) {
    currentCtrlKeys = std::make_unique<ActiveTrakControl>(state, *(entityId*)id);
  }
}
