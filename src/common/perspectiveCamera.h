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

#ifndef LD2016_COMMON_PERSPECTIVE_CAMERA_H_
#define LD2016_COMMON_PERSPECTIVE_CAMERA_H_

#include "camera.h"
#include "ecsState.generated.hpp"

namespace at3 {
  /**
   * A camera object with perspective projection.
   */
  class PerspectiveCamera : public Camera {
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
      PerspectiveCamera(ezecs::State &state, float fovy, float near, float far, glm::mat4 &transform);
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
}

#endif
