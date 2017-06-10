#ifndef VULKAN_SETUP_H
#define VULKAN_SETUP_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstring>
#include <set>
#include <vector>

#include "Cfg.h"
#include "Logger.h"
#include "VulkanHandler.h"

using std::vector;


const vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_LUNARG_standard_validation"
};

const vector<const char*> DEVICE_EXTENSIONS = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	vector<VkSurfaceFormatKHR> formats;
	vector<VkPresentModeKHR> presentModes;
};


class VulkanSetup {


	public:
		static const bool checkValidationLayerSupport();
		static VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities );
		static VkPresentModeKHR chooseSwapPresentMode(
			const vector<VkPresentModeKHR>& availablePresentModes
		);
		static VkSurfaceFormatKHR chooseSwapSurfaceFormat(
			const vector<VkSurfaceFormatKHR>& availableFormats
		);
		static VkDescriptorPool createDescriptorPool( VkDevice* logicalDevice );
		static VkPipeline createGraphicsPipeline(
			VkDevice *logicalDevice,
			VkPipelineLayout* pipelineLayout,
			VkRenderPass* renderPass,
			VkPipelineShaderStageCreateInfo shaderStages[],
			VkExtent2D* swapchainExtent
		);
		static VkInstance createInstance();
		static void createLogicalDevice(
			VkSurfaceKHR* surface,
			VkPhysicalDevice* physicalDevice,
			VkDevice* logicalDevice,
			VkQueue* graphicsQueue,
			VkQueue* presentQueue
		);
		static VkPipelineLayout createPipelineLayout( VkDevice* logicalDevice );
		static void createSurface( VkInstance* instance, GLFWwindow* window, VkSurfaceKHR* surface );
		static VkSwapchainKHR createSwapchain(
			VkSwapchainKHR* oldSwapchain,
			SwapChainSupportDetails* swapchainSupport,
			VkPhysicalDevice* physicalDevice,
			VkDevice* logicalDevice,
			VkSurfaceKHR* surface,
			VkSurfaceFormatKHR surfaceFormat,
			VkExtent2D extent
		);
		static const bool findQueueFamilyIndices(
			VkPhysicalDevice device,
			int* graphicsFamily,
			int* presentFamily,
			VkSurfaceKHR* surface
		);
		static vector<const char*> getRequiredExtensions();
		static uint32_t getVersionPBR();
		static const bool isDeviceSuitable( VkPhysicalDevice device, VkSurfaceKHR* surface );
		static void printDeviceDebugInfo( VkPhysicalDevice device );
		static SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR* surface );
		static VkPhysicalDevice selectDevice( VkInstance* instance, VkSurfaceKHR* surface );
		static void setupDebugCallback( VkInstance* instance, VkDebugReportCallbackEXT* callback );


	protected:
		static VkApplicationInfo buildApplicationInfo();
		static VkInstanceCreateInfo buildInstanceCreateInfo(
			VkApplicationInfo* appInfo,
			vector<const char*>* extensions
		);
		static const bool checkDeviceExtensionSupport( VkPhysicalDevice device );


};

#endif
