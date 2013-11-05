#define DELTA_PRECISION 0.00001f

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

typedef struct hit_t {
	float4 position;
	float4 normal;
	float distance;
	// int normalIndices[3];
} hit_t;



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
	// float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 0.0f ), seed );
	// float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 0.0f ), seed );

	// // http://www.rorydriscoll.com/2009/01/07/better-sampling/
	// // But it doesn't seem to make a difference what I do here. :(

	// float r = sqrt( u );
	// float theta = 6.28318530718f * v; // M_PI * 2.0f * v

	// float x = r * cos( theta );
	// float y = r * sin( theta );

	// return (float4)( x, y, sqrt( max( 0.0f, 1.0f - u ) ), 0.0f );

	float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 0.0f ), seed );
	float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 0.0f ), seed );
	float r = sqrt( u );
	float angle = M_PI * 2.0f * v;
	float4 sdir, tdir;

	// TODO: What is happening here?
	sdir = ( fabs( normal.x ) < 0.5f )
	     ? cross( normal, (float4)( 1.0f, 0.0f, 0.0f, 0.0f ) )
	     : cross( normal, (float4)( 0.0f, 1.0f, 0.0f, 0.0f ) );

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


inline float checkFaceIntersection(
	float4 origin, float4 ray,
	float4 a, float4 b, float4 c,
	float tNear, float tFar, hit_t* result
) {
	// First test: Intersection with plane of triangle
	float4 direction = normalize( ray - origin );
	float4 planeNormal = normalize( cross( ( b - a ), ( c - a ) ) );
	float denumerator = dot( planeNormal, direction );

	if( denumerator == 0.0f ) {
		return -1.0f;
	}

	float numerator = -dot( planeNormal, origin - a );
	float r = numerator / denumerator;

	if( r < tNear || r > tFar ) {
		return -1.0f;
	}


	// The plane has been hit.
	// Second test: Intersection with actual triangle
	float4 hit = origin + r * direction;
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
		return -1.0f;
	}

	float t = ( uDotV * wDotU - uDotU * wDotV ) / d;

	if( t < 0.0f || ( s + t ) > 1.0f ) {
		return -1.0f;
	}

	result->position = hit;
	result->distance = length( hit - origin );

	return r;
}


inline void checkFaces(
	int faceIndex, float4 origin, float4 ray,
	__global float* scVertices, __global uint* scFaces,
	__global int* kdNodeData3,
	float tNear, float tFar, hit_t* result, float* exitDistance
) {
	float4 a, b, c;
	hit_t newResult;

	int i = 1;
	int numFaces = kdNodeData3[faceIndex];

	while( i <= numFaces ) {
		int f = kdNodeData3[faceIndex + i];
		int aIndex = scFaces[f] * 3;
		int bIndex = scFaces[f + 1] * 3;
		int cIndex = scFaces[f + 2] * 3;

		a = (float4)(
			scVertices[aIndex],
			scVertices[aIndex + 1],
			scVertices[aIndex + 2],
			0.0f
		);
		b = (float4)(
			scVertices[bIndex],
			scVertices[bIndex + 1],
			scVertices[bIndex + 2],
			0.0f
		);
		c = (float4)(
			scVertices[cIndex],
			scVertices[cIndex + 1],
			scVertices[cIndex + 2],
			0.0f
		);

		newResult.distance = -1.0f;
		float r = checkFaceIntersection( origin, ray, a, b, c, tNear, tFar, &newResult );

		if( newResult.distance > -1.0f ) {
			*exitDistance = r;
			tFar = r;

			if( result->distance < 0.0f || result->distance > newResult.distance ) {
				result->distance = newResult.distance;
				result->position = newResult.position;
				result->normal = normalize( cross( ( b - a ), ( c - a ) ) ); // TODO: clean up
				// result->normalIndices[0] = aIndex;
				// result->normalIndices[1] = bIndex;
				// result->normalIndices[2] = cIndex;
			}
		}

		i++;
	}
}


/**
 * Source: https://github.com/unvirtual/cukd/blob/master/utils/intersection.h
 * which is based on: http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-7-intersecting-simple-shapes/ray-box-intersection/
 */
