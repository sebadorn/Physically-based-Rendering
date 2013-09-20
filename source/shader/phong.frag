#version 330 core


in vec3 ambient;
in vec3 diffuse;
in vec3 specular;

in vec3 vertexPosition_cameraSpace;
in vec3 vertexNormal_cameraSpace;

in vec2 texCoord;
in vec3 light0;
in float useTexture;

out vec3 color;

uniform sampler2D texUnit;


void main( void ) {
	vec3 diffuseV;
	vec3 specV;
	vec3 ambientV;
	float shininess = 60.0f;

	vec3 L = normalize( light0 - vertexPosition_cameraSpace );
	vec3 E = normalize( -vertexPosition_cameraSpace );
	vec3 R = normalize( reflect( -L, vertexNormal_cameraSpace ) );

	ambientV = ambient;
	diffuseV = clamp( diffuse * max( dot( vertexNormal_cameraSpace, L ), 0.0f ), 0.0f, 1.0f ) ;
	specV = clamp( specular * pow( max( dot( R, E ), 0.0f ), 0.3f * shininess ), 0.0f, 1.0f );

	vec3 lightColor = ( ambientV + diffuseV + specV ) * ( 1.0f - useTexture );
	vec3 texColor = texture( texUnit, texCoord ).rgb * useTexture;

	color = lightColor + texColor;
}
