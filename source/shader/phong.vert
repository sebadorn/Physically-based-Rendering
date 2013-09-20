#version 330 core


uniform mat4 mModelViewProjectionMatrix;
uniform mat4 mModelViewMatrix;
uniform mat3 mNormalMatrix;

uniform vec3 lightPosition;

layout( location = 0 ) in vec3 vertexPosition_modelSpace;
layout( location = 1 ) in vec3 vertexNormal_modelSpace;
layout( location = 2 ) in vec3 colorAmbient;
layout( location = 3 ) in vec3 colorDiffuse;
layout( location = 4 ) in vec3 colorSpecular;
layout( location = 5 ) in vec2 texture;

out vec3 ambient;
out vec3 diffuse;
out vec3 specular;

out vec3 vertexPosition_cameraSpace;
out vec3 vertexNormal_cameraSpace;

out vec2 texCoord;
out vec3 light0;


void main( void ) {
	gl_Position = mModelViewProjectionMatrix * vec4( vertexPosition_modelSpace, 1 );

	ambient = colorAmbient;
	diffuse = colorDiffuse;
	specular = colorSpecular;

	vertexPosition_cameraSpace = mat3( mModelViewMatrix ) * vertexPosition_modelSpace;
	vertexNormal_cameraSpace = normalize( mNormalMatrix * vertexNormal_modelSpace );

	texCoord = texture;
	light0 = lightPosition;
}
