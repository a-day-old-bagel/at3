#pragma once
#include "vkh.h"
#include "vkh_initializers.h"
#include "vkh_mesh.h"
#include <vector>


/*
 * This macro was found at https://stackoverflow.com/questions/7895869
 */
#if defined(_MSC_VER)
#define ALIGNED_(x) __declspec(align(x))
#else
#if defined(__GNUC__)
#define ALIGNED_(x) __attribute__ ((aligned(x)))
#endif
#endif


namespace vkh
{
	struct GlobalShaderData
	{
		ALIGNED_(16) glm::float32 time;
		ALIGNED_(16) glm::float32 lightIntensity;
		ALIGNED_(16) glm::vec2 resolution;
		ALIGNED_(16) glm::vec4 lightDir;
		ALIGNED_(16) glm::vec3 lightCol;
		ALIGNED_(16) glm::vec4 mouse;
		ALIGNED_(16) glm::vec4 worldSpaceCameraPos;
		ALIGNED_(16) glm::vec4 vpMatrix;
	};

	struct GlobalShaderDataStore
	{
		//storage for global shader data
		vkh::Allocation			mem;
		VkBuffer				buffer;
		GlobalShaderData		shaderData;
		uint32_t				size;
		void*					mappedMemory;

		VkDescriptorSetLayout	layout;
		VkDescriptorSet			descSet;
		VkSampler				sampler;
	};

	struct VkhMaterialCreateInfo
	{
		VkRenderPass renderPass;

		std::vector<VkDescriptorSetLayout> descSetLayouts;
		VkPipelineLayout* outPipelineLayout;
		VkPipeline* outPipeline;
		uint32_t pushConstantRange;
		VkShaderStageFlagBits pushConstantStages;
	};

	void initGlobalShaderData(VkhContext& ctxt);
	void createBasicMaterial(unsigned char *vertData, unsigned int vertLen, unsigned char *fragData, unsigned int fragLen,
													 VkhContext &ctxt, VkhMaterialCreateInfo &createInfo);
}
