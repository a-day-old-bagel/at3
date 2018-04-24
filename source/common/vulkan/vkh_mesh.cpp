#include "vkh_mesh.h"

namespace at3::Mesh {
  VertexRenderData *_vkRenderData;

  void setGlobalVertexLayout(std::vector<EMeshVertexAttribute> layout) {
    checkf(_vkRenderData == nullptr, "Attempting to set global vertex layout, but this has already been set");

    _vkRenderData = (VertexRenderData *) malloc(sizeof(VertexRenderData));

    _vkRenderData->attrCount = layout.size();
    _vkRenderData->attrDescriptions = (VkVertexInputAttributeDescription *) malloc(
        sizeof(VkVertexInputAttributeDescription) * _vkRenderData->attrCount);
    _vkRenderData->attributes = (EMeshVertexAttribute *) malloc(
        sizeof(EMeshVertexAttribute) * _vkRenderData->attrCount);
    memcpy(_vkRenderData->attributes, layout.data(), sizeof(EMeshVertexAttribute) * _vkRenderData->attrCount);

    uint32_t curOffset = 0;
    for (uint32_t i = 0; i < layout.size(); ++i) {
      switch (layout[i]) {
        case EMeshVertexAttribute::POSITION:
        case EMeshVertexAttribute::NORMAL:
        case EMeshVertexAttribute::TANGENT:
        case EMeshVertexAttribute::BITANGENT: {
          _vkRenderData->attrDescriptions[i] = {i, 0, VK_FORMAT_R32G32B32_SFLOAT, curOffset};
          curOffset += sizeof(glm::vec3);
        }
          break;

        case EMeshVertexAttribute::UV0:
        case EMeshVertexAttribute::UV1: {
          _vkRenderData->attrDescriptions[i] = {i, 0, VK_FORMAT_R32G32_SFLOAT, curOffset};
          curOffset += sizeof(glm::vec2);
        }
          break;
        case EMeshVertexAttribute::COLOR: {
          _vkRenderData->attrDescriptions[i] = {i, 0, VK_FORMAT_R32G32B32A32_SFLOAT, curOffset};
          curOffset += sizeof(glm::vec4);
        }
          break;
        default:
          checkf(0, "Invalid vertex attribute specified");
          break;
      }
    }

    _vkRenderData->vertexSize = curOffset;
  }

  const VertexRenderData *vertexRenderData() {
    checkf(_vkRenderData,
           "Attempting to get global vertex layout, but it has not been set yet, call setGlobalVertexLayout first.");

    return _vkRenderData;
  }
}
