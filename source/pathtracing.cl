__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;


inline float4 findIntersection( float4 ray, float4 origin, __global uint* indices, __global float* vertices ) {
	uint index;
	float4 a, b, c;
	float4 planeNormal;
	float numerator, denumerator, r;

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
	float i;
	return fract( sin( dot( seed, scale ) ) * 43758.5453f + seed, &i );
}


inline float4 cosineWeightedDirection( float seed, float4 normal ) {
	float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 1.0f ), seed );
	float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 1.0f ), seed );
	float r = sqrt( u );
	float angle = 6.283185307179586f * v;
	float4 sdir, tdir;

	if( fabs( normal.x ) < 0.5f ) {
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
	float rand = random( (float4)( 36.7539f, 50.3658f, 306.2759f, 0.0f ), seed );
	return uniformlyRandomDirection( seed ) * sqrt( rand );
}


inline float4 calculateColor( float4 origin, float4 ray, float4 light, __global uint* indices, __global float* vertices ) {
	float4 colorMask = (float4)( 1.0f );
	float4 accumulatedColor = (float4)( 0.0f );

	for( uint bounce = 0; bounce < 3; bounce++ ) {
		float4 hit = findIntersection( origin, ray, indices, vertices );

		if( hit.x == 10000.0f ) {
			break;
		}

		float4 surfaceColor = (float4)( 0.75f );
		float specularHighlight = 0.0f;
		float4 normal;

		float4 toLight = light - hit;
		toLight.w = 0.0f;
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
		__global float* eyeIn,
		__global float* ray00In, __global float* ray01In, __global float* ray10In, __global float* ray11In,
		float textureWeight, float timeSinceStart,
		__read_only image2d_t textureIn,
		__write_only image2d_t textureOut
	) {

	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };

	float4 ray00 = (float4)( ray00In[0], ray00In[1], ray00In[2], 1.0f );
	float4 ray01 = (float4)( ray01In[0], ray01In[1], ray01In[2], 1.0f );
	float4 ray10 = (float4)( ray10In[0], ray10In[1], ray10In[2], 1.0f );
	float4 ray11 = (float4)( ray11In[0], ray11In[1], ray11In[2], 1.0f );

	float2 percent = convert_float2( pos ) * 0.5 + 0.5;

	float4 initialRay = mix( mix( ray00, ray01, percent.y ), mix( ray10, ray11, percent.y ), percent.x );
	float4 eye = (float4)( eyeIn[0], eyeIn[1], eyeIn[2], 1.0f );

	float4 light = (float4)( 1.0f, 1.0f, 1.0f, 0.0f );
	float4 newLight = light + uniformlyRandomVector( timeSinceStart - 53.0f ) * 0.1f;
	float4 texturePixel = read_imagef( textureIn, sampler, pos );

	float4 calculatedColor = calculateColor( eye, initialRay, newLight, indices, vertices );
	float4 color = mix( calculatedColor, texturePixel, textureWeight );
	color.w = 1.0f;

	write_imagef( textureOut, pos, color );
}
