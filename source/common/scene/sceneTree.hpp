#pragma once

#include <SDL.h>
#include <memory>
#include <unordered_map>
#include <vector>

#include "settings.hpp"
#include "sceneObject.hpp"
#include "transformStack.hpp"

namespace at3 {

  template <typename EcsInterface> class SceneObject;

  template <typename EcsInterface>
  class SceneTree {
    private:
      std::unordered_map<typename EcsInterface::EcsId, std::shared_ptr<SceneObject<EcsInterface>>> objects;
      std::unordered_map<typename EcsInterface::EcsId, std::shared_ptr<SceneObject<EcsInterface>>> children;

    public:

      void addObject(const typename EcsInterface::EcsId & objectId, const typename EcsInterface::EcsId & parentId);
      void removeObject(const typename EcsInterface::EcsId & objectId, const typename EcsInterface::EcsId & parentId);

      void clear();

      /**
       * For any objects with transform data, updateAbsoluteTransformCaches traverses the tree,
       * storing each object's absolute world transform where the rendering process can find it.
       */
      void updateAbsoluteTransformCaches() const;
  };

  template<typename EcsInterface>
  void SceneTree<EcsInterface>::addObject(const typename EcsInterface::EcsId & objectId,
                                          const typename EcsInterface::EcsId & parentId) {
    if (parentId) {
      std::shared_ptr<SceneObject<EcsInterface>> child =
          std::make_shared<SceneObject<EcsInterface>>(objectId, parentId);
      if (objects.count(parentId)) {
        objects[parentId]->addChild(child);
      } else if (children.count(parentId)) {
        children[parentId]->addChild(child);
      } else {
        fprintf(stderr, "Attempted to add scene node to nonexistent parent node!\nAdding it to root instead.\n");
        addObject(objectId, 0);
        return; // Do not add to childrenById
      }
      children.insert({objectId, child});
    } else {
      objects.insert({objectId, std::make_shared<SceneObject<EcsInterface>>(objectId)});
    }
  }

  template<typename EcsInterface>
  void SceneTree<EcsInterface>::removeObject(const typename EcsInterface::EcsId & objectId,
                                             const typename EcsInterface::EcsId & parentId) {
    if (parentId) {
      if (objects.count(children[objectId]->getParentId())) {
        objects[children[objectId]->getParentId()]->removeChild(objectId);
      } else if (children.count(children[objectId]->getParentId())) {
        children[children[objectId]->getParentId()]->removeChild(objectId);
      }
      children[objectId]->removeChildrenFromEcs();
      children.erase(objectId);
    } else {
      objects[objectId]->removeChildrenFromEcs();
      objects.erase(objectId);
    }
  }

  template <typename EcsInterface>
  void SceneTree<EcsInterface>::clear() {
    objects.clear();
    children.clear();
  }

  template <typename EcsInterface>
  void SceneTree<EcsInterface>::updateAbsoluteTransformCaches() const {
    // Transform stack starts out empty
    TransformStack modelWorld;

    // draw each top level object
    for (auto object : this->objects) {
      object.second->traverseAndCache(modelWorld);
    }
  }
}
