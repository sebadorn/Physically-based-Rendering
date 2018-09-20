#ifndef VERTEX_H
#define VERTEX_H

#include <array>

using std::array;


struct Vertex {


	glm::vec2 pos;


	static array<VkVertexInputAttributeDescription, 1> getAttributeDescription() {
		array<VkVertexInputAttributeDescription, 1> attrDesc = {};
		attrDesc[0].binding = 0;
		attrDesc[0].location = 0;
		attrDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
		attrDesc[0].offset = offsetof( Vertex, pos );

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


#endif
