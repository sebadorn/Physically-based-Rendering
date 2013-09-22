#version 330 core


uniform mat4 modelViewProjectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat3 normalMatrix;
uniform vec3 cameraPosition;

// uniform vec3 lightPosition;
uniform float useTexture_vert;

layout( location = 0 ) in vec3 vertexPosition_modelSpace;
layout( location = 1 ) in vec3 vertexNormal_modelSpace;
layout( location = 2 ) in vec3 ambient_in;
layout( location = 3 ) in vec3 diffuse_in;
layout( location = 4 ) in vec3 specular_in;
layout( location = 5 ) in float shininess_in;
layout( location = 6 ) in float opacity_in;
layout( location = 7 ) in vec2 texture_in;

out vec3 diffuse;
out vec3 specular;
out float opacity;

out vec2 texCoord;
out float useTexture;


struct lightSource {
	vec4 position;
	vec4 diffuse;
	vec4 specular;
	float constantAttenuation, linearAttenuation, quadraticAttenuation;
	float spotCutoff, spotExponent;
	vec3 spotDirection;
};

lightSource light0 = lightSource(
	vec4( -2.0, -10.0, 3.0, 1.0 ),
	vec4( 1.0, 1.0, 1.0, 1.0 ),
	vec4( 1.0, 1.0, 1.0, 1.0 ),
	0.0, 0.1, 0.0,
	180.0, 0.0,
	vec3( 0.0, 0.0, 0.0 )
);


void main( void ) {
	vec4 v_coord4 = vec4( vertexPosition_modelSpace, 1.0 );

	gl_Position = modelViewProjectionMatrix * v_coord4;


	// Calculate for use in the fragment shader

	vec3 vertexNormal_cameraSpace = normalize( normalMatrix * vertexNormal_modelSpace );
	vec3 viewDirection = normalize( vec3(
		inverse( viewMatrix ) * vec4( cameraPosition, 1.0 ) - modelMatrix * v_coord4
	) );

	vec3 vertexToLightSource = vec3( light0.position - modelMatrix * v_coord4 );
	float distance = length( vertexToLightSource );
	vec3 lightDirection = normalize( vertexToLightSource );
	float attenuation = 1.0 / ( light0.constantAttenuation
			+ light0.linearAttenuation * distance
			+ light0.quadraticAttenuation * distance * distance );

	float dotNormLight = dot( vertexNormal_cameraSpace, lightDirection );

	// Not considering material properties yet
	vec3 ambientLighting = ambient_in;

	// Not considering material properties yet
	vec3 diffuseReflection = attenuation
			* vec3( light0.diffuse )
			* vec3( diffuse_in )
			* max( 0.0, dotNormLight );
	vec3 specularReflection;

	if( dotNormLight < 0.0 ) { // Light source on the wrong side
		specularReflection = vec3( 0.0, 0.0, 0.0 );
	}
	else {
		specularReflection = attenuation
				* vec3( light0.specular )
				* specular_in
				* pow( max(
						0.0,
						dot(
							reflect( -lightDirection, vertexNormal_cameraSpace ),
							viewDirection
						)
					), shininess_in
				);
	}


	// Forward to fragment shader
	opacity = opacity_in;
	texCoord = texture_in;
	diffuse = ambientLighting + diffuseReflection;
	specular = specularReflection;
	useTexture = useTexture_vert;
}