inline bool intersectBoundingBox(
	float4* origin, float4* direction, float* bbMin, float* bbMax,
	float* p_near, float* p_far, int* exitRope
) {
	float p_near_result = -FLT_MAX;
	float p_far_result = FLT_MAX;
	float p_near_comp, p_far_comp;

	float invDir[3] = {
		1.0f / direction->x,
		1.0f / direction->y,
		1.0f / direction->z
	};
	float originCoords[3] = { origin->x, origin->y, origin->z };

	int swapped;
	*exitRope = 0;

	for( int i = 0; i < 3; i++ ) {
		p_near_comp = ( bbMin[i] - originCoords[i] ) * invDir[i];
		p_far_comp = ( bbMax[i] - originCoords[i] ) * invDir[i];
		swapped = 1;

		if( p_near_comp > p_far_comp ) {
			float temp = p_near_comp;
			p_near_comp = p_far_comp;
			p_far_comp = temp;
			swapped = 0;
		}

		*exitRope = ( p_far_comp < p_far_result ) ? i * 2 + swapped : *exitRope;

		p_near_result = ( p_near_comp > p_near_result ) ? p_near_comp : p_near_result;
		p_far_result = ( p_far_comp < p_far_result ) ? p_far_comp : p_far_result;

		if( p_near_result > p_far_result ) {
			return false;
		}
	}

	*p_near = p_near_result;
	*p_far = p_far_result;

	return true;
}


