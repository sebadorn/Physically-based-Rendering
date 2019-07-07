#include "VulkanSetup.h"

using std::numeric_limits;


/**
 * Choose the swap extent.
 * @param  {const VkSurfaceCapabilities&} capabilities
 * @return {VkExtent2D}
 */
VkExtent2D VulkanSetup::chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) {
	if( capabilities.currentExtent.width != numeric_limits<uint32_t>::max() ) {
		return capabilities.currentExtent;
	}

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


/**
 * Choose the presentation mode.
 * @param  {const std::vector<VkPresentModeKHR>&} availablePresentModes
 * @return {VkPresentModeKHR}
 */
VkPresentModeKHR VulkanSetup::chooseSwapPresentMode(
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
VkSurfaceFormatKHR VulkanSetup::chooseSwapSurfaceFormat(
	const vector<VkSurfaceFormatKHR>& availableFormats
) {
	// Surface has no preferred format.
	if( availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED ) {
		Logger::logDebugVerbosef(
			"[VulkanSetup] Surface has no preferred format."
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
				"[VulkanSetup] Surface supports BGRA 32bit and sRGB."
			);
			return format;
		}
	}

	// Just use the first one.
	Logger::logWarning(
		"[VulkanSetup] Preferred surface format not found."
		" Selecting first one available."
	);

	return availableFormats[0];
}


/**
 * Create a VkDescriptorPool.
 * @param  {VkDevice*}        logicalDevice
 * @return {VkDescriptorPool}
 */
VkDescriptorPool VulkanSetup::createDescriptorPool( VkDevice* logicalDevice ) {
	VkDescriptorPoolSize poolSize[11] = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 3 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1 }
	};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 11;
	poolInfo.pPoolSizes = poolSize;
	poolInfo.maxSets = 11 * 1;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	VkDescriptorPool descriptorPool;
	VkResult result = vkCreateDescriptorPool(
		*logicalDevice, &poolInfo, nullptr, &descriptorPool
	);
	VulkanHandler::checkVkResult( result, "Failed to create VkDescriptorPool.", "VulkanSetup" );

	return descriptorPool;
}


/**
 * Create the descriptor set layout.
 * @param  {VkDevice*} logicalDevice
 * @return {VkDescriptorSetLayout}
 */
VkDescriptorSetLayout VulkanSetup::createDescriptorSetLayout( VkDevice* logicalDevice ) {
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.binding = 0;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.descriptorCount = 1;
	layoutBinding.pImmutableSamplers = nullptr;
	layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &layoutBinding;

	VkDescriptorSetLayout descriptorSetLayout;

	VkResult result = vkCreateDescriptorSetLayout(
		*logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout
	);
	VulkanHandler::checkVkResult( result, "Failed to create VkDescriptorSetLayout.", "VulkanSetup" );

	return descriptorSetLayout;
}


/**
 *
 * @param  {VkDevice*}                         logicalDevice
 * @param  {VkPipelineLayout*}                 pipelineLayout
 * @param  {VkRenderPass*}                     renderPass
 * @param  {VkPipelineShaderStageCreateInfo[]} shaderStages
 * @param  {VkExtend2D*}                       swapchainExtent
 * @return {VkPipeline}
 */
VkPipeline VulkanSetup::createGraphicsPipeline(
	VkDevice *logicalDevice,
	VkPipelineLayout* pipelineLayout,
	VkRenderPass* renderPass,
	VkPipelineShaderStageCreateInfo shaderStages[],
	VkExtent2D* swapchainExtent
) {
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescription = Vertex::getAttributeDescription();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = attributeDescription.size();
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) swapchainExtent->width;
	viewport.height = (float) swapchainExtent->height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = *swapchainExtent;

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

	pipelineInfo.layout = *pipelineLayout;
	pipelineInfo.renderPass = *renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	VkPipeline graphicsPipeline;
	VkResult resultPipeline = vkCreateGraphicsPipelines(
		*logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline
	);
	VulkanHandler::checkVkResult(
		resultPipeline, "Failed to create graphics VkPipeline.", "VulkanSetup"
	);
	Logger::logInfo( "[VulkanSetup] Created graphics VkPipeline." );

	return graphicsPipeline;
}


/**
 * Create the pipeline layout.
 * @param  {VkDevice*}              logicalDevice
 * @param  {VkDescriptorSetLayout*} descriptorSetLayout
 * @return {VkPipelineLayout}
 */
