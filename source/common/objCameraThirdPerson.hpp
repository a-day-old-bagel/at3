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
   * A camera object that orbits and looks at some target entity (third-person style) OR
   * can be used as a first-person camera.
   *
   *
   */
   //TODO: change naming, find a way to store this data in a component (up, back, tilt, anchor, etc.)
   // FIXME: Get rid of dependency to generate ezecs stuff in here (it's a mess.)
  // maybe just move this whole class up a level or something. Or move all the objects up a level.
  // or restructure in some other way.
  template <typename EcsInterface>
  class ObjCameraThirdPerson {
      void init(float up, float back, float tilt);
    public:
      std::shared_ptr<ObjCameraPerspective<EcsInterface>> actual;
      std::shared_ptr<Obj<EcsInterface>> gimbal;
      std::shared_ptr<Obj<EcsInterface>> orient;
      ObjCameraThirdPerson(float up, float back, float tilt);
      ObjCameraThirdPerson(float up, float back, float tilt, ezecs::transformFunc &orientationFunction);
      virtual ~ObjCameraThirdPerson();
      void anchorTo(std::shared_ptr<Obj<EcsInterface>> anchor);
  };

  template <typename EcsInterface>
  ObjCameraThirdPerson<EcsInterface>::ObjCameraThirdPerson(float up, float back, float tilt) {
    init(up, back, tilt);
  }

  template <typename EcsInterface>
  ObjCameraThirdPerson<EcsInterface>::ObjCameraThirdPerson(float up, float back, float tilt,
                                                           ezecs::transformFunc &orientationFunction) {
    init(up, back, tilt);

    // add a custom transform to the camera orient-er so that it can rotate in place if it needs to
    typename EcsInterface::EcsId orientId = orient->getId();
    SCENE_ECS->addTransform(orientId, glm::mat4(1.f));
    SCENE_ECS->addCustomModelTransform(orientId, orientationFunction);
  }

  template <typename EcsInterface>
  void ObjCameraThirdPerson<EcsInterface>::init(float up, float back, float tilt) {
    glm::mat4 ident(1.f);
    glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, /*-back*/0.f,/* 0.f*/ back}),
                                   tilt /*+ (float)M_PI * .5f*/, glm::vec3(1.0f, 0.0f, 0.0f));
    actual = std::shared_ptr<ObjCameraPerspective<EcsInterface>>
        (new ObjCameraPerspective<EcsInterface>(camMat));
    gimbal = std::shared_ptr<Obj<EcsInterface>>
        (new Obj<EcsInterface>());
    orient = std::shared_ptr<Obj<EcsInterface>>
        (new Obj<EcsInterface>());
    // add mouse controls to the camera gimbal so that it rotates with the mouse
    typename EcsInterface::EcsId gimbalId = gimbal->getId();
    SCENE_ECS->addTransform(gimbalId, {
        1,  0,  0,  0,
        0,  1,  0,  0,
        0,  0,  1,  0,
        0,  0, up,  1  // at (0, 0, up)
    });
    SCENE_ECS->addMouseControl(gimbalId);
    SCENE_ECS->setLocalMat3Override(gimbalId, true);
    gimbal->addChild(orient);
    orient->addChild(actual);
  }

  template <typename EcsInterface>
  ObjCameraThirdPerson<EcsInterface>::~ObjCameraThirdPerson() {
    //TODO: deal with gimbal?
  }

  template <typename EcsInterface>
  void ObjCameraThirdPerson<EcsInterface>::anchorTo(std::shared_ptr<Obj<EcsInterface>> anchor) {
    anchor->addChild(gimbal, SCENE_ TRANSLATION_ONLY);
  }
}