inline void traverseKdTree(
	float4 origin, float4 ray, const uint kdRoot,
	__global float* scVertices, __global uint* scFaces,
	__global float* kdNodeData1, __global int* kdNodeData2, __global int* kdNodeData3,
	__global int* kdNodeRopes, hit_t* result
) {
	float4 dir = normalize( ray - origin );
	int nodeIndex = kdRoot;

	float bbMin[3] = {
		kdNodeData1[nodeIndex * 9 + 3],
		kdNodeData1[nodeIndex * 9 + 4],
		kdNodeData1[nodeIndex * 9 + 5]
	};
	float bbMax[3] = {
		kdNodeData1[nodeIndex * 9 + 6],
		kdNodeData1[nodeIndex * 9 + 7],
		kdNodeData1[nodeIndex * 9 + 8]
	};

	float entryDistance, exitDistance, tNear, tFar;
	int exitRope;

	if( !intersectBoundingBox( &origin, &dir, bbMin, bbMax, &entryDistance, &exitDistance, &exitRope ) ) {
		return;
	}

	float4 hitNear, hitFar;
	float hitCoords[3];
	int left, right, axis, faceIndex, ropeIndex;

	int i = 0;


	while( entryDistance < exitDistance ) {
		if( i++ > 100 ) { return; } // TODO: REMOVE

		hitNear = origin + entryDistance * dir;
		hitCoords[0] = hitNear.x;
		hitCoords[1] = hitNear.y;
		hitCoords[2] = hitNear.z;

		left = kdNodeData2[nodeIndex * 5];
		right = kdNodeData2[nodeIndex * 5 + 1];
		axis = kdNodeData2[nodeIndex * 5 + 2];

		// Find a leaf node for this ray
		while( left >= 0 && right >= 0 ) {
			nodeIndex = ( hitCoords[axis] < kdNodeData1[nodeIndex * 9 + axis] ) ? left : right;
			left = kdNodeData2[nodeIndex * 5];
			right = kdNodeData2[nodeIndex * 5 + 1];
			axis = kdNodeData2[nodeIndex * 5 + 2];
		}


		// At a leaf node now, check triangle faces
		faceIndex = kdNodeData2[nodeIndex * 5 + 3];

		checkFaces(
			faceIndex, origin, ray,
			scVertices, scFaces, kdNodeData3,
			entryDistance, exitDistance,
			result, &exitDistance
		);


		// Get exit point of ray from bounding box
		bbMin[0] = kdNodeData1[nodeIndex * 9 + 3];
		bbMin[1] = kdNodeData1[nodeIndex * 9 + 4];
		bbMin[2] = kdNodeData1[nodeIndex * 9 + 5];
		bbMax[0] = kdNodeData1[nodeIndex * 9 + 6];
		bbMax[1] = kdNodeData1[nodeIndex * 9 + 7];
		bbMax[2] = kdNodeData1[nodeIndex * 9 + 8];

		intersectBoundingBox( &origin, &dir, bbMin, bbMax, &tNear, &entryDistance, &exitRope );
		hitFar = origin + entryDistance * dir;


		// Follow the rope
		ropeIndex = kdNodeData2[nodeIndex * 5 + 4];
		// nodeIndex = kdNodeRopes[ropeIndex + exitRope];

		// TODO: replace with the one-liner above ... as soon as it works
		if( fabs( hitFar.x - bbMin[0] ) < DELTA_PRECISION ) { // left
			nodeIndex = kdNodeRopes[ropeIndex];
		}
		else if( fabs( hitFar.x - bbMax[0] ) < DELTA_PRECISION ) { // right
			nodeIndex = kdNodeRopes[ropeIndex + 1];
		}
		else if( fabs( hitFar.y - bbMin[1] ) < DELTA_PRECISION ) { // bottom
			nodeIndex = kdNodeRopes[ropeIndex + 2];
		}
		else if( fabs( hitFar.y - bbMax[1] ) < DELTA_PRECISION ) { // top
			nodeIndex = kdNodeRopes[ropeIndex + 3];
		}
		else if( fabs( hitFar.z - bbMin[2] ) < DELTA_PRECISION ) { // back
			nodeIndex = kdNodeRopes[ropeIndex + 4];
		}
		else if( fabs( hitFar.z - bbMax[2] ) < DELTA_PRECISION ) { // front
			nodeIndex = kdNodeRopes[ropeIndex + 5];
		}
		else {
			return;
		}

		if( nodeIndex < 0 ) {
			// Per algorithm description in original paper, but why?
			// result->distance = -2.0f;
			return;
		}
	}
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


__kernel void findIntersectionsKdTree(
	__global float4* origins, __global float4* rays, __global float4* normals,
	__global float* scVertices, __global uint* scFaces, __global float* scNormals,
	__global float* kdNodeData1, __global int* kdNodeData2, __global int* kdNodeData3,
	__global int* kdNodeRopes,
	const uint kdRoot, const float timeSinceStart
) {
	uint workIndex = get_global_id( 0 ) + get_global_id( 1 ) * get_global_size( 1 );
	float4 origin = origins[workIndex];
	float4 ray = rays[workIndex];

	if( origin.w <= -1.0f ) {
		return;
	}

	hit_t hit;
	hit.distance = -2.0f;
	hit.position = (float4)( 0.0f, 0.0f, 0.0f, -2.0f );

	traverseKdTree(
		origin, ray, kdRoot,
		scVertices, scFaces,
		kdNodeData1, kdNodeData2, kdNodeData3, kdNodeRopes,
		&hit
	);

	origins[workIndex] = hit.position;

	if( hit.distance > -1.0f ) {
		// // TODO: Weight accordingly to distance to hit
		// uint nIndex0 = hit.normalIndices[0]; // vertex a of the face
		// uint nIndex1 = hit.normalIndices[1]; // vertex b of the face
		// uint nIndex2 = hit.normalIndices[2]; // vertex c of the face

		// float4 normal0 = (float4)( scNormals[nIndex0], scNormals[nIndex0 + 1], scNormals[nIndex0 + 2], 0.0f );
		// float4 normal1 = (float4)( scNormals[nIndex1], scNormals[nIndex1 + 1], scNormals[nIndex1 + 2], 0.0f );
		// float4 normal2 = (float4)( scNormals[nIndex2], scNormals[nIndex2 + 1], scNormals[nIndex2 + 2], 0.0f );

		// float4 normal = normalize( ( normal0 + normal1 + normal2 ) / 3.0f );

		rays[workIndex] = cosineWeightedDirection( timeSinceStart, hit.normal );
		normals[workIndex] = hit.normal;
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
