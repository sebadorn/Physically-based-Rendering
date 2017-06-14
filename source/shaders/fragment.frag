#version 450
#extension GL_ARB_separate_shader_objects : enable


layout( binding = 0 ) uniform UniformCamera {
	mat4 mvp;
} camera;

layout( location = 0 ) out vec4 outColor;


void main() {
	outColor = vec4( 0.3, 0.7, 0.0, 1.0 );
}
