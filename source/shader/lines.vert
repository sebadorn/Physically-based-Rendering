#version 330 core


uniform mat4 mvp;

layout( location = 0 ) in vec3 vertex;


void main( void ) {
	vec3 cubeMin = vec3( -0.5, -0.5, -0.5 );
	vec3 cubeMax = vec3( 0.5, 0.5, 0.5 );
	gl_Position = vec4( mix( cubeMin, cubeMax, vertex ), 1.0 ) * mvp;
}