VkPipelineLayout VulkanSetup::createPipelineLayout(
	VkDevice* logicalDevice,
	VkDescriptorSetLayout* descriptorSetLayout
) {
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 1;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	VkPipelineLayout pipelineLayout;

	VkResult result = vkCreatePipelineLayout(
		*logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout
	);
	VulkanHandler::checkVkResult( result, "Failed to create VkPipelineLayout.", "VulkanSetup" );
	Logger::logDebug( "[VulkanSetup] Created VkPipelineLayout." );

	return pipelineLayout;
}


/**
 * Create a VkShaderModule from loaded SPV code.
 * @param  {VkDevice*}                logicalDevice
 * @param  {const std::vector<char>&} code
 * @return {VkShaderModule}
 */
VkShaderModule VulkanSetup::createShaderModule(
	VkDevice* logicalDevice,
	const vector<char>& code
) {
	VkShaderModule shaderModule;

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = (uint32_t*) code.data();

	VkResult result = vkCreateShaderModule(
		*logicalDevice, &createInfo, nullptr, &shaderModule
	);
	VulkanHandler::checkVkResult( result, "Failed to create VkShaderModule." );

	return shaderModule;
}


/**
 * Create the window surface.
 * @param {VkInstance*}   instance
 * @param {GLFWwindow*}   window
 * @param {VkSurfaceKHR*} surface
 */
void VulkanSetup::createSurface( VkInstance* instance, GLFWwindow* window, VkSurfaceKHR* surface ) {
	VkResult result = glfwCreateWindowSurface( *instance, window, nullptr, surface );
	VulkanHandler::checkVkResult( result, "Failed to create VkSurfaceKHR.", "VulkanSetup" );
	Logger::logInfo( "[VulkanSetup] Window surface (VkSurfaceKHR) created." );
}


/**
 * Create the swap chain.
 * @param {VkSwapchainKHR*} oldSwapchain
 * @param {VkExtent2D}      extent
 */
VkSwapchainKHR VulkanSetup::createSwapchain(
	VkSwapchainKHR* oldSwapchain,
	SwapChainSupportDetails* swapchainSupport,
	VkPhysicalDevice* physicalDevice,
	VkDevice* logicalDevice,
	VkSurfaceKHR* surface,
	VkSurfaceFormatKHR surfaceFormat,
	VkExtent2D extent
) {
	VkPresentModeKHR presentMode = VulkanSetup::chooseSwapPresentMode( swapchainSupport->presentModes );

	uint32_t imageCount = swapchainSupport->capabilities.minImageCount + 1;

	if(
		swapchainSupport->capabilities.maxImageCount > 0 &&
		imageCount > swapchainSupport->capabilities.maxImageCount
	) {
		imageCount = swapchainSupport->capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = *surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	int graphicsFamily = -1;
	int presentFamily = -1;
	VulkanDevice::findQueueFamilyIndices( *physicalDevice, &graphicsFamily, &presentFamily, surface );
	uint32_t queueFamilyIndices[] = {
		(uint32_t) graphicsFamily,
		(uint32_t) presentFamily
	};

	if( graphicsFamily != presentFamily ) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;

		Logger::logDebugVerbosef(
			"[VulkanSetup] Image sharing mode will"
			" be VK_SHARING_MODE_CONCURRENT."
		);
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;

		Logger::logDebugVerbosef(
			"[VulkanSetup] Image sharing mode will"
			" be VK_SHARING_MODE_EXCLUSIVE."
		);
	}

	createInfo.preTransform = swapchainSupport->capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_FALSE;
	createInfo.oldSwapchain = *oldSwapchain;

	VkSwapchainKHR newSwapchain;
	VkResult result = vkCreateSwapchainKHR( *logicalDevice, &createInfo, nullptr, &newSwapchain );
	VulkanHandler::checkVkResult( result, "Failed to create VkSwapchainKHR.", "VulkanSetup" );
	Logger::logInfo( "[VulkanSetup] VkSwapchainKHR created." );

	// Destroy old swap chain if existing.
	if( *oldSwapchain != VK_NULL_HANDLE ) {
		vkDestroySwapchainKHR( *logicalDevice, *oldSwapchain, nullptr );
	}

	return newSwapchain;
}


/**
 * Query the device's swap chain support.
 * @param  {VkPhysicalDevice}        device
 * @param  {VkSurfaceKHR*}           surface
 * @return {SwapChainSupportDetails}
 */
SwapChainSupportDetails VulkanSetup::querySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR* surface ) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		device, *surface, &details.capabilities
	);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR( device, *surface, &formatCount, nullptr );

	if( formatCount > 0 ) {
		details.formats.resize( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			device, *surface, &formatCount, details.formats.data()
		);
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(
		device, *surface, &presentModeCount, nullptr
	);

	if( presentModeCount > 0 ) {
		details.presentModes.resize( presentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device, *surface, &presentModeCount, details.presentModes.data()
		);
	}

	return details;
}
