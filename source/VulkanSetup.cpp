#include "VulkanSetup.h"

using std::numeric_limits;
using std::set;
using std::string;


/**
 * Build the VkApplicationInfo for the VkInstanceCreateInfo.
 * @return {VkApplicationInfo}
 */
VkApplicationInfo VulkanSetup::buildApplicationInfo() {
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "PBR";
	appInfo.applicationVersion = VulkanSetup::getVersionPBR();
	appInfo.pEngineName = "PBR";
	appInfo.engineVersion = VulkanSetup::getVersionPBR();
	appInfo.apiVersion = VK_API_VERSION_1_0;

	return appInfo;
}


/**
 * Build the VkInstanceCreateInfo for the VkInstance.
 * @param  {VkApplicationInfo*}        appInfo
 * @param  {std::vector<const char*>*} extensions
 * @return {VkInstanceCreateInfo}
 */
VkInstanceCreateInfo VulkanSetup::buildInstanceCreateInfo(
	VkApplicationInfo* appInfo,
	vector<const char*>* extensions
) {
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = appInfo;

	createInfo.enabledExtensionCount = extensions->size();
	createInfo.ppEnabledExtensionNames = extensions->data();

	if( VulkanHandler::useValidationLayer ) {
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
const bool VulkanSetup::checkDeviceExtensionSupport( VkPhysicalDevice device ) {
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
const bool VulkanSetup::checkValidationLayerSupport() {
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
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1 },
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
	VulkanHandler::checkVkResult( result, "Failed to create VkDescriptorPool." );

	return descriptorPool;
}


/**
 * Create the descriptor set layout.
 * @param  {VkDevice*}             logicalDevice
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
	VulkanHandler::checkVkResult( result, "Failed to create VkDescriptorSetLayout." );

	return descriptorSetLayout;
}


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
 * Create a VkInstance.
 * @return {VkInstance}
 */
VkInstance VulkanSetup::createInstance() {
	if( VulkanHandler::useValidationLayer && !VulkanSetup::checkValidationLayerSupport() ) {
		Logger::logError(
			"[VulkanSetup] No validation layer support found."
			" Will proceed without validation layer."
		);
		VulkanHandler::useValidationLayer = false;
	}

	VkApplicationInfo appInfo = VulkanSetup::buildApplicationInfo();
	vector<const char*> extensions = VulkanSetup::getRequiredExtensions();
	VkInstanceCreateInfo createInfo = VulkanSetup::buildInstanceCreateInfo( &appInfo, &extensions );

	for( const char* extension : extensions ) {
		Logger::logDebugVerbosef( "[VulkanSetup] Required extension: %s", extension );
	}

	Logger::logDebugVerbosef(
		"[VulkanSetup] VkInstanceCreateInfo.enabledLayerCount = %u",
		createInfo.enabledLayerCount
	);

	VkInstance instance;
	VkResult result = vkCreateInstance( &createInfo, nullptr, &instance );
	VulkanHandler::checkVkResult( result, "Failed to create VkInstance.", "VulkanSetup" );

	return instance;
}


/**
 * Create a logical device.
 */
void VulkanSetup::createLogicalDevice(
	VkSurfaceKHR* surface,
	VkPhysicalDevice* physicalDevice,
	VkDevice* logicalDevice,
	VkQueue* graphicsQueue,
	VkQueue* presentQueue
) {
	int graphicsFamily = -1;
	int presentFamily = -1;
	VulkanSetup::findQueueFamilyIndices( *physicalDevice, &graphicsFamily, &presentFamily, surface );

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
	deviceFeatures.shaderClipDistance = VK_TRUE;
	deviceFeatures.shaderCullDistance = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = (uint32_t) queueCreateInfos.size();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = (uint32_t) DEVICE_EXTENSIONS.size();
	createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

	if( VulkanHandler::useValidationLayer ) {
		createInfo.enabledLayerCount = VALIDATION_LAYERS.size();
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	VkResult result = vkCreateDevice(
		*physicalDevice, &createInfo, nullptr, logicalDevice
	);
	VulkanHandler::checkVkResult( result, "Failed to create logical VkDevice.", "VulkanSetup" );
	Logger::logInfo( "[VulkanSetup] Logical VkDevice created." );

	vkGetDeviceQueue( *logicalDevice, graphicsFamily, 0, graphicsQueue );
	vkGetDeviceQueue( *logicalDevice, presentFamily, 0, presentQueue );
	Logger::logInfo( "[VulkanSetup] Retrieved graphics and presentation queues (VkQueue)." );
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
	VulkanSetup::findQueueFamilyIndices( *physicalDevice, &graphicsFamily, &presentFamily, surface );
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
 * Check if the device supports the graphics queue family.
 * @param  {VkPhysicalDevice} device
 * @param  {int*}             graphicsFamily
 * @param  {int*}             presentFamily
 * @param  {VkSurfaceKHR*}    surface
 * @return {const bool}
 */
const bool VulkanSetup::findQueueFamilyIndices(
	VkPhysicalDevice device, int* graphicsFamily, int* presentFamily, VkSurfaceKHR* surface
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
		vkGetPhysicalDeviceSurfaceSupportKHR( device, i, *surface, &presentSupport );

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
vector<const char*> VulkanSetup::getRequiredExtensions() {
	vector<const char*> extensions;
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

	for( uint32_t i = 0; i < glfwExtensionCount; i++ ) {
		extensions.push_back( glfwExtensions[i] );
	}

	if( VulkanHandler::useValidationLayer ) {
		extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
	}

	return extensions;
}


/**
 * Get the version number for this application/engine.
 * @return {uint32_t}
 */
uint32_t VulkanSetup::getVersionPBR() {
	const uint32_t vMajor = Cfg::get().value<uint32_t>( Cfg::VERSION_MAJOR );
	const uint32_t vMinor = Cfg::get().value<uint32_t>( Cfg::VERSION_MINOR );
	const uint32_t vPatch = Cfg::get().value<uint32_t>( Cfg::VERSION_PATCH );

	return VK_MAKE_VERSION( vMajor, vMinor, vPatch );
}


/**
 * Check if the given device is suitable for Vulkan.
 * @param  {VkPhysicalDevice} device
 * @param  {VkSurfaceKHR*}    surface
 * @return {const bool}
 */
const bool VulkanSetup::isDeviceSuitable( VkPhysicalDevice device, VkSurfaceKHR* surface ) {
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;

	vkGetPhysicalDeviceProperties( device, &properties );
	vkGetPhysicalDeviceFeatures( device, &features );

	Logger::logDebugf(
		"[VulkanSetup] Checking if device is suitable: %s",
		properties.deviceName
	);
	Logger::indentChange( 2 );

	if( properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) {
		Logger::logDebugf(
			"[VulkanSetup] Device not suitable,"
			" because it isn't a discrete GPU."
		);
		Logger::indentChange( -2 );

		return false;
	}

	if( !features.geometryShader ) {
		Logger::logDebugf(
			"[VulkanSetup] Device not suitable, because"
			" it doesn't support geometry shaders."
		);
		Logger::indentChange( -2 );

		return false;
	}

	int graphicsFamily = -1;
	int presentFamily = -1;
	const bool queuesFound = VulkanSetup::findQueueFamilyIndices(
		device, &graphicsFamily, &presentFamily, surface
	);

	if( !queuesFound ) {
		Logger::logDebugf(
			"[VulkanSetup] Device not suitable, because the"
			" necessary queue families could not be found."
		);
		Logger::indentChange( -2 );

		return false;
	}

	const bool extensionsSupported = VulkanSetup::checkDeviceExtensionSupport( device );

	if( !extensionsSupported ) {
		Logger::logDebugf(
			"[VulkanSetup] Device not suitable, because the"
			" required extensions are not supported."
		);
		Logger::indentChange( -2 );

		return false;
	}

	SwapChainSupportDetails swapChainDetails = VulkanSetup::querySwapChainSupport( device, surface );

	if( swapChainDetails.formats.empty() ) {
		Logger::logDebugf(
			"[VulkanSetup] Device not suitable, because"
			" it does not support any image formats."
		);
		Logger::indentChange( -2 );

		return false;
	}

	if( swapChainDetails.presentModes.empty() ) {
		Logger::logDebugf(
			"[VulkanSetup] Device not suitable, because it"
			" does not support any presentation modes."
		);
		Logger::indentChange( -2 );

		return false;
	}

	Logger::indentChange( -2 );

	return true;
}


/**
 * Print some debug data about the selected device.
 * @param {VkPhysicalDevice} device
 */
void VulkanSetup::printDeviceDebugInfo( VkPhysicalDevice device ) {
	if( device == VK_NULL_HANDLE ) {
		Logger::logWarning( "[VulkanSetup] No device given." );
		return;
	}

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties( device, &properties );

	Logger::logInfof( "[VulkanSetup] Name: %s", properties.deviceName );
	Logger::logInfof(
		"[VulkanSetup] Vulkan API: %u.%u.%u",
		VK_VERSION_MAJOR( properties.apiVersion ),
		VK_VERSION_MINOR( properties.apiVersion ),
		VK_VERSION_PATCH( properties.apiVersion )
	);
	Logger::logDebugf( "[VulkanSetup] Vendor ID: %u", properties.vendorID );
	Logger::logDebugf( "[VulkanSetup] Device ID: %u", properties.deviceID );
	Logger::logDebugf( "[VulkanSetup] Driver: %u", properties.driverVersion );
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


/**
 * Select a GPU.
 * @param  {VkInstance*}      instance
 * @param  {VkSurfaceKHR*}    surface
 * @return {VkPhysicalDevice}
 */
VkPhysicalDevice VulkanSetup::selectDevice( VkInstance* instance, VkSurfaceKHR* surface ) {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices( *instance, &deviceCount, nullptr );

	if( deviceCount == 0 ) {
		Logger::logError( "[VulkanSetup] No GPU with Vulkan support found." );
		throw std::runtime_error( "No GPU with Vulkan support found." );
	}

	VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
	vector<VkPhysicalDevice> devices( deviceCount );
	vkEnumeratePhysicalDevices( *instance, &deviceCount, devices.data() );

	for( const auto& device : devices ) {
		if( VulkanSetup::isDeviceSuitable( device, surface ) ) {
			selectedDevice = device;
			break;
		}
	}

	if( selectedDevice == VK_NULL_HANDLE ) {
		Logger::logError( "[VulkanSetup] None of the found GPUs support Vulkan." );
		throw std::runtime_error( "None of the found GPUs support Vulkan." );
	}

	Logger::logInfo( "[VulkanSetup] Suitable GPU found." );
	Logger::indentChange( 2 );
	VulkanSetup::printDeviceDebugInfo( selectedDevice );
	Logger::indentChange( -2 );

	return selectedDevice;
}


/**
 * Setup the Vulkan debug callback for the validation layer.
 * @param {VkInstance*}               instance
 * @param {VkDebugReportCallbackEXT*} callback
 */
void VulkanSetup::setupDebugCallback( VkInstance* instance, VkDebugReportCallbackEXT* callback ) {
	if( !VulkanHandler::useValidationLayer ) {
		return;
	}

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = VulkanHandler::debugCallback;

	auto fnCreateDebugCallback = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(
		*instance, "vkCreateDebugReportCallbackEXT"
	);

	if( fnCreateDebugCallback == nullptr ) {
		Logger::logError(
			"[VulkanSetup] Cannot setup debug callback."
			" No such function: \"vkCreateDebugReportCallbackEXT\""
		);
		throw std::runtime_error( "VK_ERROR_EXTENSION_NOT_PRESENT" );
	}

	VkResult result = fnCreateDebugCallback(
		*instance, &createInfo, nullptr, callback
	);
	VulkanHandler::checkVkResult( result, "Failed to setup debug callback.", "VulkanSetup" );
	Logger::logDebug( "[VulkanSetup] Debug callback setup." );
}
