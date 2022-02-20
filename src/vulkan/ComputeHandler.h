#ifndef COMPUTE_HANDLER
#define COMPUTE_HANDLER

#include "../Logger.h"
#include "../PathTracer.h"
#include "BuilderVk.h"

using std::vector;


struct PathTracer;


class ComputeHandler {


	public:
		VkDeviceMemory mStorageImageMemory = VK_NULL_HANDLE;
		VkFence mDrawingFence = VK_NULL_HANDLE;
		VkImage mStorageImage = VK_NULL_HANDLE;
		VkImageView mStorageImageView = VK_NULL_HANDLE;
		VkSemaphore mSemaphoreComputeDone = VK_NULL_HANDLE;

		void draw( uint32_t const frameIndex );
		void setup( PathTracer* pt );
		void teardown();


	protected:
		vector<VkCommandBuffer> allocateCommandBuffers(
			uint32_t const numCmdBuffers,
			VkCommandPool const &cmdPool
		);
		vector<VkDescriptorSet> allocateDescriptorSets(
			uint32_t const numImages,
			VkDescriptorSetLayout const &layout,
			VkDescriptorPool const &pool
		);
		VkDeviceMemory allocateStorageImageMemory( VkImage const &image );
		void beginCommandBuffer( VkCommandBuffer const &cmdBuffer );
		VkDescriptorPool createDescriptorPool( uint32_t const numImages );
		VkDescriptorSetLayout createDescriptorSetLayout();
		VkPipeline createPipeline(
			VkPipelineLayout const &layout,
			VkShaderModule const &shaderModule
		);
		VkPipelineLayout createPipelineLayout( VkDescriptorSetLayout const &descSetLayout );
		VkShaderModule createShader();
		VkImage createStorageImage( uint32_t const width, uint32_t const height );
		VkImageView createStorageImageView( VkImage const &image );
		void endCommandBuffer( VkCommandBuffer const &cmdBuffer );
		void recordCommandBuffer(
			VkCommandBuffer const &cmdBuffer,
			VkDescriptorSet const &descSet,
			VkImage const &image
		);
		void updateDescriptorSet(
			VkImageView const &imageView,
			VkDescriptorSet const &descSet
		);


	private:
		PathTracer* mPathTracer;
		vector<VkCommandBuffer> mCmdBuffers;
		vector<VkDescriptorSet> mDescSets;
		VkCommandPool mCmdPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout mDescSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool mDescPool = VK_NULL_HANDLE;
		VkPipeline mPipe = VK_NULL_HANDLE;
		VkPipelineLayout mPipeLayout = VK_NULL_HANDLE;

};


#endif
