#version 330 core


attribute vec3 vertex;


void main( void ) {
	gl_Position = vec4( vertex, 1.0 );
}
