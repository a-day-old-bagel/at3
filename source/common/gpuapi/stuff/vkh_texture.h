#pragma once
#include "vkh.h"
#include <cstdint>
#include <stb/stb_image.h>

namespace at3
{
	struct TextureAsset
	{
		VkImage image;
		at3::Allocation deviceMemory;
		VkImageView view;
		VkFormat format;

		uint32_t width;
		uint32_t height;
		uint32_t numChannels;
	};
}

namespace at3::Texture
{
	void make(TextureAsset& outAsset, const char* filepath, VkhContext& ctxt)
	{
		TextureAsset& t = outAsset;

		int texWidth, texHeight, texChannels;

		//STBI_rgb_alpha forces an alpha even if the image doesn't have one
		stbi_uc* pixels = stbi_load(filepath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		checkf(pixels, "Could not load image");

		VkDeviceSize imageSize = texWidth * texHeight * 4;

		VkBuffer stagingBuffer;
		at3::Allocation stagingBufferMemory;

		createBuffer(stagingBuffer,
			stagingBufferMemory,
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			ctxt);

		void* data;
		vkMapMemory(ctxt.device, stagingBufferMemory.handle, stagingBufferMemory.offset, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(ctxt.device, stagingBufferMemory.handle);

		stbi_image_free(pixels);

		t.width = texWidth;
		t.height = texHeight;
		t.numChannels = texChannels;
		t.format = VK_FORMAT_R8G8B8A8_UNORM;

		//VK image format must match buffer
		createImage(t.image,
			t.width, t.height,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			ctxt);

		allocMemoryForImage(t.deviceMemory, t.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ctxt);
		vkBindImageMemory(ctxt.device, t.image, t.deviceMemory.handle, t.deviceMemory.offset);

		at3::transitionImageLayout(t.image, t.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ctxt);
		at3::copyBufferToImage(stagingBuffer, t.image, t.width, t.height, ctxt);

		at3::transitionImageLayout(t.image, t.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, ctxt);

		at3::createImageView(t.view, t.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, t.image, ctxt.device);

		vkDestroyBuffer(ctxt.device, stagingBuffer, nullptr);
		at3::freeDeviceMemory(stagingBufferMemory);
	}
}
