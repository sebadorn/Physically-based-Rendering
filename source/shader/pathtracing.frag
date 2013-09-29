#version 330 core


uniform vec3 eye;

varying vec3 initialRay;

uniform float textureWeight;
// uniform float timeSinceStart;
uniform sampler2D texUnit;
// uniform float glossiness;

vec3 roomCubeMin = vec3( -1.0, -1.0, -1.0 );
vec3 roomCubeMax = vec3( 1.0, 1.0, 1.0 );

uniform mat4 light0Data1, light1Data1, light2Data1, light3Data1, light4Data1, light5Data1;
uniform vec4 light0Data2, light1Data2, light2Data2, light3Data2, light4Data2, light5Data2;
uniform int numLights;

uniform vec3 sphereCenter0;
uniform float sphereRadius0;
uniform vec3 sphereCenter1;
uniform float sphereRadius1;
uniform vec3 sphereCenter2;
uniform float sphereRadius2;
uniform vec3 sphereCenter3;
uniform float sphereRadius3;

out vec4 color;


vec2 intersectCube( vec3 origin, vec3 ray, vec3 cubeMin, vec3 cubeMax ) {
	vec3 tMin = ( cubeMin - origin ) / ray;
	vec3 tMax = ( cubeMax - origin ) / ray;
	vec3 t1 = min( tMin, tMax );
	vec3 t2 = max( tMin, tMax );
	float tNear = max( max( t1.x, t1.y ), t1.z );
	float tFar = min( min( t2.x, t2.y ), t2.z );

	return vec2( tNear, tFar );
}


vec3 normalForCube( vec3 hit, vec3 cubeMin, vec3 cubeMax ) {
	if( hit.x < cubeMin.x + 0.0001 ) { return vec3( -1.0, 0.0, 0.0 ); }
	else if( hit.x > cubeMax.x - 0.0001 ) { return vec3( 1.0, 0.0, 0.0 ); }
	else if( hit.y < cubeMin.y + 0.0001 ) { return vec3( 0.0, -1.0, 0.0 ); }
	else if( hit.y > cubeMax.y - 0.0001 ) { return vec3( 0.0, 1.0, 0.0 ); }
	else if( hit.z < cubeMin.z + 0.0001 ) { return vec3( 0.0, 0.0, -1.0 ); }
	else { return vec3( 0.0, 0.0, 1.0 ); }
}


float intersectSphere( vec3 origin, vec3 ray, vec3 sphereCenter, float sphereRadius ) {
	vec3 toSphere = origin - sphereCenter;
	float a = dot( ray, ray );
	float b = 2.0 * dot( toSphere, ray );
	float c = dot( toSphere, toSphere ) - sphereRadius * sphereRadius;
	float discriminant = b * b - 4.0 * a * c;

	if( discriminant > 0.0 ) {
		float t = ( -b - sqrt( discriminant ) ) / ( 2.0 * a );

		if( t > 0.0 ) {
			return t;
		}
	}

	return 10000.0;
}


vec3 normalForSphere( vec3 hit, vec3 sphereCenter, float sphereRadius ) {
	return ( hit - sphereCenter ) / sphereRadius;
}


float random( vec3 scale, float seed ) {
	return fract( sin( dot( gl_FragCoord.xyz + seed, scale ) ) * 43758.5453 + seed );
}


vec3 cosineWeightedDirection( float seed, vec3 normal ) {
	float u = random( vec3( 12.9898, 78.233, 151.7182 ), seed );
	float v = random( vec3( 63.7264, 10.873, 623.6736 ), seed );
	float r = sqrt( u );
	float angle = 6.283185307179586 * v;
	vec3 sdir, tdir;

	if( abs( normal.x ) < 0.5 ) {
		sdir = cross( normal, vec3( 1.0, 0.0, 0.0 ) );
	}
	else {
		sdir = cross( normal, vec3( 0.0, 1.0, 0.0 ) );
	}

	tdir = cross( normal, sdir );

	return r * cos( angle ) * sdir + r * sin( angle ) * tdir + sqrt( 1.0 - u ) * normal;
}


vec3 uniformlyRandomDirection( float seed ) {
	float u = random( vec3( 12.9898, 78.233, 151.7182 ), seed );
	float v = random( vec3( 63.7264, 10.873, 623.6736 ), seed );
	float z = 1.0 - 2.0 * u;
	float r = sqrt( 1.0 - z * z );
	float angle = 6.283185307179586 * v;

	return vec3( r * cos( angle ), r * sin( angle ), z );
}


