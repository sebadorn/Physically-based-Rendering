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
 * Create the framebuffers.
 */
void VulkanHandler::createFramebuffers() {
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
	mSwapchainImageViews.resize( mSwapchainImages.size() );

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
			Logger::logErrorf( "[VulkanHandler] Failed to create VkImageView #%u.", i );
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
 * @param {GLFWwindow*} window
 */
void VulkanHandler::createSurface( GLFWwindow* window ) {
	VkResult result = glfwCreateWindowSurface( mInstance, window, nullptr, &mSurface );

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
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR( mLogicalDevice, &createInfo, nullptr, &mSwapchain );

	if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to create VkSwapchainKHR." );
		throw std::runtime_error( "Failed to create VkSwapchainKHR." );
	}
	else {
		Logger::logInfo( "[VulkanHandler] VkSwapchainKHR created." );
	}

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
	uint32_t i = 0;

	for( const auto& imageView : mSwapchainImageViews ) {
		vkDestroyImageView( mLogicalDevice, imageView, nullptr );
		i++;
	}

	Logger::logDebugf( "[VulkanHandler] Destroyed %u VkImageViews.", i );
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
 * @param {GLFWwindow*} window
 */
void VulkanHandler::setup( GLFWwindow* window ) {
	Logger::logInfo( "[VulkanHandler] Setup beginning ..." );
	Logger::indentChange( 2 );

	mUseValidationLayer = Cfg::get().value<bool>( Cfg::VULKAN_VALIDATION_LAYER );

	if( mUseValidationLayer ) {
		Logger::logInfo( "[VulkanHandler] Validation layer usage is enabled." );
	}
	else {
		Logger::logInfo( "[VulkanHandler] Validation layer usage is disabled." );
	}

	mInstance = this->createInstance();
	this->setupDebugCallback();
	this->createSurface( window );
	mPhysicalDevice = this->selectDevice();
	this->createLogicalDevice();
	this->createSwapChain();
	this->retrieveSwapchainImageHandles();
	this->createImageViews();
	this->createRenderPass();
	this->createGraphicsPipeline();
	this->createFramebuffers();

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

	if( mFramebuffers.size() > 0 ) {
		for( size_t i = 0; i < mFramebuffers.size(); i++ ) {
			vkDestroyFramebuffer( mLogicalDevice, mFramebuffers[i], nullptr );
		}

		Logger::logDebugf( "[VulkanHandler] %u VkFramebuffers destroyed.", mFramebuffers.size() );
	}

	if( mGraphicsPipeline ) {
		vkDestroyPipeline( mLogicalDevice, mGraphicsPipeline, nullptr );
		Logger::logDebug( "[VulkanHandler] VkPipeline (graphics) destroyed." );
	}

	if( mPipelineLayout ) {
		vkDestroyPipelineLayout( mLogicalDevice, mPipelineLayout, nullptr );
		Logger::logDebug( "[VulkanHandler] VkPipelineLayout destroyed." );
	}

	if( mRenderPass ) {
		vkDestroyRenderPass( mLogicalDevice, mRenderPass, nullptr );
		Logger::logDebug( "[VulkanHandler] VkRenderPass destroyed." );
	}

	this->destroyImageViews();

	if( mSwapchain ) {
		vkDestroySwapchainKHR( mLogicalDevice, mSwapchain, nullptr );
		Logger::logDebug( "[VulkanHandler] VkSwapchainKHR destroyed." );
	}

	if( mLogicalDevice ) {
		vkDestroyDevice( mLogicalDevice, nullptr );
		Logger::logDebug( "[VulkanHandler] VkDevice destroyed." );
	}

	if( mSurface ) {
		vkDestroySurfaceKHR( mInstance, mSurface, nullptr );
		Logger::logDebug( "[VulkanHandler] VkSurfaceKHR destroyed." );
	}

	this->destroyDebugCallback();

	if( mInstance ) {
		vkDestroyInstance( mInstance, nullptr );
		Logger::logDebug( "[VulkanHandler] VkInstance destroyed." );
	}

	Logger::indentChange( -2 );
	Logger::logInfo( "[VulkanHandler] Teardown done." );
}
