#include "ModelRenderer.h"

using std::array;
using std::vector;


/**
 *
 */
void ModelRenderer::createCommandBuffers() {
	VkResult result;
	mCommandBuffers.resize( mVH->mFramebuffers.size() );

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = mCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t) mCommandBuffers.size();

	result = vkAllocateCommandBuffers( mVH->mLogicalDevice, &allocInfo, mCommandBuffers.data() );
	VulkanHandler::checkVkResult(
		result, "Failed to allocate command buffers.", "ModelRenderer"
	);

	vector<float> vertices = mObjParser->getVertices();
	uint32_t numIndices = static_cast<uint32_t>( vertices.size() );

	for( size_t i = 0; i < mCommandBuffers.size(); i++ ) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		result = vkBeginCommandBuffer( mCommandBuffers[i], &beginInfo );
		VulkanHandler::checkVkResult(
			result, "Failed to begin command buffer.", "ModelRenderer"
		);

		array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = mRenderPass;
		renderPassInfo.framebuffer = mVH->mFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = mVH->mSwapchainExtent;
		renderPassInfo.clearValueCount = static_cast<uint32_t>( clearValues.size() );
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass( mCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
		vkCmdBindPipeline( mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline );

		VkBuffer vertexBuffers[] = { mVertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers( mCommandBuffers[i], 0, 1, vertexBuffers, offsets );

		vkCmdBindIndexBuffer( mCommandBuffers[i], mIndexBuffer, 0, VK_INDEX_TYPE_UINT32 );

		vkCmdBindDescriptorSets(
			mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
			mPipelineLayout, 0, 1, &mDescriptorSets[i], 0, nullptr
		);

		vkCmdDrawIndexed( mCommandBuffers[i], numIndices, 1, 0, 0, 0 );

		vkCmdEndRenderPass( mCommandBuffers[i] );

		result = vkEndCommandBuffer( mCommandBuffers[i] );
		VulkanHandler::checkVkResult(
			result, "Failed to end command buffer.", "ModelRenderer"
		);
	}
}


/**
 *
 */
void ModelRenderer::createCommandPool() {
	VkResult result;

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = mVH->mFamilyIndexGraphics;

	result = vkCreateCommandPool( mVH->mLogicalDevice, &poolInfo, nullptr, &mCommandPool );
	VulkanHandler::checkVkResult(
		result, "Failed to create VkCommandPool.", "ModelRenderer"
	);
}


/**
 *
 */
void ModelRenderer::createDescriptorPool() {
	VkResult result;
	uint32_t numImages = static_cast<uint32_t>( mVH->mSwapchainImages.size() );

	array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = numImages;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = numImages;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>( poolSizes.size() );
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = numImages;

	result = vkCreateDescriptorPool( mVH->mLogicalDevice, &poolInfo, nullptr, &mDescriptorPool );
	VulkanHandler::checkVkResult(
		result, "Failed to create VkDescriptorPool", "ModelRenderer"
	);
}


/**
 *
 */
void ModelRenderer::createDescriptorSets() {
	VkResult result;
	uint32_t numImages = static_cast<uint32_t>( mVH->mSwapchainImages.size() );
	vector<VkDescriptorSetLayout> layouts( numImages, mDescriptorSetLayout );

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = mDescriptorPool;
	allocInfo.descriptorSetCount = numImages;
	allocInfo.pSetLayouts = layouts.data();

	mDescriptorSets.resize( numImages );
	result = vkAllocateDescriptorSets( mVH->mLogicalDevice, &allocInfo, mDescriptorSets.data() );
	VulkanHandler::checkVkResult(
		result, "Failed to allocate DescriptorSets", "ModelRenderer"
	);

	for( size_t i = 0; i < numImages; i++ ) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = mUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof( UniformBufferObject );

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = mDescriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets( mVH->mLogicalDevice, 1, &descriptorWrite, 0, nullptr );
	}
}


/**
 *
 * @param {uint32_t} frameIndex - The current frame/image index.
 */
void ModelRenderer::draw( uint32_t frameIndex ) {
	this->updateUniformBuffers( frameIndex );
}


/**
 *
 * @param {VulkanHandler*} vh
 * @param {ObjParser*}     op
 */
void ModelRenderer::setup( VulkanHandler* vh, ObjParser* op ) {
	mVH = vh;
	mObjParser = op;

	VkShaderModule vertModule;
	VkShaderModule fragModule;

	this->createShaders( &vertModule, &fragModule );
	this->createRenderPass();
	this->createCommandPool();
	this->createCommandBuffers();
	this->createDescriptorPool();
	this->createDescriptorSets();
	this->createPipeline( &vertModule, &fragModule );

	vkDestroyShaderModule( mVH->mLogicalDevice, vertModule, nullptr );
	vkDestroyShaderModule( mVH->mLogicalDevice, fragModule, nullptr );

	Logger::logDebug( "[ModelRenderer] Setup done." );
}


