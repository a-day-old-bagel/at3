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

      ~SceneTree();

      void addObject(const typename EcsInterface::EcsId & objectId);
      void addChildObject(
          const typename EcsInterface::EcsId &parentId, const typename EcsInterface::EcsId &childId);

      /*
       * TODO: object removal doesn't currently work.
       * We need to handle how child objects are treated when a parent is removed.
       * The SceneNode component must be updated for children, or they must be deleted.
       * But a SceneObject doesn't see its own SceneTree, so a top-level object can't give its children to the tree.
       * ... Ugh.
       * Maybe include a static delegate in SceneObject that an object can call to remove a child.
       * And/or put some of the handling code in the EcsInterface, since the objects already have a pointer to that.
       * But of course, you can't see the SceneTree from there.
       *
       * The behavior of removal is also a concern: Do children then inherit from a grandparent, or do they get deleted?
       * Probably inheriting from further up the tree is actually easier (unexpectedly), since it won't require the
       * removal of any entities or components, rather just a change in the parent value of the SceneNode component and
       * a call to the SceneTree (again possibly via static delegate) to register that change.
       * On the other hand, if you just removed and replaced the SceneNode component, you might be able to accomplish
       * that parent change using just the SceneSystem discovery and forget callbacks if you write them well.
       * This might be the best option: Add an "UpdateSceneNodeParent" function to the EcsInterface that replaces the
       * entire SceneNode component so that all SceneObjects can see the function and the SceneSystem feels the change.
       *
       * WAIT
       * Can't an object getting deleted just pass its list of children up to its parent and also change the component
       * value? Won't that cover all the changes that need to be made?
       * We'd need to add a parent pointer to each object, though. Maybe that's fine.
       * Wait nevermind, top-level objects would still have to be able to access the SceneTree.
       * UNLESS we just added a single "root" SceneObject to the scene tree instead of having a collection of top-level
       * SceneObjects like we do right now. That way even effectively top-level objects would still have a SceneObject
       * parent, but that parent would just never have a transform. Or it could even be used to translate everything
       * to camera coordinates so as to implement the "stationary camera" setup for better floating point precision in
       * huge scenes.
       * ......
       * .....
       * ....
       * ...
       * ..
       * .
       * Too much to think about.
       */
      void removeObject(const typename EcsInterface::EcsId &objectId);

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
  void SceneTree<EcsInterface>::addObject(const typename EcsInterface::EcsId & objectId) {
    this->objects.insert({objectId, std::make_shared<SceneObject<EcsInterface>>(objectId)});
  }

  template <typename EcsInterface>
  void SceneTree<EcsInterface>::addChildObject(
      const typename EcsInterface::EcsId &parentId, const typename EcsInterface::EcsId &childId) {
    std::shared_ptr<SceneObject<EcsInterface>> child = std::make_shared<SceneObject<EcsInterface>>(childId);
    if (objects.count(parentId)) {
      objects[parentId]->addChild(child);
    } else if (children.count(parentId)) {
      children[parentId]->addChild(child);
    } else {
      fprintf(stderr, "Attempted to add scene node to nonexistent parent node!\nAdding it to root instead.\n");
      addObject(childId);
      return; // Do not add to childrenById
    }
    children.insert({childId, child});
  }

  template <typename EcsInterface>
  void SceneTree<EcsInterface>::removeObject(const typename EcsInterface::EcsId &objectId) {
    fprintf(stderr, "Removal of scene nodes does not work yet, but you've done it anyway. Expect the unexpected.\n");
    auto idIterator = objects.find(objectId);
    if (idIterator != objects.end()) {
      objects.erase(idIterator);
    }
  }

  template <typename EcsInterface>
  void SceneTree<EcsInterface>::clear() {
    objects.clear();
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
