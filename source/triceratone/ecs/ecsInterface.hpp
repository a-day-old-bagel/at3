
#pragma once

#include "configuration.hpp"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>

#include "ezecs.hpp"

#include "configuration.hpp"
#include "scene.hpp"
#include "sceneObject.hpp"
#include "perspectiveCamera.hpp"
#include "thirdPersonCamera.hpp"
#include "skyBox.hpp"
#include "meshObject.hpp"
#include "meshObjectVk.hpp"
#include "terrainObject.hpp"
#include "debug.hpp"

#include "bulletDebug.hpp"

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

      void addTerrain(const ezecs::entityId &id, const glm::mat4 &transform, ezecs::floatVecPtr heights,
                      size_t resX, size_t resY, float sclX, float sclY, float sclZ, float minZ, float maxZ);

      void addMouseControl(const ezecs::entityId& id);
  };

  typedef Scene<EntityComponentSystemInterface> Scene_;
  typedef SceneObject<EntityComponentSystemInterface> SceneObject_;
  typedef PerspectiveCamera<EntityComponentSystemInterface> PerspectiveCamera_;
  typedef ThirdPersonCamera<EntityComponentSystemInterface> ThirdPersonCamera_;
  typedef SkyBox<EntityComponentSystemInterface> SkyBox_;
  typedef MeshObject<EntityComponentSystemInterface> MeshObject_;
  typedef MeshObjectVk<EntityComponentSystemInterface> MeshObjectVk_;
  typedef TerrainObject<EntityComponentSystemInterface> TerrainObject_;
  typedef Debug<EntityComponentSystemInterface> Debug_;
  typedef BulletDebug<EntityComponentSystemInterface> BulletDebug_;

}