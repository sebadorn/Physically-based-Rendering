#version 330 core


uniform mat4 modelMatrix, viewMatrix, modelViewProjectionMatrix;
uniform mat3 normalMatrix;
uniform vec3 cameraPosition;

uniform mat4 light0Data1, light1Data1, light2Data1, light3Data1, light4Data1, light5Data1;
uniform vec4 light0Data2, light1Data2, light2Data2, light3Data2, light4Data2, light5Data2;
uniform int numLights;

// Vertex
layout( location = 0 ) in vec3 vertexPosition_modelSpace;
layout( location = 1 ) in vec3 vertexNormal_modelSpace;

// Material
layout( location = 2 ) in vec3 ambient_in;
layout( location = 3 ) in vec3 diffuse_in;
layout( location = 4 ) in vec3 specular_in;
layout( location = 5 ) in float shininess_in;
layout( location = 6 ) in float opacity_in;
layout( location = 7 ) in vec2 texture_in;

// Outgoing to fragment shader
out vec3 diffuse, specular;
out vec2 texCoord;
out float opacity;


struct lightSource {
	vec4 position;
	vec4 diffuse;
	vec4 specular;
	float constantAttenuation, linearAttenuation, quadraticAttenuation;
	float spotCutoff, spotExponent;
	vec3 spotDirection;
};


