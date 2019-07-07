#ifndef IMGUIHANDLER_H
#define IMGUIHANDLER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <vector>

#include "Logger.h"
#include "VulkanHandler.h"

using std::vector;


struct VulkanHandler;


class ImGuiHandler {


	public:
		void draw();
		void setup( VulkanHandler* vh );
		void teardown();

		vector<VkCommandBuffer> mCommandBuffers;


	protected:
		void bindRenderData();
		void buildUIStructure();
		void createCommandBuffers();
		void createDescriptors();
		void createFences();
		void createPipeline( VkShaderModule* vertModule, VkShaderModule* fragModule );
		void createRenderPass();
		void createShaders( VkShaderModule* vertModule, VkShaderModule* fragModule );
		void drawImGuiData( ImDrawData* drawData );
		const char* getClipboardText();
		void renderDrawList( ImDrawData* drawData );
		void setClipboardText( const char* text );
		void setupFontSampler();
		size_t updateIndexBuffer( ImDrawData* drawData );
		size_t updateVertexBuffer( ImDrawData* drawData );
		void uploadFonts();
		void uploadRenderData( ImDrawData* drawData, size_t verticesSize, size_t indicesSize );


	private:
		bool mMousePressed[3] = { false, false, false };
		double mTime = 0.0f;
		float mMouseWheel = 0.0f;
		size_t mBufferMemoryAlignment = 256;
		VulkanHandler* mVH;

		vector<VkFence> mFences;
		VkBuffer mIndexBuffer = VK_NULL_HANDLE;
		VkBuffer mUploadBuffer = VK_NULL_HANDLE;
		VkBuffer mVertexBuffer = VK_NULL_HANDLE;
		VkCommandPool mCommandPool = VK_NULL_HANDLE;
		VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
		VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
		VkDeviceMemory mFontMemory = VK_NULL_HANDLE;
		VkDeviceMemory mIndexBufferMemory = VK_NULL_HANDLE;
		VkDeviceMemory mUploadBufferMemory = VK_NULL_HANDLE;
		VkDeviceMemory mVertexBufferMemory = VK_NULL_HANDLE;
		VkImage mFontImage = VK_NULL_HANDLE;
		VkImageView mFontView = VK_NULL_HANDLE;
		VkPipeline mGraphicsPipeline = VK_NULL_HANDLE;
		VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
		VkRenderPass mRenderPass = VK_NULL_HANDLE;
		VkSampler mFontSampler = VK_NULL_HANDLE;
		VkSemaphore mSemaphore = VK_NULL_HANDLE;


};

#endif
