/**
 * Check all faces of a leaf node for intersections with the given ray.
 * @param {const int}             nodeIndex
 * @param {const int}             faceIndex
 * @param {const float4*}         origin
 * @param {const float4*}         dir
 * @param {const __global float*} scVertices
 * @param {const __global uint*}  scFaces
 * @param {const __global int*}   kdNodeData3
 * @param {const float}           entryDistance
 * @param {float*}                exitDistance
 * @param {hit_t*}                result
 */
void checkFaces(
	const int nodeIndex, const int faceIndex, const float4* origin, const float4* dir,
	const __global float* scVertices, const __global uint* scFaces, const __global int* kdNodeData3,
	const float entryDistance, float* exitDistance, hit_t* result
) {
	float4 a, b, c;
	float r;
	hit_t newResult;

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

		r = checkFaceIntersection( origin, dir, &a, &b, &c, entryDistance, *exitDistance, &newResult );

		if( r > -1.0f ) {
			*exitDistance = r;

			if( result->distance < 0.0f || result->distance > newResult.distance ) {
				result->distance = newResult.distance;
				result->position = newResult.position;
				result->normalIndex = f;
				result->nodeIndex = nodeIndex;
				result->faceIndex = f / 3;
			}
		}
	}
}


/**
 * Check all faces of a leaf node for intersections with the given ray to test if it can reach a light source.
 * If a hit is found between origin and light source, the function returns immediately.
 * @param  {const int}             nodeIndex
 * @param  {const int}             faceIndex
 * @param  {const float4*}         origin
 * @param  {const float4*}         dir
 * @param  {const __global float*} scVertices
 * @param  {const __global uint*}  scFaces
 * @param  {const __global int*}   kdNodeData3
 * @param  {const float}           entryDistance
 * @param  {float*}                exitDistance
 * @return {bool}                                True, if a hit is detected between origin and light, false otherwise.
 */
bool checkFacesForShadow(
	const int nodeIndex, const int faceIndex, const float4* origin, const float4* dir,
	const __global float* scVertices, const __global uint* scFaces, const __global int* kdNodeData3,
	const float entryDistance, float* exitDistance
) {
	float4 a, b, c;
	float r;
	hit_t newResult;

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

		r = checkFaceIntersection( origin, dir, &a, &b, &c, entryDistance, *exitDistance, &newResult );

		if( r > -1.0f ) {
			*exitDistance = r;

			if( r <= 1.0f ) {
				return true;
			}
		}
	}

	return false;
}


/**
 * Traverse down the kd-tree to find a leaf node the given ray intersects.
 * @param {int*}                  nodeIndex
 * @param {const __global float*} kdNodeData1
 * @param {const __global int*}   kdNodeData2
 * @param {cost float4*}          hitNear
 * @param {float*}                bbMin
 * @param {float*}                bbMax
 */
void goToLeafNode(
	int* nodeIndex, const __global float* kdNodeData1, const __global int* kdNodeData2,
	const float4* hitNear, float* bbMin, float* bbMax
) {
	int left = kdNodeData2[(*nodeIndex) * 5];
	int right = kdNodeData2[(*nodeIndex) * 5 + 1];
	int axis = kdNodeData2[(*nodeIndex) * 5 + 2];

	while( left >= 0 && right >= 0 ) {
		*nodeIndex = ( ( (float*) hitNear )[axis] < kdNodeData1[(*nodeIndex) * 9 + axis] ) ? left : right;
		left = kdNodeData2[(*nodeIndex) * 5];
		right = kdNodeData2[(*nodeIndex) * 5 + 1];
		axis = ( 1 << axis ) & 3; // ( axis + 1 ) % 3
	}

	bbMin[0] = kdNodeData1[(*nodeIndex) * 9 + 3];
	bbMin[1] = kdNodeData1[(*nodeIndex) * 9 + 4];
	bbMin[2] = kdNodeData1[(*nodeIndex) * 9 + 5];
	bbMax[0] = kdNodeData1[(*nodeIndex) * 9 + 6];
	bbMax[1] = kdNodeData1[(*nodeIndex) * 9 + 7];
	bbMax[2] = kdNodeData1[(*nodeIndex) * 9 + 8];
}


