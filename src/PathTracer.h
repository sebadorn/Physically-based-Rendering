#ifndef PATHTRACER_H
#define PATHTRACER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <fstream>
#include <vector>

#include "accelstructures/AccelStructure.h"
#include "ActionHandler.h"
#include "Cfg.h"
#include "ImGuiHandler.h"
#include "Logger.h"
#include "ModelRenderer.h"
#include "parsers/ObjParser.h"
#include "Vertex.h"
#include "vulkan/VulkanDevice.h"
#include "vulkan/VulkanInstance.h"
#include "vulkan/VulkanSetup.h"

using std::vector;


struct ActionHandler;
struct ImGuiHandler;
struct ModelRenderer;


const vector<Vertex> vertices = {
	{ { -1.0f, -1.0f } },
	{ { +1.0f, -1.0f } },
	{ { +1.0f, +1.0f } },
	{ { -1.0f, -1.0f } },
	{ { +1.0f, +1.0f } },
	{ { -1.0f, +1.0f } }
};

struct UniformCamera {
	glm::mat4 mvp;
	glm::ivec2 size;
};

struct ModelVertices {
	vector<glm::vec4> vertices;
};


class PathTracer {


	public:
		bool mHasModel = false;
		int mFOV = 45;
		int mFamilyIndexCompute = -1;
		int mFamilyIndexGraphics = -1;
		int mFamilyIndexPresentation = -1;
		uint32_t mFrameIndex = 0;
		ActionHandler* mActionHandler = nullptr;
		GLFWwindow* mWindow = nullptr;
		glm::mat4 mModelViewProjectionMatrix;
		glm::vec3 mCameraCenter = glm::vec3( 0.0f, 0.0f, 1.0f );
		glm::vec3 mCameraEye = glm::vec3( 0.0f, 1.0f, 3.0f );
		glm::vec3 mCameraUp = glm::vec3( 0.0f, 1.0f, 0.0f );
		ModelRenderer* mModelRenderer;
		VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
		VkDevice mLogicalDevice = VK_NULL_HANDLE;
		VkExtent2D mSwapchainExtent;
		VkFormat mSwapchainFormat;
		VkPhysicalDevice mPhysicalDevice;
		VkQueue mComputeQueue;
		VkQueue mGraphicsQueue;
		VkQueue mPresentQueue;
		VkRenderPass mRenderPass = VK_NULL_HANDLE;
		VkSemaphore mImageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore mRenderFinishedSemaphore = VK_NULL_HANDLE;
		VkSurfaceKHR mSurface = VK_NULL_HANDLE;
		vector<VkFramebuffer> mFramebuffers;
		vector<VkImage> mSwapchainImages;
		vector<VkImageView> mSwapchainImageViews;

		void calculateMatrices();
		void exit();
		uint32_t findMemoryType( uint32_t typeFitler, VkMemoryPropertyFlags properties );
		void imGuiSetup();
		void initWindow();
		void mainLoop();
		void setup( ActionHandler* ah );

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
		static vector<char> loadFileSPV( const string& filename );


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
		VkDescriptorSet createDescriptorSet();
		void createFences();
		void createFramebuffers();
		void createGraphicsPipeline();
		void createImageViews();
		void createRenderPass();
		void createSemaphores();
		void createUniformBuffer();
		void createVertexBuffer();
		void destroyDebugCallback();
		void destroyImageViews();
		bool drawFrameEmpty();
		bool drawFrameModel();
		void recordCommand();
		void recreateSwapchain();
		void retrieveSwapchainImageHandles();
		void setupSwapchain();
		void updateFPS( double* lastTime, uint64_t* numFrames );
		void updateUniformBuffer();
		void waitForFencesEmpty();
		void waitForFencesModel();
		void writeUniformToDescriptorSet();

		static void onWindowResize( GLFWwindow* window, int width, int height );


	private:
		ImGuiHandler* mImGuiHandler;
		ObjParser* mObjParser;
		vector<VkCommandBuffer> mCommandBuffers;
		vector<VkCommandBuffer> mCommandBuffersNow;
		vector<VkFence> mFences;
		VkBuffer mModelVerticesBuffer = VK_NULL_HANDLE;
		VkBuffer mUniformBuffer = VK_NULL_HANDLE;
		VkBuffer mVertexBuffer = VK_NULL_HANDLE;
		VkCommandPool mCommandPool = VK_NULL_HANDLE;
		VkDebugReportCallbackEXT mDebugCallback;
		VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
		VkDeviceMemory mModelVerticesBufferMemory = VK_NULL_HANDLE;
		VkDeviceMemory mUniformBufferMemory = VK_NULL_HANDLE;
		VkDeviceMemory mVertexBufferMemory = VK_NULL_HANDLE;
		VkFence mImageAvailableFence = VK_NULL_HANDLE;
		VkFence mRenderFinishedFence = VK_NULL_HANDLE;
		VkInstance mInstance = VK_NULL_HANDLE;
		VkPipeline mGraphicsPipeline = VK_NULL_HANDLE;
		VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
		VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;


};

#endif
