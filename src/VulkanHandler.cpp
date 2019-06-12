#include "VulkanHandler.h"

using std::vector;


bool VulkanHandler::useValidationLayer = true;


/**
 * Calculate the matrices for view, model, model-view-projection and normals.
 */
void VulkanHandler::calculateMatrices() {
	glm::mat4 modelMatrix = glm::mat4( 1.0f );
	glm::mat4 viewMatrix = glm::lookAt( mCameraEye, mCameraCenter, mCameraUp );

	int width;
	int height;
	glfwGetWindowSize( mWindow, &width, &height );

	glm::mat4 projectionMatrix = glm::perspectiveFov(
		(GLfloat) mFOV, (GLfloat) width, (GLfloat) height,
		Cfg::get().value<GLfloat>( Cfg::PERS_ZNEAR ),
		Cfg::get().value<GLfloat>( Cfg::PERS_ZFAR )
	);

	mModelViewProjectionMatrix = projectionMatrix * viewMatrix * modelMatrix;
}


/**
 * Check a VkResult and throw an error if not a VK_SUCCESS.
 * @param {VkResult}    result
 * @param {const char*} errorMessage
 * @param {const char*} className
 */
void VulkanHandler::checkVkResult(
	VkResult result, const char* errorMessage, const char* className
) {
	if( result != VK_SUCCESS ) {
		Logger::logErrorf( "[%s] %s", className, errorMessage );
		throw std::runtime_error( errorMessage );
	}
}


/**
 * Copy VkBuffer.
 * @param {VkBuffer}     srcBuffer
 * @param {VkBuffer}     dstBuffer
 * @param {VkDeviceSize} size
 */
void VulkanHandler::copyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size ) {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = mCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers( mLogicalDevice, &allocInfo, &commandBuffer );

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer( commandBuffer, &beginInfo );

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer( commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion );

	vkEndCommandBuffer( commandBuffer );

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit( mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
	vkQueueWaitIdle( mGraphicsQueue );

	vkFreeCommandBuffers( mLogicalDevice, mCommandPool, 1, &commandBuffer );
}


/**
 * Create a VkBuffer.
 * @param {VkDeviceSize}          size
 * @param {VkBufferUsageFlags}    usage
 * @param {VkMemoryPropertyFlags} properties
 * @param {VkBuffer&}             buffer
 * @param {VkDeviceMemory&}       bufferMemory
 */
void VulkanHandler::createBuffer(
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory
) {
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vkCreateBuffer( mLogicalDevice, &bufferInfo, nullptr, &buffer );
	VulkanHandler::checkVkResult( result, "Failed to create VkBuffer." );

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements( mLogicalDevice, buffer, &memRequirements );

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = this->findMemoryType(
		memRequirements.memoryTypeBits,
		properties
	);

	result = vkAllocateMemory( mLogicalDevice, &allocInfo, nullptr, &bufferMemory );
	VulkanHandler::checkVkResult( result, "Failed to allocate memory." );

	vkBindBufferMemory( mLogicalDevice, buffer, bufferMemory, 0 );
}


/**
 * Create the command buffers.
 */
void VulkanHandler::createCommandBuffers() {
	// Free old command buffers.
	if( mCommandBuffers.size() > 0 ) {
		vkFreeCommandBuffers(
			mLogicalDevice,
			mCommandPool,
			mCommandBuffers.size(),
			mCommandBuffers.data()
		);
	}

	mCommandBuffersNow.resize( 2 );
	mCommandBuffers.resize( mFramebuffers.size() );

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = mCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t) mCommandBuffers.size();

	VkResult result = vkAllocateCommandBuffers(
		mLogicalDevice, &allocInfo, mCommandBuffers.data()
	);
	VulkanHandler::checkVkResult( result, "Failed to allocate VkCommandBuffers." );

	Logger::logInfof(
		"[VulkanHandler] Allocated %u VkCommandBuffers.",
		mCommandBuffers.size()
	);
}


/**
 * Create the command pool for the graphics queue.
 */
void VulkanHandler::createCommandPool() {
	int graphicsFamily = -1;
	int presentFamily = -1;
	VulkanSetup::findQueueFamilyIndices( mPhysicalDevice, &graphicsFamily, &presentFamily, &mSurface );

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = graphicsFamily;
	poolInfo.flags = 0;

	VkResult result = vkCreateCommandPool(
		mLogicalDevice, &poolInfo, nullptr, &mCommandPool
	);
	VulkanHandler::checkVkResult( result, "Failed to create VkCommandPool." );
	Logger::logInfo( "[VulkanHandler] Created VkCommandPool." );
}


/**
 * Create a descriptor set.
 * @return {VkDescriptorSet}
 */
