#pragma once

#include <SDL.h>
#include <memory>
#include <unordered_map>
#include <vector>

#include "transformRAII.hpp"
#include "math.hpp"
#include "delegate.hpp"

#define SCENE_ Obj<EcsInterface>::
#define SCENE_ECS SceneObject<EcsInterface>::ecs
#define SCENE_ID Obj<EcsInterface>::id

namespace at3 {
  class Transform;

  /**
   * This abstract class defines a typical object in a 3D graphics scene.
   * The SceneTree class defines a relational structure in which many of these objects inherit state from one another.
   */
  template<typename EcsInterface>
  class SceneObject {
    private:

      std::unordered_map<typename EcsInterface::EcsId, std::shared_ptr<SceneObject<EcsInterface>>> children;

      static std::shared_ptr<EcsInterface> ecs;
      typename EcsInterface::EcsId id, parentId;

    public:

      explicit SceneObject(const typename EcsInterface::EcsId & id, const typename EcsInterface::EcsId & parentId = 0);
      ~SceneObject();

      void addChild(std::shared_ptr<SceneObject<EcsInterface>> child);

      void removeChild(const typename EcsInterface::EcsId & id);

      void removeChildrenFromEcs();

      virtual void traverseAndCache(Transform &modelWorld);

      typename EcsInterface::EcsId getId() const;

      typename EcsInterface::EcsId getParentId() const;

      const std::unordered_map<typename EcsInterface::EcsId, std::shared_ptr<SceneObject<EcsInterface>>> &
            getChildren() const;

      static void linkEcs(std::shared_ptr<EcsInterface> &ecs);
      static void resetEcs();
  };

  template<typename EcsInterface>
  std::shared_ptr<EcsInterface> SceneObject<EcsInterface>::ecs;

  template<typename EcsInterface>
  SceneObject<EcsInterface>::SceneObject(const typename EcsInterface::EcsId & id,
      const typename EcsInterface::EcsId & parentId) : id(id), parentId(parentId) {
//    printf("Created %u\n", id);
  }

  template<typename EcsInterface>
  SceneObject<EcsInterface>::~SceneObject() {
//    printf("Destroyed %u\n", id);
  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::addChild(std::shared_ptr<SceneObject> child) {
    children.insert({child->id, child});
//    printf("Added %u to %u\n", child->id, id);
  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::removeChild(const typename EcsInterface::EcsId &id) {
    if (children.count(id)) {
      children.erase(id);
//      printf("Removed %u from %u\n", id, this->id);
    } else {
      fprintf(stderr, "Attempted to remove non-existent scene child node!\n");
    }
  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::removeChildrenFromEcs() {
    std::vector<typename EcsInterface::EcsId> idsToRemove;
    for (const auto & child : children) {
      // calling notifyOfSceneTreeRemoval directly here can invalidate the iterator by removing objects during the loop.
      // So add the ids to a temporary list and operate on that instead.
      idsToRemove.emplace_back(child.second->id);
    }
    for (const auto & id : idsToRemove) {
      ecs->notifyOfSceneTreeRemoval(id);
    }
  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::traverseAndCache(Transform &modelWorld) {
    TransformRAII mw(modelWorld);
    glm::mat4 myTransform(1.f);
    if (ecs->hasTransform(id)) {
      if (ecs->hasCustomModelTransform(id)) {
        myTransform = ecs->getCustomModelTransform(id);
      } else {
        myTransform = ecs->getTransform(id);
      }
      mw *= myTransform;
      if (ecs->hasLocalMat3Override(id)) {
        for (int i = 0; i < 3; ++i) {
          for (int j = 0; j < 3; ++j) {
            mw.peek()[i][j] = myTransform[i][j];
          }
        }
      }
      ecs->setAbsTransform(id, mw.peek());
    }
    for (auto child : children) {
      child.second->traverseAndCache(mw);
    }
  }

  template<typename EcsInterface>
  typename EcsInterface::EcsId SceneObject<EcsInterface>::getId() const {
    return id;
  }

  template<typename EcsInterface>
  typename EcsInterface::EcsId SceneObject<EcsInterface>::getParentId() const {
    return parentId;
  }

  template<typename EcsInterface>
  const std::unordered_map<typename EcsInterface::EcsId, std::shared_ptr<SceneObject<EcsInterface>>> &
      SceneObject<EcsInterface>::getChildren() const {
    return children;
  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::linkEcs(std::shared_ptr<EcsInterface> &ecs) {
    SCENE_ECS = ecs;
  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::resetEcs() {
    SCENE_ECS.reset();
  }
}