/**
 *
 */
void ModelRenderer::teardown() {
	if( mDescriptorSetLayout != VK_NULL_HANDLE ) {
		vkDestroyDescriptorSetLayout( mVH->mLogicalDevice, mDescriptorSetLayout, nullptr );
		mDescriptorSetLayout = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkDescriptorSetLayout destroyed." );
	}

	if( mDescriptorPool != VK_NULL_HANDLE ) {
		vkDestroyDescriptorPool( mVH->mLogicalDevice, mDescriptorPool, nullptr );
		mDescriptorPool = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkDescriptorPool destroyed." );
	}

	const uint32_t numBuffersMemory = mUniformBuffersMemory.size();

	for( size_t i = 0; i < numBuffersMemory; i++ ) {
		VkDeviceMemory uniformBuffer = mUniformBuffersMemory[i];

		if( uniformBuffer != VK_NULL_HANDLE ) {
			vkFreeMemory( mVH->mLogicalDevice, uniformBuffer, nullptr );
			uniformBuffer = VK_NULL_HANDLE;

			char msg[256];
			snprintf( msg, 256, "[VulkanHandler] VkDeviceMemory (uniform) freed (%d/%d).", (int) i + 1, numBuffersMemory );
			Logger::logDebugVerbose( msg );
		}
	}

	const uint32_t numBuffers = mUniformBuffers.size();

	for( size_t i = 0; i < numBuffers; i++ ) {
		VkBuffer uniformBuffer = mUniformBuffers[i];

		if( uniformBuffer != VK_NULL_HANDLE ) {
			vkDestroyBuffer( mVH->mLogicalDevice, uniformBuffer, nullptr );
			uniformBuffer = VK_NULL_HANDLE;

			char msg[256];
			snprintf( msg, 256, "[VulkanHandler] VkBuffer (uniform) freed (%d/%d).", (int) i + 1, numBuffers );
			Logger::logDebugVerbose( msg );
		}
	}

	if( mVertexBufferMemory != VK_NULL_HANDLE ) {
		vkFreeMemory( mVH->mLogicalDevice, mVertexBufferMemory, nullptr );
		mVertexBufferMemory = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkDeviceMemory (vertices) freed." );
	}

	if( mVertexBuffer != VK_NULL_HANDLE ) {
		vkDestroyBuffer( mVH->mLogicalDevice, mVertexBuffer, nullptr );
		mVertexBuffer = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkBuffer (vertices) destroyed." );
	}

	if( mCommandPool != VK_NULL_HANDLE ) {
		vkDestroyCommandPool( mVH->mLogicalDevice, mCommandPool, nullptr );
		mCommandPool = VK_NULL_HANDLE;
		Logger::logDebug( "[VulkanHandler] VkCommandPool destroyed." );
	}

	if( mGraphicsPipeline != VK_NULL_HANDLE ) {
		vkDestroyPipeline( mVH->mLogicalDevice, mGraphicsPipeline, nullptr );
		mGraphicsPipeline = VK_NULL_HANDLE;
		Logger::logDebug( "[VulkanHandler] VkPipeline (graphics) destroyed." );
	}

	if( mPipelineLayout != VK_NULL_HANDLE ) {
		vkDestroyPipelineLayout( mVH->mLogicalDevice, mPipelineLayout, nullptr );
		mPipelineLayout = VK_NULL_HANDLE;
		Logger::logDebug( "[VulkanHandler] VkPipelineLayout destroyed." );
	}

	if( mRenderPass != VK_NULL_HANDLE ) {
		vkDestroyRenderPass( mVH->mLogicalDevice, mRenderPass, nullptr );
		mRenderPass = VK_NULL_HANDLE;
		Logger::logDebug( "[VulkanHandler] VkRenderPass destroyed." );
	}
}


/**
 * Update the uniform buffers.
 * @param {uint32_t} frameIndex
 */
void ModelRenderer::updateUniformBuffers( uint32_t frameIndex ) {
	UniformBufferObject ubo = {};
	ubo.model = glm::mat4( 1.0f );
	ubo.view = glm::lookAt(
		glm::vec3( 2.0f, 2.0f, 2.0f ),
		glm::vec3( 0.0f, 0.0f, 0.0f ),
		glm::vec3( 0.0f, 0.0f, 1.0f )
	);
	ubo.proj = glm::perspective(
		glm::radians( 45.0f ),
		mVH->mSwapchainExtent.width / (float) mVH->mSwapchainExtent.height, 0.1f, 100.0f
	);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory( mVH->mLogicalDevice, mUniformBuffersMemory[frameIndex], 0, sizeof( ubo ), 0, &data );
	memcpy( data, &ubo, sizeof( ubo ) );
	vkUnmapMemory( mVH->mLogicalDevice, mUniformBuffersMemory[frameIndex] );
}
