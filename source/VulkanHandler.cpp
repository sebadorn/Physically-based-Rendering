#include "VulkanHandler.h"

using std::numeric_limits;
using std::set;
using std::string;
using std::vector;


/**
 * Build the VkApplicationInfo for the VkInstanceCreateInfo.
 * @return {VkApplicationInfo}
 */
VkApplicationInfo VulkanHandler::buildApplicationInfo() {
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "PBR";
	appInfo.applicationVersion = VulkanHandler::getVersionPBR();
	appInfo.pEngineName = "PBR";
	appInfo.engineVersion = VulkanHandler::getVersionPBR();
	appInfo.apiVersion = VK_API_VERSION_1_0;

	return appInfo;
}


/**
 * Build the VkInstanceCreateInfo for the VkInstance.
 * @param  {VkApplicationInfo*}        appInfo
 * @param  {std::vector<const char*>*} extensions
 * @return {VkInstanceCreateInfo}
 */
VkInstanceCreateInfo VulkanHandler::buildInstanceCreateInfo(
	VkApplicationInfo* appInfo,
	vector<const char*>* extensions
) {
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = appInfo;

	createInfo.enabledExtensionCount = extensions->size();
	createInfo.ppEnabledExtensionNames = extensions->data();

	if( mUseValidationLayer ) {
		createInfo.enabledLayerCount = VALIDATION_LAYERS.size();
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	return createInfo;
}


/**
 * Check if the given device supports all required extensions.
 * @param  {VkPhysicalDevice} device
 * @return {const bool}
 */
const bool VulkanHandler::checkDeviceExtensionSupport( VkPhysicalDevice device ) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(
		device, nullptr, &extensionCount, nullptr
	);

	vector<VkExtensionProperties> availableExtensions( extensionCount );
	vkEnumerateDeviceExtensionProperties(
		device, nullptr, &extensionCount, availableExtensions.data()
	);

	set<string> requiredExtensions( DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end() );

	for( const auto& extension : availableExtensions ) {
		requiredExtensions.erase( extension.extensionName );
	}

	return requiredExtensions.empty();
}


/**
 * Check if the requested validation layers are supported.
 * @return {const bool}
 */
const bool VulkanHandler::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

	vector<VkLayerProperties> availableLayers( layerCount );
	vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

	for( const char* layerName : VALIDATION_LAYERS ) {
		bool layerFound = false;

		for( const auto& layerProperties : availableLayers ) {
			if( strcmp( layerName, layerProperties.layerName ) == 0 ) {
				layerFound = true;
				break;
			}
		}

		if( !layerFound ) {
			return false;
		}
	}

	return true;
}


/**
 * Choose the swap extent.
 * @param  {const VkSurfaceCapabilities&} capabilities
 * @return {VkExtent2D}
 */
VkExtent2D VulkanHandler::chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) {
	if( capabilities.currentExtent.width != numeric_limits<uint32_t>::max() ) {
		return capabilities.currentExtent;
	}
	else {
		VkExtent2D actualExtent = {
			Cfg::get().value<uint32_t>( Cfg::WINDOW_WIDTH ),
			Cfg::get().value<uint32_t>( Cfg::WINDOW_HEIGHT )
		};

		actualExtent.width = std::max(
			capabilities.minImageExtent.width,
			std::min( capabilities.maxImageExtent.width, actualExtent.width )
		);
		actualExtent.height = std::max(
			capabilities.minImageExtent.height,
			std::min( capabilities.maxImageExtent.height, actualExtent.height )
		);

		return actualExtent;
	}
}


/**
 * Choose the presentation mode.
 * @param  {const std::vector<VkPresentModeKHR>&} availablePresentModes
 * @return {VkPresetnModeKHR}
 */
