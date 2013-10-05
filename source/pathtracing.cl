__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;


inline float4 findIntersection( __global uint* indices, __global float* vertices, __global float* normals, __global float* rayIn, __global float* originIn ) {
	uint index;
	float4 a, b, c;
	float4 planeNormal;
	float numerator, denumerator, r;

	float4 ray = (float4)( rayIn[0], rayIn[1], rayIn[2], 1.0f );
	float4 origin = (float4)( originIn[0], originIn[1], originIn[2], 1.0f );

	uint numIndices = sizeof( indices ) / sizeof( indices[0] );

	for( uint i = 0; i < numIndices; i += 3 ) {
		index = indices[i * 3];
		a = (float4)( vertices[index], vertices[index + 1], vertices[index + 2], 1.0f );
		index = indices[i * 3 + 1];
		b = (float4)( vertices[index], vertices[index + 1], vertices[index + 2], 1.0f );
		index = indices[i * 3 + 2];
		c = (float4)( vertices[index], vertices[index + 1], vertices[index + 2], 1.0f );

		// First test: Intersection with plane of triangle
		planeNormal = normalize( cross( ( b - a ), ( c - a ) ) );
		numerator = dot( planeNormal, a - origin );
		denumerator = dot( planeNormal, ray - origin );

		if( convert_int( denumerator ) == 0 ) {
			continue;
		}

		r = numerator / denumerator;

		if( convert_int( r ) < 0 ) {
			continue;
		}

		// Second test: Intersection with actual triangle
		float4 rPoint = origin + r * ( ray - origin );
		float4 u = b - a,
		       v = c - a,
		       w = rPoint - a;

		float uDotU = dot( u, u ),
		      uDotV = dot( u, v ),
		      vDotV = dot( v, v ),
		      wDotV = dot( w, v ),
		      wDotU = dot( w, u );
		float d = uDotV * uDotV - uDotU * vDotV;
		float s = ( uDotV * wDotV - vDotV * wDotU ) / d,
		      t = ( uDotV * wDotU - uDotU * wDotV ) / d;

		if( s >= 0 && t >= 0 ) {
			return ( a + s * u + t * v );
		}
	}

	return (float4)( 10000.0f, 10000.0f, 10000.0f, 1.0f );
}


inline float random( float4 scale, float seed ) {
	return fract( sin( dot( seed, scale ) ) * 43758.5453f + seed );
}



inline float4 cosineWeightedDirection( float seed, float4 normal ) {
	float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 1.0f ), seed );
	float v = random( (float4)( 63.7264f, 10.873f, 623.6736f ), seed );
	float r = sqrt( u );
	float angle = 6.283185307179586f * v;
	float4 sdir, tdir;

	if( abs( normal.x ) < 0.5f ) {
		sdir = cross( normal, (float4)( 1.0f, 0.0f, 0.0f, 1.0f ) );
	}
	else {
		sdir = cross( normal, (float4)( 0.0f, 1.0f, 0.0f, 1.0f ) );
	}

	tdir = cross( normal, sdir );

	return ( r * cos( angle ) * sdir + r * sin( angle ) * tdir + sqrt( 1.0f - u ) * normal );
}


inline float4 uniformlyRandomDirection( float seed ) {
	float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 1.0f ), seed );
	float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 1.0f ), seed );
	float z = 1.0f - 2.0f * u;
	float r = sqrt( 1.0f - z * z );
	float angle = 6.283185307179586 * v;

	return (float4)( r * cos( angle ), r * sin( angle), z, 1.0f );
}


inline float4 uniformlyRandomVector( float seed ) {
	return uniformlyRandomDirection( seed ) * sqrt( random( (float4)( 36.7539f, 50.3658f, 306.2759f, 0.0f ), seed ) );
}


inline float4 calculateColor( float4 origin, float4 ray, float4 light ) {
	float4 colorMask = (float4)( 1.0f );
	float4 accumulatedColor = (float4)( 0.0f );

	for( uint bounce = 0; bounce < 3; bounce++ ) {
		float4 hit = findIntersection( origin, ray );

		if( hit[0] == 10000.0f ) {
			break;
		}

		float4 surfaceColor = (float4)( 0.75f );
		float specularHighlight = 0.0f;
		float4 normal;

		float4 toLight = light - hit;
		toLight[3] = 0.0f;
		float diffuse = max( 0.0f, dot( normalize( toLight ), normal ) );
		float shadowIntensity = 1.0f;

		colorMask *= surfaceColor;
		accumulatedColor += colorMask * ( 0.5f * diffuse * shadowIntensity );
		accumulatedColor += colorMask * specularHighlight * shadowIntensity;
		origin = hit;
	}

	return accumulatedColor;
}


__kernel void pathTracing(
		__global uint* indices, __global float* vertices, __global float* normals,
		__global float* eye,
		__global float* ray00, __global float* ray01, __global float* ray10, __global float* ray11,
		float textureWeight, float timeSinceStart,
		__read_only image2d_t image
	) {

	vec2 percent = vertex.xy * 0.5 + 0.5; // TODO: vertex
	float4 initialRay = mix( mix( ray00, ray01, percent.y ), mix( ray10, ray11, percent.y ), percent.x );

	float4 light = (float4)( 1.0f, 1.0f, 1.0f, 0.0f );
	float4 newLight = light + uniformlyRandomVector( timeSinceStart - 53.0f ) * 0.1f;
	float4 texture = texture2D( texUnit, gl_FragCoord.xy / 512.0f ).rgb; // TODO: texture

	float4 color = mix( calculateColor( eye, initialRay, newLight ), texture, textureWeight );
	color[3] = 1.0f;
}
