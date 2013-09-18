#version 330 core


in vec3 ambient;
in vec3 diffuse;
in vec3 specular;

in vec3 vertexPosition_cameraSpace;
in vec3 vertexNormal_cameraSpace;

in vec3 light0;

out vec3 color;


void main( void ) {
	vec3 diffuseV;
	vec3 specV;
	vec3 ambientV;
	float shininess = 60.0f;

	vec3 L = normalize( light0 - vertexPosition_cameraSpace );
	vec3 E = normalize( -vertexPosition_cameraSpace );
	vec3 R = normalize( reflect( -L, vertexNormal_cameraSpace ) );

	ambientV = ambient;
	diffuseV = clamp( diffuse * max( dot( vertexNormal_cameraSpace, L ), 0.0 ), 0.0, 1.0 ) ;
	specV = clamp( specular * pow( max( dot( R, E ), 0.0 ), 0.3 * shininess ), 0.0, 1.0 );

	color = ambientV + diffuseV + specV;
}
