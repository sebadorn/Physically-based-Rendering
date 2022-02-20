#include "ComputeHandler.h"

using std::vector;


vector<VkCommandBuffer> ComputeHandler::allocateCommandBuffers(
	uint32_t const numCmdBuffers,
	VkCommandPool const &cmdPool
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
	uint32_t const numImages,
	VkDescriptorSetLayout const &layout,
	VkDescriptorPool const &pool
) {
	vector<VkDescriptorSetLayout> layouts( numImages, layout );

	VkDescriptorSetAllocateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.pNext = nullptr;
	info.descriptorPool = pool;
	info.descriptorSetCount = layouts.size();
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


VkDeviceMemory ComputeHandler::allocateStorageImageMemory( VkImage const &image ) {
	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties( mPathTracer->mPhysicalDevice, &memProps );

	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements( mPathTracer->mLogicalDevice, image, &memReq );

	VkDeviceMemory memory = VK_NULL_HANDLE;

	for( uint32_t type = 0; type < memProps.memoryTypeCount; type++ ) {
		const bool typeMatch = memReq.memoryTypeBits & ( 1 << type );
		const bool isMemDeviceLocal = ( memProps.memoryTypes[type].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		if( typeMatch && isMemDeviceLocal ) {
			VkMemoryAllocateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			info.pNext = nullptr;
			info.allocationSize = memReq.size;
			info.memoryTypeIndex = type;

			VkResult result = vkAllocateMemory( mPathTracer->mLogicalDevice, &info, nullptr, &memory );
			VulkanSetup::checkVkResult( result, "Failed to allocate storage image memory.", "ComputeHandler" );

			if( result == VK_SUCCESS ) {
				break;
			}
		}
	}

	if( memory != VK_NULL_HANDLE ) {
		VkResult result = vkBindImageMemory( mPathTracer->mLogicalDevice, image, memory, 0 );
		VulkanSetup::checkVkResult( result, "Failed to bind storage image memory.", "ComputeHandler" );
	}

	return memory;
}


void ComputeHandler::beginCommandBuffer( VkCommandBuffer const &cmdBuffer ) {
	VkCommandBufferBeginInfo info {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.pNext = nullptr;
	info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	info.pInheritanceInfo = nullptr;

	VkResult result = vkBeginCommandBuffer( cmdBuffer, &info );

	VulkanSetup::checkVkResult(
		result,
		"Failed to begin command buffer.",
		"ComputeHandler"
	);
}


VkDescriptorPool ComputeHandler::createDescriptorPool( uint32_t const numImages ) {
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


VkPipeline ComputeHandler::createPipeline(
	VkPipelineLayout const &layout,
	VkShaderModule const &shaderModule
) {
	VkPipelineShaderStageCreateInfo stageInfo = BuilderVk::pipelineShaderStageCreateInfo(
		VK_SHADER_STAGE_COMPUTE_BIT, shaderModule, u8"main"
	);
	VkComputePipelineCreateInfo info = BuilderVk::computePipelineCreateInfo( stageInfo, layout );
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


VkPipelineLayout ComputeHandler::createPipelineLayout( VkDescriptorSetLayout const &descSetLayout ) {
	vector<VkDescriptorSetLayout> descSetLayouts = { descSetLayout };
	VkPipelineLayoutCreateInfo info = BuilderVk::pipelineLayoutCreateInfo( descSetLayouts );

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


VkShaderModule ComputeHandler::createShader() {
	auto shaderCode = PathTracer::loadFileSPV( "src/shaders/compute.spv" );
	Logger::logDebug( "[ComputeHandler] Loaded shader file." );

	VkDevice device = mPathTracer->mLogicalDevice;
	VkShaderModule computeModule = VulkanSetup::createShaderModule( &device, shaderCode );
	Logger::logInfo( "[ComputeHandler] Created shader module." );

	return computeModule;
}


VkImage ComputeHandler::createStorageImage( uint32_t const width, uint32_t const height ) {
	VkImageCreateInfo createInfo = BuilderVk::imageCreateInfo2D(
		mPathTracer->mSwapchainFormat,
		width, height,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED
	);

	VkImage image;
	VkResult result = vkCreateImage( mPathTracer->mLogicalDevice, &createInfo, nullptr, &image );
	VulkanSetup::checkVkResult( result, "Failed to create image.", "ComputeHandler" );

	return image;
}


VkImageView ComputeHandler::createStorageImageView( VkImage const &image ) {
	VkImageViewCreateInfo info = BuilderVk::imageViewCreateInfo2D(
		image, mPathTracer->mSwapchainFormat
	);

	VkImageView imageView;
	VkResult result = vkCreateImageView( mPathTracer->mLogicalDevice, &info, nullptr, &imageView );
	VulkanSetup::checkVkResult( result, "Failed to create storage image view.", "ComputeHandler" );

	return imageView;
}


void ComputeHandler::draw( uint32_t const frameIndex ) {
	VkCommandBuffer cmdBuffer = mCmdBuffers[frameIndex];
	VkDescriptorSet descSet = mDescSets[frameIndex];
	VkImage image = mPathTracer->mSwapchainImages[frameIndex];

	this->updateDescriptorSet( mStorageImageView, descSet );
	this->recordCommandBuffer( cmdBuffer, descSet, image );

	VkPipelineStageFlags flags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkSubmitInfo info = BuilderVk::submitInfo(
		mPathTracer->mImageAvailableSemaphore,
		flags,
		cmdBuffer,
		mSemaphoreComputeDone
	);

	VkResult result = vkQueueSubmit( mPathTracer->mComputeQueue, 1, &info, mDrawingFence );
	VulkanSetup::checkVkResult(
		result,
		"Failed to submit command buffer to queue.",
		"ComputeHandler"
	);
}


void ComputeHandler::endCommandBuffer( VkCommandBuffer const &cmdBuffer ) {
	VkResult result = vkEndCommandBuffer( cmdBuffer );

	VulkanSetup::checkVkResult(
		result,
		"Failed to end compute command buffer.",
		"ComputeHandler"
	);
}


void ComputeHandler::recordCommandBuffer(
	VkCommandBuffer const &cmdBuffer,
	VkDescriptorSet const &descSet,
	VkImage const &image
) {
	this->beginCommandBuffer( cmdBuffer );

	uint32_t computeQueueIndex = static_cast<uint32_t>( mPathTracer->mFamilyIndexCompute );
	uint32_t presentQueueIndex = static_cast<uint32_t>( mPathTracer->mFamilyIndexPresentation );

	{
		VkImageMemoryBarrier barrier = BuilderVk::imageMemoryBarrier(
			mStorageImage,
			// src -> dst
			0,                         VK_ACCESS_SHADER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
			VK_QUEUE_FAMILY_IGNORED,   VK_QUEUE_FAMILY_IGNORED
		);

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0, 0, nullptr, 0, nullptr,
			1, &barrier
		);
	}

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

	vkCmdBindPipeline( cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPipe );

	vkCmdDispatch( cmdBuffer, mPathTracer->mSwapchainExtent.width, mPathTracer->mSwapchainExtent.height, 1 );

	{
		VkImageMemoryBarrier barrierStorageImage = BuilderVk::imageMemoryBarrier(
			mStorageImage,
			// src -> dst
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_GENERAL,    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED,    VK_QUEUE_FAMILY_IGNORED
		);
		VkImageMemoryBarrier barrierSwapchainImage = BuilderVk::imageMemoryBarrier(
			image,
			// src -> dst
			0,                         VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			presentQueueIndex,         computeQueueIndex
		);
		vector<VkImageMemoryBarrier> barriers = { barrierStorageImage, barrierSwapchainImage };

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 0, nullptr,
			static_cast<uint32_t>( barriers.size() ),
			barriers.data()
		);
	}

	{
		VkImageCopy imageCopy = BuilderVk::imageCopy(
			mPathTracer->mSwapchainExtent.width,
			mPathTracer->mSwapchainExtent.height
		);

		vkCmdCopyImage(
			cmdBuffer,
			mStorageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &imageCopy
		);
	}

	{
		VkImageMemoryBarrier barrier = BuilderVk::imageMemoryBarrier(
			image,
			// src -> dst
			VK_ACCESS_TRANSFER_WRITE_BIT,         VK_ACCESS_MEMORY_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			computeQueueIndex,                    presentQueueIndex
		);

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0, 0, nullptr, 0, nullptr,
			1, &barrier
		);
	}

	this->endCommandBuffer( cmdBuffer );
}


void ComputeHandler::setup( PathTracer* pt ) {
	Logger::logDebug( "[ComputeHandler] Setup begin." );

	mPathTracer = pt;
	VkDevice device = mPathTracer->mLogicalDevice;
	uint32_t const numImages = static_cast<uint32_t>( mPathTracer->mSwapchainImages.size() );

	// Storage image
	mStorageImage = this->createStorageImage(
		mPathTracer->mSwapchainExtent.width,
		mPathTracer->mSwapchainExtent.height
	);
	mStorageImageMemory = this->allocateStorageImageMemory( mStorageImage );
	mStorageImageView = this->createStorageImageView( mStorageImage );

	// Descriptors
	mDescSetLayout = this->createDescriptorSetLayout();
	mDescPool = this->createDescriptorPool( numImages );
	mDescSets = this->allocateDescriptorSets( numImages, mDescSetLayout, mDescPool );

	// Shader and pipeline
	VkShaderModule shaderModule = this->createShader();
	mPipeLayout = this->createPipelineLayout( mDescSetLayout );
	mPipe = this->createPipeline( mPipeLayout, shaderModule );
	vkDestroyShaderModule( device, shaderModule, nullptr );

	// Semaphore and fence
	mSemaphoreComputeDone = VulkanSetup::createSemaphore( device );
	mDrawingFence = VulkanSetup::createFence( device, VK_FENCE_CREATE_SIGNALED_BIT );

	// Commands
	VkCommandPoolCreateFlags cmdPoolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	mCmdPool = VulkanSetup::createCommandPool(
		device, cmdPoolFlags, mPathTracer->mFamilyIndexCompute
	);
	mCmdBuffers = this->allocateCommandBuffers( numImages, mCmdPool );

	Logger::logDebug( "[ComputeHandler] Setup done." );
}


void ComputeHandler::teardown() {
	Logger::logDebug( "[ComputeHandler] Teardown begins..." );
	Logger::indentChange( 2 );

	VkDevice device = mPathTracer->mLogicalDevice;

	if( mStorageImageMemory != VK_NULL_HANDLE ) {
		vkFreeMemory( mPathTracer->mLogicalDevice, mStorageImageMemory, nullptr );
		mStorageImageMemory = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ComputeHandler] VkDeviceMemory for storage image freed." );
	}

	if( mStorageImageView != VK_NULL_HANDLE ) {
		vkDestroyImageView( device, mStorageImageView, nullptr );
		mStorageImageView = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ComputeHandler] VkImageView for compute storage image destroyed." );
	}

	if( mStorageImage != VK_NULL_HANDLE ) {
		vkDestroyImage( device, mStorageImage, nullptr );
		mStorageImage = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ComputeHandler] VkImage for compute storage image destroyed." );
	}

	if( mSemaphoreComputeDone != VK_NULL_HANDLE ) {
		vkDestroySemaphore( device, mSemaphoreComputeDone, nullptr );
		mSemaphoreComputeDone = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ComputeHandler] VkSemaphore for compute destroyed." );
	}

	if( mDrawingFence != VK_NULL_HANDLE ) {
		vkDestroyFence( device, mDrawingFence, nullptr );
		mDrawingFence = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ComputeHandler] VkFence for drawing destroyed." );
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
		Logger::logDebugVerbose( "[ComputeHandler] VkCommandPool destroyed." );
	}

	if( mPipe != VK_NULL_HANDLE ) {
		vkDestroyPipeline( mPathTracer->mLogicalDevice, mPipe, nullptr );
		Logger::logDebugVerbose( "[ComputeHandler] VkPipeline destroyed." );
	}

	if( mPipeLayout != VK_NULL_HANDLE ) {
		vkDestroyPipelineLayout( device, mPipeLayout, nullptr );
		mPipeLayout = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[ComputeHandler] VkPipelineLayout destroyed." );
	}

	Logger::indentChange( -2 );
	Logger::logDebug( "[ComputeHandler] Teardown done." );
}


void ComputeHandler::updateDescriptorSet(
	VkImageView const &imageView,
	VkDescriptorSet const &descSet
) {
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
