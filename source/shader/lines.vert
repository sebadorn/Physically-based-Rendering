#version 330 core


uniform mat4 mvp;

layout( location = 0 ) in vec3 vertex;


void main( void ) {
	vec3 cubeMin = vec3( -1.0, 0.0, -1.0 );
	vec3 cubeMax = vec3( 1.0, 2.0, 1.0 );
	gl_Position = mvp * vec4( vertex, 1.0 );
}
