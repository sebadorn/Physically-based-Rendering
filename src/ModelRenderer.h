#ifndef MODEL_RENDERER
#define MODEL_RENDERER

#include <array>
#include <vector>

#include "Logger.h"
#include "parsers/ObjParser.h"
#include "PathTracer.h"
#include "vulkan/ComputeHandler.h"

using std::array;
using std::vector;


struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct ModelVertex {
	glm::vec3 pos;
	glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription desc = {};
        desc.binding = 0;
        desc.stride = sizeof( ModelVertex );
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return desc;
    }

    static array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        array<VkVertexInputAttributeDescription, 2> desc = {};

        desc[0].binding = 0;
        desc[0].location = 0;
        desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        desc[0].offset = offsetof( ModelVertex, pos );

        desc[1].binding = 0;
        desc[1].location = 1;
        desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        desc[1].offset = offsetof( ModelVertex, color );

        return desc;
    }

    bool operator==( const ModelVertex& other ) const {
        return (
        	this->pos == other.pos &&
        	this->color == other.color
        );
    }
};

struct ComputeHandler;
struct PathTracer;


class ModelRenderer {


	public:
		void draw( uint32_t frameIndex );
		void setup( PathTracer* pt, ObjParser* op );
		void teardown();

		vector<VkCommandBuffer> mCommandBuffers;
		ComputeHandler* mCompute;


	protected:
		void createCommandBuffers();
		void createCommandPool();
		void createDescriptorPool();
		void createDescriptorSets();
		void createFences();
		void createGraphicsPipeline( VkShaderModule* vertModule, VkShaderModule* fragModule );
		void createIndexBuffer();
		void createRenderPass();
		void createShaders( VkShaderModule* vertModule, VkShaderModule* fragModule );
		void createUniformBuffers();
		void createVertexBuffer();
		void updateUniformBuffers( uint32_t frameIndex );


	private:
		ObjParser* mObjParser;
		PathTracer* mPathTracer;

		vector<VkBuffer> mUniformBuffers;
		vector<VkDeviceMemory> mUniformBuffersMemory;

		VkBuffer mIndexBuffer;
		VkDeviceMemory mIndexBufferMemory;

		VkBuffer mVertexBuffer;
		VkDeviceMemory mVertexBufferMemory;

		VkImage mDepthImage;
		VkDeviceMemory mDepthImageMemory;
		VkImageView mDepthImageView;

		VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
		vector<VkDescriptorSet> mDescriptorSets;
		VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;

		VkCommandPool mCommandPool = VK_NULL_HANDLE;
		VkPipeline mGraphicsPipeline = VK_NULL_HANDLE;
		VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
		VkRenderPass mRenderPass = VK_NULL_HANDLE;
		VkSemaphore mSemaphore = VK_NULL_HANDLE;


};


#endif
