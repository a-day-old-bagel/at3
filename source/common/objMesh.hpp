#pragma once

#include <string>
#include "obj.hpp"
#include "vkc.hpp"

namespace at3 {
  template <typename EcsInterface>
  class ObjMesh : public Obj<EcsInterface> {

      int shaderStyle;
      vkc::VulkanContext<EcsInterface> *vkc;

    public:
      enum ShaderStyle {
        NOTEXTURE, FULLBRIGHT, SUNNY
      };
      ObjMesh(vkc::VulkanContext<EcsInterface> *vkc, const std::string &meshFile, const std::string &textureFile,
                   glm::mat4 &transform, int style = SUNNY);
      ObjMesh(vkc::VulkanContext<EcsInterface> *vkc, const std::string &meshFile, glm::mat4 &transform);
      virtual ~ObjMesh() = default;
  };

  template <typename EcsInterface>
  ObjMesh<EcsInterface>::ObjMesh(vkc::VulkanContext<EcsInterface> *vkc, const std::string &meshFile,
                                           const std::string &textureFile, glm::mat4 &transform,
                                           int style) :shaderStyle(style), vkc(vkc) {
    SCENE_ECS->addTransform(SCENE_ID, transform);
    vkc->registerMeshInstance(SCENE_ID, meshFile, textureFile);
  }

  template <typename EcsInterface>
  ObjMesh<EcsInterface>::ObjMesh(vkc::VulkanContext<EcsInterface> *vkc, const std::string &meshFile,
                                           glm::mat4 &transform) : shaderStyle(NOTEXTURE), vkc(vkc) {
    SCENE_ECS->addTransform(SCENE_ID, transform);
    vkc->registerMeshInstance(SCENE_ID, meshFile);
  }

}
