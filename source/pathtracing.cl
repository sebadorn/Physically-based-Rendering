__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;


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

	// http://www.rorydriscoll.com/2009/01/07/better-sampling/
	// But it doesn't seem to make a difference what I do here. :(

	float r = sqrt( u );
	float theta = 6.28318530718f * v; // M_PI * 2.0f * v

	float x = r * cos( theta );
	float y = r * sin( theta );

	return (float4)( x, y, sqrt( max( 0.0f, 1.0f - u ) ), 0.0f );
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
	float angle = 6.28318530718f * v;

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



// KERNELS


/**
 * Accumulate the colors of hit surfaces.
 * @param {__global float4*}       origins        Positions on the hit surfaces.
 * @param {__global float4*}       normals        Normals of the hit surfaces.
 * @param {__global float4*}       accColors      Accumulated color so far.
 * @param {__global float4*}       colorMasks     Color mask so far.
 * @param {const float}            textureWeight  Weight for the mixing of the textures.
 * @param {const float}            timeSinceStart Time since start of the tracer in seconds.
 * @param {__read_only image2d_t}  textureIn      Input. The generated texture so far.
 * @param {__write_only image2d_t} textureOut     Output. The generated texture now.
 */
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

	// New color (or: another color calculated by evaluating different light paths than before)
	// Accumulate the colors of each hit surface
	float4 colorMask = colorMasks[workIndex];
	float4 accumulatedColor = accColors[workIndex];

	// No hit, the path ends
	if( hit.w > -1.0f ) {
		// The farther away a shadow is, the more diffuse it becomes
		float4 newLight = light + uniformlyRandomVector( timeSinceStart ) * 0.1f;
		newLight.w = 0.0f;

		float4 normal = normals[workIndex];

		// Distance of the hit surface point to the light source
		float4 toLight = newLight - hit;
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


typedef struct {
	float coord[3];
	int nodeIndex, vertIndex;
	int vertFace0, vertFace1;
	int nodeLeft, nodeRight;
} kdNode_t;

typedef struct {
	kdNode_t node;
	uint axis;
} stackEntry_t;



inline void descendKdTree(
	float4 origin, float4 ray,
	__global kdNode_t* nodes, kdNode_t currentNode
) {
	// Evil, don't do this. Has to be changed with model.
	// (Recompile after string replacements like %STACK_SIZE%?)
	stackEntry_t nodeStack[100];
	int process = 0;
	uint axis = 0;

	while( process >= 0 ) {

	}
}


__kernel void findIntersectionKdTree(
	__global float4* origins, __global float4* rays, __global float4* normals,
	__global float* scVertices,
	__global kdNode_t* nodes, const uint kdRoot,
	const float timeSinceStart
) {
	uint workIndex = get_global_id( 0 ) + get_global_id( 1 ) * get_global_size( 1 );
	float4 origin = origins[workIndex];
	float4 ray = rays[workIndex];

	descendKdTree( origin, ray, nodes, nodes[kdRoot] );
}


/**
 * Kernel for finding the closest intersections of the rays with the scene.
 * @param {__global float4*} origins    Origins of the rays.
 * @param {__global float4*} rays       The rays into the scene.
 * @param {__global float4*} normals    Output. Normals of the hit surfaces.
 * @param {__global uint*}   scIndices  Indices of the objects in the scene.
 * @param {__global float*}  scVertices Vertices of the scene.
 * @param {__global float*}  scNormals  Normals of the vertices in the scene.
 * @param {const uint}       numIndices Number of indices.
 */
__kernel void findIntersections(
	__global float4* origins, __global float4* rays, __global float4* normals,
	__global uint* scIndices, __global float* scVertices, __global float* scNormals,
	const uint numIndices, const float timeSinceStart
) {
	uint workIndex = get_global_id( 0 ) + get_global_id( 1 ) * get_global_size( 1 );

	float4 origin = origins[workIndex];

	// Initial rays have a <w> value of 0.0f.
	// Negative values are only present in rays that previously didn't hit anything.
	// Those rays aren't of any interest.
	if( origin.w < -1.0f ) {
		return;
	}

	float4 ray = rays[workIndex];
	float4 dir = ray - origin;
	dir.w = 0.0f;

	uint index0, index1, index2;
	float4 a, b, c, planeNormal;
	float denumerator, numerator, r;

	uint4 normalIndices = (uint4)( 0.0f );
	float4 closestHit = (float4)( 0.0f, 0.0f, 0.0f, -2.0f );


	for( uint i = 0; i < numIndices; i += 3 ) {
		index0 = scIndices[i] * 3;
		index1 = scIndices[i + 1] * 3;
		index2 = scIndices[i + 2] * 3;

		a = (float4)( scVertices[index0], scVertices[index0 + 1], scVertices[index0 + 2], 0.0f );
		b = (float4)( scVertices[index1], scVertices[index1 + 1], scVertices[index1 + 2], 0.0f );
		c = (float4)( scVertices[index2], scVertices[index2 + 1], scVertices[index2 + 2], 0.0f );

		// First test: Intersection with plane of triangle
		planeNormal = cross( ( b - a ), ( c - a ) );
		denumerator = dot( planeNormal, dir );

		if( denumerator == 0.0f ) {
			continue;
		}

		numerator = -dot( planeNormal, origin - a );
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

		if( closestHit.w < -1.0f || length( hit - origin ) < length( closestHit - origin ) ) {
			closestHit = hit;
			normalIndices.x = index0;
			normalIndices.y = index1;
			normalIndices.z = index2;
		}
	}


	origins[workIndex] = closestHit;

	if( closestHit.w > -1.0f ) {
		uint nIndex0 = normalIndices.x;
		uint nIndex1 = normalIndices.y;
		uint nIndex2 = normalIndices.z;

		float4 normal0 = (float4)( scNormals[nIndex0], scNormals[nIndex0 + 1], scNormals[nIndex0 + 2], 0.0f );
		float4 normal1 = (float4)( scNormals[nIndex1], scNormals[nIndex1 + 1], scNormals[nIndex1 + 2], 0.0f );
		float4 normal2 = (float4)( scNormals[nIndex2], scNormals[nIndex2 + 1], scNormals[nIndex2 + 2], 0.0f );

		float4 normal = normalize( ( normal0 + normal1 + normal2 ) / 3.0f );

		rays[workIndex] = cosineWeightedDirection( timeSinceStart, normal );
		normals[workIndex] = normal;
	}
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
