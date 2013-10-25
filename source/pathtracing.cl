__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;


// inline float8 findIntersection( float4 origin, float4 ray, __global uint* indices, __global float* vertices, uint numIndices ) {
// 	uint index0;
// 	uint index1;
// 	uint index2;
// 	float4 a, b, c;
// 	float4 planeNormal;
// 	float numerator, denumerator;
// 	float r;

// 	float4 dir = ray - origin;
// 	dir.w = 0.0f;
// 	origin.w = 0.0f;

// 	float4 closestHit = (float4)( 10000.0f, 10000.0f, 10000.0f, 0.0f );
// 	float8 closestResult = (float8)( 10000.0f );


// 	for( uint i = 0; i < numIndices; i += 3 ) {
// 		index0 = indices[i] * 3;
// 		index1 = indices[i + 1] * 3;
// 		index2 = indices[i + 2] * 3;

// 		a = (float4)( vertices[index0], vertices[index0 + 1], vertices[index0 + 2], 0.0f );
// 		b = (float4)( vertices[index1], vertices[index1 + 1], vertices[index1 + 2], 0.0f );
// 		c = (float4)( vertices[index2], vertices[index2 + 1], vertices[index2 + 2], 0.0f );

// 		// First test: Intersection with plane of triangle
// 		planeNormal = cross( ( b - a ), ( c - a ) );
// 		numerator = -dot( planeNormal, origin - a );
// 		denumerator = dot( planeNormal, dir );

// 		if( fabs( denumerator ) < 0.00000001f ) {
// 			continue;
// 		}

// 		r = numerator / denumerator;

// 		if( r < 0.0f ) {
// 			continue;
// 		}

// 		// The plane has been hit.
// 		// Second test: Intersection with actual triangle
// 		float4 hit = origin + r * dir;
// 		hit.w = 0.0f;

// 		float4 u = b - a;
// 		float4 v = c - a;
// 		float4 w = hit - a;
// 		u.w = 0.0f;
// 		v.w = 0.0f;
// 		w.w = 0.0f;

// 		float uDotU = dot( u, u );
// 		float uDotV = dot( u, v );
// 		float vDotV = dot( v, v );
// 		float wDotV = dot( w, v );
// 		float wDotU = dot( w, u );
// 		float d = uDotV * uDotV - uDotU * vDotV;
// 		float s = ( uDotV * wDotV - vDotV * wDotU ) / d;

// 		if( s < 0.0f || s > 1.0f ) {
// 			continue;
// 		}

// 		float t = ( uDotV * wDotU - uDotU * wDotV ) / d;

// 		if( t < 0.0f || ( s + t ) > 1.0f ) {
// 			continue;
// 		}

// 		float8 result;

// 		// hit
// 		result.s0 = hit.x;
// 		result.s1 = hit.y;
// 		result.s2 = hit.z;
// 		result.s3 = r;

// 		// indices of normals
// 		result.s4 = index0;
// 		result.s5 = index1;
// 		result.s6 = index2;
// 		result.s7 = 0.0f;

// 		if( length( hit - origin ) < length( closestHit - origin ) ) {
// 			closestHit = hit;
// 			closestResult = result;
// 		}
// 	}

// 	return closestResult;
// }


/**
 * Get a random value.
 * @param  {float4} scale
 * @param  {float}  seed  Seed for the random number.
 * @return {float}        A random number.
 */
inline float random( float4 scale, float seed ) {
	float i;
	return fract( sin( dot( seed, scale ) ) * 43758.5453f + seed, &i );
}


inline float4 cosineWeightedDirection( float seed, float4 normal ) {
	float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 0.0f ), seed );
	float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 0.0f ), seed );
	float r = sqrt( u );
	float angle = M_PI * 2.0f * v;
	float4 sdir, tdir;

	// TODO: What is happening here?
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


/**
 * Get a vector in a random direction.
 * @param  {float}  seed Seed for the random value.
 * @return {float4}      Random vector.
 */
inline float4 uniformlyRandomDirection( float seed ) {
	float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 0.0f ), seed );
	float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 0.0f ), seed );
	float z = 1.0f - 2.0f * u;
	float r = sqrt( 1.0f - z * z );
	float angle = M_PI * 2.0f * v;

	return (float4)( r * cos( angle ), r * sin( angle), z, 0.0f );
}


/**
 * Get a vector in a random direction.
 * @param  {float}  seed Seed for the random value.
 * @return {float4}      Random vector.
 */
inline float4 uniformlyRandomVector( float seed ) {
	float rnd = random( (float4)( 36.7539f, 50.3658f, 306.2759f, 0.0f ), seed );

	return uniformlyRandomDirection( seed ) * sqrt( rnd );
}


float shadow(
	float4 origin, float4 ray,
	__global uint* indices, __global float* vertices, uint numIndices
) {
	// float8 intersect = findIntersection( origin, ray, indices, vertices, numIndices );

	// if( intersect.s3 < 1.0f ) {
	// 	return 0.0f;
	// }

	return 1.0f;
}


