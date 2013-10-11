__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;


// TODO: Find the closest intersection: Find all (don't stop after first hit) and
// choose the one with the smallest distance to the origin
inline float8 findIntersection( float4 origin, float4 ray, __global uint* indices, __global float* vertices, uint numIndices ) {
	uint index0;
	uint index1;
	uint index2;
	float4 a, b, c;
	float4 planeNormal;
	float numerator, denumerator;
	float r;

	float4 dir = ray - origin;
	dir.w = 0.0f;
	origin.w = 0.0f;

	float4 closestHit = (float4)( 10000.0f, 10000.0f, 10000.0f, 0.0f );
	float8 closestResult = (float8)( 10000.0f );


	for( uint i = 0; i < numIndices; i += 3 ) {
		index0 = indices[i] * 3;
		index1 = indices[i + 1] * 3;
		index2 = indices[i + 2] * 3;

		a = (float4)( vertices[index0], vertices[index0 + 1], vertices[index0 + 2], 0.0f );
		b = (float4)( vertices[index1], vertices[index1 + 1], vertices[index1 + 2], 0.0f );
		c = (float4)( vertices[index2], vertices[index2 + 1], vertices[index2 + 2], 0.0f );

		// First test: Intersection with plane of triangle
		planeNormal = cross( ( b - a ), ( c - a ) );
		numerator = -dot( planeNormal, origin - a );
		denumerator = dot( planeNormal, dir );

		if( fabs( denumerator ) < 0.00000001f ) {
			continue;
		}

		r = numerator / denumerator;

		if( r < 0.0f ) {
			continue;
		}

		// The plane has been hit.
		// Second test: Intersection with actual triangle
		float4 hit = origin + r * dir;
		hit.w = 0.0f;

		float4 u = b - a;
		float4 v = c - a;
		float4 w = hit - a;
		u.w = 0.0f;
		v.w = 0.0f;
		w.w = 0.0f;

		float uDotU = dot( u, u );
		float uDotV = dot( u, v );
		float vDotV = dot( v, v );
		float wDotV = dot( w, v );
		float wDotU = dot( w, u );
		float d = uDotV * uDotV - uDotU * vDotV;
		float s = ( uDotV * wDotV - vDotV * wDotU ) / d;

		if( s < 0.0f || s > 1.0f ) {
			continue;
		}

		float t = ( uDotV * wDotU - uDotU * wDotV ) / d;

		if( t < 0.0f || ( s + t ) > 1.0f ) {
			continue;
		}

		float8 result;

		// hit
		result.s0 = hit.x;
		result.s1 = hit.y;
		result.s2 = hit.z;
		result.s3 = 0.0f;

		// normal
		result.s4 = planeNormal.x;
		result.s5 = planeNormal.y;
		result.s6 = planeNormal.z;
		result.s7 = 0.0f;

		if( length( hit - origin ) < length( closestHit - origin ) ) {
			closestHit = hit;
			closestResult = result;
		}
	}

	return closestResult;
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


float shadow( float4 origin, float4 ray ) {
	// float tSphere0 = intersectSphere(origin, ray, sphereCenter0, sphereRadius0);
	// if(tSphere0 < 1.0) return 0.0;

	// float tSphere1 = intersectSphere(origin, ray, sphereCenter1, sphereRadius1);
	// if(tSphere1 < 1.0) return 0.0;

	// float tSphere2 = intersectSphere(origin, ray, sphereCenter2, sphereRadius2);
	// if(tSphere2 < 1.0) return 0.0;

	// float tSphere3 = intersectSphere(origin, ray, sphereCenter3, sphereRadius3);
	// if(tSphere3 < 1.0) return 0.0;

	return 1.0f;
}


inline float4 calculateColor(
		float4 origin, float4 ray, float4 light,
		__global uint* indices, __global float* vertices,
		uint numIndices, float timeSinceStart
	) {
	// Accumulate the colors of each hit surface
	float4 colorMask = (float4)( 1.0f, 1.0f, 1.0f, 1.0f );
	float4 accumulatedColor = (float4)( 0.0f );

	for( uint bounce = 0; bounce < 5; bounce++ ) {
		// Find closest surface the ray hits
		float8 intersect = findIntersection( origin, ray, indices, vertices, numIndices );

		float4 hit;
		hit.x = intersect.s0;
		hit.y = intersect.s1;
		hit.z = intersect.s2;
		hit.w = 0.0f;

		// No hit, the path ends
		if( hit.x == 10000.0f ) { // 10000.0f = arbitrary max distance, just a very high value
			break;
		}

		// Get normal of hit
		float4 normal;
		normal.x = intersect.s4;
		normal.y = intersect.s5;
		normal.z = intersect.s6;
		normal.w = 0.0f;

		normal = normalize( normal );

		// Distance of the hit surface point to the light source
		float4 toLight = light - hit;
		toLight.w = 0.0f;

		float diffuse = max( 0.0f, dot( normalize( toLight ), normal ) );
		float shadowIntensity = shadow( hit + normal * 0.0001f, toLight );
		float specularHighlight = 0.0f;
		float4 surfaceColor = (float4)( 0.9f, 0.9f, 0.9f, 1.0f );

		colorMask *= surfaceColor;
		accumulatedColor += colorMask * 0.2f * diffuse * shadowIntensity;
		accumulatedColor += colorMask * specularHighlight * shadowIntensity;

		// Next bounce
		origin = hit;
		ray = cosineWeightedDirection( timeSinceStart + convert_float( bounce ), normal );
		ray.w = 0.0f;
	}

	return accumulatedColor;
}


__kernel void pathTracing(
		__global uint* indices, __global float* vertices,
		__global float* eyeIn,
		__global float* ray00In, __global float* ray01In, __global float* ray10In, __global float* ray11In,
		const float textureWeight, const float timeSinceStart, const uint numIndices,
		__read_only image2d_t textureIn, __write_only image2d_t textureOut
	) {
	// TODO: reduce work group size
	// NVIDIA GTX 560 Ti limit: { 1024, 1024, 64 } – currently used here: { 512, 512, 1 }
	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };

	// Progress in creating the final image (pixel by pixel)
	float2 percent;
	percent.x = convert_float( pos.x ) / 512.0f * 0.5f + 0.5f;
	percent.y = convert_float( pos.y ) / 512.0f * 0.5f + 0.5f;

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
	float4 light = (float4)( 0.5f, 1.1f, 0.0f, 0.0f );

	// The farther away a shadow is, the more diffuse it becomes
	float4 newLight = light + uniformlyRandomVector( timeSinceStart - 53.0f ) * 0.1f;
	newLight.w = 0.0f;

	// Old color
	float4 texturePixel = read_imagef( textureIn, sampler, pos );

	// New color (or: another color calculated by evaluating different light paths than before)
	float4 calculatedColor = calculateColor( eye, initialRay, newLight, indices, vertices, numIndices, timeSinceStart );

	// Mix new color with previous one
	float4 color = mix( calculatedColor, texturePixel, textureWeight );
	color.w = 1.0f;

	write_imagef( textureOut, pos, color );
}
