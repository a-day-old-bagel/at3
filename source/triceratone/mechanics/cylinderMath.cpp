
#include "cylinderMath.h"
#include <SDL.h>  // just for Pi
#include <cstdio> // just for printf
#include <algorithm>  // just for clamp/min/max

namespace at3 {

  // general math stuff

  constexpr float pi = (float)M_PI;
  constexpr float halfPi = pi * 0.5f;
  constexpr float twoPi = pi * 2.f;
  constexpr float rpm = twoPi / 60.f; // multiply by this to go from revolutions/minute to radians/sec.

  template <typename T> int sign(T val) {
    return (T(0) < val) - (val < T(0));
  }

  glm::vec3 bulletToGlm(const btVector3 &vec) {
    return {vec.x(), vec.y(), vec.z()};
  }
  btVector3 glmToBullet(const glm::vec3& vec) {
    return btVector3(vec.x, vec.y, vec.z);
  }


  // cylinder specific stuff

  constexpr float angVel = 1.0553f * rpm; // generates about 9.77 m/s acceleration from spinning alone at 800m radius
  constexpr float engAcc = 1.f; // an added orthogonal 1 m/s creates slightly over 1g at 800m
  constexpr bool engIsOn = true; // toggle engine acceleration on/off for testing. Maybe make runtime later.

  constexpr glm::vec3 getNativeUp() {
    return glm::vec3(0, 0, 1);
  }
  constexpr glm::vec3 getNativeAt() {
    return glm::vec3(0, -1, 0);
  }
  constexpr glm::vec3 getNativePitchAxis() {
    return glm::vec3(-1, 0, 0);
  }

  /**
   * Get the tangential linear velocity of a point rotating in sync with the cylinder.
   * Rotation of the cylinder is counter-clockwise if looking in negative z direction (check this) ?
   * @param pos The point in standard R3
   * @return A linear velocity in standard R3
   */
  glm::vec3 getCylLinVel(const glm::vec3 & pos) {
    return glm::vec3(-pos.y, pos.x, 0.f) * angVel; // TODO: check that this magnitude is equivalent to w * r
  }
  /**
   * Get a value between -1 and 1 that represents how much of the faked cylinder gravity to apply to a moving mass.
   * @param pos The center of the mass in standard R3
   * @param nativeVel The linear velocty of the mass in standard R3
   * @return A value between -1 and 1 by which to multiply faked gravity effects
   */
  float getCylGravApplicationFactor(const glm::vec3 & pos, const glm::vec3 & nativeVel) {
    glm::vec3 cylVel = getCylLinVel(pos);
    float cylSpeed = glm::length(cylVel);
    if (!cylSpeed) { return 0.f; } // c'ya NaN
    float dot = glm::dot(nativeVel + cylVel, cylVel);
    glm::vec3 projVelOntoCyl = cylVel * (dot / powf(cylSpeed, 2));
    float gravAdjustment = (sign(dot) * (glm::length(projVelOntoCyl) / cylSpeed)) * 2.f - 1.f;

    // FIXME: To clamp or not to clamp? That is the math question.
    float result = std::clamp(gravAdjustment, -1.f, 1.f);
//    float result = gravAdjustment;

    return result;
  }
  /**
   * Get the faked cylinder gravity to apply to a mass.
   * This attempts to take into account cylinder-tangential velocity of the mass, and provide a complete picure of the
   * way an object would move inside the cylinder from the perspective of the cylinder's own reference frame.
   * I may have gotten this wrong.
   * The non-tangential component of a mass's motion might need to be rotated every step to match the tangent.
   * Right now I'm just applying a negative cylinder-gravity and hoping that it's equivalent (I haven't done the math.)
   * @param pos
   * @param nativeVel
   * @return
   */
  glm::vec3 getCylGrav(const glm::vec3 & pos, const glm::vec3 & nativeVel) {
    // magnitude is angular velocity (rad/sec) times radius squared
    glm::vec3 fakeGrav = glm::vec3(pos.x, pos.y, 0.f) * powf(angVel, 2);
    fakeGrav *= getCylGravApplicationFactor(pos, nativeVel);
    if (engIsOn) {
      fakeGrav -= engAcc * getNativeUp();
    }
    return fakeGrav;
  }

  /**
   * Get the faked cylinder gravity to apply to a mass.
   * This neglects any velocity component of the mass in the cylinder-tangential direction, and thus is wrong.
   * But it's useful for some things, like finding an appropriate "up" for some objects
   * @param pos The center of the mass in standard R3
   * @return A likely-incorrect gravity vector
   */
  glm::vec3 getNaiveCylGrav(const glm::vec3 &pos) {
    // magnitude is angular velocity (rad/sec) times radius squared
    glm::vec3 fakeGrav = glm::vec3(pos.x, pos.y, 0.f) * powf(angVel, 2);
    if (engIsOn) {
      fakeGrav -= engAcc * getNativeUp(); // acceleration due to engines generates some "gravity" along axis of cylinder
    }
    return fakeGrav;
  }
  /**
   * Get only the directional part of the naive gravity vector returned by getNaiveCylGrav.
   * @param pos The center of the mass in standard R3
   * @return A likely-incorrect gravity direction vector
   */
  glm::vec3 getNaiveCylGravDir(const glm::vec3 &pos) {
    return glm::normalize(getNaiveCylGrav(pos));
  }

  /**
   * Get a rotation from a planar z=up ground orientation to an orientation whose normal is the naive cylinder gravity.
   * @param pos The center of the mass in standard R3
   * @return A rotation from flat z=up orientation to naive cylinder gravity orientation.
   */
  glm::mat3 getCylGroundPlaneRot(const glm::vec3 &pos) {
    glm::vec3 gravDir = getNaiveCylGravDir(pos);
    float pitchAngle = acosf(-gravDir.z);
    float yawAngle = atan2f(pos.x, -pos.y);
    if (yawAngle != yawAngle) { yawAngle = 0.f; } // c'ya NaN
    glm::mat3 pitchRot = glm::mat3(glm::rotate(pitchAngle, getNativePitchAxis()));
    glm::mat3 yawRot = glm::mat3(glm::rotate(yawAngle, getNativeUp()));
    return yawRot * pitchRot;
  }
  /**
   * Get a rotation from a foward-looking view orientation to an orientation described by pitch and yaw.
   * @param pitch
   * @param yaw
   * @return A rotation described by pitch and yaw
   */
  glm::mat3 getNativeViewRot(const float &pitch, const float &yaw) {
    glm::mat3 nativeStart = glm::mat3(glm::lookAt(glm::vec3(), getNativeAt(), getNativeUp()));
    glm::mat3 nativePitch = glm::mat3(glm::rotate(pitch, getNativePitchAxis()));
    glm::mat3 nativeYaw = glm::mat3(glm::rotate(yaw, getNativeUp()));
    return nativeYaw * nativePitch * nativeStart;
  }
  /**
   * Get a full view rotation from a planar z=up ground plane while facing foward to a cylinder-gravity-up orientation
   * including pitch and yaw rotations.
   * @param pos The camera position in standard R3
   * @param pitch
   * @param yaw
   * @return A rotation described by pitch and yaw and exhibiting cylinder-gravity-up orientation
   */
  glm::mat3 getCylStandingRot(const glm::vec3 &pos, const float &pitch, const float &yaw) {
    return getCylGroundPlaneRot(pos) * getNativeViewRot(pitch, yaw);
  }
}
