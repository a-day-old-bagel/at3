#pragma once

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "obj.hpp"

namespace at3 {

  template <typename EcsInterface>
  class ObjCamera : public Obj<EcsInterface> {
    public:

      ObjCamera(glm::mat4 &transform);
      virtual ~ObjCamera();

      virtual glm::mat4 worldView();
      glm::mat4 lastWorldViewQueried = glm::mat4(1.f); // set every time worldView is called (for cache)

      virtual glm::mat4 projection(float aspect, float alpha = 1.0f) const = 0;

      virtual float getFovy() const = 0;
  };

  template <typename EcsInterface>
  ObjCamera<EcsInterface>::ObjCamera(glm::mat4 &transform) {
    SCENE_ECS->addTransform(SCENE_ID, transform);
  }

  template <typename EcsInterface>
  ObjCamera<EcsInterface>::~ObjCamera() { }

  template <typename EcsInterface>
  glm::mat4 ObjCamera<EcsInterface>::worldView() {
    lastWorldViewQueried = glm::inverse(SCENE_ECS->getAbsTransform(SCENE_ID));
    return lastWorldViewQueried;
  }
}
