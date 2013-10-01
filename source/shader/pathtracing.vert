#version 330 core


uniform mat4 modelViewProjectionMatrix;

uniform vec3 eye, ray00, ray01, ray10, ray11;
varying vec3 initialRay;


// Vertex
layout( location = 0 ) in vec3 vertexPosition_modelSpace;
// layout( location = 1 ) in vec3 vertexNormal_modelSpace;

// Material
// layout( location = 2 ) in vec3 ambient_in;
// layout( location = 3 ) in vec3 diffuse_in;
// layout( location = 4 ) in vec3 specular_in;
// layout( location = 5 ) in float shininess_in;
// layout( location = 6 ) in float opacity_in;
// layout( location = 7 ) in vec2 texture_in;


void main( void ) {
	vec4 v_coord4 = vec4( vertexPosition_modelSpace, 1.0 );
	gl_Position = modelViewProjectionMatrix * v_coord4;

	// vec2 percent = gl_Position.xy * 0.5 + 0.5;
	vec2 percent = vec2( vertexPosition_modelSpace ) * 0.5 + 0.5;
	initialRay = mix( mix( ray00, ray01, percent.y ), mix( ray10, ray11, percent.y ), percent.x );
}
