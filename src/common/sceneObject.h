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

#ifndef LD2016_COMMON_SCENE_OBJECT_H_
#define LD2016_COMMON_SCENE_OBJECT_H_

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>

#include "transformRAII.h"
#include "sceneObject.h"

#define SCENE_ SceneObject<EcsInterface>::
#define SCENE_ECS SceneObject<EcsInterface>::ecs
#define SCENE_ID SceneObject<EcsInterface>::id

namespace at3 {
  class Transform;
  /**
   * This abstract class defines a typical object in a 3D graphics scene.
   *
   * The SceneObject class has both position and orientation properties useful
   * for most rigid body objects. This class also stores the position and
   * orientation from the previous frame so that they may be interpolated with
   * the appropriate alpha value when it comes time to draw the scene object.
   */
  template <typename EcsInterface>
  class SceneObject {
    private:
      std::unordered_map<
        const SceneObject<EcsInterface> *,
        std::shared_ptr<SceneObject<EcsInterface>>
        > m_children;
      SceneObject<EcsInterface>* m_parent = NULL;
      int m_inheritedDOF = ALL;

    public:

      static EcsInterface *ecs;
      typename EcsInterface::EcsId id;

      /**
       * Constructs a scene object with the given position and orientation.
       *
       * \param position The position of the scene object.
       * \param orientation The orientation of the scene object.
       */
      SceneObject();
      /**
       * Destroys this scene object. Since SceneObject is a polymorphic class,
       * this destructor is virtual.
       */
      virtual ~SceneObject();

      /**
       * When an object is a child of another object, it inherits certain transform information
       * from its parent, up to the full 6 Degrees Of Freedom, but via this enumerator it is
       * possible to only inherit a parent's translation, or everything BUT its translation. You can do this by
       * passing in one of these enumerators when you call addChild (it will be applied to the child)
       * NOTE: setting this is ineffective for top-level scene objects.
       */
      enum InheritedDOF {
        ALL, TRANSLATION_ONLY, WITHOUT_TRANSLATION
      };

      /**
       * Adds a scene object as a child of this scene object.
       *
       * \param child Child scene object to add.
       *
       * \sa removeChild()
       */
      void addChild(std::shared_ptr<SceneObject<EcsInterface>> child, int inheritedDOF = ALL);

      /**
       * \param The address of the child scene object to look for.
       * \return True if a scene object with the given address is a child of
       * this node, false otherwise.
       */
      bool hasChild(const SceneObject<EcsInterface> *address);

      /**
       * Removes the child of this scene object with the given address.
       *
       * \param address The memory address of the child to remove.
       *
       * \sa addChild()
       */
      void removeChild(const SceneObject<EcsInterface> *address);

      /**
       * Derived classes can implement this to receive an SDL event. All SDL
       * events are passed to each of the top-level scene objects in the scene.
       * Scene objects may choose to pass events to their children by calling
       * this method on their children. By returning true, implementing classes
       * can absorb the given event and mask it from handled elsewhere.
       *
       * \param event The current SDL event to be considered.
       * \return True if the given event was handled, and false otherwise.
       */
      virtual bool handleEvent(const SDL_Event &event);

      /**
       * Draws this scene object in the scene. The default behavior of this
       * method is to draw nothing.
       *
       * \param modelWorld The model-space to world-space transform for this
       * scene object's position and orientation.
       * \param worldView The world-space to view-space transform for the
       * position and orientation of the camera currently being used.
       * \param projection The view-space to projection-space transform for
       * the camera that is currently being used.
       * \param alpha The simulation keyframe weight for animating this object
       * between keyframes.
       * \param debug Flag indicating whether or not debug information is to be
       * drawn.
       *
       * Derived classes must implement this method in order for their scene
       * objects to be visible.
       */
      virtual void draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection,
                        bool debug);

      /**
       * Was private, but template friend classes do not work in a cross-platform way... :(
       * This method recursively draws this object and all of its children.
       */
      void m_draw(Transform &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection,
                  bool debug);

      void reverseTransformLookup(glm::mat4 &wv, int whichDOFs = ALL) const;

