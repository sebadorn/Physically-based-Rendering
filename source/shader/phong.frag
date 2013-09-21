#version 330 core


in vec3 ambient;
in vec3 diffuse;
in vec3 specular;
in float shininess;

in vec3 vertexPosition_cameraSpace;
in vec3 vertexNormal_cameraSpace;

in vec2 texCoord;
in vec3 light0;
in float useTexture;

out vec4 color;

uniform sampler2D texUnit;


void main( void ) {
	vec3 diffuseV;
	vec3 specularV;
	vec3 ambientV;

	vec3 L = normalize( light0 - vertexPosition_cameraSpace );
	vec3 E = normalize( -vertexPosition_cameraSpace );
	vec3 R = normalize( reflect( -L, vertexNormal_cameraSpace ) );

	ambientV = ambient;
	diffuseV = clamp( diffuse * max( dot( vertexNormal_cameraSpace, L ), 0.0f ), 0.0f, 1.0f ) ;
	specularV = clamp( specular * pow( max( dot( R, E ), 0.0f ), 0.3f * shininess ), 0.0f, 1.0f );

	vec4 lightColor = vec4( ( ambientV + diffuseV + specularV ) * ( 1.0f - useTexture ), 1.0f );
	vec4 texColor = texture( texUnit, texCoord ).rgba * useTexture;

	// color = vec4( lightColor + texColor, 1.0f );
	color = texColor;
}
