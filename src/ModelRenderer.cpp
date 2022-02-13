#include "ModelRenderer.h"

using std::array;
using std::vector;



/**
 *
 * @param {PathTracer*} pt
 * @param {ObjParser*}  op
 */
void ModelRenderer::setup( PathTracer* pt, ObjParser* op ) {
	Logger::logDebug( "[ModelRenderer] Setup begin." );

	mPathTracer = pt;
	mObjParser = op;

	mCompute = new ComputeHandler();
	mCompute->setup( pt );

	Logger::logDebug( "[ModelRenderer] Setup done." );
}


/**
 *
 */
void ModelRenderer::teardown() {
	mCompute->teardown();
	delete mCompute;
}


/**
 * Update the uniform buffers.
 * @param {uint32_t} frameIndex
 */
void ModelRenderer::updateUniformBuffers( uint32_t frameIndex ) {
	UniformBufferObject ubo = {};
	ubo.model = glm::mat4( 1.0f );
	ubo.view = glm::lookAt(
		glm::vec3( 2.0f, 2.0f, 2.0f ),
		glm::vec3( 0.0f, 0.0f, 0.0f ),
		glm::vec3( 0.0f, 0.0f, 1.0f )
	);
	float ratio = (float) mPathTracer->mSwapchainExtent.width / (float) mPathTracer->mSwapchainExtent.height;
	ubo.proj = glm::perspective(
		glm::radians( 45.0f ),
		ratio, 0.1f, 100.0f
	);
	ubo.proj[1][1] *= -1;

	// void* data;
	// vkMapMemory( mPathTracer->mLogicalDevice, mUniformBuffersMemory[frameIndex], 0, sizeof( ubo ), 0, &data );
	// memcpy( data, &ubo, sizeof( ubo ) );
	// vkUnmapMemory( mPathTracer->mLogicalDevice, mUniformBuffersMemory[frameIndex] );
}
