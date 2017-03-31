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

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "perspectiveCamera.h"

namespace ld2016 {
  PerspectiveCamera::PerspectiveCamera(ezecs::State& state, float fovy, float near, float far,
      const glm::vec3 &position, const glm::quat &orientation)
    : Camera(position, orientation, state)
  {
    ezecs::CompOpReturn status = this->state->add_Perspective(id, fovy, near, far);
    assert(status == ezecs::SUCCESS);
  }

  PerspectiveCamera::~PerspectiveCamera() {
  }

  glm::mat4 PerspectiveCamera::projection(
      float aspect, float alpha) const
  {
    ezecs::Perspective* perspective;
    ezecs::CompOpReturn status = state->get_Perspective(id, &perspective);
    assert(status == ezecs::SUCCESS);

    // Linearly interpolate changes in FOV between ticks
    float fovy = (1.0f - alpha) * perspective->prevFovy + alpha * perspective->fovy;
    return glm::perspective(fovy, aspect, perspective->near, perspective->far);
  }

  float PerspectiveCamera::focalLength() const {
    ezecs::Perspective* perspective;
    ezecs::CompOpReturn status = state->get_Perspective(id, &perspective);
    assert(status == ezecs::SUCCESS);
    return 1.0f / tan(0.5f * perspective->fovy);
  }
  void PerspectiveCamera::setFar(float far) {
    ezecs::Perspective* perspective;
    ezecs::CompOpReturn status = state->get_Perspective(id, &perspective);
    assert(status == ezecs::SUCCESS);
    perspective->far = far;
  }
  float PerspectiveCamera::fovy() const {
    ezecs::Perspective* perspective;
    ezecs::CompOpReturn status = state->get_Perspective(id, &perspective);
    assert(status == ezecs::SUCCESS);
    return perspective->fovy;
  }
}