VkDescriptorSet VulkanHandler::createDescriptorSet() {
	VkDescriptorSetLayout layouts[] = { mDescriptorSetLayout };

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = mDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VkDescriptorSet descriptorSet;
	VkResult result = vkAllocateDescriptorSets( mLogicalDevice, &allocInfo, &descriptorSet );
	VulkanHandler::checkVkResult( result, "Failed to allocate descriptor set." );

	return descriptorSet;
}


/**
 * Create the fences.
 */
void VulkanHandler::createFences() {
	mFences.resize( 2 );

	VkFenceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkResult result = vkCreateFence( mLogicalDevice, &info, nullptr, &mImageAvailableFence );
	VulkanHandler::checkVkResult( result, "Failed to create fence (image available)." );

	result = vkCreateFence( mLogicalDevice, &info, nullptr, &mRenderFinishedFence );
	VulkanHandler::checkVkResult( result, "Failed to create fence (render finished)." );

	mFences[0] = mImageAvailableFence;
	mFences[1] = mRenderFinishedFence;

	Logger::logInfof(
		"[VulkanHandler] Created %u VkFences.",
		mFences.size()
	);
}


/**
 * Create the framebuffers.
 */
void VulkanHandler::createFramebuffers() {
	// Destroy old framebuffers.
	if( mFramebuffers.size() > 0 ) {
		for( size_t i = 0; i < mFramebuffers.size(); i++ ) {
			vkDestroyFramebuffer( mLogicalDevice, mFramebuffers[i], nullptr );
		}
	}

	mFramebuffers.resize( mSwapchainImageViews.size() );

	for( size_t i = 0; i < mSwapchainImageViews.size(); i++ ) {
		VkImageView attachments[] = {
			mSwapchainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = mRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = mSwapchainExtent.width;
		framebufferInfo.height = mSwapchainExtent.height;
		framebufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(
			mLogicalDevice, &framebufferInfo, nullptr, &( mFramebuffers[i] )
		);
		VulkanHandler::checkVkResult( result, "Failed to create VkFramebuffer." );
	}

	Logger::logInfof(
		"[VulkanHandler] Created %u VkFramebuffers.",
		mFramebuffers.size()
	);
}


/**
 * Create the graphics pipeline.
 */
void VulkanHandler::createGraphicsPipeline() {
	// Destroy old graphics pipeline.
	if( mGraphicsPipeline != VK_NULL_HANDLE ) {
		vkDestroyPipeline( mLogicalDevice, mGraphicsPipeline, nullptr );
	}

	// Destroy old pipeline layout.
	if( mPipelineLayout != VK_NULL_HANDLE ) {
		vkDestroyPipelineLayout( mLogicalDevice, mPipelineLayout, nullptr );
	}

	mPipelineLayout = VulkanSetup::createPipelineLayout( &mLogicalDevice, &mDescriptorSetLayout );

	auto vertShaderCode = this->loadFileSPV( "src/shaders/vert.spv" );
	auto fragShaderCode = this->loadFileSPV( "src/shaders/frag.spv" );
	Logger::logDebug( "[VulkanHandler] Loaded shader files." );

	VkShaderModule vertShaderModule = this->createShaderModule( vertShaderCode );
	VkShaderModule fragShaderModule = this->createShaderModule( fragShaderCode );
	Logger::logInfo( "[VulkanHandler] Created shader modules." );

	VkPipelineShaderStageCreateInfo vertShaderCreateInfo = {};
	vertShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderCreateInfo.module = vertShaderModule;
	vertShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderCreateInfo = {};
	fragShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderCreateInfo.module = fragShaderModule;
	fragShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {
		vertShaderCreateInfo,
		fragShaderCreateInfo
	};

	mGraphicsPipeline = VulkanSetup::createGraphicsPipeline(
		&mLogicalDevice, &mPipelineLayout, &mRenderPass, shaderStages, &mSwapchainExtent
	);

	vkDestroyShaderModule( mLogicalDevice, vertShaderModule, nullptr );
	vkDestroyShaderModule( mLogicalDevice, fragShaderModule, nullptr );
	Logger::logDebug( "[VulkanHandler] Destroyed shader modules. (Not needed anymore.)" );
}


/**
 * Create the swapchain image views.
 */
void VulkanHandler::createImageViews() {
	this->destroyImageViews();
	mSwapchainImageViews.resize( mSwapchainImages.size(), VK_NULL_HANDLE );

	for( uint32_t i = 0; i < mSwapchainImages.size(); i++ ) {
		VkImageView imageView = {};
		mSwapchainImageViews[i] = imageView;

		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = mSwapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = mSwapchainFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(
			mLogicalDevice, &createInfo, nullptr, &( mSwapchainImageViews[i] )
		);
		VulkanHandler::checkVkResult( result, "Failed to create VkImageView." );
	}

	Logger::logDebugf( "[VulkanHandler] Created %u VkImageViews.", mSwapchainImages.size() );
}


/**
 * Create the VkRenderPass.
 */
void VulkanHandler::createRenderPass() {
	// Destroy old render pass.
	if( mRenderPass != VK_NULL_HANDLE ) {
		vkDestroyRenderPass( mLogicalDevice, mRenderPass, nullptr );
	}

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = mSwapchainFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
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

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass(
		mLogicalDevice, &renderPassInfo, nullptr, &mRenderPass
	);
	VulkanHandler::checkVkResult( result, "Failed to create VkRenderPass." );
	Logger::logInfo( "[VulkanHandler] Created VkRenderPass." );

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	vkCreateRenderPass(
		mLogicalDevice, &renderPassInfo, nullptr, &mRenderPassInitial
	);
}


/**
 * Create semaphores for the draw frames function.
 */
void VulkanHandler::createSemaphores() {
	VkResult result;
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	result = vkCreateSemaphore(
		mLogicalDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphore
	);
	VulkanHandler::checkVkResult(
		result, "Failed to create VkSemaphore (image available)."
	);
	Logger::logDebugVerbose( "[VulkanHandler] Created VkSemaphore (image available)." );

	result = vkCreateSemaphore(
		mLogicalDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphore
	);
	VulkanHandler::checkVkResult(
		result, "Failed to create VkSemaphore (render finished)."
	);
	Logger::logDebugVerbose( "[VulkanHandler] Created VkSemaphore (render finished)." );
}


/**
 * Create a VkShaderModule from loaded SPV code.
 * @param  {const std::vector<char>&} code
 * @return {VkShaderModule}
 */
VkShaderModule VulkanHandler::createShaderModule( const vector<char>& code ) {
	VkShaderModule shaderModule;

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = (uint32_t*) code.data();

	VkResult result = vkCreateShaderModule(
		mLogicalDevice, &createInfo, nullptr, &shaderModule
	);
	VulkanHandler::checkVkResult( result, "Failed to create VkShaderModule." );

	return shaderModule;
}


/**
 * Create the uniform buffer.
 */
void VulkanHandler::createUniformBuffer() {
	VkDeviceSize bufferSize = sizeof( UniformCamera );

	this->createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		mUniformBuffer,
		mUniformBufferMemory
	);
}


