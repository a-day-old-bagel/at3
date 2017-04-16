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
#include "ecsState.generated.hpp"

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
  class SceneObject {
    friend class Scene;
    private:
      std::unordered_map<
        const SceneObject *,
        std::shared_ptr<SceneObject>
        > m_children;
      SceneObject* m_parent = NULL;

      /**
       *
       * This method recursively draws this object and all of its children.
       */
      void m_draw(Transform &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection,
                  bool debug);

    protected:
      ezecs::State* state;
      ezecs::entityId id;

    public:
      /**
       * Constructs a scene object with the given position and orientation.
       *
       * \param position The position of the scene object.
       * \param orientation The orientation of the scene object.
       */
      SceneObject(ezecs::State& state);
      /**
       * Destroys this scene object. Since SceneObject is a polymorphic class,
       * this destructor is virtual.
       */
      virtual ~SceneObject();

      /**
       * Adds a scene object as a child of this scene object.
       *
       * \param child Child scene object to add.
       *
       * \sa removeChild()
       */
      void addChild(std::shared_ptr<SceneObject> child);

      /**
       * \param The address of the child scene object to look for.
       * \return True if a scene object with the given address is a child of
       * this node, false otherwise.
       */
      bool hasChild(const SceneObject *address);

      /**
       * Removes the child of this scene object with the given address.
       *
       * \param address The memory address of the child to remove.
       *
       * \sa addChild()
       */
      void removeChild(const SceneObject *address);

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

      void reverseTransformLookup(glm::mat4 &wv) const;

      /**
       * Get ID
       * @return id of this entity according to ECS
       */
      ezecs::entityId getId() const;
  };
}

#endif