VkPresentModeKHR VulkanHandler::chooseSwapPresentMode(
	const vector<VkPresentModeKHR>& availablePresentModes
) {
	for( const auto& presentMode : availablePresentModes ) {
		if( presentMode == VK_PRESENT_MODE_MAILBOX_KHR ) {
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}


/**
 * Choose the format for the swap sufrace.
 * @param  {const std::vector<VkSurfaceFormatKHR>&} availableFormats
 * @return {VkSurfaceFormatKHR}
 */
VkSurfaceFormatKHR VulkanHandler::chooseSwapSurfaceFormat(
	const vector<VkSurfaceFormatKHR>& availableFormats
) {
	// Surface has no preferred format.
	if( availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED ) {
		Logger::logDebugVerbosef(
			"[VulkanHandler] Surface has no preferred format."
			" Choosing BGRA 32bit and sRGB."
		);
		return {
			VK_FORMAT_B8G8R8A8_UNORM,
			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		};
	}

	// Look if our preferred combination is available.
	for( const auto& format : availableFormats ) {
		if(
			format.format == VK_FORMAT_B8G8R8A8_UNORM &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		) {
			Logger::logDebugVerbosef(
				"[VulkanHandler] Surface supports BGRA 32bit and sRGB."
			);
			return format;
		}
	}

	// Just use the first one.
	Logger::logWarning(
		"[VulkanHandler] Preferred surface format not found."
		" Selecting first one available."
	);

	return availableFormats[0];
}


/**
 * Create the command buffers.
 */
void VulkanHandler::createCommandBuffers() {
	// Free old command buffers.
	if( mCommandBuffers.size() > 0 ) {
		vkFreeCommandBuffers(
			mLogicalDevice, mCommandPool, mCommandBuffers.size(), mCommandBuffers.data()
		);
	}

	mCommandBuffers.resize( mFramebuffers.size() );

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = mCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t) mCommandBuffers.size();

	VkResult result = vkAllocateCommandBuffers(
		mLogicalDevice, &allocInfo, mCommandBuffers.data()
	);

	if( result != VK_SUCCESS ) {
		Logger::logErrorf(
			"[VulkanHandler] Failed to allocate %u VkCommandBuffers.",
			mCommandBuffers.size()
		);
		throw std::runtime_error( "Failed to allocate VkCommandBuffers." );
	}
	else {
		Logger::logInfof(
			"[VulkanHandler] Allocated %u VkCommandBuffers.",
			mCommandBuffers.size()
		);
	}

	for( size_t i = 0; i < mCommandBuffers.size(); i++ ) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer( mCommandBuffers[i], &beginInfo );

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = mRenderPass;
		renderPassInfo.framebuffer = mFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = mSwapchainExtent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(
			mCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE
		);
		vkCmdBindPipeline(
			mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline
		);
		vkCmdDraw( mCommandBuffers[i], 3, 1, 0, 0 );
		vkCmdEndRenderPass( mCommandBuffers[i] );

		VkResult resultEnd = vkEndCommandBuffer( mCommandBuffers[i] );

		if( resultEnd != VK_SUCCESS ) {
			Logger::logErrorf(
				"[VulkanHandler] Failed to record command buffer (%u/%u).",
				i, mCommandBuffers.size()
			);
			throw std::runtime_error( "Failed to record command buffer." );
		}
	}

	Logger::logDebug( "[VulkanHandler] Recorded command buffers." );
}


/**
 * Create the command pool for the graphics queue.
 */
