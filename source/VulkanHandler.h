#ifndef VULKANHANDLER_H
#define VULKANHANDLER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <set>
#include <vector>

#include "Cfg.h"
#include "Logger.h"

using std::array;
using std::vector;


struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;

	static array<VkVertexInputAttributeDescription, 2> getAttributeDescription() {
		array<VkVertexInputAttributeDescription, 2> attrDesc = {};
		attrDesc[0].binding = 0;
		attrDesc[0].location = 0;
		attrDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
		attrDesc[0].offset = offsetof( Vertex, pos );

		attrDesc[1].binding = 0;
		attrDesc[1].location = 1;
		attrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrDesc[1].offset = offsetof( Vertex, color );

		return attrDesc;
	}

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDesc = {};
		bindingDesc.binding = 0;
		bindingDesc.stride = sizeof( Vertex );
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDesc;
	}
};

const vector<Vertex> vertices = {
	{ { 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
	{ { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
	{ { -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } }
};


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
		void initWindow();
		const bool isDeviceSuitable( VkPhysicalDevice device );
		vector<char> loadFileSPV( const string& filename );
		void mainLoop();
		void printDeviceDebugInfo( VkPhysicalDevice device );
		void setup();
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
		void createCommandBuffers();
		void createCommandPool();
		void createFramebuffers();
		void createGraphicsPipeline();
		void createImageViews();
		VkInstance createInstance();
		void createLogicalDevice();
		void createRenderPass();
		void createSemaphores();
		void createSurface();
		void createSwapChain();
		void createVertexBuffer();
		void destroyDebugCallback();
		void destroyImageViews();
		void drawFrame();
		const bool findQueueFamilyIndices(
			VkPhysicalDevice device,
			int* graphicsFamily,
			int* presentFamily
		);
		uint32_t findMemoryType( uint32_t typeFitler, VkMemoryPropertyFlags properties );
		SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device );
		void recreateSwapchain();
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
		static void onWindowResize( GLFWwindow* window, int width, int height );

	private:
		bool mUseValidationLayer;
		GLFWwindow* mWindow = nullptr;
		vector<VkCommandBuffer> mCommandBuffers;
		vector<VkFramebuffer> mFramebuffers;
		vector<VkImage> mSwapchainImages;
		vector<VkImageView> mSwapchainImageViews;
		VkBuffer mVertexBuffer = VK_NULL_HANDLE;
		VkCommandPool mCommandPool = VK_NULL_HANDLE;
		VkDebugReportCallbackEXT mDebugCallback;
		VkDevice mLogicalDevice = VK_NULL_HANDLE;
		VkDeviceMemory mVertexBufferMemory = VK_NULL_HANDLE;
		VkExtent2D mSwapchainExtent;
		VkFormat mSwapchainFormat;
		VkInstance mInstance = VK_NULL_HANDLE;
		VkPhysicalDevice mPhysicalDevice;
		VkPipeline mGraphicsPipeline = VK_NULL_HANDLE;
		VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
		VkQueue mGraphicsQueue;
		VkQueue mPresentQueue;
		VkRenderPass mRenderPass = VK_NULL_HANDLE;
		VkSemaphore mImageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore mRenderFinishedSemaphore = VK_NULL_HANDLE;
		VkSurfaceKHR mSurface = VK_NULL_HANDLE;
		VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;

};

#endif
