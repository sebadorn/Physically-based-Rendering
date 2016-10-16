#ifndef VULKANHANDLER_H
#define VULKANHANDLER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
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


struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	vector<VkSurfaceFormatKHR> formats;
	vector<VkPresentModeKHR> presentModes;
};


class VulkanHandler {

	public:
		const bool checkValidationLayerSupport();
		VkShaderModule createShaderModule( const vector<char>& code );
		vector<const char*> getRequiredExtensions();
		const bool isDeviceSuitable( VkPhysicalDevice device );
		vector<char> loadFileSPV( const string& filename );
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
		VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities );
		VkPresentModeKHR chooseSwapPresentMode(
			const vector<VkPresentModeKHR>& availablePresentModes
		);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(
			const vector<VkSurfaceFormatKHR>& availableFormats
		);
		void createFramebuffers();
		void createGraphicsPipeline();
		void createImageViews();
		VkInstance createInstance();
		void createLogicalDevice();
		void createRenderPass();
		void createSurface( GLFWwindow* window );
		void createSwapChain();
		void destroyDebugCallback();
		void destroyImageViews();
		const bool findQueueFamilyIndices(
			VkPhysicalDevice device,
			int* graphicsFamily,
			int* presentFamily
		);
		SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device );
		void retrieveSwapchainImageHandles();
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
		vector<VkFramebuffer> mFramebuffers;
		vector<VkImage> mSwapchainImages;
		vector<VkImageView> mSwapchainImageViews;
		VkDebugReportCallbackEXT mDebugCallback;
		VkDevice mLogicalDevice;
		VkExtent2D mSwapchainExtent;
		VkFormat mSwapchainFormat;
		VkInstance mInstance;
		VkPhysicalDevice mPhysicalDevice;
		VkPipeline mGraphicsPipeline;
		VkPipelineLayout mPipelineLayout;
		VkQueue mGraphicsQueue;
		VkQueue mPresentQueue;
		VkRenderPass mRenderPass;
		VkSurfaceKHR mSurface;
		VkSwapchainKHR mSwapchain;

};

#endif
