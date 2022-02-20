#ifndef BUILDERVK_H
#define BUILDERVK_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

using std::vector;


namespace BuilderVk {


	inline VkComputePipelineCreateInfo computePipelineCreateInfo(
		VkPipelineShaderStageCreateInfo const &stageInfo,
		VkPipelineLayout const &layout
	) {
		VkComputePipelineCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = 0;
		info.stage = stageInfo;
		info.layout = layout;
		info.basePipelineHandle = VK_NULL_HANDLE;
		info.basePipelineIndex = -1;

		return info;
	}


	inline VkImageCopy imageCopy(
		uint32_t const width,
		uint32_t const height
	) {
		VkImageCopy imageCopy = {};
		imageCopy.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		imageCopy.srcOffset = { 0, 0, 0 };
		imageCopy.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		imageCopy.dstOffset = { 0, 0, 0 };
		imageCopy.extent = { width, height, 1 };

		return imageCopy;
	}


	inline VkImageCreateInfo imageCreateInfo2D(
		VkFormat const &format,
		uint32_t const width, uint32_t const height,
		VkSampleCountFlagBits const &samples,
		VkImageUsageFlags const &usage,
		VkImageLayout const &layout
	) {
		VkImageCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.format = format;
		createInfo.extent = { width, height, 1 };
		createInfo.mipLevels = 1;
		createInfo.arrayLayers = 1;
		createInfo.samples = samples;
		createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		createInfo.usage = usage;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
		createInfo.initialLayout = layout;

		return createInfo;
	}


	inline VkImageSubresourceRange imageSubresourceRange(
		VkImageAspectFlags const &aspectFlags
	) {
		VkImageSubresourceRange range {};
		range.aspectMask = aspectFlags;
		range.baseMipLevel = 0;
		range.levelCount = VK_REMAINING_MIP_LEVELS;
		range.baseArrayLayer = 0;
		range.layerCount = VK_REMAINING_ARRAY_LAYERS;

		return range;
	}


	inline VkImageMemoryBarrier imageMemoryBarrier(
		VkImage const &image,
		VkImageAspectFlags const &aspectFlags,
		VkAccessFlags const &srcAccessMask,
		VkAccessFlags const &dstAccessMask,
		VkImageLayout const &oldLayout,
		VkImageLayout const &newLayout,
		uint32_t const srcQueueFamilyIndex,
		uint32_t const dstQueueFamilyIndex
	) {
		VkImageSubresourceRange range = imageSubresourceRange( aspectFlags );

		VkImageMemoryBarrier barrier {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext = nullptr;
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
		barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
		barrier.image = image;
		barrier.subresourceRange = range;

		return barrier;
	}


	inline VkImageMemoryBarrier imageMemoryBarrier(
		VkImage const &image,
		VkAccessFlags const &srcAccessMask,
		VkAccessFlags const &dstAccessMask,
		VkImageLayout const &oldLayout,
		VkImageLayout const &newLayout,
		uint32_t const srcQueueFamilyIndex,
		uint32_t const dstQueueFamilyIndex
	) {
		return imageMemoryBarrier(
			image, VK_IMAGE_ASPECT_COLOR_BIT,
			srcAccessMask, dstAccessMask,
			oldLayout, newLayout,
			srcQueueFamilyIndex, dstQueueFamilyIndex
		);
	}


	inline VkImageViewCreateInfo imageViewCreateInfo2D(
		VkImage const &image,
		VkFormat const &format
	) {
		VkComponentMapping components = {};
		components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		VkImageSubresourceRange range = imageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT );

		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = 0;
		info.image = image;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = format;
		info.components = components;
		info.subresourceRange = range;

		return info;
	}


	inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
		vector<VkDescriptorSetLayout> const &descSetLayouts
	) {
		VkPipelineLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = 0;
		info.setLayoutCount = descSetLayouts.size();
		info.pSetLayouts = descSetLayouts.data();
		info.pushConstantRangeCount = 0;
		info.pPushConstantRanges = nullptr;

		return info;
	}


	inline VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(
		VkShaderStageFlagBits const &stage,
		VkShaderModule const &module,
		char const *pName
	) {
		VkPipelineShaderStageCreateInfo stageInfo {};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.pNext = nullptr;
		stageInfo.flags = 0;
		stageInfo.stage = stage;
		stageInfo.module = module;
		stageInfo.pName = pName;
		stageInfo.pSpecializationInfo = nullptr;

		return stageInfo;
	}


	inline VkSemaphoreCreateInfo semaphoreCreateInfo() {
		VkSemaphoreCreateInfo info {};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = 0;

		return info;
	}


	inline VkSubmitInfo submitInfo(
		vector<VkSemaphore> const &waitSemaphores,
		vector<VkPipelineStageFlags> const &flags,
		vector<VkCommandBuffer> const &commandBuffers,
		vector<VkSemaphore> const &signalSemaphores
	) {
		VkSubmitInfo info {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.pNext = nullptr;
		info.waitSemaphoreCount = waitSemaphores.size();
		info.pWaitSemaphores = waitSemaphores.data();
		info.pWaitDstStageMask = flags.data();
		info.commandBufferCount = commandBuffers.size();
		info.pCommandBuffers = commandBuffers.data();
		info.signalSemaphoreCount = signalSemaphores.size();
		info.pSignalSemaphores = signalSemaphores.data();

		return info;
	}


	inline VkSubmitInfo submitInfo(
		VkSemaphore const &waitSemaphore,
		VkPipelineStageFlags const &flags,
		VkCommandBuffer const &commandBuffer,
		VkSemaphore const &signalSemaphore
	) {
		VkSubmitInfo info {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.pNext = nullptr;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &waitSemaphore;
		info.pWaitDstStageMask = &flags;
		info.commandBufferCount = 1;
		info.pCommandBuffers = &commandBuffer;
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &signalSemaphore;

		return info;
	}


}

#endif
