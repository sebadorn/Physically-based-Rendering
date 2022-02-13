#include "ComputeHandler.h"

using std::vector;


vector<VkCommandBuffer> ComputeHandler::allocateCommandBuffers(
	uint32_t numCmdBuffers,
	VkCommandPool cmdPool
) {
	VkCommandBufferAllocateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.pNext = nullptr;
	info.commandPool = cmdPool;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = numCmdBuffers;

	VkDevice device = mPathTracer->mLogicalDevice;
	vector<VkCommandBuffer> cmdBuffers( numCmdBuffers );
	VkResult result = vkAllocateCommandBuffers( device, &info, cmdBuffers.data() );

	VulkanSetup::checkVkResult(
		result,
		"Failed to allocate command buffers.",
		"ComputeHandler"
	);

	Logger::logInfof( "[ComputeHandler] Allocated %d command buffers.", numCmdBuffers );

	return cmdBuffers;
}


vector<VkDescriptorSet> ComputeHandler::allocateDescriptorSets(
	uint32_t numImages,
	VkDescriptorSetLayout layout,
	VkDescriptorPool pool
) {
	vector<VkDescriptorSetLayout> layouts( numImages, layout );

	VkDescriptorSetAllocateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.pNext = nullptr;
	info.descriptorPool = pool;
	info.descriptorSetCount = static_cast<uint32_t>( layouts.size() );
	info.pSetLayouts = layouts.data();

	VkDevice device = mPathTracer->mLogicalDevice;
	vector<VkDescriptorSet> descSets( layouts.size() );
	VkResult result = vkAllocateDescriptorSets( device, &info, descSets.data() );

	VulkanSetup::checkVkResult(
		result,
		"Failed to allocate compute descriptor sets.",
		"ComputeHandler"
	);

	Logger::logInfof( "[ComputeHandler] Allocated %d DescriptorSets.", descSets.size() );

	return descSets;
}


void ComputeHandler::beginCommandBuffer( VkCommandBuffer cmdBuffer ) {
	VkCommandBufferBeginInfo info {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pNext = nullptr;
	info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	info.pInheritanceInfo = nullptr;

	VkResult result = vkBeginCommandBuffer( cmdBuffer, &info );

	VulkanSetup::checkVkResult(
		result,
		"Failed to begin command buffer.",
		"ComputeHandler"
	);
}


VkCommandPool ComputeHandler::createCommandPool() {
	VkCommandPoolCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = 0;
	info.queueFamilyIndex = static_cast<uint32_t>( mPathTracer->mFamilyIndexCompute );

	VkDevice device = mPathTracer->mLogicalDevice;
	VkCommandPool cmdPool;
	VkResult result = vkCreateCommandPool( device, &info, nullptr, &cmdPool );

	VulkanSetup::checkVkResult(
		result,
		"Failed to create compute command pool.",
		"ComputeHandler"
	);

	Logger::logInfo( "[ComputeHandler] Created CommandPool." );

	return cmdPool;
}


VkDescriptorPool ComputeHandler::createDescriptorPool( uint32_t numImages ) {
	VkDescriptorPoolSize poolSize {};
	poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSize.descriptorCount = numImages;

	VkDescriptorPoolCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = 0;
	info.maxSets = numImages;
	info.poolSizeCount = 1;
	info.pPoolSizes = &poolSize;

	VkDevice device = mPathTracer->mLogicalDevice;
	VkDescriptorPool pool;
	VkResult result = vkCreateDescriptorPool( device, &info, nullptr, &pool );

	VulkanSetup::checkVkResult(
		result,
		"Failed to create descriptor pool.",
		"ComputeHandler"
	);

	Logger::logInfo( "[ComputeHandler] Created DescriptorPool." );

	return pool;
}


VkDescriptorSetLayout ComputeHandler::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding binding {};
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = 0;
	info.bindingCount = 1;
	info.pBindings = &binding;

	VkDevice device = mPathTracer->mLogicalDevice;
	VkDescriptorSetLayout layout;
	VkResult result = vkCreateDescriptorSetLayout( device, &info, nullptr, &layout );

	VulkanSetup::checkVkResult(
		result,
		"Failed to create descriptor set layout.",
		"ComputeHandler"
	);

	Logger::logInfo( "[ComputeHandler] Created DescriptorSetLayout." );

	return layout;
}


