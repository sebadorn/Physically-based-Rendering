#ifndef COMPUTE_HANDLER
#define COMPUTE_HANDLER

#include "../Logger.h"
#include "../PathTracer.h"

using std::vector;


struct PathTracer;


class ComputeHandler {


	public:
		void draw( uint32_t frameIndex );
		void setup( PathTracer* pt );
		void teardown();

		VkDeviceMemory mStorageImageMemory = VK_NULL_HANDLE;
		VkFence mDrawingFence = VK_NULL_HANDLE;
		VkImage mStorageImage = VK_NULL_HANDLE;
		VkImageView mStorageImageView = VK_NULL_HANDLE;
		VkSemaphore mSemaphoreComputeDone = VK_NULL_HANDLE;


	protected:
		vector<VkCommandBuffer> allocateCommandBuffers(
			uint32_t numCmdBuffers,
			VkCommandPool cmdPool
		);
		vector<VkDescriptorSet> allocateDescriptorSets(
			uint32_t numImages,
			VkDescriptorSetLayout layout,
			VkDescriptorPool pool
		);
		VkDeviceMemory allocateStorageImageMemory( VkImage image );
		void beginCommandBuffer( VkCommandBuffer cmdBuffer );
		VkCommandPool createCommandPool();
		VkDescriptorPool createDescriptorPool( uint32_t numImages );
		VkDescriptorSetLayout createDescriptorSetLayout();
		VkPipeline createPipeline( VkPipelineLayout layout, VkShaderModule shaderModule );
		VkPipelineLayout createPipelineLayout( VkDescriptorSetLayout descSetLayout );
		VkSemaphore createSemaphore();
		VkShaderModule createShader();
		VkImage createStorageImage( uint32_t width, uint32_t height );
		VkImageView createStorageImageView( VkImage image );
		void endCommandBuffer( VkCommandBuffer cmdBuffer );
		void initCommandBuffers( uint32_t index );
		void recordCommandBuffer(
			VkCommandBuffer cmdBuffer,
			VkDescriptorSet descSet,
			VkImage image,
			uint32_t width,
			uint32_t height
		);
		void updateDescriptorSet( VkImageView imageView, VkDescriptorSet descSet );


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
