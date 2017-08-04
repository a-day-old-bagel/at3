#pragma once

#include <SDL.h>
#include <memory>
#include <unordered_map>
#include <vector>

#include "settings.h"
#include "graphicsBackend.h"
#include "camera.h"
#include "sceneObject.h"
#include "transformStack.h"

namespace at3 {

  template <typename EcsInterface> class Camera;
  template <typename EcsInterface> class SceneObject;

  /**
   * This class implements a scene graph.
   */
  template <typename EcsInterface>
  class Scene {
    private:
      std::unordered_map<const SceneObject<EcsInterface> *, std::shared_ptr<SceneObject<EcsInterface>>> m_objects;

    public:

      ~Scene();

      /**
       * Adds a top level object to the scene graph
       */
      void addObject(std::shared_ptr<SceneObject<EcsInterface>> object);

      /**
       * Removes a top level object from the scene graph. This should
       * automatically deallocate the shared_ptrs that it keeps for its
       * children, and so recursively deal with all children as well (I think)
       *
       * \param address The memory address of the object.
       */
      void removeObject(const SceneObject<EcsInterface> *address);

      /**
       * Clears the scene graph
       */
      void clear();

      /**
       * Draws the scene graph recursively
       *
       * \param camera A Camera object
       * \param aspect The aspect ratio
       * \param alpha Currently unused, since Bullet does its own interpolation
       * \param debug Indicates that object should draw with debug information
       */
      void draw(Camera<EcsInterface> &camera, bool debug = false) const;
  };

  template <typename EcsInterface>
  Scene<EcsInterface>::~Scene() {
    printf("Scene is destructing.\n");
  }

  template <typename EcsInterface>
  void Scene<EcsInterface>::addObject(std::shared_ptr<SceneObject<EcsInterface>> object) {
    this->m_objects.insert({object.get(), object});
  }

  template <typename EcsInterface>
  void Scene<EcsInterface>::removeObject(const SceneObject<EcsInterface> *address) {
    auto iterator = m_objects.find(address);
    if (iterator != m_objects.end()) {
      m_objects.erase(iterator);
    }
  }

  template <typename EcsInterface>
  void Scene<EcsInterface>::clear() {
    m_objects.clear();
  }

  template <typename EcsInterface>
  void Scene<EcsInterface>::draw(Camera<EcsInterface> &camera, bool debug) const
  {
    // Transform stack starts out empty
    TransformStack modelWorld;

    // Get camera transforms
    auto worldView = camera.worldView();
    auto projection = camera.projection(graphicsBackend::getAspect());

    // draw each top level object
    for (auto object : this->m_objects) {
      object.second->m_draw(
          modelWorld,
          worldView,
          projection,
          debug);
    }
  }
}
