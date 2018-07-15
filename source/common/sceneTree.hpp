#pragma once

#include <SDL.h>
#include <memory>
#include <unordered_map>
#include <vector>

#include "settings.hpp"
#include "objCamera.hpp"
#include "obj.hpp"
#include "transformStack.hpp"

namespace at3 {

  template <typename EcsInterface> class ObjCamera;
  template <typename EcsInterface> class Obj;

  /**
   * This class implements a scene graph.
   */
  template <typename EcsInterface>
  class SceneTree {
    private:
      // TODO: Use the ECS ID instead of the raw pointer as a key here? Will this make removal more convenient?
      std::unordered_map<const Obj<EcsInterface> *, std::shared_ptr<Obj<EcsInterface>>> objects;

    public:

      ~SceneTree();

      /**
       * Adds a top level object to the scene graph
       */
      void addObject(std::shared_ptr<Obj<EcsInterface>> object);

      /**
       * Removes a top level object from the scene graph. This should
       * automatically deallocate the shared_ptrs that it keeps for its
       * children, and so recursively deal with all children as well (I think)
       *
       * \param address The memory address of the object.
       */
      void removeObject(const Obj<EcsInterface> *address);

      /**
       * Clears the scene graph
       */
      void clear();

      /**
       * For any objects with transform data, updateAbsoluteTransformCaches traverses the tree,
       * storing each object's absolute world transform where the rendering process can find it.
       */
      void updateAbsoluteTransformCaches() const;
  };

  template <typename EcsInterface>
  SceneTree<EcsInterface>::~SceneTree() = default;

  template <typename EcsInterface>
  void SceneTree<EcsInterface>::addObject(std::shared_ptr<Obj<EcsInterface>> object) {
    this->objects.insert({object.get(), object});
  }

  template <typename EcsInterface>
  void SceneTree<EcsInterface>::removeObject(const Obj<EcsInterface> *address) {
    auto iterator = objects.find(address);
    if (iterator != objects.end()) {
      objects.erase(iterator);
    }
  }

  template <typename EcsInterface>
  void SceneTree<EcsInterface>::clear() {
    objects.clear();
  }

  template <typename EcsInterface>
  void SceneTree<EcsInterface>::updateAbsoluteTransformCaches() const
  {
    // Transform stack starts out empty
    TransformStack modelWorld;

    // draw each top level object
    for (auto object : this->objects) {
      object.second->traverseAndCache(modelWorld);
    }
  }
}
