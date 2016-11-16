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


	protected:
		void bindRenderData();
		void createDescriptors();
		void createPipeline( VkShaderModule* vertModule, VkShaderModule* fragModule );
		void createShaders( VkShaderModule* vertModule, VkShaderModule* fragModule );
		void drawImGuiData( ImDrawData* drawData );
		const char* getClipboardText();
		void renderDrawList( ImDrawData* drawData );
		void setClipboardText( const char* text );
		void setupFontSampler();
		size_t updateIndexBuffer( ImDrawData* drawData );
		size_t updateVertexBuffer( ImDrawData* drawData );
		void uploadRenderData( ImDrawData* drawData, size_t verticesSize, size_t indicesSize );


	private:
		VulkanHandler* mVH;

		vector<VkCommandBuffer> mCommandBuffers;
		VkBuffer mIndexBuffer = VK_NULL_HANDLE;
		VkBuffer mVertexBuffer = VK_NULL_HANDLE;
		VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
		VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
		VkDeviceMemory mIndexBufferMemory = VK_NULL_HANDLE;
		VkDeviceMemory mVertexBufferMemory = VK_NULL_HANDLE;
		VkPipeline mGraphicsPipeline = VK_NULL_HANDLE;
		VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
		VkSampler mFontSampler = VK_NULL_HANDLE;


};

#endif
