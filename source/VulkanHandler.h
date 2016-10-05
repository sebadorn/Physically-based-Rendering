#ifndef VULKANHANDLER_H
#define VULKANHANDLER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstring>
#include <vector>

#include "Cfg.h"
#include "Logger.h"

using std::vector;


const vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_LUNARG_standard_validation"
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
		VkDebugReportCallbackEXT mDebugCallback;
		VkQueue mGraphicsQueue;
		VkDevice mLogicalDevice = VK_NULL_HANDLE;
		VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
		VkInstance mInstance;
		VkSurfaceKHR mSurface;
		bool mUseValidationLayer;

};

#endif
