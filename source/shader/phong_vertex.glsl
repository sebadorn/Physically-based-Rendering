#version 330 core

// uniform vec4 ambientMat;
// uniform vec4 diffuseMat;
// uniform vec4 specMat;
// uniform float specPow;

// varying vec3 N;
// varying vec3 v;

layout( location = 0 ) in vec3 vertexPosition_modelspace;

void main( void ) {
	// v = vec3( gl_ModelViewMatrix * gl_Vertex );
	// N = normalize( gl_NormalMatrix * gl_Normal );
	// gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_Position.xyz = vertexPosition_modelspace;
	gl_Position.w = 1.0;
}