vec3 uniformlyRandomVector( float seed ) {
	return uniformlyRandomDirection( seed ) * sqrt( random( vec3( 36.7539, 50.3658, 306.2759 ), seed ) );
}


float shadow( vec3 origin, vec3 ray ) {
	float tSphere0 = intersectSphere( origin, ray, sphereCenter0, sphereRadius0 );
	if( tSphere0 < 1.0 ) { return 0.0; }

	float tSphere1 = intersectSphere( origin, ray, sphereCenter1, sphereRadius1 );
	if( tSphere1 < 1.0 ) { return 0.0; }

	float tSphere2 = intersectSphere( origin, ray, sphereCenter2, sphereRadius2 );
	if( tSphere2 < 1.0 ) { return 0.0; }

	float tSphere3 = intersectSphere( origin, ray, sphereCenter3, sphereRadius3 );
	if( tSphere3 < 1.0 ) { return 0.0; }

	return 1.0;
}


vec3 calculateColor( vec3 origin, vec3 ray, vec3 light ) {
	vec3 colorMask = vec3( 1.0 );
	vec3 accumulatedColor = vec3( 0.0 );

	for( int bounce = 0; bounce < 5; bounce++ ) {
		vec2 tRoom = intersectCube( origin, ray, roomCubeMin, roomCubeMax );
		float tSphere0 = intersectSphere( origin, ray, sphereCenter0, sphereRadius0 );
		float tSphere1 = intersectSphere( origin, ray, sphereCenter1, sphereRadius1 );
		float tSphere2 = intersectSphere( origin, ray, sphereCenter2, sphereRadius2 );
		float tSphere3 = intersectSphere( origin, ray, sphereCenter3, sphereRadius3 );
		float t = 10000.0;

		if( tRoom.x < tRoom.y ) t = tRoom.y;
		if( tSphere0 < t ) t = tSphere0;
		if( tSphere1 < t ) t = tSphere1;
		if( tSphere2 < t ) t = tSphere2;
		if( tSphere3 < t ) t = tSphere3;

		vec3 hit = origin + ray * t;
		vec3 surfaceColor = vec3( 0.75 );
		float specularHighlight = 0.0;
		vec3 normal;

		if( t == tRoom.y ) {
			normal = -normalForCube( hit, roomCubeMin, roomCubeMax );
			if( hit.x < -0.9999 ) {
				surfaceColor = vec3( 0.1, 0.5, 1.0 );
			}
			else if( hit.x > 0.9999 ) {
				surfaceColor = vec3( 1.0, 0.9, 0.1 );
			}
			ray = cosineWeightedDirection( timeSinceStart + float( bounce ), normal );
		}
		else if( t == 10000.0 ) {
			break;
		}
		else {
			if( false );
			else if( t == tSphere0 ) { normal = normalForSphere( hit, sphereCenter0, sphereRadius0 ); }
			else if( t == tSphere1 ) { normal = normalForSphere( hit, sphereCenter1, sphereRadius1 ); }
			else if( t == tSphere2 ) { normal = normalForSphere( hit, sphereCenter2, sphereRadius2 ); }
			else if( t == tSphere3 ) { normal = normalForSphere( hit, sphereCenter3, sphereRadius3 ); }

			ray = cosineWeightedDirection( timeSinceStart + float( bounce ), normal );
		}

		vec3 toLight = light - hit;
		float diffuse = max( 0.0, dot( normalize( toLight ), normal ) );
		float shadowIntensity = shadow( hit + normal * 0.0001, toLight );

		colorMask *= surfaceColor;
		accumulatedColor += colorMask * ( 0.5 * diffuse * shadowIntensity );
		accumulatedColor += colorMask * specularHighlight * shadowIntensity;
		origin = hit;
	}

	return accumulatedColor;
}


void main() {
	vec3 light = vec3( light0Data1[0][0], light0Data1[0][1], light0Data1[0][2] );

	vec3 newLight = light + uniformlyRandomVector( timeSinceStart - 53.0 ) * 0.1;
	vec3 texture = texture2D( texUnit, gl_FragCoord.xy / 512.0 ).rgb;

	color = vec4( mix( calculateColor( eye, initialRay, newLight ), texture, textureWeight ), 1.0 );
}
