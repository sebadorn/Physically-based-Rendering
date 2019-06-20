#include "VulkanDevice.h"

using std::set;


/**
 * Check if the given device supports all required extensions.
 * @param  {VkPhysicalDevice} device
 * @return {const bool}
 */
const bool VulkanDevice::checkDeviceExtensionSupport( VkPhysicalDevice device ) {
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
 * Create a logical device.
 * @param {VkSurfaceKHR*}     surface
 * @param {VkPhysicalDevice*} physicalDevice
 * @param {VkDevice*}         logicalDevice
 * @param {VkQueue*}          graphicsQueue
 * @param {VkQueue*}          presentQueue
 */
void VulkanDevice::createLogicalDevice(
	VkSurfaceKHR* surface,
	VkPhysicalDevice* physicalDevice,
	VkDevice* logicalDevice,
	VkQueue* graphicsQueue,
	VkQueue* presentQueue
) {
	int graphicsFamily = -1;
	int presentFamily = -1;
	VulkanDevice::findQueueFamilyIndices( *physicalDevice, &graphicsFamily, &presentFamily, surface );

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
	Logger::logInfo( "[VulkanDevice] Logical VkDevice created." );

	vkGetDeviceQueue( *logicalDevice, graphicsFamily, 0, graphicsQueue );
	vkGetDeviceQueue( *logicalDevice, presentFamily, 0, presentQueue );
	Logger::logInfo( "[VulkanDevice] Retrieved graphics and presentation queues (VkQueue)." );
}


/**
 * Check if the device supports the graphics queue family.
 * @param  {VkPhysicalDevice} device
 * @param  {int*}             graphicsFamily
 * @param  {int*}             presentFamily
 * @param  {VkSurfaceKHR*}    surface
 * @return {const bool}
 */
const bool VulkanDevice::findQueueFamilyIndices(
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
 * Check if the given device is suitable for Vulkan.
 * @param  {VkPhysicalDevice} device
 * @param  {VkSurfaceKHR*}    surface
 * @return {const bool}
 */
const bool VulkanDevice::isDeviceSuitable( VkPhysicalDevice device, VkSurfaceKHR* surface ) {
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;

	vkGetPhysicalDeviceProperties( device, &properties );
	vkGetPhysicalDeviceFeatures( device, &features );

	Logger::logDebugf(
		"[VulkanDevice] Checking if device is suitable: %s",
		properties.deviceName
	);
	Logger::indentChange( 2 );

	if( properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) {
		Logger::logDebugf(
			"[VulkanDevice] Device not suitable,"
			" because it isn't a discrete GPU."
		);
		Logger::indentChange( -2 );

		return false;
	}

	if( !features.geometryShader ) {
		Logger::logDebugf(
			"[VulkanDevice] Device not suitable, because"
			" it doesn't support geometry shaders."
		);
		Logger::indentChange( -2 );

		return false;
	}

	int graphicsFamily = -1;
	int presentFamily = -1;
	const bool queuesFound = VulkanDevice::findQueueFamilyIndices(
		device, &graphicsFamily, &presentFamily, surface
	);

	if( !queuesFound ) {
		Logger::logDebugf(
			"[VulkanDevice] Device not suitable, because the"
			" necessary queue families could not be found."
		);
		Logger::indentChange( -2 );

		return false;
	}

	const bool extensionsSupported = VulkanDevice::checkDeviceExtensionSupport( device );

	if( !extensionsSupported ) {
		Logger::logDebugf(
			"[VulkanDevice] Device not suitable, because the"
			" required extensions are not supported."
		);
		Logger::indentChange( -2 );

		return false;
	}

	SwapChainSupportDetails swapChainDetails = VulkanSetup::querySwapChainSupport( device, surface );

	if( swapChainDetails.formats.empty() ) {
		Logger::logDebugf(
			"[VulkanDevice] Device not suitable, because"
			" it does not support any image formats."
		);
		Logger::indentChange( -2 );

		return false;
	}

	if( swapChainDetails.presentModes.empty() ) {
		Logger::logDebugf(
			"[VulkanDevice] Device not suitable, because it"
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
void VulkanDevice::printDeviceDebugInfo( VkPhysicalDevice device ) {
	if( device == VK_NULL_HANDLE ) {
		Logger::logWarning( "[VulkanDevice] No device given." );
		return;
	}

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties( device, &properties );

	Logger::logInfof( "[VulkanDevice] Name: %s", properties.deviceName );
	Logger::logInfof(
		"[VulkanDevice] Vulkan API: %u.%u.%u",
		VK_VERSION_MAJOR( properties.apiVersion ),
		VK_VERSION_MINOR( properties.apiVersion ),
		VK_VERSION_PATCH( properties.apiVersion )
	);
	Logger::logDebugf( "[VulkanDevice] Vendor ID: %u", properties.vendorID );
	Logger::logDebugf( "[VulkanDevice] Device ID: %u", properties.deviceID );
	Logger::logDebugf( "[VulkanDevice] Driver: %u", properties.driverVersion );
}


/**
 * Select a GPU.
 * @param  {VkInstance*}      instance
 * @param  {VkSurfaceKHR*}    surface
 * @return {VkPhysicalDevice}
 */
VkPhysicalDevice VulkanDevice::selectDevice( VkInstance* instance, VkSurfaceKHR* surface ) {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices( *instance, &deviceCount, nullptr );

	if( deviceCount == 0 ) {
		Logger::logError( "[VulkanDevice] No GPU with Vulkan support found." );
		throw std::runtime_error( "No GPU with Vulkan support found." );
	}

	VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
	vector<VkPhysicalDevice> devices( deviceCount );
	vkEnumeratePhysicalDevices( *instance, &deviceCount, devices.data() );

	for( const auto& device : devices ) {
		if( VulkanDevice::isDeviceSuitable( device, surface ) ) {
			selectedDevice = device;
			break;
		}
	}

	if( selectedDevice == VK_NULL_HANDLE ) {
		Logger::logError( "[VulkanDevice] None of the found GPUs support Vulkan." );
		throw std::runtime_error( "None of the found GPUs support Vulkan." );
	}

	Logger::logInfo( "[VulkanDevice] Suitable GPU found." );
	Logger::indentChange( 2 );
	VulkanDevice::printDeviceDebugInfo( selectedDevice );
	Logger::indentChange( -2 );

	return selectedDevice;
}