void VulkanHandler::createCommandPool() {
	int graphicsFamily = -1;
	int presentFamily = -1;
	this->findQueueFamilyIndices( mPhysicalDevice, &graphicsFamily, &presentFamily );

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = graphicsFamily;
	poolInfo.flags = 0;

	VkResult result = vkCreateCommandPool(
		mLogicalDevice, &poolInfo, nullptr, &mCommandPool
	);

	if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to create VkCommandPool." );
		throw std::runtime_error( "Failed to create VkCommandPool." );
	}
	else {
		Logger::logInfo( "[VulkanHandler] Created VkCommandPool." );
	}
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

		if( result != VK_SUCCESS ) {
			Logger::logError( "[VulkanHandler] Failed to create VkFramebuffer." );
			throw std::runtime_error( "Failed to create VkFramebuffer." );
		}
	}

	Logger::logInfof( "[VulkanHandler] Created %u VkFramebuffers.", mFramebuffers.size() );
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

	auto vertShaderCode = this->loadFileSPV( "source/shaders/vert.spv" );
	auto fragShaderCode = this->loadFileSPV( "source/shaders/frag.spv" );
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


	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) mSwapchainExtent.width;
	viewport.height = (float) mSwapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = mSwapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;


	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;


	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;


	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;


	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 1;
	dynamicState.pDynamicStates = dynamicStates;


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	VkResult resultLayout = vkCreatePipelineLayout(
		mLogicalDevice, &pipelineLayoutInfo, nullptr, &mPipelineLayout
	);

	if( resultLayout != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to create VkPipelineLayout." );
		throw std::runtime_error( "Failed to create VkPipelineLayout." );
	}
	else {
		Logger::logDebug( "[VulkanHandler] Created VkPipelineLayout." );
	}


	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;

	pipelineInfo.layout = mPipelineLayout;
	pipelineInfo.renderPass = mRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	VkResult resultPipeline = vkCreateGraphicsPipelines(
		mLogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline
	);

	if( resultPipeline != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to create graphics VkPipeline." );
		throw std::runtime_error( "Failed to create graphics VkPipeline." );
	}
	else {
		Logger::logInfo( "[VulkanHandler] Created graphics VkPipeline." );
	}


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

		if( result != VK_SUCCESS ) {
			Logger::logErrorf( "[VulkanHandler] Failed to create VkImageView %u.", i );
			throw std::runtime_error( "Failed to create VkImageView." );
		}
	}

	Logger::logDebugf( "[VulkanHandler] Created %u VkImageViews.", mSwapchainImages.size() );
}


/**
 * Create a VkInstance.
 * @return {VkInstance}
 */
VkInstance VulkanHandler::createInstance() {
	if( mUseValidationLayer && !this->checkValidationLayerSupport() ) {
		Logger::logError( "[VulkanHandler] No validation layer support found. Will proceed without validation layer." );
		mUseValidationLayer = false;
	}

	VkApplicationInfo appInfo = this->buildApplicationInfo();
	vector<const char*> extensions = this->getRequiredExtensions();
	VkInstanceCreateInfo createInfo = this->buildInstanceCreateInfo( &appInfo, &extensions );

	for( const char* extension : extensions ) {
		Logger::logDebugVerbosef( "[VulkanHandler] Required extension: %s", extension );
	}

	Logger::logDebugVerbosef(
		"[VulkanHandler] VkInstanceCreateInfo.enabledLayerCount = %u",
		createInfo.enabledLayerCount
	);

	VkInstance instance;
	VkResult result = vkCreateInstance( &createInfo, nullptr, &instance );

	if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to create VkInstance." );
		throw std::runtime_error( "Failed to create VkInstance." );
	}

	return instance;
}


/**
 * Create a logical device.
 */
void VulkanHandler::createLogicalDevice() {
	int graphicsFamily = -1;
	int presentFamily = -1;
	this->findQueueFamilyIndices( mPhysicalDevice, &graphicsFamily, &presentFamily );

	float queuePriority = 1.0f;
	vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	set<int> uniqueQueueFamilies = { graphicsFamily, presentFamily };

	for( int queueFamily : uniqueQueueFamilies ) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.push_back( queueCreateInfo );
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = (uint32_t) queueCreateInfos.size();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = (uint32_t) DEVICE_EXTENSIONS.size();
	createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

	if( mUseValidationLayer ) {
		createInfo.enabledLayerCount = VALIDATION_LAYERS.size();
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	VkResult result = vkCreateDevice( mPhysicalDevice, &createInfo, nullptr, &mLogicalDevice );

	if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to create logical device (VkDevice)." );
		throw std::runtime_error( "Failed to create logical device." );
	}
	else {
		Logger::logInfo( "[VulkanHandler] Logical device (VkDevice) created." );
	}

	vkGetDeviceQueue( mLogicalDevice, graphicsFamily, 0, &mGraphicsQueue );
	vkGetDeviceQueue( mLogicalDevice, presentFamily, 0, &mPresentQueue );
	Logger::logInfo( "[VulkanHandler] Retrieved graphics and presentation queues (VkQueue)." );
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

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass( mLogicalDevice, &renderPassInfo, nullptr, &mRenderPass );

	if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to create VkRenderPass." );
		throw std::runtime_error( "Failed to create VkRenderPass." );
	}
	else {
		Logger::logInfo( "[VulkanHandler] Created VkRenderPass." );
	}
}


