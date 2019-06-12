#version 450
#extension GL_ARB_separate_shader_objects : enable


layout( binding = 0 ) uniform UniformCamera {
	mat4 mvp; // Model-view-projection matrix
	ivec2 size; // x: width, y: height
} camera;


// Shader Storage Buffer Objects
// Only the bottom-most element can have a dynamic length.

layout( binding = 1 ) buffer ModelVertices {
	vec4 vertices[];
};

// layout( binding = 2 ) buffer ModelNormals {
// 	vec4 normals[];
// };


layout( location = 0 ) in vec2 pos; // [-1, 1]
layout( location = 0 ) out vec4 outColor;


void main() {
	outColor = vec4( 0.3, 0.7, 0.0, 1.0 );
}
