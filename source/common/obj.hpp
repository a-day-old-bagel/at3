#pragma once

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SDL.h>
#include <memory>
#include <unordered_map>

#include "transformRAII.hpp"

#define SCENE_ Obj<EcsInterface>::
#define SCENE_ECS Obj<EcsInterface>::ecs
#define SCENE_ID Obj<EcsInterface>::id

namespace at3 {
  class Transform;

  /**
   * This abstract class defines a typical object in a 3D graphics scene.
   *
   * The SceneObject class has both position and orientation properties useful
   * for most rigid body objects.
   */
  template<typename EcsInterface>
  class Obj {
    private:
      std::unordered_map<const Obj<EcsInterface> *, std::shared_ptr<Obj<EcsInterface>>> children;
      Obj<EcsInterface> *parent = nullptr;

    public:

      static EcsInterface *ecs;
      typename EcsInterface::EcsId id;

      Obj();
      virtual ~Obj();

      /**
       * Adds a scene object as a child of this scene object.
       *
       * \param child Child scene object to add.
       *
       * \sa removeChild()
       */
      void addChild(std::shared_ptr<Obj<EcsInterface>> child);

      /**
       * \param The address of the child scene object to look for.
       * \return True if a scene object with the given address is a child of
       * this node, false otherwise.
       */
      bool hasChild(const Obj<EcsInterface> *address);

      /**
       * Removes the child of this scene object with the given address.
       *
       * \param address The memory address of the child to remove.
       *
       * \sa addChild()
       */
      void removeChild(const Obj<EcsInterface> *address);

      /**
       * Caches this scene object's absolute world transform inside it's ECS transform component.
       * This is done by traversing the scene graph. This objects children will inherit its transform.
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
      static void linkEcs(EcsInterface &ecs);
  };

  template<typename EcsInterface>
  EcsInterface *Obj<EcsInterface>::ecs = NULL;

  template<typename EcsInterface>
  Obj<EcsInterface>::Obj() {
    id = ecs->createEntity();
  }

  template<typename EcsInterface>
  Obj<EcsInterface>::~Obj() {
    ecs->destroyEntity(id);
  }

  template<typename EcsInterface>
  void Obj<EcsInterface>::addChild(std::shared_ptr<Obj> child) {
    children.insert({child.get(), child});
    child.get()->parent = this;
  }

  template<typename EcsInterface>
  void Obj<EcsInterface>::removeChild(const Obj *address) {
    auto iterator = children.find(address);
    assert(iterator != children.end());
    // TODO: Set the parent member of this child to nullptr (SceneObjects
    // do not have parent members at the time of this writing).
    children.erase(iterator);
  }

  template<typename EcsInterface>
  bool Obj<EcsInterface>::hasChild(const Obj *address) {
    return children.find(address) != children.end();
  }

  template<typename EcsInterface>
  void Obj<EcsInterface>::traverseAndCache(Transform &modelWorld) {
    TransformRAII mw(modelWorld);
    glm::mat4 myTransform(1.f);
    if (ecs->hasTransform(id)) {
      if (ecs->hasCustomModelTransform(id)) {
        myTransform = ecs->getCustomModelTransform(id);
      } else {
        myTransform = ecs->getTransform(id);
      }
//      if (ecs->hasLocalMat3Override(id)) {
//        myTransform[3][2] += 5.f;
////        printf("%u\n", id);
//      }
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
  typename EcsInterface::EcsId Obj<EcsInterface>::getId() const {
    return id;
  }

  template<typename EcsInterface>
  void Obj<EcsInterface>::linkEcs(EcsInterface &ecs) {
    SCENE_ECS = &ecs;
  }
}
