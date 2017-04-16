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

#include "transformRAII.h"

#include "sceneObject.h"

namespace at3 {
  SceneObject::SceneObject(ezecs::State& state) : state(&state) {
    ezecs::CompOpReturn status;
    status = this->state->createEntity(&id);
    assert(status == ezecs::SUCCESS);
  }
  SceneObject::~SceneObject() {
    ezecs::CompOpReturn status;
    status = state->deleteEntity(id);
    assert(status == ezecs::SUCCESS);
  }
  void SceneObject::addChild(std::shared_ptr<SceneObject> child) {
    m_children.insert({child.get(), child});
    child.get()->m_parent = this;
  }
  void SceneObject::removeChild(const SceneObject *address) {
    auto iterator = m_children.find(address);
    assert(iterator != m_children.end());
    // TODO: Set the m_parent member of this child to nullptr (SceneObjects
    // do not have m_parent members at the time of this writing).
    m_children.erase(iterator);
  }
  bool SceneObject::hasChild(const SceneObject *address) {
    return m_children.find(address) != m_children.end();
  }

  void SceneObject::m_draw(Transform &modelWorld, const glm::mat4 &worldView,
      const glm::mat4 &projection, bool debug)
  {
    TransformRAII mw(modelWorld);

    ezecs::compMask compsPresent = state->getComponents(id);
    assert(compsPresent);

    if (compsPresent & ezecs::PLACEMENT) {
      ezecs::Placement *placement;
      state->get_Placement(id, &placement);
      mw *= placement->mat;
    }

    /*if (compsPresent & ezecs::POSITION) {
      ezecs::Position *position;
      state->get_Position(id, &position);
      // Translate the object into position
      mw *= glm::translate(glm::mat4(), position->getVec(alpha));
    }
    if (compsPresent & ezecs::ORIENTATION) {
      ezecs::Orientation *orientation;
      state->get_Orientation(id, &orientation);
      // Apply the object orientation as a rotation
      mw *= glm::mat4_cast(orientation->getQuat(alpha));
    }*/

    // Delegate the actual drawing to derived classes
    this->draw(mw.peek(), worldView, projection, debug);

    // Draw our children
    for (auto child : m_children) {
      child.second->m_draw(mw, worldView, projection, debug);
    }
  }

  void SceneObject::reverseTransformLookup(glm::mat4 &wv) const {

    ezecs::compMask compsPresent = state->getComponents(id);
    assert(compsPresent);

    if (compsPresent & ezecs::PLACEMENT) {
      ezecs::Placement *placement;
      state->get_Placement(id, &placement);
      wv *= glm::inverse(placement->mat);
    }

    /*if (compsPresent & ezecs::ORIENTATION) {
      ezecs::Orientation *orientation;
      state->get_Orientation(id, &orientation);
      wv *= glm::mat4_cast(glm::inverse(orientation->getQuat(alpha)));
    }
    if (compsPresent & ezecs::POSITION) {
      ezecs::Position *position;
      state->get_Position(id, &position);
      wv *= glm::translate(glm::mat4(), -1.f * position->getVec(alpha));
    }*/

    if (m_parent != NULL) {
      m_parent->reverseTransformLookup(wv);
    }

  }

  bool SceneObject::handleEvent(const SDL_Event &event) { return false; }
  void
  SceneObject::draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection,
                    bool debug) { }
  ezecs::entityId SceneObject::getId() const {
    return id;
  }
}
