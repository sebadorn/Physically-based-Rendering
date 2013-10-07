__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;


inline float8 findIntersection( float4 ray, float4 origin, __global uint* indices, __global float* vertices ) {
	uint index0, index1, index2;
	float4 a, b, c;
	float4 planeNormal;
	float numerator, denumerator, r;

	uint numIndices = sizeof( indices ) / sizeof( indices[0] );

	for( uint i = 0; i < numIndices; i += 3 ) {
		index0 = indices[i * 3];
		index1 = indices[i * 3 + 1];
		index2 = indices[i * 3 + 2];

		a = (float4)( vertices[index0], vertices[index0 + 1], vertices[index0 + 2], 1.0f );
		b = (float4)( vertices[index1], vertices[index1 + 1], vertices[index1 + 2], 1.0f );
		c = (float4)( vertices[index2], vertices[index2 + 1], vertices[index2 + 2], 1.0f );

		// First test: Intersection with plane of triangle
		planeNormal = normalize( cross( ( b - a ), ( c - a ) ) );
		planeNormal.w = 0.0f;
		a.w = 0.0f;

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
			float4 hit = ( a + s * u + t * v );
			float8 result;

			// hit
			result.s0 = hit.s0;
			result.s1 = hit.s1;
			result.s2 = hit.s2;
			result.s3 = hit.s3;

			// normal
			result.s4 = planeNormal.s0;
			result.s5 = planeNormal.s1;
			result.s6 = planeNormal.s2;
			result.s7 = planeNormal.s3;

			return result;
		}
	}

	return (float8)( 10000.0f );
}


inline float random( float4 scale, float seed ) {
	float i;

	return fract( sin( dot( seed, scale ) ) * 43758.5453f + seed, &i );
}


inline float4 cosineWeightedDirection( float seed, float4 normal ) {
	float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 0.0f ), seed );
	float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 0.0f ), seed );
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
	float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 0.0f ), seed );
	float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 0.0f ), seed );
	float z = 1.0f - 2.0f * u;
	float r = sqrt( 1.0f - z * z );
	float angle = 6.283185307179586 * v;

	return (float4)( r * cos( angle ), r * sin( angle), z, 1.0f );
}


inline float4 uniformlyRandomVector( float seed ) {
	float rnd = random( (float4)( 36.7539f, 50.3658f, 306.2759f, 0.0f ), seed );

	return uniformlyRandomDirection( seed ) * sqrt( rnd );
}


inline float4 calculateColor( float4 origin, float4 ray, float4 light, __global uint* indices, __global float* vertices, __global float* normals, float timeSinceStart ) {
	float4 colorMask = (float4)( 1.0f );
	float4 accumulatedColor = (float4)( 0.0f );

	for( uint bounce = 0; bounce < 3; bounce++ ) {
		float8 intersect = findIntersection( origin, ray, indices, vertices );

		float4 hit;
		hit.s0 = intersect.s0;
		hit.s1 = intersect.s1;
		hit.s2 = intersect.s2;
		hit.s3 = 0.0f;

		if( hit.x == 10000.0f ) {
			break;
		}

		float4 surfaceColor = (float4)( 0.75f );
		float specularHighlight = 0.0f;

		// Get normal of hit
		float4 normal;
		normal.s0 = intersect.s4;
		normal.s1 = intersect.s5;
		normal.s2 = intersect.s6;
		normal.s3 = 0.0f;

		float4 toLight = light - hit;
		toLight.w = 0.0f;

		float diffuse = max( 0.0f, dot( normalize( toLight ), normal ) );
		float shadowIntensity = 1.0f; // TODO: shadow( hit + normal * 0.0001f, toLight );

		colorMask *= surfaceColor;
		accumulatedColor += colorMask * ( 0.5f * diffuse * shadowIntensity );
		accumulatedColor += colorMask * specularHighlight * shadowIntensity;

		// Next bounce
		origin = hit;
		ray = cosineWeightedDirection( timeSinceStart + as_float( bounce ), normal );
	}

	return accumulatedColor;
}


__kernel void pathTracing(
		__global uint* indices, __global float* vertices, __global float* normals,
		__global float* eyeIn,
		__global float* ray00In, __global float* ray01In, __global float* ray10In, __global float* ray11In,
		__global float* compact,
		__read_only image2d_t textureIn, __write_only image2d_t textureOut
	) {

	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };

	float textureWeight = compact[0];
	float timeSinceStart = compact[1];

	float4 ray00 = (float4)( ray00In[0], ray00In[1], ray00In[2], 1.0f );
	float4 ray01 = (float4)( ray01In[0], ray01In[1], ray01In[2], 1.0f );
	float4 ray10 = (float4)( ray10In[0], ray10In[1], ray10In[2], 1.0f );
	float4 ray11 = (float4)( ray11In[0], ray11In[1], ray11In[2], 1.0f );

	float2 percent = convert_float2( pos ) * 0.5 + 0.5;

	float4 initialRay = mix( mix( ray00, ray01, percent.y ), mix( ray10, ray11, percent.y ), percent.x );
	float4 eye = (float4)( eyeIn[0], eyeIn[1], eyeIn[2], 1.0f );

	float4 light = (float4)( 1.0f, 1.0f, 1.0f, 0.0f );
	float4 newLight = light + uniformlyRandomVector( timeSinceStart - 53.0f ) * 0.1f;
	newLight.w = 0.0f;

	float4 texturePixel = read_imagef( textureIn, sampler, pos );

	float4 calculatedColor = calculateColor( eye, initialRay, newLight, indices, vertices, normals, timeSinceStart );
	float4 color = mix( calculatedColor, texturePixel, textureWeight );
	color.w = 1.0f;

	write_imagef( textureOut, pos, color );
}
