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
  void SceneObject::addChild(std::shared_ptr<SceneObject> child, int inheritedDOF /* = ALL */) {
    m_children.insert({child.get(), child});
    child.get()->m_parent = this;
    child.get()->m_inheritedDOF = inheritedDOF;
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
    glm::mat4 myTransform;

    ezecs::compMask compsPresent = state->getComponents(id);
    assert(compsPresent);

    if (compsPresent & ezecs::PLACEMENT) {
      ezecs::Placement *placement;
      state->get_Placement(id, &placement);
      myTransform = placement->mat;
      mw *= myTransform;
    }

    // Delegate the actual drawing to derived classes
    this->draw(mw.peek(), worldView, projection, debug);

    // Draw our children
    for (auto child : m_children) {
      switch (child.second->m_inheritedDOF) {
        case TRANSLATION_ONLY: {
          TransformRAII mw_transOnly(modelWorld);
          mw_transOnly *= glm::mat4({
                              1,                 0,                 0, 0,
                              0,                 1,                 0, 0,
                              0,                 0,                 1, 0,
              myTransform[3][0], myTransform[3][1], myTransform[3][2], 1
          });
          child.second->m_draw(mw_transOnly, worldView, projection, debug);
        } break;
        case WITHOUT_TRANSLATION: {
          TransformRAII mw_noTrans(modelWorld);
          mw_noTrans *= glm::mat4({
              myTransform[0][0], myTransform[0][1], myTransform[0][2], 0,
              myTransform[1][0], myTransform[1][1], myTransform[1][2], 0,
              myTransform[2][0], myTransform[2][1], myTransform[2][2], 0,
                              0,                 0,                 0, 1
          });
          child.second->m_draw(mw_noTrans, worldView, projection, debug);
        } break;
        default: {
          child.second->m_draw(mw, worldView, projection, debug);
        } break;
      }
    }
  }

  void SceneObject::reverseTransformLookup(glm::mat4 &wv, int whichDOFs /* = ALL */) const {

    ezecs::compMask compsPresent = state->getComponents(id);
    assert(compsPresent);

    if (compsPresent & ezecs::PLACEMENT) {
      ezecs::Placement *placement;
      state->get_Placement(id, &placement);
      glm::mat4 fullTransform = placement->mat;
      switch (whichDOFs) {
        case TRANSLATION_ONLY: {
          wv *= glm::inverse(glm::mat4({
                                1,                   0,                   0, 0,
                                0,                   1,                   0, 0,
                                0,                   0,                   1, 0,
              fullTransform[3][0], fullTransform[3][1], fullTransform[3][2], 1
          }));
        } break;
        case WITHOUT_TRANSLATION: {
          wv *= glm::inverse(glm::mat4({
              fullTransform[0][0], fullTransform[0][1], fullTransform[0][2], 0,
              fullTransform[1][0], fullTransform[1][1], fullTransform[1][2], 0,
              fullTransform[2][0], fullTransform[2][1], fullTransform[2][2], 0,
                                0,                   0,                   0, 1
          }));
        } break;
        default: {
          wv *= glm::inverse(fullTransform);
        } break;
      }
    }

    if (m_parent != NULL) {
      m_parent->reverseTransformLookup(wv, m_inheritedDOF);
    }

  }

  bool SceneObject::handleEvent(const SDL_Event &event) {
    return false;
  }

  void SceneObject::draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection,
                    bool debug) { }

  ezecs::entityId SceneObject::getId() const {
    return id;
  }
}
