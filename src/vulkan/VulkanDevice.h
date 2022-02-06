#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include <set>
#include <vector>

#include "../Logger.h"
#include "../PathTracer.h"

using std::vector;


const vector<const char*> DEVICE_EXTENSIONS = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


class VulkanDevice {


	public:
		static void createLogicalDevice(
			VkSurfaceKHR* surface,
			VkPhysicalDevice* physicalDevice,
			VkDevice* logicalDevice,
			VkQueue* graphicsQueue,
			VkQueue* presentQueue,
			VkQueue* computeQueue
		);
		static const bool findQueueFamilyIndices(
			VkPhysicalDevice device,
			int* graphicsFamily,
			int* presentFamily,
			int* computeFamily,
			VkSurfaceKHR* surface
		);
		static vector<const char*> getRequiredExtensions();
		static const bool isDeviceSuitable( VkPhysicalDevice device, VkSurfaceKHR* surface );
		static void printDeviceDebugInfo( VkPhysicalDevice device );
		static VkPhysicalDevice selectDevice( VkInstance* instance, VkSurfaceKHR* surface );


	protected:
		static const bool checkDeviceExtensionSupport( VkPhysicalDevice device );


};

#endif
