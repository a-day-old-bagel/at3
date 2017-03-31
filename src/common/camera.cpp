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

#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

namespace ld2016 {
  Camera::Camera(const glm::vec3 &position, const glm::quat &orientation, ezecs::State& state) : SceneObject(state)
  {
    ezecs::CompOpReturn status;
    status = this->state->add_Position(id, position);
    assert(status == ezecs::SUCCESS);
    status = this->state->add_Orientation(id, orientation);
    assert(status == ezecs::SUCCESS);
  }

  Camera::~Camera() {
  }

  glm::mat4 Camera::worldView(float alpha) const {
    glm::mat4 wv;
    reverseTransformLookup(wv, alpha);
//    wv = glm::inverse(wv);

    /*// Translate the world so the camera is positioned in the center, and then
    // rotate the world to be aligned with the camera's orientation
    ecs::Orientation* orientation;
    ecs::CompOpReturn status = state->getOrientation(id, &orientation);
    assert(status == ecs::SUCCESS);
    wv *= glm::mat4_cast(glm::inverse(orientation->getQuat(alpha)));
    ecs::Position* position;
    status = state->getPosition(id, &position);
    assert(status == ecs::SUCCESS);
    wv *= glm::translate(glm::mat4(), -1.f * position->getVec(alpha));*/

    return wv;
  }
}
