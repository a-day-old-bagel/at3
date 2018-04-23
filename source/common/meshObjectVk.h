#pragma once

#include <string>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "sceneObject.h"
#include "vulkanContext.h"

namespace at3 {
  template <typename EcsInterface>
  class MeshObjectVk : public SceneObject<EcsInterface> {

      int shaderStyle;
      VulkanContext<EcsInterface> *vkc;

    public:
      enum ShaderStyle {
        NOTEXTURE, FULLBRIGHT, SUNNY
      };
      MeshObjectVk(VulkanContext<EcsInterface> *vkc, const std::string &meshFile, const std::string &textureFile, glm::mat4 &transform, int style);
      MeshObjectVk(VulkanContext<EcsInterface> *vkc, const std::string &meshFile, glm::mat4 &transform);
      virtual ~MeshObjectVk() = default;

      virtual void draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &proj, bool debug);
  };

  template <typename EcsInterface>
  MeshObjectVk<EcsInterface>::MeshObjectVk(VulkanContext<EcsInterface> *vkc, const std::string &meshFile,
                                           const std::string &textureFile, glm::mat4 &transform,
                                           int style) :shaderStyle(style), vkc(vkc) {
    SCENE_ECS->addTransform(SCENE_ID, transform);
    vkc->registerMeshInstance(meshFile, SCENE_ID);
  }

  template <typename EcsInterface>
  MeshObjectVk<EcsInterface>::MeshObjectVk(VulkanContext<EcsInterface> *vkc, const std::string &meshFile,
                                           glm::mat4 &transform) : shaderStyle(NOTEXTURE), vkc(vkc) {
    SCENE_ECS->addTransform(SCENE_ID, transform);
    vkc->registerMeshInstance(meshFile, SCENE_ID);
  }

  template <typename EcsInterface>
  void MeshObjectVk<EcsInterface>::draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView,
                                             const glm::mat4 &proj, bool debug)
  {  }
}
