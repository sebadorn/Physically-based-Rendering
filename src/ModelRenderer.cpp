#include "ModelRenderer.h"

using std::array;
using std::vector;


/**
 *
 */
void ModelRenderer::createCommandBuffers() {
	VkResult result;
	mCommandBuffers.resize( mPathTracer->mFramebuffers.size() );

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = mCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t) mCommandBuffers.size();

	result = vkAllocateCommandBuffers( mPathTracer->mLogicalDevice, &allocInfo, mCommandBuffers.data() );
	VulkanSetup::checkVkResult(
		result, "Failed to allocate command buffers.", "ModelRenderer"
	);

	vector<float> vertices = mObjParser->getVertices();
	uint32_t numIndices = static_cast<uint32_t>( vertices.size() );

	for( size_t i = 0; i < mCommandBuffers.size(); i++ ) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		result = vkBeginCommandBuffer( mCommandBuffers[i], &beginInfo );
		VulkanSetup::checkVkResult(
			result, "Failed to begin command buffer.", "ModelRenderer"
		);

		array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = mRenderPass;
		renderPassInfo.framebuffer = mPathTracer->mFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = mPathTracer->mSwapchainExtent;
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
		VulkanSetup::checkVkResult(
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
	poolInfo.queueFamilyIndex = mPathTracer->mFamilyIndexGraphics;

	result = vkCreateCommandPool( mPathTracer->mLogicalDevice, &poolInfo, nullptr, &mCommandPool );
	VulkanSetup::checkVkResult(
		result, "Failed to create VkCommandPool.", "ModelRenderer"
	);
}


/**
 *
 */
void ModelRenderer::createDescriptorPool() {
	VkResult result;
	uint32_t numImages = static_cast<uint32_t>( mPathTracer->mSwapchainImages.size() );

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

	result = vkCreateDescriptorPool( mPathTracer->mLogicalDevice, &poolInfo, nullptr, &mDescriptorPool );
	VulkanSetup::checkVkResult(
		result, "Failed to create VkDescriptorPool", "ModelRenderer"
	);

	Logger::logDebug( "[ModelRenderer] Created VkDescriptorPool." );
}


/**
 *
 */
void ModelRenderer::createDescriptorSets() {
	VkResult result;
	uint32_t numImages = static_cast<uint32_t>( mPathTracer->mSwapchainImages.size() );
	vector<VkDescriptorSetLayout> layouts( numImages, mDescriptorSetLayout );

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = mDescriptorPool;
	allocInfo.descriptorSetCount = numImages;
	allocInfo.pSetLayouts = layouts.data();

	mDescriptorSets.resize( numImages );
	result = vkAllocateDescriptorSets( mPathTracer->mLogicalDevice, &allocInfo, mDescriptorSets.data() );
	VulkanSetup::checkVkResult(
		result, "Failed to allocate DescriptorSets", "ModelRenderer"
	);

	// // This part crashes -> Segmentation fault
	// // mUniformBuffers has no data yet?
	// for( size_t i = 0; i < numImages; i++ ) {
	// 	VkDescriptorBufferInfo bufferInfo = {};
	// 	bufferInfo.buffer = mUniformBuffers[i];
	// 	bufferInfo.offset = 0;
	// 	bufferInfo.range = sizeof( UniformBufferObject );

	// 	VkWriteDescriptorSet descriptorWrite = {};
	// 	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	// 	descriptorWrite.dstSet = mDescriptorSets[i];
	// 	descriptorWrite.dstBinding = 0;
	// 	descriptorWrite.dstArrayElement = 0;
	// 	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	// 	descriptorWrite.descriptorCount = 1;
	// 	descriptorWrite.pBufferInfo = &bufferInfo;

	// 	vkUpdateDescriptorSets( mPathTracer->mLogicalDevice, 1, &descriptorWrite, 0, nullptr );
	// }

	Logger::logDebug( "[ModelRenderer] Created VkDescriptorSets." );
}


/**
 * Create the graphics pipeline.
 * @param {VkShaderModule*} vertModule
 * @param {VkShaderModule*} fragModule
 */
