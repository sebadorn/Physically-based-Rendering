#version 450
#extension GL_ARB_separate_shader_objects : enable


layout( location = 0 ) in vec2 position;
// layout( location = 0 ) in vec3 objVertices;


void main() {
	gl_Position = vec4( position, 0.0, 1.0 );
	// mat4 model = mat4( 1.0 );
	// mat4 view = mat4( 1.0 );
	// mat4 projection = mat4( 1.0 );
	// mat4 mvp = projection * view * model;
	// gl_Position = mvp * vec4( objVertices, 1.0 );
}