/**
 * Find the closest hit of the ray with a surface.
 * Uses stackless kd-tree traversal.
 * @param {const float4*}         origin
 * @param {const float4*}         dir
 * @param {const uint}            kdRoot
 * @param {const __global float*} scVertices
 * @param {const __global uint*}  scFaces
 * @param {const __global float*} kdNodeData1
 * @param {const __global int*}   kdNodeData2
 * @param {const __global int*}   kdNodeData3
 * @param {const __global int*}   kdNodeRopes
 * @param {hit_t*}                result
 */
inline void traverseKdTree(
	const float4* origin, const float4* dir, const uint kdRoot,
	const __global float* scVertices, const __global uint* scFaces,
	const __global float* kdNodeData1, const __global int* kdNodeData2, const __global int* kdNodeData3,
	const __global int* kdNodeRopes, hit_t* result
) {
	int nodeIndex = kdRoot;
	const uint ni9 = nodeIndex * 9;

	float bbMin[3] = {
		kdNodeData1[ni9 + 3],
		kdNodeData1[ni9 + 4],
		kdNodeData1[ni9 + 5]
	};
	float bbMax[3] = {
		kdNodeData1[ni9 + 6],
		kdNodeData1[ni9 + 7],
		kdNodeData1[ni9 + 8]
	};

	float entryDistance, exitDistance, tNear;
	int exitRope;

	if( !intersectBoundingBox( origin, dir, bbMin, bbMax, &entryDistance, &exitDistance, &exitRope ) ) {
		return;
	}

	entryDistance = fmax( entryDistance, 0.0f );

	float4 hitNear;
	int faceIndex, ropeIndex;

	while( nodeIndex >= 0 && entryDistance < exitDistance ) {
		// Find a leaf node for this ray
		hitNear = (*origin) + entryDistance * (*dir);
		goToLeafNode( &nodeIndex, kdNodeData1, kdNodeData2, &hitNear, bbMin, bbMax );

		// At a leaf node now, check triangle faces
		faceIndex = kdNodeData2[nodeIndex * 5 + 3];

		checkFaces(
			nodeIndex, faceIndex, origin, dir, scVertices, scFaces, kdNodeData3,
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
	}
}


/**
 * Test if from the current hit location there is an unobstracted direct path to the light source.
 * @param  {const float4*}         origin
 * @param  {const float4*}         toLight
 * @param  {const uint}            startNode
 * @param  {const __global float*} scVertices
 * @param  {const __global uint*}  scFaces
 * @param  {const __global float*} kdNodeData1
 * @param  {const __global int*}   kdNodeData2
 * @param  {const __global int*}   kdNodeData3
 * @param  {const __global int*}   kdNodeRopes
 * @return {bool}
 */
inline bool shadowTest(
	const float4* origin, const float4* dir, const uint startNode,
	const __global float* scVertices, const __global uint* scFaces,
	const __global float* kdNodeData1, const __global int* kdNodeData2, const __global int* kdNodeData3,
	const __global int* kdNodeRopes
) {
	int nodeIndex = startNode;
	const uint ni9 = nodeIndex * 9;

	float bbMin[3] = {
		kdNodeData1[ni9 + 3],
		kdNodeData1[ni9 + 4],
		kdNodeData1[ni9 + 5]
	};
	float bbMax[3] = {
		kdNodeData1[ni9 + 6],
		kdNodeData1[ni9 + 7],
		kdNodeData1[ni9 + 8]
	};

	float entryDistance, exitDistance, tNear;
	int exitRope;

	entryDistance = 0.0f;
	exitDistance = 0.9f;

	float4 hitNear;
	int faceIndex, ropeIndex;
	// int i = 0; // TODO: remove

	while( nodeIndex >= 0 && entryDistance < exitDistance ) {
		// if( i++ > 1000 ) return false; // TODO: remove

		// Find a leaf node for this ray
		hitNear = (*origin) + entryDistance * (*dir);
		goToLeafNode( &nodeIndex, kdNodeData1, kdNodeData2, &hitNear, bbMin, bbMax );

		// At a leaf node now, check triangle faces
		faceIndex = kdNodeData2[nodeIndex * 5 + 3];

		if( checkFacesForShadow(
			nodeIndex, faceIndex, origin, dir, scVertices, scFaces, kdNodeData3,
			entryDistance, &exitDistance
		) ) {
			return true;
		}

		// Exit leaf node
		intersectBoundingBox( origin, dir, bbMin, bbMax, &tNear, &entryDistance, &exitRope );

		// Follow the rope
		ropeIndex = kdNodeData2[nodeIndex * 5 + 4];
		nodeIndex = kdNodeRopes[ropeIndex + exitRope];
	}

	return false;
}