void ModelRenderer::createGraphicsPipeline( VkShaderModule* vertModule, VkShaderModule* fragModule ) {
	// Destroy old graphics pipeline.
	if( mGraphicsPipeline != VK_NULL_HANDLE ) {
		vkDestroyPipeline( mPathTracer->mLogicalDevice, mGraphicsPipeline, nullptr );
	}

	// Destroy old pipeline layout.
	if( mPipelineLayout != VK_NULL_HANDLE ) {
		vkDestroyPipelineLayout( mPathTracer->mLogicalDevice, mPipelineLayout, nullptr );
	}

	mPipelineLayout = VulkanSetup::createPipelineLayout( &mPathTracer->mLogicalDevice, &mDescriptorSetLayout );

	VkPipelineShaderStageCreateInfo vertShaderCreateInfo = {};
	vertShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderCreateInfo.module = *vertModule;
	vertShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderCreateInfo = {};
	fragShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderCreateInfo.module = *fragModule;
	fragShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {
		vertShaderCreateInfo,
		fragShaderCreateInfo
	};

	mGraphicsPipeline = VulkanSetup::createGraphicsPipeline(
		&mPathTracer->mLogicalDevice, &mPipelineLayout, &mRenderPass, shaderStages, &mPathTracer->mSwapchainExtent
	);
}


/**
 * Create the VkRenderPass.
 */
void ModelRenderer::createRenderPass() {
	// Destroy old render pass.
	if( mRenderPass != VK_NULL_HANDLE ) {
		vkDestroyRenderPass( mPathTracer->mLogicalDevice, mRenderPass, nullptr );
	}

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = mPathTracer->mSwapchainFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subPass = {};
	subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPass.colorAttachmentCount = 1;
	subPass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subPass;

	// NOTE: Not necessary?

	// VkSubpassDependency dependency = {};
	// dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	// dependency.dstSubpass = 0;
	// dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	// dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	// dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// renderPassInfo.dependencyCount = 1;
	// renderPassInfo.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass(
		mPathTracer->mLogicalDevice, &renderPassInfo, nullptr, &mRenderPass
	);
	VulkanSetup::checkVkResult( result, "Failed to create VkRenderPass." );
	Logger::logInfo( "[ModelRenderer] Created VkRenderPass." );
}


/**
 *
 * @param {VkShaderModule*} vertModule
 * @param {VkShaderModule*} fragModule
 * @param {VkShaderModule*} computeModule
 */
void ModelRenderer::createShaders( VkShaderModule* vertModule, VkShaderModule* fragModule, VkShaderModule* computeModule ) {
	auto vertShaderCode = PathTracer::loadFileSPV( "src/shaders/vert.spv" );
	auto fragShaderCode = PathTracer::loadFileSPV( "src/shaders/frag.spv" );
	auto computeShaderCode = PathTracer::loadFileSPV( "src/shaders/compute.spv" );
	Logger::logDebug( "[ModelRenderer] Loaded shader files." );

	VkDevice logicalDevice = mPathTracer->mLogicalDevice;
	*vertModule = VulkanSetup::createShaderModule( &logicalDevice, vertShaderCode );
	*fragModule = VulkanSetup::createShaderModule( &logicalDevice, fragShaderCode );
	*computeModule = VulkanSetup::createShaderModule( &logicalDevice, computeShaderCode );
	Logger::logInfo( "[ModelRenderer] Created shader modules." );
}


/**
 *
 * @param {uint32_t} frameIndex - The current frame/image index.
 */
void ModelRenderer::draw( uint32_t frameIndex ) {
	// this->updateUniformBuffers( frameIndex );
}


/**
 *
 * @param {PathTracer*} pt
 * @param {ObjParser*}  op
 */
