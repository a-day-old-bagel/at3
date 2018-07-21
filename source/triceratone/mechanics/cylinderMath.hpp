
#pragma once

#include "math.hpp"

namespace at3 {

  /**
   * Get the faked cylinder gravity to apply to a mass.
   * This attempts to take into account cylinder-tangential velocity of the mass, and provide a complete picure of the
   * way an object would move inside the cylinder from the perspective of the cylinder's own reference frame.
   * I may have gotten this wrong.
   * The non-tangential component of a mass's motion might need to be rotated every step to match the tangent.
   * Right now I'm just applying a negative cylinder-gravity and hoping that it's equivalent (I haven't done the math.)
   *
   * EDIT: TODO: FIXME: This is not completely right - a projectile shot "upwards" from the ground surface should curve,
   * but right now I'm only applying the curve based on tangential velocity, while neglecting change to radial velocity.
   * It might not be correct to just add another curve factor multiplied by 1 - dot(tangential, vel), but that might
   * be closer and good enough.
   *
   * @param pos
   * @param nativeVel
   * @return
   */
  glm::vec3 getCylGrav(const glm::vec3 & pos, const glm::vec3 & nativeVel);

  /**
   * Get the faked cylinder gravity to apply to a mass.
   * This neglects any velocity component of the mass in the cylinder-tangential direction, and thus is wrong.
   * But it's useful for some things, like finding an appropriate "up" for some objects
   * @param pos The center of the mass in standard R3
   * @return A likely-incorrect gravity vector
   */
  glm::vec3 getNaiveCylGrav(const glm::vec3 &pos);

  /**
   * Get only the directional part of the naive gravity vector returned by getNaiveCylGrav.
   * @param pos The center of the mass in standard R3
   * @return A likely-incorrect gravity direction vector
   */
  glm::vec3 getNaiveCylGravDir(const glm::vec3 &pos);

  /**
   * Get a full view rotation from a planar z=up ground plane while facing foward to a cylinder-gravity-up orientation
   * including pitch and yaw rotations.
   * @param pos The camera position in standard R3
   * @param pitch
   * @param yaw
   * @return A rotation described by pitch and yaw and exhibiting cylinder-gravity-up orientation
   */
  glm::mat3 getCylStandingRot(const glm::vec3 &pos, const float &pitch, const float &yaw);
}
