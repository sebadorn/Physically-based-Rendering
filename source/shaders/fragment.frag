#version 450
#extension GL_ARB_separate_shader_objects : enable


layout( location = 0 ) in vec2 inPosition;

layout( location = 0 ) out vec4 outColor;


void main() {
	float r = ( inPosition.x + 1.0 ) * 0.5;
	float g = ( inPosition.y + 1.0 ) * 0.5;
	float b = 0.5;
	outColor = vec4( r, g, b, 1.0 );
}
