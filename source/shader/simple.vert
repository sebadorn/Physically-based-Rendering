#version 330 core


uniform mat4 mvp;

layout( location = 0 ) in vec3 vertex;


void main( void ) {
	gl_Position = mvp * vec4( vertex, 1.0 );
}