void ModelRenderer::setup( PathTracer* pt, ObjParser* op ) {
	Logger::logDebug( "[ModelRenderer] Setup begin." );

	mPathTracer = pt;
	mObjParser = op;

	VkShaderModule vertModule;
	VkShaderModule fragModule;
	VkShaderModule computeModule;

	mDescriptorSetLayout = VulkanSetup::createDescriptorSetLayout( &( mPathTracer->mLogicalDevice ) );
	// this->createRenderPass();
	this->createShaders( &vertModule, &fragModule, &computeModule );
	// this->createDescriptorPool();
	// this->createDescriptorSets();
	// this->createGraphicsPipeline( &vertModule, &fragModule );
	// this->createCommandPool();
	// this->createCommandBuffers();


	//

	VkResult result;
	const uint32_t computeImageBinding = 0;
	const uint32_t imageCount = static_cast<uint32_t>( mPathTracer->mSwapchainImages.size() );

	int w, h;
	glfwGetFramebufferSize( mPathTracer->mWindow, &w, &h );


	VkDescriptorSetLayoutBinding computeDSLayoutBinding {
		computeImageBinding,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		1,
		VK_SHADER_STAGE_COMPUTE_BIT,
		nullptr
	};

	VkDescriptorSetLayoutCreateInfo dsLayoutInfo {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		1,
		&computeDSLayoutBinding
	};

	VkDescriptorSetLayout computeDSLayout;
	result = vkCreateDescriptorSetLayout( mPathTracer->mLogicalDevice, &dsLayoutInfo, nullptr, &computeDSLayout );
	VulkanSetup::checkVkResult( result, "Failed to create compute descriptor set layout." );


	VkDescriptorPoolSize poolSize {
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		imageCount
	};

	VkDescriptorPoolCreateInfo dPoolInfo {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		nullptr,
		0,
		imageCount,
		1,
		&poolSize
	};

	VkDescriptorPool computeDPool;
	result = vkCreateDescriptorPool( mPathTracer->mLogicalDevice, &dPoolInfo, nullptr, &computeDPool );
	VulkanSetup::checkVkResult( result, "Failed to create compute descriptor pool." );


	std::vector<VkDescriptorSetLayout> dsLayouts( imageCount, computeDSLayout );

	VkDescriptorSetAllocateInfo dsInfo {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		computeDPool,
		static_cast<uint32_t>( dsLayouts.size() ),
		dsLayouts.data()
	};

	std::vector<VkDescriptorSet> computeDSets( dsLayouts.size() );
	result = vkAllocateDescriptorSets( mPathTracer->mLogicalDevice, &dsInfo, computeDSets.data() );
	VulkanSetup::checkVkResult( result, "Failed to allocate compute descriptor sets." );


	VkPipelineLayoutCreateInfo pipelineLayoutInfo {
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		1,
		&computeDSLayout,
		0,
		nullptr
	};

	VkPipelineLayout computePipelineLayout;
	result = vkCreatePipelineLayout( mPathTracer->mLogicalDevice, &pipelineLayoutInfo, nullptr, &computePipelineLayout );
	VulkanSetup::checkVkResult( result, "Failed to create compute pipeline layout." );


	const VkPipelineShaderStageCreateInfo computeShaderStage {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		nullptr,
		0,
		VK_SHADER_STAGE_COMPUTE_BIT,
		computeModule,
		u8"main",
		nullptr
	};

	VkComputePipelineCreateInfo pipelineInfo {
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		nullptr,
		0,
		computeShaderStage,
		computePipelineLayout,
		VK_NULL_HANDLE,
		-1
	};

	VkPipeline computePipeline;
	result = vkCreateComputePipelines( mPathTracer->mLogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline );
	VulkanSetup::checkVkResult( result, "Failed to create compute pipeline." );


	VkSemaphoreCreateInfo semaphoreInfo {
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		nullptr,
		0
	};

	VkSemaphore computeDoneSemaphore;
	result = vkCreateSemaphore( mPathTracer->mLogicalDevice, &semaphoreInfo, nullptr, &computeDoneSemaphore );
	VulkanSetup::checkVkResult( result, "Failed to create semaphore for compute step." );


	VkCommandPoolCreateInfo commandPoolInfo {
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr,
		0,
		static_cast<uint32_t>( mPathTracer->mFamilyIndexCompute )
	};

	VkCommandPool computeCommandPool;
	result = vkCreateCommandPool( mPathTracer->mLogicalDevice, &commandPoolInfo, nullptr, &computeCommandPool );
	VulkanSetup::checkVkResult( result, "Failed to create compute command pool." );


	VkCommandBufferAllocateInfo commandBufferInfo {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		computeCommandPool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		imageCount
	};

	std::vector<VkCommandBuffer> computeCommandBuffers( imageCount );
	result = vkAllocateCommandBuffers( mPathTracer->mLogicalDevice, &commandBufferInfo, computeCommandBuffers.data() );
	VulkanSetup::checkVkResult( result, "Failed to allocate compute command buffers." );

	for( size_t i = 0; i < imageCount; i++ ) {
		// update descriptor set
		VkDescriptorImageInfo imageInfo {
			VK_NULL_HANDLE,
			mPathTracer->mSwapchainImageViews[i],
			VK_IMAGE_LAYOUT_GENERAL
		};

		VkWriteDescriptorSet descriptorWrite {
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			nullptr,
			computeDSets[i],
			computeImageBinding,
			0,
			1,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			&imageInfo,
			nullptr,
			nullptr
		};

		vkUpdateDescriptorSets( mPathTracer->mLogicalDevice, 1, &descriptorWrite, 0, nullptr );

		// begin command buffer
		VkCommandBufferBeginInfo commandBufferInfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
			nullptr
		};

		result = vkBeginCommandBuffer( computeCommandBuffers[i], &commandBufferInfo );
		VulkanSetup::checkVkResult( result, "Failed to begin compute command buffer." );

		// record
		vkCmdBindPipeline( computeCommandBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline );
		vkCmdBindDescriptorSets(
			computeCommandBuffers[i],
			VK_PIPELINE_BIND_POINT_COMPUTE,
			computePipelineLayout,
			0,
			static_cast<uint32_t>( computeDSets.size() ),
			computeDSets.data(),
			0,
			nullptr
		);

		// {
		// 	VkImageSubresourceRange range {
		// 		VK_IMAGE_ASPECT_COLOR_BIT,
		// 		0, VK_REMAINING_MIP_LEVELS,
		// 		0, VK_REMAINING_ARRAY_LAYERS
		// 	};

		// 	VkImageMemoryBarrier imageBarrier {
		// 		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		// 		nullptr,
		// 		0,
		// 		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		// 		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		// 		VK_IMAGE_LAYOUT_GENERAL,
		// 		static_cast<uint32_t>( mPathTracer->mFamilyIndexGraphics ),
		// 		static_cast<uint32_t>( mPathTracer->mFamilyIndexCompute ),
		// 		mPathTracer->mSwapchainImages[i],
		// 		range
		// 	};

		// 	vkCmdPipelineBarrier(
		// 		computeCommandBuffers[i],
		// 		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		// 		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		// 		0, 0, nullptr, 0, nullptr, 1,
		// 		&imageBarrier
		// 	);
		// }

		vkCmdDispatch( computeCommandBuffers[i], w, h, 1 );

		{
			VkImageSubresourceRange range {
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, VK_REMAINING_MIP_LEVELS,
				0, VK_REMAINING_ARRAY_LAYERS
			};

			VkImageMemoryBarrier imageBarrier {
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				VK_ACCESS_SHADER_WRITE_BIT,
				0,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				static_cast<uint32_t>( mPathTracer->mFamilyIndexCompute ),
				static_cast<uint32_t>( mPathTracer->mFamilyIndexPresentation ),
				mPathTracer->mSwapchainImages[i],
				range
			};

			vkCmdPipelineBarrier(
				computeCommandBuffers[i],
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0, 0, nullptr, 0, nullptr, 1,
				&imageBarrier
			);
		}

		// end command buffer
		result = vkEndCommandBuffer( computeCommandBuffers[i] );
		VulkanSetup::checkVkResult( result, "Failed to end compute command buffer." );
	}


	vector<VkCommandBuffer> transferCommandBuffers( imageCount );
	result = vkAllocateCommandBuffers( mPathTracer->mLogicalDevice, &commandBufferInfo, transferCommandBuffers.data() );
	VulkanSetup::checkVkResult( result, "Failed to allocate transfer command buffers." );

	for( size_t i = 0; i < transferCommandBuffers.size(); i++ ) {
		// begin command buffer
		VkCommandBufferBeginInfo commandBufferInfo {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
			nullptr
		};

		result = vkBeginCommandBuffer( transferCommandBuffers[i], &commandBufferInfo );
		VulkanSetup::checkVkResult( result, "Failed to begin transfer command buffer." );

		{
			VkImageSubresourceRange range {
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, VK_REMAINING_MIP_LEVELS,
				0, VK_REMAINING_ARRAY_LAYERS
			};

			VkImageMemoryBarrier imageBarrier {
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				0,
				0,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				static_cast<uint32_t>( mPathTracer->mFamilyIndexCompute ),
				static_cast<uint32_t>( mPathTracer->mFamilyIndexPresentation ),
				mPathTracer->mSwapchainImages[i],
				range
			};

			vkCmdPipelineBarrier(
				transferCommandBuffers[i],
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0, 0, nullptr, 0, nullptr, 1,
				&imageBarrier
			);
		}

		// end command buffer
		result = vkEndCommandBuffer( transferCommandBuffers[i] );
		VulkanSetup::checkVkResult( result, "Failed to end transfer command buffer." );
	}

	//


	vkDestroyShaderModule( mPathTracer->mLogicalDevice, vertModule, nullptr );
	vkDestroyShaderModule( mPathTracer->mLogicalDevice, fragModule, nullptr );
	vkDestroyShaderModule( mPathTracer->mLogicalDevice, computeModule, nullptr );

	Logger::logDebug( "[ModelRenderer] Setup done." );
}


