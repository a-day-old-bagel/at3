#pragma once

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "sceneObject.h"

namespace at3 {

  template <typename EcsInterface>
  class Camera : public SceneObject<EcsInterface> {
    public:

      Camera(glm::mat4 &transform);
      virtual ~Camera();

      virtual glm::mat4 worldView();
      glm::mat4 lastWorldViewQueried; // set every time worldView is called (for cache)

      virtual glm::mat4 projection(float aspect, float alpha = 1.0f) const = 0;

      virtual float getFovy() const = 0;
  };

  template <typename EcsInterface>
  Camera<EcsInterface>::Camera(glm::mat4 &transform) {
    SCENE_ECS->addTransform(SCENE_ID, transform);
  }

  template <typename EcsInterface>
  Camera<EcsInterface>::~Camera() { }

  template <typename EcsInterface>
  glm::mat4 Camera<EcsInterface>::worldView() {
    glm::mat4 wv;

    // TODO: Reinvestigate this horrible crap
    SCENE_ reverseTransformLookup(wv, true, true, SceneObject<EcsInterface>::InheritedDOF::ALL);

    lastWorldViewQueried = wv;
    return wv;
  }
}
