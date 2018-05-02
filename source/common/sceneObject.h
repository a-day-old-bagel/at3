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


#include "transformRAII.h"
//#include "sceneObject.h"

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
  template<typename EcsInterface>
  class SceneObject {
    private:
      std::unordered_map<
          const SceneObject<EcsInterface> *,
          std::shared_ptr<SceneObject<EcsInterface>>
      > m_children;
      SceneObject<EcsInterface> *m_parent = NULL;
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
       * Draws this scene object in the scene. The default behavior of this
       * method is to draw nothing.
       *
       * \param modelWorld The model-space to world-space transform for this
       * scene object's position and orientation.
       * \param worldView The world-space to view-space transform for the
       * position and orientation of the camera currently being used.
       * \param projection The view-space to projection-space transform for
       * the camera that is currently being used.
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

      /**
       * Caches this scene object's absolute world transform inside it's ECS transform component.
       * This is done by traversing the scene graph. This objects children will inherit its transform.
       *
       * @param modelWorld
       */
      virtual void m_traverseAndCache(Transform &modelWorld);

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
  EcsInterface *SceneObject<EcsInterface>::ecs = NULL;

  template<typename EcsInterface>
  SceneObject<EcsInterface>::SceneObject() {
    id = ecs->createEntity();
  }

  template<typename EcsInterface>
  SceneObject<EcsInterface>::~SceneObject() {
    ecs->destroyEntity(id);
  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::addChild(std::shared_ptr<SceneObject> child, int inheritedDOF /* = ALL */) {
    m_children.insert({child.get(), child});
    child.get()->m_parent = this;
    child.get()->m_inheritedDOF = inheritedDOF;
  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::removeChild(const SceneObject *address) {
    auto iterator = m_children.find(address);
    assert(iterator != m_children.end());
    // TODO: Set the m_parent member of this child to nullptr (SceneObjects
    // do not have m_parent members at the time of this writing).
    m_children.erase(iterator);
  }

  template<typename EcsInterface>
  bool SceneObject<EcsInterface>::hasChild(const SceneObject *address) {
    return m_children.find(address) != m_children.end();
  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::m_draw(Transform &modelWorld, const glm::mat4 &worldView,
                                         const glm::mat4 &projection, bool debug) {
    TransformRAII mw(modelWorld);
    glm::mat4 myTransform(1.f);
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
                                        1, 0, 0, 0,
                                        0, 1, 0, 0,
                                        0, 0, 1, 0,
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
                                      0, 0, 0, 1
                                  });
          child.second->m_draw(mw_noTrans, worldView, projection, debug);
        } return;
        default:
          break;
      }
      child.second->m_draw(mw, worldView, projection, debug);
    }
  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::m_traverseAndCache(Transform &modelWorld) {
    TransformRAII mw(modelWorld);
    glm::mat4 myTransform(1.f);
    bool hasTransform = false;

    if (ecs->hasTransform(id)) {
      myTransform = ecs->getTransform(id);
      mw *= myTransform;
      if (ecs->hasTransformFunction(id)) {
        mw *= ecs->getTransformFunction(id);
      }
      ecs->setAbsTransform(id, mw.peek());
      hasTransform = true;
    }

    // Draw our children
    for (auto child : m_children) {
      switch (child.second->m_inheritedDOF) {
        case TRANSLATION_ONLY: {
          if (hasTransform) { break; }
          TransformRAII mw_transOnly(modelWorld);
          mw_transOnly *= glm::mat4({
                                        1, 0, 0, 0,
                                        0, 1, 0, 0,
                                        0, 0, 1, 0,
                                        myTransform[3][0], myTransform[3][1], myTransform[3][2], 1
                                    });
          child.second->m_traverseAndCache(mw_transOnly);
        }
          return;
        case WITHOUT_TRANSLATION: {
          if (hasTransform) { break; }
          TransformRAII mw_noTrans(modelWorld);
          mw_noTrans *= glm::mat4({
                                      myTransform[0][0], myTransform[0][1], myTransform[0][2], 0,
                                      myTransform[1][0], myTransform[1][1], myTransform[1][2], 0,
                                      myTransform[2][0], myTransform[2][1], myTransform[2][2], 0,
                                      0, 0, 0, 1
                                  });
          child.second->m_traverseAndCache(mw_noTrans);
        }
          return;
        default:
          break;
      }
      child.second->m_traverseAndCache(mw);
    }

//    // Draw our children TODO: Why isn't this way correct? Why are there returns after each case above? Does the above case limit objects to one child each? The same goes for m_draw...
//    for (auto child : m_children) {
//      switch (child.second->m_inheritedDOF) {
//        case TRANSLATION_ONLY: {
//          if (hasTransform) { break; }
//          TransformRAII mw_transOnly(modelWorld);
//          mw_transOnly *= glm::mat4({
//                                        1, 0, 0, 0,
//                                        0, 1, 0, 0,
//                                        0, 0, 1, 0,
//                                        myTransform[3][0], myTransform[3][1], myTransform[3][2], 1
//                                    });
//          child.second->m_traverseAndCache(mw_transOnly);
//        } break;
//        case WITHOUT_TRANSLATION: {
//          if (hasTransform) { break; }
//          TransformRAII mw_noTrans(modelWorld);
//          mw_noTrans *= glm::mat4({
//                                      myTransform[0][0], myTransform[0][1], myTransform[0][2], 0,
//                                      myTransform[1][0], myTransform[1][1], myTransform[1][2], 0,
//                                      myTransform[2][0], myTransform[2][1], myTransform[2][2], 0,
//                                      0, 0, 0, 1
//                                  });
//          child.second->m_traverseAndCache(mw_noTrans);
//        } break;
//        default: {
//          child.second->m_traverseAndCache(mw);
//        } break;
//      }
//    }

  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::reverseTransformLookup(glm::mat4 &wv, bool forceTraverse, bool includeTransFunc,
                                                           int whichDOFs) const {
    if ( ! forceTraverse) {
      if ( ! includeTransFunc) {
        printf("SceneObject::reverseTransformLookup: forceTraverse is set to false, so includeTransFunc is ignored.\n");
      }
      if (ecs->hasTransform(id)) {
        glm::mat4 fullTransform = ecs->getAbsTransform(id);
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
      } else if (m_parent != NULL) {
        m_parent->reverseTransformLookup(wv, forceTraverse, includeTransFunc, m_inheritedDOF);
      }
    } else { // the way that actually works for the camera :(
      if (ecs->hasTransform(id)) {
        glm::mat4 fullTransform = ecs->getTransform(id);
        if (includeTransFunc && ecs->hasTransformFunction(id)) {
          fullTransform *= ecs->getTransformFunction(id);
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

      if (m_parent != NULL) {
        m_parent->reverseTransformLookup(wv, forceTraverse, includeTransFunc, m_inheritedDOF);
      }
    }
  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView,
                                       const glm::mat4 &projection, bool debug) { }

  template<typename EcsInterface>
  typename EcsInterface::EcsId SceneObject<EcsInterface>::getId() const {
    return id;
  }

  template<typename EcsInterface>
  void SceneObject<EcsInterface>::linkEcs(EcsInterface &ecs) {
    SCENE_ECS = &ecs;
  }
}
