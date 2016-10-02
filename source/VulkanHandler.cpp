#include "VulkanHandler.h"

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
	const int graphicsQueueIndex = this->findGraphicsQueueIndex( mPhysicalDevice );
	float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = 0;

	if( mUseValidationLayer ) {
		createInfo.enabledLayerCount = VALIDATION_LAYERS.size();
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	VkResult result = vkCreateDevice( mPhysicalDevice, &createInfo, nullptr, &mLogicalDevice );

	if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to create logical device." );
		throw std::runtime_error( "Failed to create logical device." );
	}
	else {
		Logger::logInfo( "[VulkanHandler] Logical device created." );
	}

	vkGetDeviceQueue( mLogicalDevice, graphicsQueueIndex, 0, &mGraphicsQueue );
	Logger::logInfo( "[VulkanHandler] Graphics queue handle retrieved." );
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
 * Check if the device supports the graphics queue family.
 * @param  {VkPhysicalDevice} device
 * @return {const int}
 */
const int VulkanHandler::findGraphicsQueueIndex( VkPhysicalDevice device ) {
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

	vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties(
		device, &queueFamilyCount, queueFamilies.data()
	);

	int i = 0;
	int index = -1;

	for( const auto& queueFamily : queueFamilies ) {
		i++;

		if( queueFamily.queueCount <= 0 ) {
			continue;
		}

		if( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
			index = i;
			break;
		}
	}

	return index;
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

	if( properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) {
		Logger::logDebugVerbosef(
			"[VulkanHandler] Device not suitable, because it isn't a discrete GPU: %s",
			properties.deviceName
		);

		return false;
	}

	if( !features.geometryShader ) {
		Logger::logDebugVerbosef(
			"[VulkanHandler] Device not suitable, because it doesn't support geometry shaders: %s",
			properties.deviceName
		);

		return false;
	}

	if( this->findGraphicsQueueIndex( device ) < 0 ) {
		return false;
	}

	return true;
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

	mUseValidationLayer = Cfg::get().value<bool>( Cfg::VULKAN_VALIDATION_LAYER );

	if( mUseValidationLayer ) {
		Logger::logInfo( "[VulkanHandler] Validation layer usage is enabled." );
	}
	else {
		Logger::logInfo( "[VulkanHandler] Validation layer usage is disabled." );
	}

	mInstance = this->createInstance();
	this->setupDebugCallback();
	mPhysicalDevice = this->selectDevice();
	this->createLogicalDevice();

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

	if( mLogicalDevice != VK_NULL_HANDLE ) {
		vkDestroyDevice( mLogicalDevice, nullptr );
		Logger::logDebug( "[VulkanHandler] VkDevice destroyed." );
	}

	this->destroyDebugCallback();

	if( mInstance ) {
		vkDestroyInstance( mInstance, nullptr );
		Logger::logDebug( "[VulkanHandler] VkInstance destroyed." );
	}

	Logger::indentChange( -2 );
	Logger::logInfo( "[VulkanHandler] Teardown done." );
}