/**
 * Create vertex buffer.
 */
void VulkanHandler::createVertexBuffer() {
	VkDeviceSize bufferSize = sizeof( vertices[0] ) * vertices.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	this->createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	void* data;
	vkMapMemory( mLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data );
	memcpy( data, vertices.data(), (size_t) bufferSize );
	vkUnmapMemory( mLogicalDevice, stagingBufferMemory );

	this->createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mVertexBuffer,
		mVertexBufferMemory
	);
	this->copyBuffer( stagingBuffer, mVertexBuffer, bufferSize );

	vkFreeMemory( mLogicalDevice, stagingBufferMemory, nullptr );
	vkDestroyBuffer( mLogicalDevice, stagingBuffer, nullptr );
}


/**
 * Debug callback for the validation layer.
 * @param  {VkDebugReportFlagsEXT}      flags
 * @param  {VkDebugReportObjectTypeEXT} objType
 * @param  {uint64_t}                   obj
 * @param  {size_t}                     location
 * @param  {int32_t}                    code
 * @param  {const char*}                layerPrefix
 * @param  {const char*}                msg
 * @param  {void*}                      userData
 * @return {VKAPI_ATTR VkBool32 VKAPI_CALL}
 */
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanHandler::debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData
) {
	Logger::logErrorf( "[VulkanHandler] Validation layer: %s", msg );

	return VK_FALSE;
}


/**
 * Destroy the validation layer debug callback.
 */
void VulkanHandler::destroyDebugCallback() {
	if( !mDebugCallback ) {
		return;
	}

	auto fnDestroyDebugCallback = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(
		mInstance, "vkDestroyDebugReportCallbackEXT"
	);

	if( fnDestroyDebugCallback != nullptr ) {
		fnDestroyDebugCallback( mInstance, mDebugCallback, nullptr );
		Logger::logDebug( "[VulkanHandler] Debug callback destroyed." );
	}
}


/**
 * Destroy the image views.
 */
void VulkanHandler::destroyImageViews() {
	for( uint32_t i = 0; i < mSwapchainImageViews.size(); i++ ) {
		VkImageView imageView = mSwapchainImageViews[i];

		if( imageView != VK_NULL_HANDLE ) {
			vkDestroyImageView( mLogicalDevice, imageView, nullptr );
			mSwapchainImageViews[i] = VK_NULL_HANDLE;
			Logger::logDebugVerbosef( "[VulkanHandler] Destroyed VkImageView %u.", i );
		}
	}
}


/**
 * Draw the next frame.
 * @return {bool}
 */