VkPipeline ComputeHandler::createPipeline( VkPipelineLayout layout, VkShaderModule shaderModule ) {
	// Some attributes are read-only.
	const VkPipelineShaderStageCreateInfo stageInfo {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
		nullptr,      // pNext
		0,            // flags
		VK_SHADER_STAGE_COMPUTE_BIT, // stage
		shaderModule, // module
		u8"main",     // pName
		nullptr       // pSpecializationInfo
	};

	VkComputePipelineCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = 0;
	info.stage = stageInfo;
	info.layout = layout;
	info.basePipelineHandle = VK_NULL_HANDLE;
	info.basePipelineIndex = -1;

	VkDevice device = mPathTracer->mLogicalDevice;
	VkPipeline pipe;
	VkResult result = vkCreateComputePipelines( device, VK_NULL_HANDLE, 1, &info, nullptr, &pipe );

	VulkanSetup::checkVkResult(
		result,
		"Failed to create pipeline.",
		"ComputeHandler"
	);

	Logger::logInfo( "[ComputeHandler] Created Pipeline." );

	return pipe;
}


VkPipelineLayout ComputeHandler::createPipelineLayout( VkDescriptorSetLayout descSetLayout ) {
	VkPipelineLayoutCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = 0;
	info.setLayoutCount = 1;
	info.pSetLayouts = &descSetLayout;
	info.pushConstantRangeCount = 0;
	info.pPushConstantRanges = nullptr;

	VkDevice device = mPathTracer->mLogicalDevice;
	VkPipelineLayout layout;
	VkResult result = vkCreatePipelineLayout( device, &info, nullptr, &layout );

	VulkanSetup::checkVkResult(
		result,
		"Failed to create pipeline layout.",
		"ComputeHandler"
	);

	Logger::logInfo( "[ComputeHandler] Created PipelineLayout." );

	return layout;
}


VkSemaphore ComputeHandler::createSemaphore() {
	VkSemaphoreCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = 0;

	VkDevice device = mPathTracer->mLogicalDevice;
	VkSemaphore semaphore;
	VkResult result = vkCreateSemaphore( device, &info, nullptr, &semaphore );

	VulkanSetup::checkVkResult(
		result,
		"Failed to create semaphore.",
		"ComputeHandler"
	);

	Logger::logInfo( "[ComputeHandler] Created Semaphore." );

	return semaphore;
}


VkShaderModule ComputeHandler::createShader() {
	auto shaderCode = PathTracer::loadFileSPV( "src/shaders/compute.spv" );
	Logger::logDebug( "[ComputeHandler] Loaded shader file." );

	VkDevice device = mPathTracer->mLogicalDevice;
	VkShaderModule computeModule = VulkanSetup::createShaderModule( &device, shaderCode );
	Logger::logInfo( "[ComputeHandler] Created shader module." );

	return computeModule;
}


