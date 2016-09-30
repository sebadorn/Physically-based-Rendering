#include "VulkanHandler.h"

using std::vector;


/**
 * Setup Vulkan: Create a VkInstance,
 * check for support, pick a device etc.
 */
void VulkanHandler::setup() {
	this->mUseValidationLayer = Cfg::get().value<bool>( Cfg::VULKAN_VALIDATION_LAYER );
	this->mInstance = this->createInstance();
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
	if( this->mUseValidationLayer && !this->checkValidationLayerSupport() ) {
		Logger::logError( "[VulkanHandler] No validation layer support. Will proceed without validation layer." );
	}

	const int vMajor = Cfg::get().value<int>( Cfg::VERSION_MAJOR );
	const int vMinor = Cfg::get().value<int>( Cfg::VERSION_MINOR );
	const int vPatch = Cfg::get().value<int>( Cfg::VERSION_PATCH );

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "PBR";
	appInfo.applicationVersion = VK_MAKE_VERSION( vMajor, vMinor, vPatch );
	appInfo.pEngineName = "PBR";
	appInfo.engineVersion = VK_MAKE_VERSION( vMajor, vMinor, vPatch );
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	if( this->mUseValidationLayer ) {
		createInfo.enabledLayerCount = VALIDATION_LAYERS.size();
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	VkInstance instance;
	VkResult result = vkCreateInstance( &createInfo, nullptr, &instance );

	if( result != VK_SUCCESS ) {
		Logger::logError( "[VulkanHandler] Failed to create VkInstance." );
	}

	return instance;
}
