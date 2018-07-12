#pragma once

#include "definitions.hpp"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

#include "objCameraPerspective.hpp"

namespace at3 {
  /**
   * A camera object that orbits and looks at some target entity (third-person style)
   */
  template <typename EcsInterface>
  class ObjCameraThirdPerson {
    public:
      std::shared_ptr<ObjCameraPerspective<EcsInterface>> mpCamera;
      std::shared_ptr<Obj<EcsInterface>> mpCamGimbal;
      ObjCameraThirdPerson(float up, float back, float tilt);
      virtual ~ObjCameraThirdPerson();
  };

  template <typename EcsInterface>
  ObjCameraThirdPerson<EcsInterface>::ObjCameraThirdPerson(float up, float back, float tilt) {
    glm::mat4 ident(1.f);
    glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, -back, 0.f}), tilt, glm::vec3(1.0f, 0.0f, 0.0f));
    mpCamera = std::shared_ptr<ObjCameraPerspective<EcsInterface>>
        (new ObjCameraPerspective<EcsInterface>(camMat));
    mpCamGimbal = std::shared_ptr<Obj<EcsInterface>>
        (new Obj<EcsInterface>());
    // add mouse controls to the camera gimbal so that it rotates with the mouse
    typename EcsInterface::EcsId gimbalId = mpCamGimbal->getId();
    SCENE_ECS->addTransform(gimbalId, {
        1,  0,  0,  0,
        0,  1,  0,  0,
        0,  0,  1,  0,
        0,  0, up,  1  // at (0, 0, up)
    });
    SCENE_ECS->addMouseControl(gimbalId);
    mpCamGimbal->addChild(mpCamera);
  }

  template <typename EcsInterface>
  ObjCameraThirdPerson<EcsInterface>::~ObjCameraThirdPerson() {
    //TODO: deal with gimbal?
  }
}