/**
 *
 */
void ModelRenderer::teardown() {
	if( mDescriptorSetLayout != VK_NULL_HANDLE ) {
		vkDestroyDescriptorSetLayout( mPathTracer->mLogicalDevice, mDescriptorSetLayout, nullptr );
		mDescriptorSetLayout = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ModelRenderer] VkDescriptorSetLayout destroyed." );
	}

	if( mDescriptorPool != VK_NULL_HANDLE ) {
		vkDestroyDescriptorPool( mPathTracer->mLogicalDevice, mDescriptorPool, nullptr );
		mDescriptorPool = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ModelRenderer] VkDescriptorPool destroyed." );
	}

	const uint32_t numBuffersMemory = mUniformBuffersMemory.size();

	for( size_t i = 0; i < numBuffersMemory; i++ ) {
		VkDeviceMemory uniformBuffer = mUniformBuffersMemory[i];

		if( uniformBuffer != VK_NULL_HANDLE ) {
			vkFreeMemory( mPathTracer->mLogicalDevice, uniformBuffer, nullptr );
			uniformBuffer = VK_NULL_HANDLE;

			char msg[256];
			snprintf( msg, 256, "[ModelRenderer] VkDeviceMemory (uniform) freed (%d/%d).", (int) i + 1, numBuffersMemory );
			Logger::logDebugVerbose( msg );
		}
	}

	const uint32_t numBuffers = mUniformBuffers.size();

	for( size_t i = 0; i < numBuffers; i++ ) {
		VkBuffer uniformBuffer = mUniformBuffers[i];

		if( uniformBuffer != VK_NULL_HANDLE ) {
			vkDestroyBuffer( mPathTracer->mLogicalDevice, uniformBuffer, nullptr );
			uniformBuffer = VK_NULL_HANDLE;

			char msg[256];
			snprintf( msg, 256, "[ModelRenderer] VkBuffer (uniform) freed (%d/%d).", (int) i + 1, numBuffers );
			Logger::logDebugVerbose( msg );
		}
	}

	if( mVertexBufferMemory != VK_NULL_HANDLE ) {
		vkFreeMemory( mPathTracer->mLogicalDevice, mVertexBufferMemory, nullptr );
		mVertexBufferMemory = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ModelRenderer] VkDeviceMemory (vertices) freed." );
	}

	if( mVertexBuffer != VK_NULL_HANDLE ) {
		vkDestroyBuffer( mPathTracer->mLogicalDevice, mVertexBuffer, nullptr );
		mVertexBuffer = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ModelRenderer] VkBuffer (vertices) destroyed." );
	}

	if( mCommandPool != VK_NULL_HANDLE ) {
		vkDestroyCommandPool( mPathTracer->mLogicalDevice, mCommandPool, nullptr );
		mCommandPool = VK_NULL_HANDLE;
		Logger::logDebug( "[ModelRenderer] VkCommandPool destroyed." );
	}

	if( mGraphicsPipeline != VK_NULL_HANDLE ) {
		vkDestroyPipeline( mPathTracer->mLogicalDevice, mGraphicsPipeline, nullptr );
		mGraphicsPipeline = VK_NULL_HANDLE;
		Logger::logDebug( "[ModelRenderer] VkPipeline (graphics) destroyed." );
	}

	if( mPipelineLayout != VK_NULL_HANDLE ) {
		vkDestroyPipelineLayout( mPathTracer->mLogicalDevice, mPipelineLayout, nullptr );
		mPipelineLayout = VK_NULL_HANDLE;
		Logger::logDebug( "[ModelRenderer] VkPipelineLayout destroyed." );
	}

	if( mRenderPass != VK_NULL_HANDLE ) {
		vkDestroyRenderPass( mPathTracer->mLogicalDevice, mRenderPass, nullptr );
		mRenderPass = VK_NULL_HANDLE;
		Logger::logDebug( "[ModelRenderer] VkRenderPass destroyed." );
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
	float ratio = (float) mPathTracer->mSwapchainExtent.width / (float) mPathTracer->mSwapchainExtent.height;
	ubo.proj = glm::perspective(
		glm::radians( 45.0f ),
		ratio, 0.1f, 100.0f
	);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory( mPathTracer->mLogicalDevice, mUniformBuffersMemory[frameIndex], 0, sizeof( ubo ), 0, &data );
	memcpy( data, &ubo, sizeof( ubo ) );
	vkUnmapMemory( mPathTracer->mLogicalDevice, mUniformBuffersMemory[frameIndex] );
}
