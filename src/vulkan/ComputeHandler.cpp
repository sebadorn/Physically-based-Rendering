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


VkDeviceMemory ComputeHandler::allocateStorageImageMemory( VkImage image ) {
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


void ComputeHandler::beginCommandBuffer( VkCommandBuffer cmdBuffer ) {
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


VkCommandPool ComputeHandler::createCommandPool() {
	VkCommandPoolCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
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


VkImage ComputeHandler::createStorageImage( uint32_t width, uint32_t height ) {
	VkImage image;
	VkResult result;

	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.format = mPathTracer->mSwapchainFormat;
	createInfo.extent = { width, height, 1 };
	createInfo.mipLevels = 1;
	createInfo.arrayLayers = 1;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = nullptr;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	result = vkCreateImage( mPathTracer->mLogicalDevice, &createInfo, nullptr, &image );
	VulkanSetup::checkVkResult( result, "Failed to create image.", "ComputeHandler" );

	return image;
}


VkImageView ComputeHandler::createStorageImageView( VkImage image ) {
	VkComponentMapping components = {};
	components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	VkImageSubresourceRange range = {};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = VK_REMAINING_MIP_LEVELS;
	range.baseArrayLayer = 0;
	range.layerCount = VK_REMAINING_ARRAY_LAYERS;

	VkImageViewCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = 0;
	info.image = image;
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.format = mPathTracer->mSwapchainFormat;
	info.components = components;
	info.subresourceRange = range;

	VkImageView imageView;
	VkResult result = vkCreateImageView( mPathTracer->mLogicalDevice, &info, nullptr, &imageView );
	VulkanSetup::checkVkResult( result, "Failed to create storage image view.", "ComputeHandler" );

	return imageView;
}


void ComputeHandler::draw( uint32_t frameIndex ) {
	this->initCommandBuffers( frameIndex );

	VkCommandBuffer cmdBuffer = mCmdBuffers[frameIndex];
	VkPipelineStageFlags flags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	VkSubmitInfo info {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.pNext = nullptr;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &( mPathTracer->mImageAvailableSemaphore );
	info.pWaitDstStageMask = &flags;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &cmdBuffer;
	info.signalSemaphoreCount = 1;
	info.pSignalSemaphores = &mSemaphoreComputeDone;

	vkResetFences( mPathTracer->mLogicalDevice, 1, &mDrawingFence );

	VkResult result = vkQueueSubmit( mPathTracer->mComputeQueue, 1, &info, mDrawingFence );
	VulkanSetup::checkVkResult(
		result,
		"Failed to submit command buffer to queue.",
		"ComputeHandler"
	);
}


void ComputeHandler::endCommandBuffer( VkCommandBuffer cmdBuffer ) {
	VkResult result = vkEndCommandBuffer( cmdBuffer );

	VulkanSetup::checkVkResult(
		result,
		"Failed to end compute command buffer.",
		"ComputeHandler"
	);
}


void ComputeHandler::initCommandBuffers( uint32_t index ) {
	VkImage image = mPathTracer->mSwapchainImages[index];
	VkImageView imageView = mPathTracer->mSwapchainImageViews[index];

	VkDescriptorSet descSet = mDescSets[index];
	VkCommandBuffer cmdBuffer = mCmdBuffers[index];

	this->updateDescriptorSet( imageView, descSet );
	this->recordCommandBuffer(
		cmdBuffer, descSet, image,
		mPathTracer->mSwapchainExtent.width,
		mPathTracer->mSwapchainExtent.height
	);
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
	uint32_t presentQueueIndex = static_cast<uint32_t>( mPathTracer->mFamilyIndexPresentation );

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
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = mStorageImage;
		barrier.subresourceRange = range;

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

	vkCmdDispatch( cmdBuffer, width / 32 + 1, height / 32 + 1, 1 );

	{
		VkImageSubresourceRange range1 {};
		range1.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range1.baseMipLevel = 0;
		range1.levelCount = VK_REMAINING_MIP_LEVELS;
		range1.baseArrayLayer = 0;
		range1.layerCount = VK_REMAINING_ARRAY_LAYERS;

		VkImageMemoryBarrier barrier1 {};
		barrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier1.pNext = nullptr;
		barrier1.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier1.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier1.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier1.image = mStorageImage;
		barrier1.subresourceRange = range1;


		VkImageSubresourceRange range2 {};
		range2.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range2.baseMipLevel = 0;
		range2.levelCount = VK_REMAINING_MIP_LEVELS;
		range2.baseArrayLayer = 0;
		range2.layerCount = VK_REMAINING_ARRAY_LAYERS;

		VkImageMemoryBarrier barrier2 {};
		barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier2.pNext = nullptr;
		barrier2.srcAccessMask = 0;
		barrier2.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier2.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier2.srcQueueFamilyIndex = presentQueueIndex;
		barrier2.dstQueueFamilyIndex = computeQueueIndex;
		barrier2.image = image;
		barrier2.subresourceRange = range2;


		vector<VkImageMemoryBarrier> barriers = { barrier1, barrier2 };

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
		VkImageCopy imageCopy = {};
		imageCopy.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		imageCopy.srcOffset = { 0, 0, 0 };
		imageCopy.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		imageCopy.dstOffset = { 0, 0, 0 };
		imageCopy.extent = { width, height, 1 };

		vkCmdCopyImage(
			cmdBuffer,
			mStorageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &imageCopy
		);
	}

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
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.srcQueueFamilyIndex = computeQueueIndex;
		barrier.dstQueueFamilyIndex = presentQueueIndex;
		barrier.image = image;
		barrier.subresourceRange = range;

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
	const uint32_t numImages = static_cast<uint32_t>( mPathTracer->mSwapchainImages.size() );

	int w, h;
	glfwGetFramebufferSize( mPathTracer->mWindow, &w, &h );
	const uint32_t width = static_cast<uint32_t>( w );
	const uint32_t height = static_cast<uint32_t>( h );

	mStorageImage = this->createStorageImage( width, height );
	mStorageImageMemory = this->allocateStorageImageMemory( mStorageImage );
	mStorageImageView = this->createStorageImageView( mStorageImage );

	mDescSetLayout = this->createDescriptorSetLayout();
	mDescPool = this->createDescriptorPool( numImages );
	mDescSets = this->allocateDescriptorSets( numImages, mDescSetLayout, mDescPool );

	VkShaderModule shaderModule = this->createShader();
	mPipeLayout = this->createPipelineLayout( mDescSetLayout );
	mPipe = this->createPipeline( mPipeLayout, shaderModule );

	mSemaphoreComputeDone = this->createSemaphore();

	VkFenceCreateInfo info = {
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		nullptr,
		VK_FENCE_CREATE_SIGNALED_BIT
	};
	VkResult result = vkCreateFence( mPathTracer->mLogicalDevice, &info, nullptr, &mDrawingFence );
	VulkanSetup::checkVkResult( result, "Failed to create fence (compute drawing).", "ComputeHandler" );

	mCmdPool = this->createCommandPool();
	mCmdBuffers = this->allocateCommandBuffers( numImages, mCmdPool );

	vkDestroyShaderModule( mPathTracer->mLogicalDevice, shaderModule, nullptr );

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
