#include "vkh_material.h"
#include "config.h"

namespace vkh
{
	GlobalShaderDataStore globalData;

	void initGlobalShaderData(VkhContext& ctxt)
	{
		static bool isInitialized = false;
		if (!isInitialized)
		{

			uint32_t structSize = static_cast<uint32_t>(sizeof(GlobalShaderData));
			size_t uboAlignment = ctxt.gpu.deviceProps.limits.minUniformBufferOffsetAlignment;

			globalData.size = (structSize / uboAlignment) * uboAlignment + ((structSize % uboAlignment) > 0 ? uboAlignment : 0);

			vkh::createBuffer(
				globalData.buffer,
				globalData.mem,
				globalData.size,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				ctxt);

			vkMapMemory(ctxt.device, globalData.mem.handle, globalData.mem.offset, globalData.size, 0, &globalData.mappedMemory);

			VkSamplerCreateInfo createInfo = vkh::samplerCreateInfo(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f);
			VkResult res = vkCreateSampler(ctxt.device, &createInfo, 0, &globalData.sampler);


			checkf(res == VK_SUCCESS, "Error creating global sampler");

			isInitialized = true;

		}
	}

	void createBasicMaterial(unsigned char *vertData, unsigned int vertLen, unsigned char *fragData, unsigned int fragLen,
													 VkhContext &ctxt, VkhMaterialCreateInfo &createInfo)
	{
		VkPipelineShaderStageCreateInfo shaderStages[2];

		shaderStages[0] = vkh::shaderPipelineStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT);
		vkh::createShaderModule(shaderStages[0].module, vertData, vertLen, ctxt);

		shaderStages[1] = vkh::shaderPipelineStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT);
		vkh::createShaderModule(shaderStages[1].module, fragData, fragLen, ctxt);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkh::pipelineLayoutCreateInfo(createInfo.descSetLayouts.data(), createInfo.descSetLayouts.size());

		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.offset = 0;
		pushConstantRange.size = createInfo.pushConstantRange;
		pushConstantRange.stageFlags = createInfo.pushConstantStages;

		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		pipelineLayoutInfo.pushConstantRangeCount = 1;

		VkResult res = vkCreatePipelineLayout(ctxt.device, &pipelineLayoutInfo, nullptr, createInfo.outPipelineLayout);
		checkf(res == VK_SUCCESS, "Error creating pipeline layout");

		const VertexRenderData* vertexLayout = vkh::Mesh::vertexRenderData();

		VkVertexInputBindingDescription bindingDescription = vkh::vertexInputBindingDescription(0, vertexLayout->vertexSize, VK_VERTEX_INPUT_RATE_VERTEX);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkh::pipelineVertexInputStateCreateInfo();
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = vertexLayout->attrCount;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = &vertexLayout->attrDescriptions[0];

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkh::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
		VkViewport viewport = vkh::viewport(0, 0, static_cast<float>(ctxt.swapChain.extent.width), static_cast<float>(ctxt.swapChain.extent.height),0.0f, 1.0f);
		VkRect2D scissor = vkh::rect2D(0, 0, ctxt.swapChain.extent.width, ctxt.swapChain.extent.height);
		VkPipelineViewportStateCreateInfo viewportState = vkh::pipelineViewportStateCreateInfo(&viewport, 1, &scissor, 1);

		VkPipelineRasterizationStateCreateInfo rasterizer = vkh::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
		VkPipelineMultisampleStateCreateInfo multisampling = vkh::pipelineMultisampleStateCreateInfo();

		VkPipelineColorBlendAttachmentState colorBlendAttachment = vkh::pipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlending = vkh::pipelineColorBlendStateCreateInfo(colorBlendAttachment);

		VkPipelineDepthStencilStateCreateInfo depthStencil = vkh::pipelineDepthStencilStateCreateInfo(
			VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_LESS_OR_EQUAL);

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // Optional
		pipelineInfo.layout = *createInfo.outPipelineLayout;
		pipelineInfo.renderPass = createInfo.renderPass;
		pipelineInfo.pDepthStencilState = &depthStencil;

		pipelineInfo.subpass = 0;

		//can use this to create new pipelines by deriving from old ones
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		res = vkCreateGraphicsPipelines(ctxt.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, createInfo.outPipeline);
		checkf(res == VK_SUCCESS, "Error creating graphics pipeline");

		vkDestroyShaderModule(ctxt.device, shaderStages[0].module, nullptr);
		vkDestroyShaderModule(ctxt.device, shaderStages[1].module, nullptr);

	}
}
