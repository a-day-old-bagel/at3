#pragma once

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
    SCENE_ reverseTransformLookup(wv);
    lastWorldViewQueried = wv;
    return wv;
  }
}