/**
 * Create semaphores for the draw frames function.
 */
void VulkanHandler::createSemaphores() {
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult result = vkCreateSemaphore(
		mLogicalDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphore
	);

	if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to create VkSemaphore (image available)." );
		throw std::runtime_error( "Failed to create VkSemaphore (image available)." );
	}
	else {
		Logger::logDebugVerbose( "[VulkanHandler] Created VkSemaphore (image available)." );
	}

	result = vkCreateSemaphore(
		mLogicalDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphore
	);

	if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to create VkSemaphore (render finished)." );
		throw std::runtime_error( "Failed to create VkSemaphore (render finished)." );
	}
	else {
		Logger::logDebugVerbose( "[VulkanHandler] Created VkSemaphore (render finished)." );
	}
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

	VkResult result = vkCreateShaderModule( mLogicalDevice, &createInfo, nullptr, &shaderModule );

	if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to create shader module." );
		throw std::runtime_error( "Failed to create shader module." );
	}

	return shaderModule;
}


/**
 * Create the window surface.
 */
void VulkanHandler::createSurface() {
	VkResult result = glfwCreateWindowSurface( mInstance, mWindow, nullptr, &mSurface );

	if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to create window surface (VkSurfaceKHR)." );
		throw std::runtime_error( "Failed to create window surface." );
	}
	else {
		Logger::logInfo( "[VulkanHandler] Window surface (VkSurfaceKHR) created." );
	}
}


/**
 * Create the swap chain.
 */
void VulkanHandler::createSwapChain() {
	SwapChainSupportDetails swapChainSupport = this->querySwapChainSupport( mPhysicalDevice );

	VkSurfaceFormatKHR surfaceFormat = this->chooseSwapSurfaceFormat( swapChainSupport.formats );
	VkPresentModeKHR presentMode = this->chooseSwapPresentMode( swapChainSupport.presentModes );
	VkExtent2D extent = this->chooseSwapExtent( swapChainSupport.capabilities );

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if(
		swapChainSupport.capabilities.maxImageCount > 0 &&
		imageCount > swapChainSupport.capabilities.maxImageCount
	) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = mSurface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	int graphicsFamily = -1;
	int presentFamily = -1;
	this->findQueueFamilyIndices( mPhysicalDevice, &graphicsFamily, &presentFamily );
	uint32_t queueFamilyIndices[] = {
		(uint32_t) graphicsFamily,
		(uint32_t) presentFamily
	};

	if( graphicsFamily != presentFamily ) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;

		Logger::logDebugVerbosef(
			"[VulkanHandler] Image sharing mode will"
			" be VK_SHARING_MODE_CONCURRENT."
		);
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;

		Logger::logDebugVerbosef(
			"[VulkanHandler] Image sharing mode will"
			" be VK_SHARING_MODE_EXCLUSIVE."
		);
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_FALSE;

	VkSwapchainKHR oldSwapchain = mSwapchain;
	createInfo.oldSwapchain = oldSwapchain;

	VkSwapchainKHR newSwapchain;
	VkResult result = vkCreateSwapchainKHR( mLogicalDevice, &createInfo, nullptr, &newSwapchain );

	if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to create VkSwapchainKHR." );
		throw std::runtime_error( "Failed to create VkSwapchainKHR." );
	}
	else {
		Logger::logInfo( "[VulkanHandler] VkSwapchainKHR created." );
	}

	// Destroy old swap chain if existing.
	if( mSwapchain != VK_NULL_HANDLE ) {
		vkDestroySwapchainKHR( mLogicalDevice, mSwapchain, nullptr );
	}

	mSwapchain = newSwapchain;
	mSwapchainFormat = surfaceFormat.format;
	mSwapchainExtent = extent;
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
 */
