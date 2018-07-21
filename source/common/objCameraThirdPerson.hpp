#pragma once


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
    public:
      std::shared_ptr<ObjCameraPerspective<EcsInterface>> actual;
      std::shared_ptr<Obj<EcsInterface>> gimbal;
      ObjCameraThirdPerson(float back, float tilt);
      virtual ~ObjCameraThirdPerson();
      void anchorTo(std::shared_ptr<Obj<EcsInterface>> anchor);
  };

  template <typename EcsInterface>
  ObjCameraThirdPerson<EcsInterface>::ObjCameraThirdPerson(float back, float tilt) {
    glm::mat4 ident(1.f);
    glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, 0.f, back}), tilt , glm::vec3(1.0f, 0.0f, 0.0f));
    actual = std::shared_ptr<ObjCameraPerspective<EcsInterface>>
        (new ObjCameraPerspective<EcsInterface>(camMat));
    gimbal = std::shared_ptr<Obj<EcsInterface>>
        (new Obj<EcsInterface>());
    // add mouse controls to the camera gimbal so that it rotates with the mouse
    typename EcsInterface::EcsId gimbalId = gimbal->getId();
    SCENE_ECS->addTransform(gimbalId, ident);
    SCENE_ECS->setLocalMat3Override(gimbalId, true);
    SCENE_ECS->addMouseControl(gimbalId);
    gimbal->addChild(actual);
  }

  template <typename EcsInterface>
  ObjCameraThirdPerson<EcsInterface>::~ObjCameraThirdPerson() {
    //TODO: deal with gimbal?
  }

  template <typename EcsInterface>
  void ObjCameraThirdPerson<EcsInterface>::anchorTo(std::shared_ptr<Obj<EcsInterface>> anchor) {
    anchor->addChild(gimbal);
  }
}