// inline float4 calculateColor(
// 		float4 origin, float4 ray, float4 light,
// 		__global uint* indices, __global float* vertices, __global float* normals,
// 		uint numIndices, float timeSinceStart
// 	) {
// 	// Accumulate the colors of each hit surface
// 	float4 colorMask = (float4)( 1.0f, 1.0f, 1.0f, 1.0f );
// 	float4 accumulatedColor = (float4)( 0.0f );

// 	for( uint bounce = 0; bounce < 5; bounce++ ) {
// 		// Find closest surface the ray hits
// 		float8 intersect = findIntersection( origin, ray, indices, vertices, numIndices );

// 		float4 hit;
// 		hit.x = intersect.s0;
// 		hit.y = intersect.s1;
// 		hit.z = intersect.s2;
// 		hit.w = 0.0f;

// 		// No hit, the path ends
// 		if( hit.x == 10000.0f ) { // 10000.0f = arbitrary max distance, just a very high value
// 			break;
// 		}

// 		// Get normal of hit
// 		uint nIndex0 = convert_uint( intersect.s4 );
// 		uint nIndex1 = convert_uint( intersect.s5 );
// 		uint nIndex2 = convert_uint( intersect.s6 );

// 		float4 normal0 = (float4)( normals[nIndex0], normals[nIndex0 + 1], normals[nIndex0 + 2], 0.0f );
// 		float4 normal1 = (float4)( normals[nIndex1], normals[nIndex1 + 1], normals[nIndex1 + 2], 0.0f );
// 		float4 normal2 = (float4)( normals[nIndex2], normals[nIndex2 + 1], normals[nIndex2 + 2], 0.0f );

// 		float4 normal = ( normal0 + normal1 + normal2 ) / 3.0f;
// 		normal = normalize( normal );

// 		// Distance of the hit surface point to the light source
// 		float4 toLight = light - hit;
// 		toLight.w = 0.0f;

// 		float diffuse = max( 0.0f, dot( normalize( toLight ), normal ) );
// 		// 0.0001f – meaning?
// 		float shadowIntensity = shadow( hit + normal * 0.0001f, toLight, indices, vertices, numIndices );
// 		float specularHighlight = 0.0f; // Disabled
// 		float4 surfaceColor = (float4)( 0.9f, 0.9f, 0.9f, 1.0f );

// 		colorMask *= surfaceColor;
// 		// 0.2f – meaning?
// 		accumulatedColor += colorMask * 0.2f * diffuse * shadowIntensity;
// 		accumulatedColor += colorMask * specularHighlight * shadowIntensity;

// 		// Next bounce
// 		origin = hit;
// 		ray = cosineWeightedDirection( timeSinceStart + convert_float( bounce ), normal );
// 		ray.w = 0.0f;
// 	}

// 	return accumulatedColor;
// }


__kernel void findIntersections(
	__global float4* origins, __global float4* rays, __global float4* normals,
	__global uint* scIndices, __global float* scVertices, __global float* scNormals,
	const uint numIndices
) {
	uint workIndex = get_global_id( 0 ) + get_global_id( 1 ) * get_global_size( 1 );

	float4 origin = origins[workIndex];
	float4 ray = rays[workIndex];
	float4 dir = ray - origin;
	dir.w = 0.0f;

	uint index0, index1, index2;
	float4 a, b, c, planeNormal;
	float denumerator, numerator, r;

	uint4 normalIndices = (uint4)( 0.0f );
	float4 closestHit = (float4)( 10000.0f, 10000.0f, 10000.0f, 0.0f );


	for( uint i = 0; i < numIndices; i += 3 ) {
		index0 = scIndices[i] * 3;
		index1 = scIndices[i + 1] * 3;
		index2 = scIndices[i + 2] * 3;

		a = (float4)( scVertices[index0], scVertices[index0 + 1], scVertices[index0 + 2], 0.0f );
		b = (float4)( scVertices[index1], scVertices[index1 + 1], scVertices[index1 + 2], 0.0f );
		c = (float4)( scVertices[index2], scVertices[index2 + 1], scVertices[index2 + 2], 0.0f );

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

		if( length( hit - origin ) < length( closestHit - origin ) ) {
			closestHit = hit;
			normalIndices.x = index0;
			normalIndices.y = index1;
			normalIndices.z = index2;
		}
	}

	uint nIndex0 = normalIndices.x;
	uint nIndex1 = normalIndices.y;
	uint nIndex2 = normalIndices.z;

	float4 normal0 = (float4)( scNormals[nIndex0], scNormals[nIndex0 + 1], scNormals[nIndex0 + 2], 0.0f );
	float4 normal1 = (float4)( scNormals[nIndex1], scNormals[nIndex1 + 1], scNormals[nIndex1 + 2], 0.0f );
	float4 normal2 = (float4)( scNormals[nIndex2], scNormals[nIndex2 + 1], scNormals[nIndex2 + 2], 0.0f );

	float4 normal = normalize( ( normal0 + normal1 + normal2 ) / 3.0f );

	origins[workIndex] = closestHit;
	rays[workIndex] = cosineWeightedDirection( /*timeSinceStart + */convert_float( workIndex ), normal );
	normals[workIndex] = normal;
}


