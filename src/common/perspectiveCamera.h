#pragma once

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

namespace at3 {
  /**
   * A camera object with perspective projection.
   */
  template <typename EcsInterface>
  class PerspectiveCamera : public Camera<EcsInterface> {
    public:
      /**
       * Constructs a perspective camera object with the given parameters.
       *
       * \param fovy The y-axis viewing angle of the camera, in radians (not
       * degrees).
       * \param near The distance of the near plane of the camera. For the most
       * depth precision, this value should be as large as is deemed acceptable
       * for the given application.
       * \param far The distance of the farplane of the camera. In practice,
       * this value can be as large as necessary with no repercussion.
       * \param position The camera position.
       * \param orientation The camera orientation. For best results, this
       * orientation should be pointed directly at the follow point with the
       * z-axis pointing up.
       */
      PerspectiveCamera(float fovy, float near, float far, glm::mat4 &transform);
      virtual ~PerspectiveCamera();

      void setFar(float far);

      /**
       * Implements a perspective projection for this camera object.
       *
       * \param aspect The viewport width to height ratio. This aspect ratio is
       * preserved in the projection returned.
       * \param alpha Weight between the previous and the current tick, for
       * animating the camera projection between ticks.
       * \return A perspective transform matrix for this camera's perspective
       * projection settings.
       */
      glm::mat4 projection(float aspect, float alpha = 1.0f) const;

      float focalLength() const;

      float fovy() const;
  };

  template <typename EcsInterface>
  PerspectiveCamera<EcsInterface>::PerspectiveCamera(float fovy, float near, float far, glm::mat4 &transform)
      : Camera<EcsInterface>(transform) {
    SCENE_ECS->addPerspective(SCENE_ID, fovy, near, far);
  }

  template <typename EcsInterface>
  PerspectiveCamera<EcsInterface>::~PerspectiveCamera() {
  }

  template <typename EcsInterface>
  glm::mat4 PerspectiveCamera<EcsInterface>::projection(
      float aspect, float alpha) const
  {
    // Linearly interpolate changes in FOV between ticks
    float fovy = (1.0f - alpha) * SCENE_ECS->getFovyPrev(SCENE_ID) + alpha * SCENE_ECS->getFovy(SCENE_ID);
    return glm::perspective(fovy, aspect, SCENE_ECS->getNear(SCENE_ID), SCENE_ECS->getFar(SCENE_ID));
  }

  template <typename EcsInterface>
  float PerspectiveCamera<EcsInterface>::focalLength() const {
    return 1.0f / tan(0.5f * SCENE_ECS->getFovy(SCENE_ID));
  }

  template <typename EcsInterface>
  void PerspectiveCamera<EcsInterface>::setFar(float far) {
    SCENE_ECS->setFar(SCENE_ID, far);
  }

  template <typename EcsInterface>
  float PerspectiveCamera<EcsInterface>::fovy() const {
    return SCENE_ECS->getFovy(SCENE_ID);
  }
}
