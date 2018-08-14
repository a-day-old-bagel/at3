#pragma once

#include <SDL.h>
#include <memory>
#include <unordered_map>

#include "transformRAII.hpp"
#include "math.hpp"

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
      std::unordered_map<const SceneObject<EcsInterface> *, std::shared_ptr<SceneObject<EcsInterface>>> children;
      SceneObject<EcsInterface> *parent = nullptr;

    public:

      static std::shared_ptr<EcsInterface> ecs;
      typename EcsInterface::EcsId id;

      explicit SceneObject(const typename EcsInterface::EcsId & id);
      virtual ~SceneObject();

      /**
       * Adds a scene object as a child of this scene object.
       *
       * \param child Child scene object to add.
       *
       * \sa removeChild()
       */
      void addChild(std::shared_ptr<SceneObject<EcsInterface>> child);

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
       * Caches this scene object's absolute world transform inside it's ECS transform component.
       * This is done by traversing the scene tree.
       *
       * @param modelWorld
       */
      virtual void traverseAndCache(Transform &modelWorld);

      /**
       * Get ID
       * @return id of this entity according to ECS
       */
      typename EcsInterface::EcsId getId() const;

      /**
       *
       */
      static void linkEcs(std::shared_ptr<EcsInterface> &ecs);
      static void resetEcs();
  };

  template<typename EcsInterface>
  std::shared_ptr<EcsInterface> SceneObject<EcsInterface>::ecs;

  template<typename EcsInterface>
  SceneObject<EcsInterface>::SceneObject(const typename EcsInterface::EcsId & id) : id(id) { }

  template<typename EcsInterface>
  SceneObject<EcsInterface>::~SceneObject() { }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::addChild(std::shared_ptr<SceneObject> child) {
    children.insert({child.get(), child});
    child.get()->parent = this;
  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::removeChild(const SceneObject *address) {
    auto iterator = children.find(address);
    assert(iterator != children.end());
    // TODO: Set the parent member of this child to nullptr (SceneObjects
    // do not have parent members at the time of this writing).
    children.erase(iterator);
  }

  template<typename EcsInterface>
  bool SceneObject<EcsInterface>::hasChild(const SceneObject *address) {
    return children.find(address) != children.end();
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
  void SceneObject<EcsInterface>::linkEcs(std::shared_ptr<EcsInterface> &ecs) {
    SCENE_ECS = ecs;
  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::resetEcs() {
    SCENE_ECS.reset();
  }
}
