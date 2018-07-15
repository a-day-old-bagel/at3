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
      Obj<EcsInterface> *parent = NULL;
      int inheritedDOF = ALL;

    public:

      static EcsInterface *ecs;
      typename EcsInterface::EcsId id;

      Obj();
      virtual ~Obj();

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
      void addChild(std::shared_ptr<Obj<EcsInterface>> child, int inheritedDOF = ALL);

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

      void reverseTransformLookup(glm::mat4 &wv, bool forceTraverse = false, bool includeTransFunc = true,
                                        int whichDOFs = ALL) const;

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
  void Obj<EcsInterface>::addChild(std::shared_ptr<Obj> child, int inheritedDOF /* = ALL */) {
    children.insert({child.get(), child});
    child.get()->parent = this;
    child.get()->inheritedDOF = inheritedDOF;
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
      mw *= myTransform;
      ecs->setAbsTransform(id, mw.peek());
    }

    for (auto child : children) {
      switch (child.second->inheritedDOF) {
        case TRANSLATION_ONLY: {
          TransformRAII mw_transOnly(modelWorld);
          mw_transOnly *= glm::mat4({
                                        1, 0, 0, 0,
                                        0, 1, 0, 0,
                                        0, 0, 1, 0,
                                        myTransform[3][0], myTransform[3][1], myTransform[3][2], 1
                                    });
          child.second->traverseAndCache(mw_transOnly);
        } break;
        case WITHOUT_TRANSLATION: {
          TransformRAII mw_noTrans(modelWorld);
          mw_noTrans *= glm::mat4({
                                      myTransform[0][0], myTransform[0][1], myTransform[0][2], 0,
                                      myTransform[1][0], myTransform[1][1], myTransform[1][2], 0,
                                      myTransform[2][0], myTransform[2][1], myTransform[2][2], 0,
                                      0, 0, 0, 1
                                  });
          child.second->traverseAndCache(mw_noTrans);
        } break;
        default: child.second->traverseAndCache(mw);
      }
    }
  }

  template<typename EcsInterface>
  void Obj<EcsInterface>::reverseTransformLookup(glm::mat4 &wv, bool forceTraverse, bool includeTransFunc,
                                                           int whichDOFs) const {


//    wv = glm::inverse(ecs->getAbsTransform(id));


    if ( ! forceTraverse) { // FIXME: NOT WORKING - inverse abs trans should be enough
//      if ( ! includeTransFunc) {
//        printf("Obj::reverseTransformLookup: forceTraverse is set to false, so includeTransFunc is ignored.\n");
//      }
//      if (ecs->hasTransform(id)) {
//        glm::mat4 fullTransform = ecs->getAbsTransform(id);
//        switch (whichDOFs) {
//          case TRANSLATION_ONLY: {
//            wv *= glm::inverse(glm::mat4({
//                                             1, 0, 0, 0,
//                                             0, 1, 0, 0,
//                                             0, 0, 1, 0,
//                                             fullTransform[3][0], fullTransform[3][1], fullTransform[3][2], 1
//                                         }));
//          } break;
//          case WITHOUT_TRANSLATION: {
//            wv *= glm::inverse(glm::mat4({
//                                             fullTransform[0][0], fullTransform[0][1], fullTransform[0][2], 0,
//                                             fullTransform[1][0], fullTransform[1][1], fullTransform[1][2], 0,
//                                             fullTransform[2][0], fullTransform[2][1], fullTransform[2][2], 0,
//                                             0, 0, 0, 1
//                                         }));
//          } break;
//          default: {
//            wv *= glm::inverse(fullTransform);
//          } break;
//        }
//      } else if (parent != NULL) {
//        parent->reverseTransformLookup(wv, forceTraverse, includeTransFunc, inheritedDOF);
//      }
    } else { // the way that actually works for the camera :(
      if (ecs->hasTransform(id)) {
        glm::mat4 fullTransform;
        if (includeTransFunc && ecs->hasCustomModelTransform(id)) {
          fullTransform = ecs->getCustomModelTransform(id);
        } else {
          fullTransform = ecs->getTransform(id);
        }
        switch (whichDOFs) {
          case TRANSLATION_ONLY: {
            wv *= glm::inverse(glm::mat4({
                                             1, 0, 0, 0,
                                             0, 1, 0, 0,
                                             0, 0, 1, 0,
                                             fullTransform[3][0], fullTransform[3][1], fullTransform[3][2], 1
                                         }));
          } break;
          case WITHOUT_TRANSLATION: {
            wv *= glm::inverse(glm::mat4({
                                             fullTransform[0][0], fullTransform[0][1], fullTransform[0][2], 0,
                                             fullTransform[1][0], fullTransform[1][1], fullTransform[1][2], 0,
                                             fullTransform[2][0], fullTransform[2][1], fullTransform[2][2], 0,
                                             0, 0, 0, 1
                                         }));
          } break;
          default: {
            wv *= glm::inverse(fullTransform);
          } break;
        }
      }

      if (parent != NULL) {
        parent->reverseTransformLookup(wv, forceTraverse, includeTransFunc, inheritedDOF);
      }
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
