#ifndef VULKANHANDLER_H
#define VULKANHANDLER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstring>
#include <set>
#include <vector>

#include "Cfg.h"
#include "Logger.h"

using std::vector;


const vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_LUNARG_standard_validation"
};

const vector<const char*> DEVICE_EXTENSIONS = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


class VulkanHandler {

	public:
		const bool checkValidationLayerSupport();
		vector<const char*> getRequiredExtensions();
		const bool isDeviceSuitable( VkPhysicalDevice device );
		void printDeviceDebugInfo( VkPhysicalDevice device );
		void setup( GLFWwindow* window );
		void teardown();

		static uint32_t getVersionPBR();

	protected:
		VkApplicationInfo buildApplicationInfo();
		VkInstanceCreateInfo buildInstanceCreateInfo(
			VkApplicationInfo* appInfo,
			vector<const char*>* extensions
		);
		const bool checkDeviceExtensionSupport( VkPhysicalDevice device );
		VkInstance createInstance();
		void createLogicalDevice();
		void createSurface( GLFWwindow* window );
		void destroyDebugCallback();
		const bool findQueueFamilyIndices(
			VkPhysicalDevice device,
			int* graphicsFamily,
			int* presentFamily
		);
		VkPhysicalDevice selectDevice();
		void setupDebugCallback();

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugReportFlagsEXT flags,
			VkDebugReportObjectTypeEXT objType,
			uint64_t obj,
			size_t location,
			int32_t code,
			const char* layerPrefix,
			const char* msg,
			void* userData
		);

	private:
		bool mUseValidationLayer;
		VkDebugReportCallbackEXT mDebugCallback;
		VkDevice mLogicalDevice = VK_NULL_HANDLE;
		VkInstance mInstance;
		VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
		VkQueue mGraphicsQueue;
		VkQueue mPresentQueue;
		VkSurfaceKHR mSurface;

};

#endif
