#ifndef VULKANHANDLER_H
#define VULKANHANDLER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui.h>

#include <glm/glm.hpp>
#include <fstream>
#include <vector>

#include "ActionHandler.h"
#include "Cfg.h"
#include "ImGuiHandler.h"
#include "Logger.h"
#include "Vertex.h"
#include "VulkanSetup.h"

using std::vector;


struct ActionHandler;
struct ImGuiHandler;


const vector<Vertex> vertices = {
	{ { -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f } },
	{ { 1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f } },
	{ { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
	{ { -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f } },
	{ { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
	{ { -1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } }
};


class VulkanHandler {


	public:
		uint32_t mFrameIndex = 0;
		ActionHandler* mActionHandler = nullptr;
		GLFWwindow* mWindow = nullptr;
		VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
		VkDevice mLogicalDevice = VK_NULL_HANDLE;
		VkExtent2D mSwapchainExtent;
		VkPhysicalDevice mPhysicalDevice;
		VkQueue mGraphicsQueue;
		VkQueue mPresentQueue;
		VkRenderPass mRenderPass = VK_NULL_HANDLE;
		VkRenderPass mRenderPassInitial = VK_NULL_HANDLE;
		VkSurfaceKHR mSurface = VK_NULL_HANDLE;
		vector<VkFramebuffer> mFramebuffers;
		vector<VkImage> mSwapchainImages;

		VkShaderModule createShaderModule( const vector<char>& code );
		uint32_t findMemoryType( uint32_t typeFitler, VkMemoryPropertyFlags properties );
		void imGuiSetup();
		void initWindow();
		vector<char> loadFileSPV( const string& filename );
		void mainLoop();
		void setup( ActionHandler* ah );
		void teardown();

		static bool useValidationLayer;
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
		static void checkVkResult( VkResult result, const char* errorMessage, const char* className = "VulkanHandler" );


	protected:
		void copyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size );
		void createBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer& buffer,
			VkDeviceMemory& bufferMemory
		);
		void createCommandBuffers();
		void createCommandPool();
		void createDescriptorPool();
		void createFences();
		void createFramebuffers();
		void createGraphicsPipeline();
		void createImageViews();
		void createRenderPass();
		void createSemaphores();
		void createVertexBuffer();
		void destroyDebugCallback();
		void destroyImageViews();
		bool drawFrame();
		void recordCommand();
		void recreateSwapchain();
		void retrieveSwapchainImageHandles();
		void setupSwapchain();
		void updateFPS( double* lastTime, uint64_t* numFrames );
		void waitForFences();

		static void onWindowResize( GLFWwindow* window, int width, int height );


	private:
		ImGuiHandler* mImGuiHandler;
		vector<VkCommandBuffer> mCommandBuffers;
		vector<VkCommandBuffer> mCommandBuffersNow;
		vector<VkFence> mFences;
		vector<VkImageView> mSwapchainImageViews;
		VkBuffer mVertexBuffer = VK_NULL_HANDLE;
		VkCommandPool mCommandPool = VK_NULL_HANDLE;
		VkDebugReportCallbackEXT mDebugCallback;
		VkDeviceMemory mVertexBufferMemory = VK_NULL_HANDLE;
		VkFence mImageAvailableFence = VK_NULL_HANDLE;
		VkFence mRenderFinishedFence = VK_NULL_HANDLE;
		VkFormat mSwapchainFormat;
		VkInstance mInstance = VK_NULL_HANDLE;
		VkPipeline mGraphicsPipeline = VK_NULL_HANDLE;
		VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
		VkSemaphore mImageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore mRenderFinishedSemaphore = VK_NULL_HANDLE;
		VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;


};

#endif