bool VulkanHandler::drawFrame() {
	VkResult result = vkAcquireNextImageKHR(
		mLogicalDevice,
		mSwapchain,
		std::numeric_limits< uint64_t >::max(), // disable timeout
		mImageAvailableSemaphore,
		mImageAvailableFence,
		&mFrameIndex
	);

	if( result == VK_ERROR_OUT_OF_DATE_KHR ) {
		this->recreateSwapchain();
		return true;
	}
	else if( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR ) {
		Logger::logError( "[VulkanHandler] Failed to acquire swap chain image." );
		throw std::runtime_error( "Failed to acquire swap chain image." );
	}

	this->recordCommand();
	mImGuiHandler->draw();

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { mImageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// Render main image.
	mCommandBuffersNow[0] = mCommandBuffers[mFrameIndex];
	// Then render user interface on top.
	mCommandBuffersNow[1] = mImGuiHandler->mCommandBuffers[mFrameIndex];

	submitInfo.commandBufferCount = 2;
	submitInfo.pCommandBuffers = &mCommandBuffersNow[0];

	result = vkQueueSubmit( mGraphicsQueue, 1, &submitInfo, mRenderFinishedFence );
	VulkanHandler::checkVkResult( result, "Failed to submit graphics queue." );

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapchains[] = { mSwapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &mFrameIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR( mPresentQueue, &presentInfo );

	if( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ) {
		this->recreateSwapchain();
		return true;
	}
	else if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to present swap chain image." );
		throw std::runtime_error( "Failed to present swap chain image." );
	}

	return false;
}


/**
 * Find suitable memory type.
 * @param  {uint32_t}              typeFilter
 * @param  {VkMemoryPropertyFlags} properties
 * @return {uint32_t}
 */
uint32_t VulkanHandler::findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties ) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties( mPhysicalDevice, &memProperties );

	for( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ ) {
		if(
			( typeFilter & ( 1 << i ) ) &&
			( memProperties.memoryTypes[i].propertyFlags & properties ) == properties
		) {
			return i;
		}
	}

	Logger::logError( "[VulkanHandler] Failed to find suitable memory type." );
	throw std::runtime_error( "Failed to find suitable memory type." );
}


/**
 * Initialize the window.
 */
void VulkanHandler::initWindow() {
	glfwInit();

	int glfwVersionMajor = 0;
	int glfwVersionMinor = 0;
	int glfwVersionRev = 0;
	glfwGetVersion( &glfwVersionMajor, &glfwVersionMinor, &glfwVersionRev );

	Logger::logInfof(
		"[VulkanHandler] GLFW version: %d.%d.%d",
		glfwVersionMajor, glfwVersionMinor, glfwVersionRev
	);

	if( !glfwVulkanSupported() ) {
		Logger::logError( "[VulkanHandler] GLFW says it doesn't support Vulkan." );
		glfwTerminate();

		throw std::runtime_error( "GLFW does not support Vulkan." );
	}

	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	glfwWindowHint( GLFW_RESIZABLE, GLFW_TRUE );

	mWindow = glfwCreateWindow(
		Cfg::get().value<int>( Cfg::WINDOW_WIDTH ),
		Cfg::get().value<int>( Cfg::WINDOW_HEIGHT ),
		"PBR-Vulkan", nullptr, nullptr
	);
	glfwSetWindowUserPointer( mWindow, this );
	glfwSetWindowSizeCallback( mWindow, VulkanHandler::onWindowResize );
}


/**
 * Load an SPV file.
 * @param  {const std::string&} filename
 * @return {std::vector<char>}
 */
vector<char> VulkanHandler::loadFileSPV( const string& filename ) {
	std::ifstream file( filename, std::ios::ate | std::ios::binary );

	if( !file.is_open() ) {
		Logger::logErrorf( "[VulkanHandler] Failed to open SPV file: %s", filename );
		throw std::runtime_error( "Failed to open file." );
	}

	size_t filesize = (size_t) file.tellg();
	vector<char> buffer( filesize );

	file.seekg( 0 );
	file.read( buffer.data(), filesize );
	file.close();

	return buffer;
}


/**
 * Load the loaded model into buffers for the shader.
 * @param {ObjParser*}      op
 * @param {AccelStructure*} accelStruct
 */
