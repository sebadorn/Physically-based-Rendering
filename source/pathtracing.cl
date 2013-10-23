__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;


typedef struct {
	float coord[3];
	int nodeIndex, vertIndex;
	int vertFace0, vertFace1;
	int nodeLeft, nodeRight;
} kdNode_t;

typedef struct {
	float4 hit;
	float distance;
	uint normalIndex0, normalIndex1, normalIndex2;
} bestResult_t;

typedef struct {
	kdNode_t node;
	uint axis;
} stackEntry_t;

#define KD_DIM 3
#define T_MAX 10000.0f


inline float checkPlaneIntersection( float4 origin, float4 ray, kdNode_t node, uint axis, float tmax ) {
	float4 a = (float4)( node.coord[0], node.coord[1], node.coord[2], 0.0f );
	float4 b = (float4)( a.x, a.y, a.z, 0.0f );
	float4 c = (float4)( a.x, a.y, a.z, 0.0f );

	if( axis == 0 ) {
		b.y += tmax; c.z += tmax;
	}
	else if( axis == 1 ) {
		b.x += tmax; c.z += tmax;
	}
	else if( axis == 2 ) {
		b.x += tmax; c.y += tmax;
	}

	float4 direction = ray - origin;
	float4 planeNormal = cross( ( b - a ), ( c - a ) );
	float numerator = -dot( planeNormal, origin - a );
	float denumerator = dot( planeNormal, direction );

	if( denumerator < 0.0f ) {
		return tmax + 1.0f;
	}

	return numerator / denumerator;
}


inline bool checkTriangleIntersection(
	float4 origin, float4 ray, __global float* vertices,
	kdNode_t node, float t, float tmax,
	bestResult_t* newResult
) {
	float4 a = (float4)( node.coord[0], node.coord[1], node.coord[2], 0.0f );
	float4 b = (float4)( vertices[node.vertFace0], vertices[node.vertFace0 + 1], vertices[node.vertFace0 + 2], 0.0f );
	float4 c = (float4)( vertices[node.vertFace1], vertices[node.vertFace1 + 1], vertices[node.vertFace1 + 2], 0.0f );
	float4 hit = origin + t * ( ray - origin );
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
	float r = ( uDotV * wDotV - vDotV * wDotU ) / d;

	if( r < 0.0f || r > 1.0f ) {
		return false;
	}

	float s = ( uDotV * wDotU - uDotU * wDotV ) / d;

	if( s < 0.0f || ( r + s ) > 1.0f ) {
		return false;
	}

	newResult->hit = hit;
	newResult->distance = length( hit - origin );

	return true;
}


inline void descendKdTree(
	float4 origin, float4 ray, __global float* vertices,
	__global kdNode_t* nodes, kdNode_t node, uint axis, float tmax,
	bestResult_t* result
) {
	// Evil, don't do this. Has to be changed with model.
	// (Recompile after string replacements like %STACK_SIZE%?)
	stackEntry_t nodeStack[100];
	int process = 0;

	stackEntry_t currentNode;
	stackEntry_t first;
	first.node = node;
	first.axis = axis;

	nodeStack[0] = first;

	while( process >= 0 ) {
		currentNode = nodeStack[process];
		float t = checkPlaneIntersection( origin, ray, currentNode.node, currentNode.axis, tmax );

		// Ray intersects with splitting plane
		if( t >= 0.0f && t <= tmax ) { // TODO: adjust tmin and tmax
			bestResult_t newResult;
			newResult.distance = -1.0f;
			bool hitFound = checkTriangleIntersection( origin, ray, vertices, currentNode.node, t, tmax, &newResult );

			if( hitFound ) {
				if( result->distance < 0.0f || result->distance > newResult.distance ) {
					*result = newResult;
					result->normalIndex0 = currentNode.node.vertIndex;
					result->normalIndex1 = currentNode.node.vertFace0;
					result->normalIndex2 = currentNode.node.vertFace1;
				}
			}

			if( currentNode.node.nodeLeft < 0 && currentNode.node.nodeRight < 0 ) {
				process--;
				continue;
			}

			if( currentNode.node.nodeLeft >= 0 ) {
				stackEntry_t leftNode;
				leftNode.node = nodes[currentNode.node.nodeLeft];
				leftNode.axis = ( currentNode.axis + 1 ) % KD_DIM;

				process += ( currentNode.node.nodeRight >= 0 ) ? 1 : 0;
				nodeStack[process] = leftNode;
			}

			if( currentNode.node.nodeRight >= 0 ) {
				stackEntry_t rightNode;
				rightNode.node = nodes[currentNode.node.nodeLeft];
				rightNode.axis = ( currentNode.axis + 1 ) % KD_DIM;
				nodeStack[process] = rightNode;
			}
		}

		// Ray doesn't intersect splitting plane
		else {
			kdNode_t nextNode;
			uint axis = currentNode.axis;

			// Ray on "right" side of the plane, proceed with only this part of the tree
			if( currentNode.node.coord[axis] < ( ray - origin )[axis] ) {
				if( currentNode.node.nodeRight >= 0 ) {
					nextNode = nodes[currentNode.node.nodeRight];
				}
				else {
					process--;
					continue;
				}
			}
			// Ray on "left" side of the plane, proceed with only this part of the tree
			else {
				if( currentNode.node.nodeLeft >= 0 ) {
					nextNode = nodes[currentNode.node.nodeLeft];
				}
				else {
					process--;
					continue;
				}
			}

			currentNode.node = nextNode;
			currentNode.axis = ( currentNode.axis + 1 ) % KD_DIM;
			nodeStack[process] = currentNode;
		}
	}
}


