
#ifndef AT3_ECSINTERFACE_H
#define AT3_ECSINTERFACE_H

#include <glm/glm.hpp>
#include "ezecs.hpp"

#include "scene.h"
#include "sceneObject.h"
#include "perspectiveCamera.h"
#include "thirdPersonCamera.h"
#include "skyBox.h"
#include "meshObject.h"
#include "terrainObject.h"
#include "debug.h"

#include "bulletDebug.h"

namespace at3 {

  class DualityInterface {
      ezecs::State* state;
    public:
      typedef ezecs::entityId EcsId;
      typedef ezecs::State State;
      explicit DualityInterface(ezecs::State* state);
      ezecs::entityId createEntity();
      void destroyEntity(const ezecs::entityId& id);

      void addTransform(const ezecs::entityId& id, const glm::mat4& transform);
      bool hasTransform(const ezecs::entityId& id);
      glm::mat4 getTransform(const ezecs::entityId& id);
      void setTransform(const ezecs::entityId& id, const glm::mat4& transform);

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

  typedef Scene<DualityInterface> Scene_;
  typedef SceneObject<DualityInterface> SceneObject_;
  typedef PerspectiveCamera<DualityInterface> PerspectiveCamera_;
  typedef ThirdPersonCamera<DualityInterface> ThirdPersonCamera_;
  typedef SkyBox<DualityInterface> SkyBox_;
  typedef MeshObject<DualityInterface> MeshObject_;
  typedef TerrainObject<DualityInterface> TerrainObject_;
  typedef Debug<DualityInterface> Debug_;
  typedef BulletDebug<DualityInterface> BulletDebug_;

}

#endif //AT3_ECSINTERFACE_H
