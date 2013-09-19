#version 330 core


uniform mat4 mModelViewProjectionMatrix;

layout( location = 0 ) in vec3 vertexPosition_modelSpace;
layout( location = 1 ) in vec3 vertexNormal_modelSpace;

out vec3 vn;


void main( void ) {
	gl_Position = mModelViewProjectionMatrix * vec4( vertexPosition_modelSpace, 1 );
	vn = vertexNormal_modelSpace;
}