inline bestResult_t findIntersection(
	float4 origin, float4 ray, uint numIndices,
	__global float* vertices, __global kdNode_t* nodes, int kdRoot
) {
	uint axis = 0;
	float tmax = T_MAX;
	bestResult_t result;
	result.distance = -1.0f;

	descendKdTree( origin, ray, vertices, nodes, nodes[kdRoot], 0, tmax, &result );

	return result;
}


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


inline float4 uniformlyRandomDirection( float seed ) {
	float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 0.0f ), seed );
	float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 0.0f ), seed );
	float z = 1.0f - 2.0f * u;
	float r = sqrt( 1.0f - z * z );
	float angle = M_PI * 2.0f * v;

	return (float4)( r * cos( angle ), r * sin( angle), z, 0.0f );
}


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


inline float4 calculateColor(
	float4 origin, float4 ray, float4 light,
	__global uint* indices, __global float* vertices, __global float* normals,
	uint numIndices, float timeSinceStart,
	__global kdNode_t* nodes, int kdRoot
) {
	// Accumulate the colors of each hit surface
	float4 colorMask = (float4)( 1.0f, 1.0f, 1.0f, 1.0f );
	float4 accumulatedColor = (float4)( 0.0f );

	for( uint bounce = 0; bounce < 5; bounce++ ) {
		// Find closest surface the ray hits
		bestResult_t result = findIntersection( origin, ray, numIndices, vertices, nodes, kdRoot );

		// No hit, the path ends
		if( result.distance <= -1.0f ) {
			break;
		}

		// Get normal of hit
		uint nIndex0 = result.normalIndex0;
		uint nIndex1 = result.normalIndex1;
		uint nIndex2 = result.normalIndex2;

		float4 normal0 = (float4)( normals[nIndex0], normals[nIndex0 + 1], normals[nIndex0 + 2], 0.0f );
		float4 normal1 = (float4)( normals[nIndex1], normals[nIndex1 + 1], normals[nIndex1 + 2], 0.0f );
		float4 normal2 = (float4)( normals[nIndex2], normals[nIndex2 + 1], normals[nIndex2 + 2], 0.0f );

		float4 normal = ( normal0 + normal1 + normal2 ) / 3.0f;
		normal = normalize( normal );

		// Distance of the hit surface point to the light source
		float4 toLight = light - result.hit;
		toLight.w = 0.0f;

		float diffuse = max( 0.0f, dot( normalize( toLight ), normal ) );
		// 0.0001f – meaning?
		float shadowIntensity = shadow( result.hit + normal * 0.0001f, toLight, indices, vertices, numIndices );
		float specularHighlight = 0.0f; // Disabled
		float4 surfaceColor = (float4)( 0.9f, 0.9f, 0.9f, 1.0f );

		colorMask *= surfaceColor;
		// 0.2f – meaning?
		accumulatedColor += colorMask * 0.2f * diffuse * shadowIntensity;
		accumulatedColor += colorMask * specularHighlight * shadowIntensity;

		// Next bounce
		origin = result.hit;
		ray = cosineWeightedDirection( timeSinceStart + convert_float( bounce ), normal );
		ray.w = 0.0f;
	}

	return accumulatedColor;
}


__kernel void pathTracing(
	__global uint* indices, __global float* vertices, __global float* normals,
	const uint numIndices,
	__global kdNode_t* kdNodes, int kdRoot,
	__global float* eyeIn, __global float* wVecIn, __global float* uVecIn, __global float* vVecIn,
	const float textureWeight, const float timeSinceStart, const float fovRad,
	__read_only image2d_t textureIn, __write_only image2d_t textureOut
) {
	// TODO: reduce work group size
	// NVIDIA GTX 560 Ti limit: { 1024, 1024, 64 } – currently used here: { 512, 512, 1 }
	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };

	// Camera eye
	float4 eye = (float4)( eyeIn[0], eyeIn[1], eyeIn[2], 0.0f );

	// Initial ray (first of the to be created path) shooting from the eye and into the scene
	float4 w = (float4)( wVecIn[0], wVecIn[1], wVecIn[2], 0.0f );
	float4 u = (float4)( uVecIn[0], uVecIn[1], uVecIn[2], 0.0f );
	float4 v = (float4)( vVecIn[0], vVecIn[1], vVecIn[2], 0.0f );

	float width = get_global_size( 0 );
	float height = get_global_size( 1 );
	float pxWidth = 2.0f * tan( fovRad / 2.0f ) / width;
	float pxHeight = 2.0f * tan( fovRad / 2.0f ) / height;

	float4 initialRay = eye + w
			- width / 2.0f * pxWidth * u
			+ height / 2.0f * pxHeight * v
			+ pxWidth / 2.0f * u
			- pxHeight / 2.0f * v;
	initialRay += pos.x * pxWidth * u;
	initialRay -= ( height - pos.y ) * pxHeight * v;
	initialRay.w = 0.0f;

	// Lighting
	float4 light = (float4)( 0.3f, 1.6f, 0.0f, 0.0f );

	// The farther away a shadow is, the more diffuse it becomes
	float4 newLight = light + uniformlyRandomVector( timeSinceStart ) * 0.1f;
	newLight.w = 0.0f;

	// Old color
	float4 texturePixel = read_imagef( textureIn, sampler, pos );

	// New color (or: another color calculated by evaluating different light paths than before)
	float4 calculatedColor = calculateColor(
		eye, initialRay, newLight,
		indices, vertices, normals,
		numIndices, timeSinceStart,
		kdNodes, kdRoot
	);

	// Mix new color with previous one
	float4 color = mix( calculatedColor, texturePixel, textureWeight );
	color.w = 1.0f;

	write_imagef( textureOut, pos, color );
}