void main( void ) {
	vec4 v_coord4 = vec4( vertexPosition_modelSpace, 1.0 );

	gl_Position = modelViewProjectionMatrix * v_coord4;


	// Get light(s)

	lightSource light0, light1, light2, light3, light4, light5;

	if( numLights == 1 ) {
		light0 = lightSource(
			vec4( light0Data1[0][0], light0Data1[0][1], light0Data1[0][2], light0Data1[0][3] ),
			vec4( light0Data1[1][0], light0Data1[1][1], light0Data1[1][2], light0Data1[1][3] ),
			vec4( light0Data1[2][0], light0Data1[2][1], light0Data1[2][2], light0Data1[2][3] ),
			light0Data1[3][0], light0Data1[3][1], light0Data1[3][2],
			light0Data1[3][3], light0Data2[0],
			vec3( light0Data2[1], light0Data2[2], light0Data2[3] )
		);
	}
	if( numLights == 2 ) {
		light1 = lightSource(
			vec4( light1Data1[0][0], light1Data1[0][1], light1Data1[0][2], light1Data1[0][3] ),
			vec4( light1Data1[1][0], light1Data1[1][1], light1Data1[1][2], light1Data1[1][3] ),
			vec4( light1Data1[2][0], light1Data1[2][1], light1Data1[2][2], light1Data1[2][3] ),
			light1Data1[3][0], light1Data1[3][1], light1Data1[3][2],
			light1Data1[3][3], light1Data2[0],
			vec3( light1Data2[1], light1Data2[2], light1Data2[3] )
		);
	}
	if( numLights == 3 ) {
		light2 = lightSource(
			vec4( light2Data1[0][0], light2Data1[0][1], light2Data1[0][2], light2Data1[0][3] ),
			vec4( light2Data1[1][0], light2Data1[1][1], light2Data1[1][2], light2Data1[1][3] ),
			vec4( light2Data1[2][0], light2Data1[2][1], light2Data1[2][2], light2Data1[2][3] ),
			light2Data1[3][0], light2Data1[3][1], light2Data1[3][2],
			light2Data1[3][3], light2Data2[0],
			vec3( light2Data2[1], light2Data2[2], light2Data2[3] )
		);
	}
	if( numLights == 4 ) {
		light3 = lightSource(
			vec4( light3Data1[0][0], light3Data1[0][1], light3Data1[0][2], light3Data1[0][3] ),
			vec4( light3Data1[1][0], light3Data1[1][1], light3Data1[1][2], light3Data1[1][3] ),
			vec4( light3Data1[2][0], light3Data1[2][1], light3Data1[2][2], light3Data1[2][3] ),
			light3Data1[3][0], light3Data1[3][1], light3Data1[3][2],
			light3Data1[3][3], light3Data2[0],
			vec3( light3Data2[1], light3Data2[2], light3Data2[3] )
		);
	}
	if( numLights == 5 ) {
		light4 = lightSource(
			vec4( light4Data1[0][0], light4Data1[0][1], light4Data1[0][2], light4Data1[0][3] ),
			vec4( light4Data1[1][0], light4Data1[1][1], light4Data1[1][2], light4Data1[1][3] ),
			vec4( light4Data1[2][0], light4Data1[2][1], light4Data1[2][2], light4Data1[2][3] ),
			light4Data1[3][0], light4Data1[3][1], light4Data1[3][2],
			light4Data1[3][3], light4Data2[0],
			vec3( light4Data2[1], light4Data2[2], light4Data2[3] )
		);
	}
	if( numLights == 6 ) {
		light5 = lightSource(
			vec4( light5Data1[0][0], light5Data1[0][1], light5Data1[0][2], light5Data1[0][3] ),
			vec4( light5Data1[1][0], light5Data1[1][1], light5Data1[1][2], light5Data1[1][3] ),
			vec4( light5Data1[2][0], light5Data1[2][1], light5Data1[2][2], light5Data1[2][3] ),
			light5Data1[3][0], light5Data1[3][1], light5Data1[3][2],
			light5Data1[3][3], light5Data2[0],
			vec3( light5Data2[1], light5Data2[2], light5Data2[3] )
		);
	}

	lightSource lights[6] = lightSource[6]( light0, light1, light2, light3, light4, light5 );


	// Calculate for use in the fragment shader

	vec3 vertexNormal_cameraSpace = normalize( normalMatrix * vertexNormal_modelSpace );
	vec3 viewDirection = normalize( vec3(
		inverse( viewMatrix ) * vec4( cameraPosition, 1.0 ) - modelMatrix * v_coord4
	) );

	vec3 vertexToLightSource;
	float distance;
	vec3 lightDirection;
	float attenuation;
	float dotNormLight;

	vec3 ambientLighting = ambient_in;
	vec3 diffuseReflection = vec3( 0.0 );
	vec3 specularReflection = vec3( 0.0 );

	for( int i = 0; i < numLights; i++ ) {
		if( lights[i].position.w == 0.0 ) { // directional light
			attenuation = 1.0;
			lightDirection = normalize( vec3( lights[i].position ) );
		}
		else {
			vertexToLightSource = vec3( lights[i].position - modelMatrix * v_coord4 );
			distance = length( vertexToLightSource );
			lightDirection = normalize( vertexToLightSource );
			attenuation = 1.0 / ( lights[i].constantAttenuation
					+ lights[i].linearAttenuation * distance
					+ lights[i].quadraticAttenuation * distance * distance );

			if( lights[i].spotCutoff <= 90.0 ) { // spot light
				float clampedCosine = max(
					0.0, dot( -lightDirection, normalize( lights[i].spotDirection ) )
				);

				if( clampedCosine < cos( radians( lights[i].spotCutoff ) ) ) { // outside of spotlight cone
					attenuation = 0.0;
				}
				else {
					attenuation += pow( clampedCosine, lights[i].spotExponent );
				}
			}
		}

		dotNormLight = dot( vertexNormal_cameraSpace, lightDirection );

		// Not considering material properties yet
		diffuseReflection += attenuation
				* vec3( lights[i].diffuse )
				* vec3( diffuse_in )
				* max( 0.0, dotNormLight );

		if( dotNormLight >= 0.0 ) { // Light source on the correct side
			specularReflection += attenuation
					* vec3( lights[i].specular )
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
	}


	// Forward to fragment shader
	opacity = opacity_in;
	texCoord = texture_in;
	diffuse = ambientLighting + diffuseReflection;
	specular = specularReflection;
}
