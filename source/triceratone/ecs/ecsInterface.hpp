
#pragma once

#include "math.hpp"
#include "ezecs.hpp"
#include "sceneTree.hpp"
#include "obj.hpp"
#include "objCameraPerspective.hpp"
#include "objCameraThirdPerson.hpp"
#include "objMesh.hpp"

namespace at3 {

  /**
   * The purpose of an "entity component system interface" is to provide an abstraction layer around whatever entity
   * component system or other game state machine you choose to implement, to be accessed by the scene tree and other
   * internal engine code. This is helpful because depending on factors like whether you're networking the game or not,
   * or how you organize your actual components in your ECS implementation, the API of your ECS (or equivalent state
   * machine) may change. An interface like this provides a consistent API to the lower-level engine stuff (scene tree
   * and rendering routines), while you can modify what these calls actually do within your ECS/state however you want.
   * It doesn't even have to wrap a real ECS, for that matter. It just has to act like it does.
   *
   * In my case, I'm also using this interface as a network access layer, and so I also access it from any of my high-
   * level code that needs to add or remove things from the ECS.
   *
   * For example, in a simple single-player game for which you've implemented a straightforward ECS, you can basically
   * just wrap a pointer to your ECS and some methods to access a subset of its functions in here. In that case, you'd
   * probably be asking yourself why you need an extra layer like this at all.
   * But what if you don't have a transform component and instead decide to use a combination of position and rotation
   * components? Or what if you keep all your transform information inside your physics component? I don't want to have
   * to redesign the scene tree or rendering code to work with that, and I don't want to have to write my ECS to conform
   * to what those structures expect (this is a low-coupling argument), so this interface gets around all that.
   * Or maybe more significantly, if you need to network your game and have to, say, make a request to the server when
   * you want a new entity to be made, it's better to write that functionality into an abstraction layer like this one
   * than to mash network code into your actual ECS or into the scene tree code (this is a high-cohesion argument).
   *
   * This interface is passed to the Game class as a template argument, and it is assumed to (and must) contain several
   * things in order to compile. You may notice that it would have been more natural in an OO language to make this a
   * derived class instead with the required members being overrides of virtual base members, but one of the tenets of
   * AT3 is to use templates instead wherever possible to aid in compile-time optimization. If you disagree with this
   * practice because of your "30+ years of programming experience," I'm sorry, but I don't really care.
   *
   * Those things which are required for any implementation are marked below in this particular implementation.
   */
  class EntityComponentSystemInterface {

      ezecs::State* state;

    public:

      explicit EntityComponentSystemInterface(ezecs::State* state);

      /*
       * All of the following members of EntityComponentSystemInterface are required by any such interface.
       * This includes the following two typedefs, the aliases of which must be reproduced in your own implementation,
       * and all of the public member methods here, whose signatures must also be reproduced.
       */

      typedef ezecs::entityId EcsId;
      typedef ezecs::State State;

      EcsId createEntity();
      void destroyEntity(const EcsId& id);

      /*
       * A transform is the transformation matrix from model space to world space encoding both world position and
       * world orientation. An absolute transform is the same, except that it is the cached transformation matrix that
       * also includes all of the inherited transforms from parent objects in the conceptual scene tree. The scene tree
       * will attempt to write to and read from this cached field, so you must implement some sort of data storage
       * to allow this effect. In my case, my "placement" component includes both a transform and an absolute transform
       * matrix, the latter used by the scene tree for caching as described.
       *
       * If hasTransform returns true, it indicates that both the transform and absolute transform fields exist to be
       * accessed by getTransform, getAbsTransform, and setAbsTransform. If hasTransform returns false, addTransform
       * may be used to create these fields.
       */
      bool hasTransform(const EcsId& id);
      void addTransform(const EcsId& id, const glm::mat4& transform);
      glm::mat4 getTransform(const EcsId& id);
      glm::mat4 getAbsTransform(const EcsId& id);
      void setAbsTransform(const EcsId& id, const glm::mat4& transform);

      /*
       * A local mat3 override is a mechanism to allow an object in the scene tree to inherit only the positions of its
       * parent objects. Effectively, if hasLocalMat3Override returns true, then that entity will always have the
       * orientation and scale given by getTransform, regardless of whether it inherits its absolute transform from
       * other objects. This override property can be enabled or disabled by using setLocalMat3Override.
       * This is useful for things like camera control, where it's convenient to have a camera follow an object, but
       * keep its own orientation information separate from its parent object's orientation.
       */
      bool hasLocalMat3Override(const EcsId &id);
      void setLocalMat3Override(const EcsId &id, bool value);

      /*
       * A custom model transform is an additional transform that is applied to an object before its normal transform
       * matrices are applied. This is useful for applying dynamic visual model transformations that depend on world
       * transform or time or something, without polluting the regular transform information for that object. This is
       * especially important if, as in my case, you don't keep separate position and rotation data for objects, but
       * instead rely on the transformation matrices themselves to hold state.
       */
      bool hasCustomModelTransform(const EcsId &id);
      glm::mat4 getCustomModelTransform(const EcsId &id);

      /*
       * These methods assume you keep some sort of state representing your cameras, which will include keeping
       * data like the field of view and the near ans far planes.
       */
      void addPerspective(const EcsId &id, float fovy, float near, float far);
      float getFovy(const EcsId& id);
      float getFovyPrev(const EcsId& id);
      float getNear(const EcsId& id);
      float getFar(const EcsId& id);
      void setFar(const EcsId &id, float far);

      /*
       * This is used when creating mouse-controlled cameras, but it's not a very clean or portable way to manage this,
       * so I need to rethink it. Sorry. I'll probably move the third-person camera code up into my implementation
       * layer instead of putting it in common, since it's NOT really common to all kinds of games. TODO: this.
       */
      void addMouseControl(const EcsId& id);
  };

  /*
   * None of this is required in your implementation, and is just a helper for me to shorten type names that would
   * normally be long and ugly because of template arguments. They make high level code (the stuff in the class you
   * write that CRTP-inherits from Game) much cleaner looking.
   */
  typedef SceneTree<EntityComponentSystemInterface> Scene;
  typedef Obj<EntityComponentSystemInterface> Object;
  typedef ObjCameraPerspective<EntityComponentSystemInterface> PerspectiveCamera;
  typedef ObjCameraThirdPerson<EntityComponentSystemInterface> ThirdPersonCamera;
  typedef ObjMesh<EntityComponentSystemInterface> Mesh;

}