      /**
       * Get ID
       * @return id of this entity according to ECS
       */
      typename EcsInterface::EcsId getId() const;

      /**
       *
       */
      static void linkEcs(EcsInterface& ecs);
  };

  template <typename EcsInterface>
  EcsInterface *SceneObject<EcsInterface>::ecs = NULL;

  template <typename EcsInterface>
  SceneObject<EcsInterface>::SceneObject() {
    id = ecs->createEntity();
  }

  template <typename EcsInterface>
  SceneObject<EcsInterface>::~SceneObject() {
    ecs->destroyEntity(id);
  }

  template <typename EcsInterface>
  void SceneObject<EcsInterface>::addChild(std::shared_ptr<SceneObject> child, int inheritedDOF /* = ALL */) {
    m_children.insert({child.get(), child});
    child.get()->m_parent = this;
    child.get()->m_inheritedDOF = inheritedDOF;
  }

  template <typename EcsInterface>
  void SceneObject<EcsInterface>::removeChild(const SceneObject *address) {
    auto iterator = m_children.find(address);
    assert(iterator != m_children.end());
    // TODO: Set the m_parent member of this child to nullptr (SceneObjects
    // do not have m_parent members at the time of this writing).
    m_children.erase(iterator);
  }

  template <typename EcsInterface>
  bool SceneObject<EcsInterface>::hasChild(const SceneObject *address) {
    return m_children.find(address) != m_children.end();
  }

  template <typename EcsInterface>
  void SceneObject<EcsInterface>::m_draw(Transform &modelWorld, const glm::mat4 &worldView,
      const glm::mat4 &projection, bool debug)
  {
    TransformRAII mw(modelWorld);
    glm::mat4 myTransform;
    bool hasTransform = false;

    if (ecs->hasTransform(id)) {
      myTransform = ecs->getTransform(id);
      mw *= myTransform;
      hasTransform = true;
    }

    // Delegate the actual drawing to derived classes
    this->draw(mw.peek(), worldView, projection, debug);

    // Draw our children
    for (auto child : m_children) {
      switch (child.second->m_inheritedDOF) {
        case TRANSLATION_ONLY: {
          if (hasTransform) { break; }
          TransformRAII mw_transOnly(modelWorld);
          mw_transOnly *= glm::mat4({
                              1,                 0,                 0, 0,
                              0,                 1,                 0, 0,
                              0,                 0,                 1, 0,
              myTransform[3][0], myTransform[3][1], myTransform[3][2], 1
          });
          child.second->m_draw(mw_transOnly, worldView, projection, debug);
        } return;
        case WITHOUT_TRANSLATION: {
          if (hasTransform) { break; }
          TransformRAII mw_noTrans(modelWorld);
          mw_noTrans *= glm::mat4({
              myTransform[0][0], myTransform[0][1], myTransform[0][2], 0,
              myTransform[1][0], myTransform[1][1], myTransform[1][2], 0,
              myTransform[2][0], myTransform[2][1], myTransform[2][2], 0,
                              0,                 0,                 0, 1
          });
          child.second->m_draw(mw_noTrans, worldView, projection, debug);
        } return;
        default: break;
      }
      child.second->m_draw(mw, worldView, projection, debug);
    }
  }

  template <typename EcsInterface>
  void SceneObject<EcsInterface>::reverseTransformLookup(glm::mat4 &wv, int whichDOFs /* = ALL */) const {

    if (ecs->hasTransform(id)) {
      glm::mat4 fullTransform = ecs->getTransform(id);
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

  template <typename EcsInterface>
  bool SceneObject<EcsInterface>::handleEvent(const SDL_Event &event) {
    return false;
  }

  template <typename EcsInterface>
  void SceneObject<EcsInterface>::draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection,
                    bool debug) { }

  template <typename EcsInterface>
  typename EcsInterface::EcsId SceneObject<EcsInterface>::getId() const {
    return id;
  }

  template <typename EcsInterface>
  void SceneObject<EcsInterface>::linkEcs(EcsInterface& ecs) {
    SCENE_ECS = &ecs;
  }
}

#endif

