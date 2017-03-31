/*
 * Copyright (c) 2016 Jonathan Glines, Galen Cochrane
 * Jonathan Glines <jonathan@glines.net>
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

#ifndef LD2016_COMMON_WASD_CAMERA_H_
#define LD2016_COMMON_WASD_CAMERA_H_

#include "perspectiveCamera.h"
#include "ecsState.generated.hpp"

namespace ld2016 {
  /**
   * Class representing perspective camera that is controlled by the WASD or
   * arrow keys and the mouse. This is essentially the same as the traditional
   * first-person-shooter with noclip mode turned on.
   */
  class WasdCamera : public PerspectiveCamera {

    public:
      /**
       * Constructs a WasdCamera object. The WASD camera does not expose any
       * settings other than those provided by the PerspectiveCamera class from
       * which it inherits.
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
      WasdCamera(ezecs::State& state, float fovy, float near, float far,
                 const glm::vec3 &position, const glm::quat &orientation);
  };
}

#endif
