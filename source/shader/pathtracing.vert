#version 330 core


attribute vec3 vertex;

uniform vec3 eye, ray00, ray01, ray10, ray11;

out vec3 initialRay;


void main( void ) {
	vec2 percent = vertex.xy * 0.5 + 0.5;
	initialRay = mix( mix( ray00, ray01, percent.y ), mix( ray10, ray11, percent.y ), percent.x );
	gl_Position = vec4( vertex, 1.0 );
}
