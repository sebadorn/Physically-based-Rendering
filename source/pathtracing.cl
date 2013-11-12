#define EPSILON 0.00001f

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

typedef struct hit_t {
	float4 position;
	float4 normal;
	float distance;
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
	float angle = 6.28318530718f * v; // M_PI * 2.0f * v
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


/**
 * Face intersection test after Möller and Trumbore.
 * Without backface culling.
 * @param  origin [description]
 * @param  dir    [description]
 * @param  a      [description]
 * @param  b      [description]
 * @param  c      [description]
 * @param  tNear  [description]
 * @param  tFar   [description]
 * @param  result [description]
 * @return        [description]
 */
inline float checkFaceIntersection(
	float4* origin, float4* dir, float4 a, float4 b, float4 c,
	float tNear, float tFar, hit_t* result
) {
	float4 edge1 = b - a;
	float4 edge2 = c - a;
	float4 pVec = cross( *dir, edge2 );
	float det = dot( edge1, pVec );

	if( fabs( det ) < EPSILON ) {
		return -2.0f;
	}

	float4 tVec = (*origin) - a;
	float u = dot( tVec, pVec ) / det;

	if( u < 0.0f || u > 1.0f ) {
		return -2.0f;
	}

	float4 qVec = cross( tVec, edge1 );
	float v = dot( *dir, qVec ) / det;

	if( v < 0.0f || u + v > 1.0f ) {
		return -2.0f;
	}

	float t = dot( edge2, qVec ) / det;

	// TODO: Should be clipped between tNear and tFar, but
	// that produces wrong results even with 1 bounce.
	// No check at all results in smooth surfaces even with more
	// bounces. But that is wrong, too.
	if( t < tNear - EPSILON || t > tFar + EPSILON ) {
		return -2.0f;
	}

	result->position = (*origin) + t * (*dir);
	result->distance = length( result->position - (*origin) );
	result->normal = normalize( cross( edge1, edge2 ) );

	return t;
}


/**
 * Check all faces of a leaf node for intersections with the given ray.
 * @param faceIndex     [description]
 * @param origin        [description]
 * @param dir           [description]
 * @param scVertices    [description]
 * @param scFaces       [description]
 * @param kdNodeData3   [description]
 * @param entryDistance [description]
 * @param exitDistance  [description]
 * @param result        [description]
 */
inline void checkFaces(
	int faceIndex, float4* origin, float4* dir,
	__global float* scVertices, __global uint* scFaces, __global int* kdNodeData3,
	float entryDistance, float* exitDistance, hit_t* result
) {
	float4 a, b, c;
	float closest, r;
	hit_t newResult;
	bool hitOneFace = false;

	int i = 0;
	int numFaces = kdNodeData3[faceIndex];
	int aIndex, bIndex, cIndex, f;

	while( ++i <= numFaces ) {
		f = kdNodeData3[faceIndex + i];
		aIndex = scFaces[f] * 3;
		bIndex = scFaces[f + 1] * 3;
		cIndex = scFaces[f + 2] * 3;

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

		r = checkFaceIntersection( origin, dir, a, b, c, entryDistance, *exitDistance, &newResult );

		if( r > -1.0f ) {
			*exitDistance = r;

			if( result->distance > newResult.distance || result->distance < 0.0f ) {
				result->distance = newResult.distance;
				result->position = newResult.position;
				result->normal = newResult.normal;
			}
		}
	}
}


/**
 * Source: http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-7-intersecting-simple-shapes/ray-box-intersection/
 * Source: "An Efficient and Robust Ray–Box Intersection Algorithm", Williams et al.
 */
inline bool intersectBoundingBox(
	float4* origin, float4* direction, float* bbMin, float* bbMax,
	float* tNear, float* tFar, int* exitRope
) {
	float invDir[3] = {
		1.0f / direction->x,
		1.0f / direction->y,
		1.0f / direction->z
	};
	float bounds[2][3] = {
		bbMin[0], bbMin[1], bbMin[2],
		bbMax[0], bbMax[1], bbMax[2]
	};
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	bool signX = invDir[0] < 0.0f;
	bool signY = invDir[1] < 0.0f;
	bool signZ = invDir[2] < 0.0f;

	// X
	tmin = ( bounds[signX][0] - origin->x ) * invDir[0];
	tmax = ( bounds[1 - signX][0] - origin->x ) * invDir[0];
	// Y
	tymin = ( bounds[signY][1] - origin->y ) * invDir[1];
	tymax = ( bounds[1 - signY][1] - origin->y ) * invDir[1];

	if( tmin > tymax || tymin > tmax ) {
		return false;
	}

	// X vs. Y
	*exitRope = ( tymax < tmax ) ? 3 - signY : 1 - signX;
	tmin = ( tymin > tmin ) ? tymin : tmin;
	tmax = ( tymax < tmax ) ? tymax : tmax;
	// Z
	tzmin = ( bounds[signZ][2] - origin->z ) * invDir[2];
	tzmax = ( bounds[1 - signZ][2] - origin->z ) * invDir[2];

	if( tmin > tzmax || tzmin > tmax ) {
		return false;
	}

	// Z vs. previous winner
	*exitRope = ( tzmax < tmax ) ? 5 - signZ : *exitRope;
	tmin = ( tzmin > tmin ) ? tzmin : tmin;
	tmax = ( tzmax < tmax ) ? tzmax : tmax;
	// Result
	*tNear = tmin;
	*tFar = tmax;

	return true;
}


/**
 * Test, if a point is inside a given axis aligned bounding box.
 * @param  bbMin [description]
 * @param  bbMax [description]
 * @param  p     [description]
 * @return       [description]
 */
inline bool isInsideBoundingBox( float* bbMin, float* bbMax, float4 p ) {
	return (
		p.x >= bbMin[0] && p.y >= bbMin[1] && p.z >= bbMin[2] &&
		p.x <= bbMax[0] && p.y <= bbMax[1] && p.z <= bbMax[2]
	);
}


/**
 * Find the closest hit of the ray with a surface.
 * Uses stackless kd-tree traversal.
 * @param origin      [description]
 * @param dir         [description]
 * @param kdRoot      [description]
 * @param scVertices  [description]
 * @param scFaces     [description]
 * @param kdNodeData1 [description]
 * @param kdNodeData2 [description]
 * @param kdNodeData3 [description]
 * @param kdNodeRopes [description]
 * @param result      [description]
 */
inline void traverseKdTree(
	float4* origin, float4* dir, const uint kdRoot,
	__global float* scVertices, __global uint* scFaces,
	__global float* kdNodeData1, __global int* kdNodeData2, __global int* kdNodeData3,
	__global int* kdNodeRopes, hit_t* result
) {
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

	float entryDistance, exitDistance, tNear;
	int exitRope;

	if( !intersectBoundingBox( origin, dir, bbMin, bbMax, &entryDistance, &exitDistance, &exitRope ) ) {
		return;
	}
	if( exitDistance < 0.0f ) {
		return;
	}

	entryDistance = ( entryDistance < 0.0f ) ? 0.0f : entryDistance;


	float4 hitNear;
	int left, right, axis, faceIndex, ropeIndex;
	int i = 0;

	while( entryDistance < exitDistance ) {
		// TODO: Infinite loop bug seems to be fixed!
		// Remove this line later. Just keeping it a little longer
		// as precaution while tackling some other issue.
		if( i++ > 1000 ) { return; }


		// Find a leaf node for this ray

		hitNear = (*origin) + entryDistance * (*dir);
		left = kdNodeData2[nodeIndex * 5];
		right = kdNodeData2[nodeIndex * 5 + 1];
		axis = kdNodeData2[nodeIndex * 5 + 2];

		while( left >= 0 && right >= 0 ) {
			nodeIndex = ( ( (float*) &hitNear )[axis] < kdNodeData1[nodeIndex * 9 + axis] ) ? left : right;
			left = kdNodeData2[nodeIndex * 5];
			right = kdNodeData2[nodeIndex * 5 + 1];
			axis = kdNodeData2[nodeIndex * 5 + 2];
		}

		bbMin[0] = kdNodeData1[nodeIndex * 9 + 3];
		bbMin[1] = kdNodeData1[nodeIndex * 9 + 4];
		bbMin[2] = kdNodeData1[nodeIndex * 9 + 5];
		bbMax[0] = kdNodeData1[nodeIndex * 9 + 6];
		bbMax[1] = kdNodeData1[nodeIndex * 9 + 7];
		bbMax[2] = kdNodeData1[nodeIndex * 9 + 8];


		// At a leaf node now, check triangle faces

		faceIndex = kdNodeData2[nodeIndex * 5 + 3];

		checkFaces(
			faceIndex, origin, dir, scVertices, scFaces, kdNodeData3,
			entryDistance, &exitDistance, result
		);


		// Exit leaf node

		intersectBoundingBox( origin, dir, bbMin, bbMax, &tNear, &entryDistance, &exitRope );

		if( entryDistance >= exitDistance ) {
			return;
		}


		// Follow the rope

		ropeIndex = kdNodeData2[nodeIndex * 5 + 4];
		nodeIndex = kdNodeRopes[ropeIndex + exitRope];

		if( nodeIndex < 0 ) {
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
	float4 light = (float4)( 0.3f, 1.4f, 0.3f, 0.0f );

	// New color (or: another color calculated by evaluating different light paths than before)
	// Accumulate the colors of each hit surface
	float4 colorMask = colorMasks[workIndex];
	float4 accumulatedColor = accColors[workIndex];

	// No hit, the path ends
	if( hit.w > -1.0f ) {
		// The farther away a shadow is, the more diffuse it becomes
		float4 newLight = light/* + uniformlyRandomVector( timeSinceStart ) * 0.1f*/;
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
		accumulatedColor += colorMask * 0.4f * diffuse * shadowIntensity;
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


/**
 * Search the kd-tree to find the closest intersection of the ray with a surface.
 * @param origins        [description]
 * @param rays           [description]
 * @param normals        [description]
 * @param scVertices     [description]
 * @param scFaces        [description]
 * @param kdNodeData1    [description]
 * @param kdNodeData2    [description]
 * @param kdNodeData3    [description]
 * @param kdNodeRopes    [description]
 * @param kdRoot         [description]
 * @param timeSinceStart [description]
 */
__kernel void findIntersectionsKdTree(
	__global float4* origins, __global float4* rays, __global float4* normals,
	__global float* scVertices, __global uint* scFaces, //__global float* scNormals,
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

	origin.w = 0.0f;
	ray.w = 0.0f;
	float4 dir = normalize( ray - origin );

	hit_t hit;
	hit.distance = -2.0f;
	hit.position = (float4)( 0.0f, 0.0f, 0.0f, -2.0f );

	traverseKdTree(
		&origin, &dir, kdRoot, scVertices, scFaces,
		kdNodeData1, kdNodeData2, kdNodeData3, kdNodeRopes, &hit
	);

	origins[workIndex] = hit.position;

	if( hit.distance > -1.0f ) {
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
