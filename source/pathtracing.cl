__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;


// TODO: Find the closest intersection: Find all (don't stop after first hit) and
// choose the one with the smallest distance to the origin
inline float8 findIntersection( float4 ray, float4 origin, __global uint* indices, __global float* vertices, uint numIndices ) {
	uint index0, index1, index2;
	float4 a, b, c, planeNormal;
	float numerator, denumerator, r;

	for( uint i = 0; i < numIndices; i += 3 ) {
		index0 = indices[i] * 3;
		index1 = indices[i + 1] * 3;
		index2 = indices[i + 2] * 3;

		a = (float4)( vertices[index0], vertices[index0 + 1], vertices[index0 + 2], 0.0f );
		b = (float4)( vertices[index1], vertices[index1 + 1], vertices[index1 + 2], 0.0f );
		c = (float4)( vertices[index2], vertices[index2 + 1], vertices[index2 + 2], 0.0f );

		// First test: Intersection with plane of triangle
		planeNormal = normalize( cross( ( b - a ), ( c - a ) ) );

		numerator = dot( planeNormal, a - origin );
		denumerator = dot( planeNormal, ray - origin );

		if( as_int( denumerator ) == 0 ) {
			continue;
		}

		r = numerator / denumerator;

		if( as_int( r ) < 0 ) {
			continue;
		}

		// Second test: Intersection with actual triangle
		float4 rPoint = origin + r * ( ray - origin );
		rPoint.w = 0.0f;

		float4 u = b - a,
		       v = c - a,
		       w = rPoint - a;
		u.w = 0.0f;
		v.w = 0.0f;
		w.w = 0.0f;

		float uDotU = dot( u, u ),
		      uDotV = dot( u, v ),
		      vDotV = dot( v, v ),
		      wDotV = dot( w, v ),
		      wDotU = dot( w, u );
		float d = uDotV * uDotV - uDotU * vDotV;
		float s = ( uDotV * wDotV - vDotV * wDotU ) / d,
		      t = ( uDotV * wDotU - uDotU * wDotV ) / d;

		if( s >= 0.0f && t >= 0.0f ) {
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
		sdir = cross( normal, (float4)( 1.0f, 0.0f, 0.0f, 0.0f ) );
	}
	else {
		sdir = cross( normal, (float4)( 0.0f, 1.0f, 0.0f, 0.0f ) );
	}

	tdir = cross( normal, sdir );

	float4 part1 = r * cos( angle ) * sdir;
	float4 part2 = r * sin( angle ) * tdir;
	float4 part3 = sqrt( 1.0f - u ) * normal;

	return ( part1 + part2 + part3 );
}


inline float4 uniformlyRandomDirection( float seed ) {
	float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 0.0f ), seed );
	float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 0.0f ), seed );
	float z = 1.0f - 2.0f * u;
	float r = sqrt( 1.0f - z * z );
	float angle = 6.283185307179586f * v;

	return (float4)( r * cos( angle ), r * sin( angle), z, 0.0f );
}


inline float4 uniformlyRandomVector( float seed ) {
	float rnd = random( (float4)( 36.7539f, 50.3658f, 306.2759f, 0.0f ), seed );

	return uniformlyRandomDirection( seed ) * sqrt( rnd );
}


inline float4 calculateColor(
		float4 origin, float4 ray, float4 light,
		__global uint* indices, __global float* vertices,
		uint numIndices, float timeSinceStart
	) {
	// Accumulate the colors of each hit surface
	float4 colorMask = (float4)( 1.0f, 1.0f, 1.0f, 0.0f );
	float4 accumulatedColor = (float4)( 0.0f );

	for( uint bounce = 0; bounce < 3; bounce++ ) {
		// Find closest surface the ray hits
		float8 intersect = findIntersection( origin, ray, indices, vertices, numIndices );

		float4 hit;
		hit.s0 = intersect.s0;
		hit.s1 = intersect.s1;
		hit.s2 = intersect.s2;
		hit.s3 = 0.0f;
break;
		// No hit, the path ends
		if( hit.x == 10000.0f ) { // 10000.0f = arbitrary max distance, just a very high value
			break;
		}

		float specularHighlight = 0.0f; // TODO: actual sepcular value, not predefined
		float4 surfaceColor = (float4)( 0.75f, 0.75f, 0.75f, 0.0f ); // TODO: actual color, not predefined

		// Get normal of hit
		float4 normal;
		normal.s0 = intersect.s4;
		normal.s1 = intersect.s5;
		normal.s2 = intersect.s6;
		normal.s3 = 0.0f;

		// Distance of the hit surface point to the light source
		float4 toLight = light - hit;
		toLight.w = 0.0f;

		float diffuse = max( 0.0f, dot( normalize( toLight ), normal ) );
		float shadowIntensity = 1.0f; // TODO: shadow( hit + normal * 0.0001f, toLight );

		colorMask *= surfaceColor;
		accumulatedColor += colorMask * ( 0.5f * diffuse * shadowIntensity );
		accumulatedColor += colorMask * specularHighlight * shadowIntensity;

		// Next bounce
		origin = hit;
		ray = cosineWeightedDirection( timeSinceStart + bounce, normal );
		ray.w = 0.0f;
	}

	return accumulatedColor;
}


__kernel void pathTracing(
		__global uint* indices, __global float* vertices,// __global float* normals,
		__global float* eyeIn,
		__global float* ray00In, __global float* ray01In, __global float* ray10In, __global float* ray11In,
		const float textureWeight, const float timeSinceStart, const uint numIndices,
		__read_only image2d_t textureIn, __write_only image2d_t textureOut
	) {
	// TODO: reduce work group size
	// NVIDIA GTX 560 Ti limit: 1024, 1024, 64 – currently used here: 512, 512, 1
	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };

	// Progress in creating the final image (pixel by pixel)
	float2 percent = as_float2( pos ) / 512.0f * 0.5f + 0.5f;

	// Camera eye
	float4 eye = (float4)( eyeIn[0], eyeIn[1], eyeIn[2], 0.0f );

	// Initial ray (first of the to be created path) shooting from the eye and into the scene
	float4 ray00 = (float4)( ray00In[0], ray00In[1], ray00In[2], 0.0f );
	float4 ray01 = (float4)( ray01In[0], ray01In[1], ray01In[2], 0.0f );
	float4 ray10 = (float4)( ray10In[0], ray10In[1], ray10In[2], 0.0f );
	float4 ray11 = (float4)( ray11In[0], ray11In[1], ray11In[2], 0.0f );
	float4 initialRay = mix( mix( ray00, ray01, percent.y ), mix( ray10, ray11, percent.y ), percent.x );
	initialRay.w = 0.0f;

	// Lighting
	float4 light = (float4)( 1.0f, 1.0f, 1.0f, 0.0f );
	float4 newLight = light + uniformlyRandomVector( timeSinceStart - 53.0f ) * 0.1f;
	newLight.w = 0.0f;

	// Old color
	float4 texturePixel = read_imagef( textureIn, sampler, pos );

	// New color (or: another color calculated by evaluating different light paths than before)
	float4 calculatedColor = calculateColor( eye, initialRay, newLight, indices, vertices, numIndices, timeSinceStart );

	// Mix new color with previous one
	float4 color = mix( calculatedColor, texturePixel, textureWeight );
	// float4 color;
	color.w = 1.0f;
	// color.x = 0.5f;
	// color.y = 0.8f;
	// color.z = 0.2f;

	write_imagef( textureOut, pos, color );
}