/**
 * Kernel to generate the initial rays into the scene.
 * @param {__global float*}  eyeIn   Camera eye position.
 * @param {__global float*}  wVecIn  Vector from the camera eye to the screen.
 * @param {__global float*}  uVecIn  Vector alongside the X axis of the screen.
 * @param {__global float*}  vVecIn  Vector alongside the Y axis of the screen.
 * @param {const float}      fovRad  Field of view in radians.
 * @param {__global float4*} origins Output. The origin of each ray. (The camera eye.)
 * @param {__global float4*} rays    Output. The generated rays for each pixel.
 */
__kernel void generateRays(
	__global float* eyeIn,
	__global float* wVecIn, __global float* uVecIn, __global float* vVecIn,
	const float fovRad,
	__global float4* origins, __global float4* rays,
	__global float4* accColors, __global float4* colorMasks
) {
	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };
	uint workIndex = pos.x + pos.y * get_global_size( 1 );

	// Camera eye
	float4 eye = (float4)( eyeIn[0], eyeIn[1], eyeIn[2], 0.0f );

	// Initial ray (first of the to be created path) shooting from the eye and into the scene
	float4 w = (float4)( wVecIn[0], wVecIn[1], wVecIn[2], 0.0f );
	float4 u = (float4)( uVecIn[0], uVecIn[1], uVecIn[2], 0.0f );
	float4 v = (float4)( vVecIn[0], vVecIn[1], vVecIn[2], 0.0f );

	float width = get_global_size( 0 );
	float height = get_global_size( 1 );
	float f = 2.0f * tan( fovRad / 2.0f );
	float pxWidth = f / width;
	float pxHeight = f / height;

	float4 initialRay = eye + w
			- width / 2.0f * pxWidth * u
			+ height / 2.0f * pxHeight * v
			+ pxWidth / 2.0f * u
			- pxHeight / 2.0f * v;
	initialRay += pos.x * pxWidth * u;
	initialRay -= ( height - pos.y ) * pxHeight * v;
	initialRay.w = 0.0f;

	rays[workIndex] = initialRay;
	origins[workIndex] = eye;

	accColors[workIndex] = (float4)( 0.0f );
	colorMasks[workIndex] = (float4)( 1.0f );
}


__kernel void accumulateColors(
	__global float4* origins, __global float4* normals,
	__global float4* accColors, __global float4* colorMasks,
	const float textureWeight, const float timeSinceStart,
	__read_only image2d_t textureIn, __write_only image2d_t textureOut
) {
	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };
	uint workIndex = pos.x + pos.y * get_global_size( 1 );

	float4 hit = origins[workIndex];

	// Lighting
	float4 light = (float4)( 0.3f, 1.6f, 0.0f, 0.0f );

	// The farther away a shadow is, the more diffuse it becomes
	float4 newLight = light + uniformlyRandomVector( timeSinceStart ) * 0.1f;
	newLight.w = 0.0f;

	// New color (or: another color calculated by evaluating different light paths than before)
	// Accumulate the colors of each hit surface
	float4 colorMask = colorMasks[workIndex];
	float4 accumulatedColor = accColors[workIndex];


	// No hit, the path ends
	if( hit.x != 10000.0f ) { // 10000.0f = arbitrary max distance, just a very high value
		float4 normal = normals[workIndex];

		// Distance of the hit surface point to the light source
		float4 toLight = light - hit;
		toLight.w = 0.0f;

		float diffuse = max( 0.0f, dot( normalize( toLight ), normal ) );
		float shadowIntensity = 1.0f;//shadow( hit + normal * 0.0001f, toLight, indices, vertices, numIndices ); // 0.0001f – meaning?
		float specularHighlight = 0.0f; // Disabled for now
		float4 surfaceColor = (float4)( 0.9f, 0.9f, 0.9f, 1.0f );

		colorMask *= surfaceColor;
		accumulatedColor += colorMask * 0.2f * diffuse * shadowIntensity; // 0.2f – meaning?
		accumulatedColor += colorMask * specularHighlight * shadowIntensity;
	}

	accColors[workIndex] = accumulatedColor;
	colorMasks[workIndex] = colorMask;


	// Mix new color with previous one
	float4 texturePixel = read_imagef( textureIn, sampler, pos );
	float4 color = mix( accumulatedColor, texturePixel, textureWeight );
	color.w = 1.0f;

	write_imagef( textureOut, pos, color );
}