void VulkanHandler::loadModelIntoBuffers( ObjParser* op, AccelStructure* accelStruct ) {
	mObjParser = op;


	// TODO:
	// Pass model data to fragment shader. But how? I cannot
	// use vectors with dynamic length in shaders. Or should I
	// recompile the shaders after loading the model? Maybe
	// or probably there are certain methods to achieve this.
	//
	// -> Shader Storage Buffer Objects?
	// http://www.geeks3d.com/20140704/tutorial-introduction-to-opengl-4-3-shader-storage-buffers-objects-ssbo-demo/


	ModelVertices modelVert = {};
	vector<float> vertices = op->getVertices();

	for( int i = 0; i < vertices.size(); i += 3 ) {
		float v0 = vertices[i];
		float v1 = vertices[i + 1];
		float v2 = vertices[i + 2];
		glm::vec4 vec = glm::vec4( v0, v1, v2, 0.0 );

		modelVert.vertices.push_back( vec );
	}

	VkDeviceSize vertBufSize = sizeof( glm::vec4 ) * modelVert.vertices.size();
	VkBuffer vertStagingBuf;
	VkDeviceMemory vertStagingBufMem;

	this->createBuffer(
		vertBufSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vertStagingBuf,
		vertStagingBufMem
	);

	void* data;
	vkMapMemory( mLogicalDevice, vertStagingBufMem, 0, vertBufSize, 0, &data );
	memcpy( data, modelVert.vertices.data(), (size_t) vertBufSize );



	// TODO: Bind buffer when recording the command for the current render pass?


	// // Vertices
	// vector<float> objVertices = op->getVertices();
	// VkDeviceSize bufferSize = sizeof( float ) * objVertices.size();
	// VkBuffer stagingBuffer;
	// VkDeviceMemory stagingBufferMemory;

	// this->createBuffer(
	// 	bufferSize,
	// 	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	// 	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	// 	stagingBuffer,
	// 	stagingBufferMemory
	// );

	// void* data;
	// vkMapMemory( mLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data );
	// memcpy( data, objVertices.data(), (size_t) bufferSize );
	// vkUnmapMemory( mLogicalDevice, stagingBufferMemory );


	// Shader Storage Buffer Object
	VkBuffer storageBuffer;
	VkDeviceMemory storageBufferMemory;

	this->createBuffer(
		vertBufSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		storageBuffer,
		storageBufferMemory
	);

	// Has to be added to command buffer?
	// https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-5

	// memcpy( storageBuffer, vertStagingBuf, (size_t) vertBufSize );

	vkUnmapMemory( mLogicalDevice, vertStagingBufMem );
	vkFreeMemory( mLogicalDevice, vertStagingBufMem, nullptr );
	vkDestroyBuffer( mLogicalDevice, vertStagingBuf, nullptr );

	// this->createBuffer(
	// 	bufferSize,
	// 	VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	// 	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	// 	mModelVerticesBuffer,
	// 	mModelVerticesBufferMemory
	// );
	// this->copyBuffer( stagingBuffer, mModelVerticesBuffer, bufferSize );

	// vkFreeMemory( mLogicalDevice, stagingBufferMemory, nullptr );
	// vkDestroyBuffer( mLogicalDevice, stagingBuffer, nullptr );
}


/**
 * Start the main loop.
 */
void VulkanHandler::mainLoop() {
	double lastTime = 0.0;
	uint64_t numFrames = 0;
	int numFramebuffers = mFramebuffers.size();

	// while( !glfwWindowShouldClose( mWindow ) ) {
	for( int i = 0; i < 5000; i++ ) { // Playing it save for now.
		if( glfwWindowShouldClose( mWindow ) ) {
			break;
		}

		glfwPollEvents();

		this->drawFrame();
		this->updateFPS( &lastTime, &numFrames );
		this->waitForFences();

		// We have two slightly different render passes. The first one
		// has a color attachment with "loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR"
		// and is used for the first rendering of each frame. After that it
		// is destroyed and the second render pass will be used, which uses
		// "loadOp = VK_ATTACHMENT_LOAD_OP_LOAD". This is necessary to
		// keep the image content after the first command (main rendering),
		// so the second command (UI) will be visible on top.
		//
		// Error message, if LOAD_OP_LOAD is used from the get go:
		//     vkCmdBeginRenderPass(): Cannot read invalid swapchain image,
		//                             please fill the memory before using.

		if( mRenderPassInitial != VK_NULL_HANDLE && numFrames >= numFramebuffers ) {
			vkDestroyRenderPass( mLogicalDevice, mRenderPassInitial, nullptr );
			mRenderPassInitial = VK_NULL_HANDLE;
		}
	}

	VkResult result = vkDeviceWaitIdle( mLogicalDevice );
	VulkanHandler::checkVkResult( result, "Failed to wait until idle." );
}


/**
 * Handle window resize events.
 * @param {GLFWwindow*} window
 * @param {int}         width
 * @param {int]         height
 */
void VulkanHandler::onWindowResize( GLFWwindow* window, int width, int height ) {
	if( width == 0 || height == 0 ) {
		return;
	}

	VulkanHandler* vk = reinterpret_cast<VulkanHandler*>( glfwGetWindowUserPointer( window ) );
	vk->recreateSwapchain();
}


/**
 * Record the command for this render pass.
 */
