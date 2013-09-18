#version 330 core

// varying vec3 N;
// varying vec3 v;

layout( location = 0 ) in vec3 vertexPosition_modelspace;
layout( location = 1 ) in vec3 vertexColor;

out vec3 fragmentColor;

uniform mat4 modelViewProjection;


void main( void ) {
	// v = vec3( gl_ModelViewMatrix * gl_Vertex );
	// N = normalize( gl_NormalMatrix * gl_Normal );
	// gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	vec4 v = vec4( vertexPosition_modelspace, 1 );
	gl_Position = modelViewProjection * v;

	fragmentColor = vertexColor;
}