void ComputeHandler::draw( uint32_t frameIndex ) {
	{
		VkCommandBuffer cmdBuffer = mCmdBuffers[frameIndex];
		VkPipelineStageFlags flags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

		VkSubmitInfo info {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.pNext = nullptr;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &( mPathTracer->mImageAvailableSemaphore );
		info.pWaitDstStageMask = &flags;
		info.pCommandBuffers = &cmdBuffer;
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &mSemaphoreComputeDone;

		VkResult result = vkQueueSubmit( mPathTracer->mComputeQueue, 1, &info, VK_NULL_HANDLE );
		VulkanSetup::checkVkResult(
			result,
			"Failed to submit command buffer to queue.",
			"ComputeHandler"
		);
	}

	{
		VkCommandBuffer cmdBufferTransfer = mCmdBuffersTransfer[frameIndex];
		VkPipelineStageFlags flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

		VkSubmitInfo info {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.pNext = nullptr;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &mSemaphoreComputeDone;
		info.pWaitDstStageMask = &flags;
		info.pCommandBuffers = &cmdBufferTransfer;
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &mSemaphoreTransferDone;

		VkResult result = vkQueueSubmit( mPathTracer->mPresentQueue, 1, &info, VK_NULL_HANDLE );
		VulkanSetup::checkVkResult(
			result,
			"Failed to submit transfer command buffer to queue.",
			"ComputeHandler"
		);
	}
}


void ComputeHandler::endCommandBuffer( VkCommandBuffer cmdBuffer ) {
	VkResult result = vkEndCommandBuffer( cmdBuffer );

	VulkanSetup::checkVkResult(
		result,
		"Failed to end compute command buffer.",
		"ComputeHandler"
	);
}


void ComputeHandler::initCommandBuffers() {
	int width;
	int height;
	glfwGetFramebufferSize( mPathTracer->mWindow, &width, &height );

	for( size_t i = 0; i < mCmdBuffers.size(); i++ ) {
		VkImage image = mPathTracer->mSwapchainImages[i];
		VkImageView imageView = mPathTracer->mSwapchainImageViews[i];

		VkDescriptorSet descSet = mDescSets[i];
		VkCommandBuffer cmdBuffer = mCmdBuffers[i];

		this->updateDescriptorSet( imageView, descSet );
		this->recordCommandBuffer( cmdBuffer, descSet, image, width, height );
	}

	Logger::logInfo( "[ComputeHandler] Initialized command buffers." );
}


void ComputeHandler::initCommandBuffersTransfer() {
	uint32_t computeQueueIndex = static_cast<uint32_t>( mPathTracer->mFamilyIndexCompute );
	uint32_t presentQueueIndex = static_cast<uint32_t>( mPathTracer->mFamilyIndexPresentation );

	for( size_t i = 0; i < mCmdBuffersTransfer.size(); i++ ) {
		VkImage image = mPathTracer->mSwapchainImages[i];
		VkCommandBuffer cmdBuffer = mCmdBuffersTransfer[i];

		this->beginCommandBuffer( cmdBuffer );

		VkImageSubresourceRange range {};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = VK_REMAINING_MIP_LEVELS;
		range.baseArrayLayer = 0;
		range.layerCount = VK_REMAINING_ARRAY_LAYERS;

		VkImageMemoryBarrier barrier {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext = nullptr;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.srcQueueFamilyIndex = computeQueueIndex;
		barrier.dstQueueFamilyIndex = presentQueueIndex;
		barrier.image = image;
		barrier.subresourceRange = range;

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0, 0, nullptr, 0, nullptr, 1,
			&barrier
		);

		this->endCommandBuffer( cmdBuffer );
	}

	Logger::logInfo( "[ComputeHandler] Initialized command buffers for transfer." );
}


void ComputeHandler::recordCommandBuffer(
	VkCommandBuffer cmdBuffer,
	VkDescriptorSet descSet,
	VkImage image,
	uint32_t width,
	uint32_t height
) {
	this->beginCommandBuffer( cmdBuffer );

	uint32_t computeQueueIndex = static_cast<uint32_t>( mPathTracer->mFamilyIndexCompute );
	uint32_t graphicsQueueIndex = static_cast<uint32_t>( mPathTracer->mFamilyIndexGraphics );
	uint32_t presentQueueIndex = static_cast<uint32_t>( mPathTracer->mFamilyIndexPresentation );

	vkCmdBindPipeline( cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPipe );

	vkCmdBindDescriptorSets(
		cmdBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		mPipeLayout,
		0,        // firstSet
		1,        // descriptorSetCount
		&descSet, // pDescriptorSets
		0,        // dynamicOffsetCount
		nullptr   // pDynamicOffsets
	);

	{
		VkImageSubresourceRange range {};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = VK_REMAINING_MIP_LEVELS;
		range.baseArrayLayer = 0;
		range.layerCount = VK_REMAINING_ARRAY_LAYERS;

		VkImageMemoryBarrier barrier {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext = nullptr;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcQueueFamilyIndex = graphicsQueueIndex;
		barrier.dstQueueFamilyIndex = computeQueueIndex;
		barrier.image = image;
		barrier.subresourceRange = range;

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 1,
			&barrier
		);
	}

	vkCmdDispatch( cmdBuffer, width, height, 1 );

	{
		VkImageSubresourceRange range {};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = VK_REMAINING_MIP_LEVELS;
		range.baseArrayLayer = 0;
		range.layerCount = VK_REMAINING_ARRAY_LAYERS;

		VkImageMemoryBarrier barrier {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.pNext = nullptr;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = 0;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.srcQueueFamilyIndex = computeQueueIndex;
		barrier.dstQueueFamilyIndex = presentQueueIndex;
		barrier.image = image;
		barrier.subresourceRange = range;

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0, 0, nullptr, 0, nullptr, 1,
			&barrier
		);
	}

	this->endCommandBuffer( cmdBuffer );
}


void ComputeHandler::setup( PathTracer* pt ) {
	Logger::logDebug( "[ComputeHandler] Setup begin." );

	mPathTracer = pt;
	const uint32_t numImages = static_cast<uint32_t>( mPathTracer->mSwapchainImages.size() );

	mDescSetLayout = this->createDescriptorSetLayout();
	mDescPool = this->createDescriptorPool( numImages );
	mDescSets = this->allocateDescriptorSets( numImages, mDescSetLayout, mDescPool );

	VkShaderModule shaderModule = this->createShader();
	mPipeLayout = this->createPipelineLayout( mDescSetLayout );
	mPipe = this->createPipeline( mPipeLayout, shaderModule );

	mSemaphoreComputeDone = this->createSemaphore();
	mSemaphoreTransferDone = this->createSemaphore();

	mCmdPool = this->createCommandPool();
	mCmdBuffers = this->allocateCommandBuffers( numImages, mCmdPool );
	mCmdBuffersTransfer = this->allocateCommandBuffers( numImages, mCmdPool );
	this->initCommandBuffers();
	this->initCommandBuffersTransfer();

	vkDestroyShaderModule( mPathTracer->mLogicalDevice, shaderModule, nullptr );

	Logger::logDebug( "[ComputeHandler] Setup done." );
}


void ComputeHandler::teardown() {
	Logger::logDebug( "[ComputeHandler] Teardown begins..." );
	Logger::indentChange( 2 );

	VkDevice device = mPathTracer->mLogicalDevice;

	if( mSemaphoreComputeDone != VK_NULL_HANDLE ) {
		vkDestroySemaphore( device, mSemaphoreComputeDone, nullptr );
		mSemaphoreComputeDone = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ComputeHandler] VkSemaphore for compute destroyed." );
	}

	if( mSemaphoreTransferDone != VK_NULL_HANDLE ) {
		vkDestroySemaphore( device, mSemaphoreTransferDone, nullptr );
		mSemaphoreTransferDone = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ComputeHandler] VkSemaphore for transfer destroyed." );
	}

	if( mDescSetLayout != VK_NULL_HANDLE ) {
		vkDestroyDescriptorSetLayout( device, mDescSetLayout, nullptr );
		mDescSetLayout = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ComputeHandler] VkDescriptorSetLayout destroyed." );
	}

	if( mDescPool != VK_NULL_HANDLE ) {
		vkDestroyDescriptorPool( device, mDescPool, nullptr );
		mDescPool = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ComputeHandler] VkDescriptorPool destroyed." );
	}

	if( mCmdPool != VK_NULL_HANDLE ) {
		vkDestroyCommandPool( device, mCmdPool, nullptr );
		mCmdPool = VK_NULL_HANDLE;
		Logger::logDebug( "[ComputeHandler] VkCommandPool destroyed." );
	}

	if( mPipe != VK_NULL_HANDLE ) {
		vkDestroyPipeline( mPathTracer->mLogicalDevice, mPipe, nullptr );
		Logger::logDebug( "[ComputeHandler] VkPipeline destroyed." );
	}

	if( mPipeLayout != VK_NULL_HANDLE ) {
		vkDestroyPipelineLayout( device, mPipeLayout, nullptr );
		mPipeLayout = VK_NULL_HANDLE;
		Logger::logDebug( "[ComputeHandler] VkPipelineLayout destroyed." );
	}

	Logger::indentChange( -2 );
	Logger::logDebug( "[ComputeHandler] Teardown done." );
}


void ComputeHandler::updateDescriptorSet( VkImageView imageView, VkDescriptorSet descSet ) {
	VkDescriptorImageInfo imageInfo {};
	imageInfo.sampler = VK_NULL_HANDLE;
	imageInfo.imageView = imageView;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkWriteDescriptorSet write {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;
	write.dstSet = descSet;
	write.dstBinding = 0;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	write.pImageInfo = &imageInfo;
	write.pBufferInfo = nullptr;
	write.pTexelBufferView = nullptr;

	VkDevice device = mPathTracer->mLogicalDevice;
	vkUpdateDescriptorSets( device, 1, &write, 0, nullptr );
}
