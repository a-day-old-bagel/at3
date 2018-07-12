
#pragma once

#include "definitions.hpp"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>

#include "ezecs.hpp"

#include "definitions.hpp"
#include "sceneTree.hpp"
#include "obj.hpp"
#include "objCameraPerspective.hpp"
#include "objCameraThirdPerson.hpp"
#include "objMesh.hpp"

namespace at3 {

  class EntityComponentSystemInterface {
      ezecs::State* state;
    public:
      typedef ezecs::entityId EcsId;
      typedef ezecs::State State;
      explicit EntityComponentSystemInterface(ezecs::State* state);
      ezecs::entityId createEntity();
      void destroyEntity(const ezecs::entityId& id);

      void addTransform(const ezecs::entityId& id, const glm::mat4& transform);
      bool hasTransform(const ezecs::entityId& id);
      glm::mat4 getTransform(const ezecs::entityId& id);
      void setTransform(const ezecs::entityId& id, const glm::mat4& transform);
      glm::mat4 getAbsTransform(const ezecs::entityId& id);
      void setAbsTransform(const ezecs::entityId& id, const glm::mat4& transform);

      bool hasTransformFunction(const ezecs::entityId& id);
      glm::mat4 getTransformFunction(const ezecs::entityId& id);

      void addPerspective(const ezecs::entityId &id, float fovy, float near, float far);
      float getFovy(const ezecs::entityId& id);
      float getFovyPrev(const ezecs::entityId& id);
      float getNear(const ezecs::entityId& id);
      float getFar(const ezecs::entityId& id);
      void setFar(const ezecs::entityId &id, float far);

      void addMouseControl(const ezecs::entityId& id);
  };

  typedef SceneTree<EntityComponentSystemInterface> Scene;
  typedef Obj<EntityComponentSystemInterface> Object;
  typedef ObjCameraPerspective<EntityComponentSystemInterface> PerspectiveCamera;
  typedef ObjCameraThirdPerson<EntityComponentSystemInterface> ThirdPersonCamera;
  typedef ObjMesh<EntityComponentSystemInterface> Mesh;

}
