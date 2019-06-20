#ifndef VULKAN_SETUP_H
#define VULKAN_SETUP_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstring>
#include <set>
#include <vector>

#include "../Cfg.h"
#include "../Logger.h"
#include "../VulkanHandler.h"
#include "VulkanDevice.h"

using std::vector;


const vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_LUNARG_standard_validation"
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
		static VkDescriptorSetLayout createDescriptorSetLayout( VkDevice* logicalDevice );
		static VkPipeline createGraphicsPipeline(
			VkDevice *logicalDevice,
			VkPipelineLayout* pipelineLayout,
			VkRenderPass* renderPass,
			VkPipelineShaderStageCreateInfo shaderStages[],
			VkExtent2D* swapchainExtent
		);
		static VkInstance createInstance();
		static VkPipelineLayout createPipelineLayout(
			VkDevice* logicalDevice,
			VkDescriptorSetLayout* descriptorSetLayout
		);
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
		static vector<const char*> getRequiredExtensions();
		static uint32_t getVersionPBR();
		static SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR* surface );
		static void setupDebugCallback( VkInstance* instance, VkDebugReportCallbackEXT* callback );


	protected:
		static VkApplicationInfo buildApplicationInfo();
		static VkInstanceCreateInfo buildInstanceCreateInfo(
			VkApplicationInfo* appInfo,
			vector<const char*>* extensions
		);


};

#endif