void VulkanHandler::drawFrame() {
	uint32_t imageIndex;

	VkResult result = vkAcquireNextImageKHR(
		mLogicalDevice,
		mSwapchain,
		std::numeric_limits< uint64_t >::max(), // disable timeout
		mImageAvailableSemaphore,
		VK_NULL_HANDLE,
		&imageIndex
	);

	if( result == VK_ERROR_OUT_OF_DATE_KHR ) {
		this->recreateSwapchain();
		return;
	}
	else if( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR ) {
		Logger::logError( "[VulkanHandler] Failed to acquire swap chain image." );
		throw std::runtime_error( "Failed to acquire swap chain image." );
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { mImageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mCommandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	result = vkQueueSubmit( mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );

	if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to submit queue (graphics)." );
		throw std::runtime_error( "Failed to submit queue (graphics)." );
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapchains[] = { mSwapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR( mPresentQueue, &presentInfo );

	if( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ) {
		this->recreateSwapchain();
	}
	else if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to present swap chain image." );
		throw std::runtime_error( "Failed to present swap chain image." );
	}
}


/**
 * Check if the device supports the graphics queue family.
 * @param  {VkPhysicalDevice} device
 * @param  {int*}             graphicsFamily
 * @param  {int*}             presentFamily
 * @return {const bool}
 */
const bool VulkanHandler::findQueueFamilyIndices(
	VkPhysicalDevice device, int* graphicsFamily, int* presentFamily
) {
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

	vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties(
		device, &queueFamilyCount, queueFamilies.data()
	);

	*graphicsFamily = -1;
	*presentFamily = -1;
	int i = -1;

	for( const auto& queueFamily : queueFamilies ) {
		i++;

		if( queueFamily.queueCount <= 0 ) {
			continue;
		}

		if( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
			*graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR( device, i, mSurface, &presentSupport );

		if( presentSupport ) {
			*presentFamily = i;
		}

		if( *graphicsFamily >= 0 && *presentFamily >= 0 ) {
			return true;
		}
	}

	return false;
}


/**
 * Get a list of the required extensions.
 * @return {std::vector<const char*>}
 */
vector<const char*> VulkanHandler::getRequiredExtensions() {
	vector<const char*> extensions;
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

	for( uint32_t i = 0; i < glfwExtensionCount; i++ ) {
		extensions.push_back( glfwExtensions[i] );
	}

	if( mUseValidationLayer ) {
		extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
	}

	return extensions;
}


/**
 * Get the version number for this application/engine.
 * @return {uint32_t}
 */
uint32_t VulkanHandler::getVersionPBR() {
	const uint32_t vMajor = Cfg::get().value<uint32_t>( Cfg::VERSION_MAJOR );
	const uint32_t vMinor = Cfg::get().value<uint32_t>( Cfg::VERSION_MINOR );
	const uint32_t vPatch = Cfg::get().value<uint32_t>( Cfg::VERSION_PATCH );

	return VK_MAKE_VERSION( vMajor, vMinor, vPatch );
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
 * Check if the given device is suitable for Vulkan.
 * @param  {VkPhysicalDevice} device
 * @return {const bool}
 */
const bool VulkanHandler::isDeviceSuitable( VkPhysicalDevice device ) {
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;

	vkGetPhysicalDeviceProperties( device, &properties );
	vkGetPhysicalDeviceFeatures( device, &features );

	Logger::logDebugf(
		"[VulkanHandler] Checking if device is suitable: %s",
		properties.deviceName
	);
	Logger::indentChange( 2 );

	if( properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) {
		Logger::logDebugf(
			"[VulkanHandler] Device not suitable,"
			" because it isn't a discrete GPU."
		);
		Logger::indentChange( -2 );

		return false;
	}

	if( !features.geometryShader ) {
		Logger::logDebugf(
			"[VulkanHandler] Device not suitable, because"
			" it doesn't support geometry shaders."
		);
		Logger::indentChange( -2 );

		return false;
	}

	int graphicsFamily = -1;
	int presentFamily = -1;
	const bool queuesFound = this->findQueueFamilyIndices(
		device, &graphicsFamily, &presentFamily
	);

	if( !queuesFound ) {
		Logger::logDebugf(
			"[VulkanHandler] Device not suitable, because the"
			" necessary queue families could not be found."
		);
		Logger::indentChange( -2 );

		return false;
	}

	const bool extensionsSupported = this->checkDeviceExtensionSupport( device );

	if( !extensionsSupported ) {
		Logger::logDebugf(
			"[VulkanHandler] Device not suitable, because the"
			" required extensions are not supported."
		);
		Logger::indentChange( -2 );

		return false;
	}

	SwapChainSupportDetails swapChainDetails = this->querySwapChainSupport( device );

	if( swapChainDetails.formats.empty() ) {
		Logger::logDebugf(
			"[VulkanHandler] Device not suitable, because"
			" it does not support any image formats."
		);
		Logger::indentChange( -2 );

		return false;
	}

	if( swapChainDetails.presentModes.empty() ) {
		Logger::logDebugf(
			"[VulkanHandler] Device not suitable, because it"
			" does not support any presentation modes."
		);
		Logger::indentChange( -2 );

		return false;
	}

	Logger::indentChange( -2 );

	return true;
}


/**
 * Load an SPV file.
 * @param  {const std::string&} filename
 * @return {std::vector<char>}
 */
vector<char> VulkanHandler::loadFileSPV( const string& filename ) {
	std::ifstream file( filename, std::ios::ate | std::ios::binary );

	if (!file.is_open()) {
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
 * Start the main loop.
 */
void VulkanHandler::mainLoop() {
	double lastTime = 0.0;
	uint64_t nbFrames = 0;

	while( !glfwWindowShouldClose( mWindow ) ) {
		glfwPollEvents();
		this->drawFrame();

		double currentTime = glfwGetTime();
		double delta = currentTime - lastTime;
		nbFrames++;

		if( delta >= 1.0 ) {
			char title[32];
			snprintf( title, 32, "PBR (FPS: %3.2f)", (double) nbFrames / delta );

			glfwSetWindowTitle( mWindow, title );
			nbFrames = 0;
			lastTime = currentTime;
		}
	}

	vkDeviceWaitIdle( mLogicalDevice );
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
 * Print some debug data about the selected device.
 * @param {VkPhysicalDevice} device
 */
void VulkanHandler::printDeviceDebugInfo( VkPhysicalDevice device ) {
	if( device == VK_NULL_HANDLE ) {
		Logger::logWarning( "[VulkanHandler] No device given." );
		return;
	}

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties( device, &properties );

	Logger::logInfof( "[VulkanHandler] Name: %s", properties.deviceName );
	Logger::logInfof(
		"[VulkanHandler] Vulkan API: %u.%u.%u",
		VK_VERSION_MAJOR( properties.apiVersion ),
		VK_VERSION_MINOR( properties.apiVersion ),
		VK_VERSION_PATCH( properties.apiVersion )
	);
	Logger::logDebugf( "[VulkanHandler] Vendor ID: %u", properties.vendorID );
	Logger::logDebugf( "[VulkanHandler] Device ID: %u", properties.deviceID );
	Logger::logDebugf( "[VulkanHandler] Driver: %u", properties.driverVersion );
}


/**
 * Query the device's swap chain support.
 * @param  {VkPhysicalDevice}        device
 * @return {SwapChainSupportDetails}
 */
SwapChainSupportDetails VulkanHandler::querySwapChainSupport( VkPhysicalDevice device ) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		device, mSurface, &details.capabilities
	);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR( device, mSurface, &formatCount, nullptr );

	if( formatCount > 0 ) {
		details.formats.resize( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			device, mSurface, &formatCount, details.formats.data()
		);
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(
		device, mSurface, &presentModeCount, nullptr
	);

	if( presentModeCount > 0 ) {
		details.presentModes.resize( presentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device, mSurface, &presentModeCount, details.presentModes.data()
		);
	}

	return details;
}


/**
 * Recreate the swapchain.
 */
void VulkanHandler::recreateSwapchain() {
	Logger::mute();
	Logger::logDebug( "[VulkanHandler] Recreating swap chain ..." );
	Logger::indentChange( 2 );

	vkDeviceWaitIdle( mLogicalDevice );

	this->createSwapChain();
	this->retrieveSwapchainImageHandles();
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
 * Select a GPU.
 * @return {VkPhysicalDevice}
 */
VkPhysicalDevice VulkanHandler::selectDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices( mInstance, &deviceCount, nullptr );

	if( deviceCount == 0 ) {
		Logger::logError( "[VulkanHandler] No GPU with Vulkan support found." );
		throw std::runtime_error( "No GPU with Vulkan support found." );
	}

	VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
	vector<VkPhysicalDevice> devices( deviceCount );
	vkEnumeratePhysicalDevices( mInstance, &deviceCount, devices.data() );

	for( const auto& device : devices ) {
		if( this->isDeviceSuitable( device ) ) {
			selectedDevice = device;
			break;
		}
	}

	if( selectedDevice == VK_NULL_HANDLE ) {
		Logger::logError( "[VulkanHandler] None of the found GPUs support Vulkan." );
		throw std::runtime_error( "None of the found GPUs support Vulkan." );
	}

	Logger::logInfo( "[VulkanHandler] Suitable GPU found." );
	Logger::indentChange( 2 );
	this->printDeviceDebugInfo( selectedDevice );
	Logger::indentChange( -2 );

	return selectedDevice;
}


/**
 * Setup Vulkan: Create a VkInstance,
 * check for support, pick a device etc.
 */
void VulkanHandler::setup() {
	Logger::logInfo( "[VulkanHandler] Setup beginning ..." );
	Logger::indentChange( 2 );

	this->initWindow();

	mUseValidationLayer = Cfg::get().value<bool>( Cfg::VULKAN_VALIDATION_LAYER );

	if( mUseValidationLayer ) {
		Logger::logInfo( "[VulkanHandler] Validation layer usage is enabled." );
	}
	else {
		Logger::logInfo( "[VulkanHandler] Validation layer usage is disabled." );
	}

	mInstance = this->createInstance();
	this->setupDebugCallback();
	this->createSurface();
	mPhysicalDevice = this->selectDevice();
	this->createLogicalDevice();
	this->createSwapChain();
	this->retrieveSwapchainImageHandles();
	this->createImageViews();
	this->createRenderPass();
	this->createGraphicsPipeline();
	this->createFramebuffers();
	this->createCommandPool();
	this->createCommandBuffers();
	this->createSemaphores();

	Logger::indentChange( -2 );
	Logger::logInfo( "[VulkanHandler] Setup done." );
}


/**
 * Setup the Vulkan debug callback for the validation layer.
 */
void VulkanHandler::setupDebugCallback() {
	if( !mUseValidationLayer ) {
		return;
	}

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = VulkanHandler::debugCallback;

	auto fnCreateDebugCallback = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(
		mInstance, "vkCreateDebugReportCallbackEXT"
	);

	if( fnCreateDebugCallback == nullptr ) {
		Logger::logError( "[VulkanHandler] Cannot setup debug callback. No such function: \"vkCreateDebugReportCallbackEXT\"" );
		throw std::runtime_error( "VK_ERROR_EXTENSION_NOT_PRESENT" );
	}

	VkResult createResult = fnCreateDebugCallback(
		mInstance, &createInfo, nullptr, &mDebugCallback
	);

	if( createResult != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to setup debug callback." );
		throw std::runtime_error( "Failed to setup debug callback." );
	}
	else {
		Logger::logDebug( "[VulkanHandler] Debug callback setup." );
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

	this->destroyImageViews();
	Logger::logDebugf( "[VulkanHandler] Destroyed %u VkImageViews.", mSwapchainImageViews.size() );

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
