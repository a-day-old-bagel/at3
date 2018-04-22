#pragma once

#include <vector>
#include "config.h"
#include "vkh.h"

#define GLM_FORCE_RADIANS
#if USE_AT3_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>


namespace at3
{
	enum class EMeshVertexAttribute : uint8_t
	{
		POSITION,
		UV0,
		UV1,
		NORMAL,
		TANGENT,
		BITANGENT,
		COLOR
	};

	struct VertexRenderData
	{
		VkVertexInputAttributeDescription* attrDescriptions;
		EMeshVertexAttribute* attributes;
		uint32_t attrCount;
		uint32_t vertexSize;
	};

	struct MeshAsset
	{
		VkBuffer buffer;
		Allocation bufferMemory;

		uint32_t vOffset;
		uint32_t iOffset;

		uint32_t vCount;
		uint32_t iCount;

		glm::vec3 min;
		glm::vec3 max;
	};
}

namespace at3::Mesh
{
	void setGlobalVertexLayout(std::vector<EMeshVertexAttribute> layout);

	const VertexRenderData* vertexRenderData();

	uint32_t make(MeshAsset& outAsset, VkhContext& ctxt, float* vertices, uint32_t vertexCount, uint32_t* indices, uint32_t indexCount);
	void quad(MeshAsset& outAsset, VkhContext& ctxt, float width = 2.0f, float height = 2.0f, float xOffset = 0.0f, float yOffset = 0.0f);
}
