
#include "cylinderMath.h"
#include <SDL.h>
#include <cstdio>

namespace at3 {

  constexpr float pi = (float)M_PI;
  constexpr float halfPi = pi * 0.5f;
  constexpr float twoPi = pi * 2.f;
  constexpr float rpm = twoPi / 60.f; // multiply by this to go from revolutions/minute to radians/sec.
  constexpr float angVel = (1.f / 16.f) * rpm;
  constexpr float engAcc = 0.5f; // engine acceleration in meters/second

  constexpr glm::vec3 getNativeUp() {
    return glm::vec3(0, 0, 1);
  }
  constexpr glm::vec3 getNativeAt() {
    return glm::vec3(0, -1, 0);
  }
  constexpr glm::vec3 getNativePitchAxis() {
    return glm::vec3(-1, 0, 0);
  }

  glm::vec3 getCylGrav(const glm::vec3 & pos) {
    glm::vec3 gravity = glm::vec3(pos.x, pos.y, 0.f) * angVel; // magnitude is angular velocity (rad/sec) times radius
    gravity -= engAcc * getNativeUp(); // acceleration due to engines generates some "gravity" along axis of cylinder
    return gravity;
  }
  glm::vec3 getCylGravDir(const glm::vec3 & pos) {
    return glm::normalize(getCylGrav(pos));
  }
  float getCylGravMag(const glm::vec3 & pos) {
    return glm::length(getCylGrav(pos));
  }
  glm::mat3 getCylGroundPlaneRot(const glm::vec3 &pos) {
    glm::vec3 gravDir = getCylGravDir(pos);
    float pitchAngle = acosf(-gravDir.z);
    float yawAngle = atan2f(pos.x, -pos.y);
    glm::mat3 pitchRot = glm::mat3(glm::rotate(pitchAngle, getNativePitchAxis()));
    glm::mat3 yawRot = glm::mat3(glm::rotate(yawAngle, getNativeUp()));
    return yawRot * pitchRot;
  }
  glm::mat3 getNativeViewRot(const float &pitch, const float &yaw) {
    glm::mat3 nativeStart = glm::mat3(glm::lookAt(glm::vec3(), getNativeAt(), getNativeUp()));
    glm::mat3 nativePitch = glm::mat3(glm::rotate(pitch, getNativePitchAxis()));
    glm::mat3 nativeYaw = glm::mat3(glm::rotate(yaw, getNativeUp()));
    return nativeYaw * nativePitch * nativeStart;
  }
  glm::mat3 getCylStandingRot(const glm::vec3 &pos, const float &pitch, const float &yaw) {
    return getCylGroundPlaneRot(pos) * getNativeViewRot(pitch, yaw);
  }
}