void VulkanHandler::recordCommand() {
	VkResult result = vkResetCommandPool( mLogicalDevice, mCommandPool, 0 );
	VulkanHandler::checkVkResult( result, "Resetting command pool failed." );

	this->calculateMatrices();
	this->updateUniformBuffer();

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer( mCommandBuffers[mFrameIndex], &beginInfo );

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.framebuffer = mFramebuffers[mFrameIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = mSwapchainExtent;

	if( mRenderPassInitial ) {
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		renderPassInfo.renderPass = mRenderPassInitial;
	}
	else {
		renderPassInfo.renderPass = mRenderPass;
	}

	vkCmdBeginRenderPass(
		mCommandBuffers[mFrameIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE
	);
	vkCmdBindPipeline(
		mCommandBuffers[mFrameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline
	);

	vkCmdBindDescriptorSets(
		mCommandBuffers[mFrameIndex],
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		mPipelineLayout, 0, 1,
		&mDescriptorSet, 0, nullptr
	);

	VkBuffer vertexBuffers[] = { mVertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers( mCommandBuffers[mFrameIndex], 0, 1, vertexBuffers, offsets );
	vkCmdDraw( mCommandBuffers[mFrameIndex], vertices.size(), 1, 0, 0 );

	vkCmdEndRenderPass( mCommandBuffers[mFrameIndex] );

	result = vkEndCommandBuffer( mCommandBuffers[mFrameIndex] );
	VulkanHandler::checkVkResult( result, "Failed to record command buffer." );
}


/**
 * Recreate the swapchain.
 */
void VulkanHandler::recreateSwapchain() {
	Logger::mute();
	Logger::logDebug( "[VulkanHandler] Recreating swap chain ..." );
	Logger::indentChange( 2 );

	vkDeviceWaitIdle( mLogicalDevice );

	this->setupSwapchain();
	this->createImageViews();
	this->createRenderPass();
	this->createGraphicsPipeline();
	this->createFramebuffers();
	this->createCommandBuffers();

	Logger::indentChange( -2 );
	Logger::logDebug( "[VulkanHandler] Swap chain recreated." );
	Logger::unmute();
}


/**
 * Retrieve the handles for the swapchain images.
 */
void VulkanHandler::retrieveSwapchainImageHandles() {
	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR( mLogicalDevice, mSwapchain, &imageCount, nullptr );
	mSwapchainImages.resize( imageCount );
	vkGetSwapchainImagesKHR( mLogicalDevice, mSwapchain, &imageCount, mSwapchainImages.data() );

	Logger::logDebug( "[VulkanHandler] Retrieved swapchain VkImage handles." );
}


/**
 * Setup Vulkan: Create a VkInstance,
 * check for support, pick a device etc.
 * @param {ActionHandler*} actionHandler
 */
void VulkanHandler::setup( ActionHandler* actionHandler ) {
	Logger::logInfo( "[VulkanHandler] Setup beginning ..." );
	Logger::indentChange( 2 );
	Logger::logInfof( "[VulkanHandler] VK_HEADER_VERSION: %d", VK_HEADER_VERSION );

	mActionHandler = actionHandler;
	this->initWindow();

	VulkanHandler::setupValidationLayer();

	mInstance = VulkanSetup::createInstance();
	VulkanSetup::setupDebugCallback( &mInstance, &mDebugCallback );
	VulkanSetup::createSurface( &mInstance, mWindow, &mSurface );
	mPhysicalDevice = VulkanSetup::selectDevice( &mInstance, &mSurface );
	VulkanSetup::createLogicalDevice(
		&mSurface, &mPhysicalDevice, &mLogicalDevice,
		&mGraphicsQueue, &mPresentQueue
	);

	this->setupSwapchain();
	this->createImageViews();
	this->createRenderPass();
	mDescriptorSetLayout = VulkanSetup::createDescriptorSetLayout( &mLogicalDevice );
	this->createGraphicsPipeline();
	this->createFramebuffers();
	this->createCommandPool();
	this->createVertexBuffer();
	this->createUniformBuffer();
	mDescriptorPool = VulkanSetup::createDescriptorPool( &mLogicalDevice );
	mDescriptorSet = this->createDescriptorSet();
	this->writeUniformToDescriptorSet();
	this->createCommandBuffers();
	this->createFences();
	this->createSemaphores();

	mImGuiHandler = new ImGuiHandler();
	mImGuiHandler->setup( this );

	Logger::indentChange( -2 );
	Logger::logInfo( "[VulkanHandler] Setup done." );
}


/**
 * Setup everything swapchain related.
 */
void VulkanHandler::setupSwapchain() {
	SwapChainSupportDetails swapchainSupport = VulkanSetup::querySwapChainSupport( mPhysicalDevice, &mSurface );
	VkSurfaceFormatKHR surfaceFormat = VulkanSetup::chooseSwapSurfaceFormat( swapchainSupport.formats );

	mSwapchainExtent = VulkanSetup::chooseSwapExtent( swapchainSupport.capabilities );
	mSwapchainFormat = surfaceFormat.format;

	mSwapchain = VulkanSetup::createSwapchain(
		&mSwapchain, &swapchainSupport,
		&mPhysicalDevice, &mLogicalDevice,
		&mSurface, surfaceFormat,
		mSwapchainExtent
	);

	this->retrieveSwapchainImageHandles();
}


/**
 * Set validation layer usage from config file.
 */
void VulkanHandler::setupValidationLayer() {
	VulkanHandler::useValidationLayer = Cfg::get().value<bool>( Cfg::VULKAN_VALIDATION_LAYER );

	if( VulkanHandler::useValidationLayer ) {
		Logger::logInfo( "[VulkanHandler] Validation layer usage is enabled." );
	}
	else {
		Logger::logInfo( "[VulkanHandler] Validation layer usage is disabled." );
	}
}


/**
 * Clean up everything Vulkan related.
 */
void VulkanHandler::teardown() {
	Logger::logInfo( "[VulkanHandler] Teardown beginning ..." );
	Logger::indentChange( 2 );

	if( mWindow != nullptr ) {
		glfwDestroyWindow( mWindow );
		glfwTerminate();
		mWindow = nullptr;

		Logger::logDebug( "[VulkanHandler] GLFW window destroyed and terminated." );
	}

	if( mImageAvailableSemaphore != VK_NULL_HANDLE ) {
		vkDestroySemaphore( mLogicalDevice, mImageAvailableSemaphore, nullptr );
		mImageAvailableSemaphore = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkSemaphore (image available) destroyed." );
	}

	if( mRenderFinishedSemaphore != VK_NULL_HANDLE ) {
		vkDestroySemaphore( mLogicalDevice, mRenderFinishedSemaphore, nullptr );
		mRenderFinishedSemaphore = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkSemaphore (render finished) destroyed." );
	}

	if( mImageAvailableFence != VK_NULL_HANDLE ) {
		vkDestroyFence( mLogicalDevice, mImageAvailableFence, nullptr );
		mImageAvailableFence = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkFence (image available) destroyed." );
	}

	if( mRenderFinishedFence != VK_NULL_HANDLE ) {
		vkDestroyFence( mLogicalDevice, mRenderFinishedFence, nullptr );
		mRenderFinishedFence = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkFence (render finished) destroyed." );
	}

	mImGuiHandler->teardown();
	delete mImGuiHandler;

	if( mDescriptorSetLayout != VK_NULL_HANDLE ) {
		vkDestroyDescriptorSetLayout( mLogicalDevice, mDescriptorSetLayout, nullptr );
		mDescriptorSetLayout = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkDescriptorSetLayout destroyed." );
	}

	if( mDescriptorPool != VK_NULL_HANDLE ) {
		vkDestroyDescriptorPool( mLogicalDevice, mDescriptorPool, nullptr );
		mDescriptorPool = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkDescriptorPool destroyed." );
	}

	if( mUniformBufferMemory != VK_NULL_HANDLE ) {
		vkFreeMemory( mLogicalDevice, mUniformBufferMemory, nullptr );
		mUniformBufferMemory = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkDeviceMemory (uniform) freed." );
	}

	if( mUniformBuffer != VK_NULL_HANDLE ) {
		vkDestroyBuffer( mLogicalDevice, mUniformBuffer, nullptr );
		mUniformBuffer = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkBuffer (uniform) destroyed." );
	}

	if( mVertexBufferMemory != VK_NULL_HANDLE ) {
		vkFreeMemory( mLogicalDevice, mVertexBufferMemory, nullptr );
		mVertexBufferMemory = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkDeviceMemory (vertices) freed." );
	}

	if( mVertexBuffer != VK_NULL_HANDLE ) {
		vkDestroyBuffer( mLogicalDevice, mVertexBuffer, nullptr );
		mVertexBuffer = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkBuffer (vertices) destroyed." );
	}

	if( mModelVerticesBufferMemory != VK_NULL_HANDLE ) {
		vkFreeMemory( mLogicalDevice, mModelVerticesBufferMemory, nullptr );
		mModelVerticesBufferMemory = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkDeviceMemory (model vertices) freed." );
	}

	if( mModelVerticesBuffer != VK_NULL_HANDLE ) {
		vkDestroyBuffer( mLogicalDevice, mModelVerticesBuffer, nullptr );
		mModelVerticesBuffer = VK_NULL_HANDLE;
		Logger::logDebugVerbose( "[VulkanHandler] VkBuffer (model vertices) destroyed." );
	}

	if( mCommandPool != VK_NULL_HANDLE ) {
		vkDestroyCommandPool( mLogicalDevice, mCommandPool, nullptr );
		mCommandPool = VK_NULL_HANDLE;
		Logger::logDebug( "[VulkanHandler] VkCommandPool destroyed." );
	}

	if( mFramebuffers.size() > 0 ) {
		for( size_t i = 0; i < mFramebuffers.size(); i++ ) {
			vkDestroyFramebuffer( mLogicalDevice, mFramebuffers[i], nullptr );
		}

		Logger::logDebugf( "[VulkanHandler] %u VkFramebuffers destroyed.", mFramebuffers.size() );
	}

	if( mGraphicsPipeline != VK_NULL_HANDLE ) {
		vkDestroyPipeline( mLogicalDevice, mGraphicsPipeline, nullptr );
		mGraphicsPipeline = VK_NULL_HANDLE;
		Logger::logDebug( "[VulkanHandler] VkPipeline (graphics) destroyed." );
	}

	if( mPipelineLayout != VK_NULL_HANDLE ) {
		vkDestroyPipelineLayout( mLogicalDevice, mPipelineLayout, nullptr );
		mPipelineLayout = VK_NULL_HANDLE;
		Logger::logDebug( "[VulkanHandler] VkPipelineLayout destroyed." );
	}

	if( mRenderPass != VK_NULL_HANDLE ) {
		vkDestroyRenderPass( mLogicalDevice, mRenderPass, nullptr );
		mRenderPass = VK_NULL_HANDLE;
		Logger::logDebug( "[VulkanHandler] VkRenderPass destroyed." );
	}

	if( mRenderPassInitial != VK_NULL_HANDLE ) {
		vkDestroyRenderPass( mLogicalDevice, mRenderPassInitial, nullptr );
		mRenderPassInitial = VK_NULL_HANDLE;
		Logger::logDebug( "[VulkanHandler] VkRenderPass (initial) destroyed." );
	}

	this->destroyImageViews();
	Logger::logDebugf(
		"[VulkanHandler] Destroyed %u VkImageViews.",
		mSwapchainImageViews.size()
	);

	if( mSwapchain != VK_NULL_HANDLE ) {
		vkDestroySwapchainKHR( mLogicalDevice, mSwapchain, nullptr );
		mSwapchain = VK_NULL_HANDLE;
		Logger::logDebug( "[VulkanHandler] VkSwapchainKHR destroyed." );
	}

	if( mLogicalDevice != VK_NULL_HANDLE ) {
		vkDestroyDevice( mLogicalDevice, nullptr );
		mLogicalDevice = VK_NULL_HANDLE;
		Logger::logDebug( "[VulkanHandler] VkDevice destroyed." );
	}

	if( mSurface != VK_NULL_HANDLE ) {
		vkDestroySurfaceKHR( mInstance, mSurface, nullptr );
		mSurface = VK_NULL_HANDLE;
		Logger::logDebug( "[VulkanHandler] VkSurfaceKHR destroyed." );
	}

	this->destroyDebugCallback();

	if( mInstance != VK_NULL_HANDLE ) {
		vkDestroyInstance( mInstance, nullptr );
		mInstance = VK_NULL_HANDLE;
		Logger::logDebug( "[VulkanHandler] VkInstance destroyed." );
	}

	Logger::indentChange( -2 );
	Logger::logInfo( "[VulkanHandler] Teardown done." );
}


/**
 * Update the FPS counter.
 * @param {double*}   lastTime
 * @param {uint64_t*} numFrames
 */
void VulkanHandler::updateFPS( double* lastTime, uint64_t* numFrames ) {
	double currentTime = glfwGetTime();
	double delta = currentTime - *lastTime;

	*numFrames += 1;

	if( delta >= 1.0 ) {
		char title[32];
		snprintf( title, 32, "PBR (FPS: %3.2f)", (double) *numFrames / delta );

		glfwSetWindowTitle( mWindow, title );
		*lastTime = currentTime;
		*numFrames = 0;
	}
}


/**
 * Update the uniform buffer.
 */
void VulkanHandler::updateUniformBuffer() {
	void* data;

	UniformCamera uniformCamera = {};
	uniformCamera.mvp = mModelViewProjectionMatrix;

	glfwGetWindowSize( mWindow, &uniformCamera.size.x, &uniformCamera.size.y );

	vkMapMemory(
		mLogicalDevice, mUniformBufferMemory, 0,
		sizeof( uniformCamera ), 0, &data
	);
	memcpy( data, &uniformCamera, sizeof( uniformCamera ) );
	vkUnmapMemory( mLogicalDevice, mUniformBufferMemory );
}


/**
 * Wait for the fences, then reset them.
 */
void VulkanHandler::waitForFences() {
	VkResult result = vkWaitForFences(
		mLogicalDevice,
		mFences.size(),
		&mFences[0],
		VK_TRUE,
		std::numeric_limits<uint64_t>::max()
	);

	VulkanHandler::checkVkResult( result, "Failed to wait for fence(s)." );

	vkResetFences( mLogicalDevice, mFences.size(), &mFences[0] );
}


/**
 * Write the uniform buffer to the descriptor set.
 */
void VulkanHandler::writeUniformToDescriptorSet() {
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = mUniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof( UniformCamera );

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = mDescriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo;
	descriptorWrite.pImageInfo = nullptr;
	descriptorWrite.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets( mLogicalDevice, 1, &descriptorWrite, 0, nullptr );
}
