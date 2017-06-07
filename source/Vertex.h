#ifndef VERTEX_H
#define VERTEX_H

#include <array>

using std::array;


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


#endif
