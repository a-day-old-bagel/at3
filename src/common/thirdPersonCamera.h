/*
 * Copyright (c) 2016 Jonathan Glines
 * Jonathan Glines <jonathan@glines.net>
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

#ifndef LD2016_COMMON_THIRDPERSON_CAMERA_H_
#define LD2016_COMMON_THIRDPERSON_CAMERA_H_

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "perspectiveCamera.h"

namespace at3 {
  /**
   * A camera object that orbits and looks at some target entity (third-person style)
   */
  template <typename EcsInterface>
  class ThirdPersonCamera {
    public:
      std::shared_ptr<PerspectiveCamera<EcsInterface>> mpCamera;
      std::shared_ptr<SceneObject<EcsInterface>> mpCamGimbal;
      ThirdPersonCamera(float fovy, float near, float far, float up, float back, float tilt);
      virtual ~ThirdPersonCamera();
  };

  template <typename EcsInterface>
  ThirdPersonCamera<EcsInterface>::ThirdPersonCamera(float fovy, float near, float far,
                                                     float up, float back, float tilt) {
    glm::mat4 ident;
    glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, -back, 0.f}), tilt, glm::vec3(1.0f, 0.0f, 0.0f));
    mpCamera = std::shared_ptr<PerspectiveCamera<EcsInterface>>
        (new PerspectiveCamera<EcsInterface>(fovy, near, far, camMat));
    mpCamGimbal = std::shared_ptr<SceneObject<EcsInterface>>
        (new SceneObject<EcsInterface>());
    // add mouse controls to the camera gimbal so that it rotates with the mouse
    typename EcsInterface::EcsId gimbalId = mpCamGimbal->getId();
    SCENE_ECS->addTransform(gimbalId, {
        1,  0,  0,  0,
        0,  1,  0,  0,
        0,  0,  1,  0,
        0,  0, up,  1  // at (0, 0, up)
    });
    SCENE_ECS->addMouseControl(gimbalId);
    mpCamGimbal->addChild(mpCamera);
  }

  template <typename EcsInterface>
  ThirdPersonCamera<EcsInterface>::~ThirdPersonCamera() {
    //TODO: deal with gimbal?
  }
}

#endif
