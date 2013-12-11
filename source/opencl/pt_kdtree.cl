/**
 * Check all faces of a leaf node for intersections with the given ray.
 * @param {const int}             nodeIndex
 * @param {const int}             faceIndex
 * @param {const float4*}         origin
 * @param {const float4*}         dir
 * @param {const global float*}   scVertices
 * @param {const global uint*}    scFaces
 * @param {const global int*}     kdNodeData3
 * @param {const float}           entryDistance
 * @param {float*}                exitDistance
 * @param {hit_t*}                result
 */
void checkFaces(
	const int nodeIndex, const int faceIndex, const float4* origin, const float4* dir,
	const int numFaces, const global float* kdNodeData3,
	const float entryDistance, float* exitDistance, hit_t* result
) {
	float4 a = (float4)( 0.0f );
	float4 b = (float4)( 0.0f );
	float4 c = (float4)( 0.0f );
	float4 prefetch_a = (float4)( 0.0f );
	float4 prefetch_b = (float4)( 0.0f );
	float4 prefetch_c = (float4)( 0.0f );
	float r;

	uint j = faceIndex;

	prefetch_a.x = kdNodeData3[j];
	prefetch_a.y = kdNodeData3[j + 1];
	prefetch_a.z = kdNodeData3[j + 2];
	prefetch_b.x = kdNodeData3[j + 3];
	prefetch_b.y = kdNodeData3[j + 4];
	prefetch_b.z = kdNodeData3[j + 5];
	prefetch_c.x = kdNodeData3[j + 6];
	prefetch_c.y = kdNodeData3[j + 7];
	prefetch_c.z = kdNodeData3[j + 8];

	for( uint i = 0; i < numFaces; i += 10 ) {
		a = prefetch_a;
		b = prefetch_b;
		c = prefetch_c;

		j = faceIndex + i + 10;
		prefetch_a.x = kdNodeData3[j];
		prefetch_a.y = kdNodeData3[j + 1];
		prefetch_a.z = kdNodeData3[j + 2];
		prefetch_b.x = kdNodeData3[j + 3];
		prefetch_b.y = kdNodeData3[j + 4];
		prefetch_b.z = kdNodeData3[j + 5];
		prefetch_c.x = kdNodeData3[j + 6];
		prefetch_c.y = kdNodeData3[j + 7];
		prefetch_c.z = kdNodeData3[j + 8];

		r = checkFaceIntersection(
			*origin, *dir, a, b, c,
			entryDistance, *exitDistance
		);

		if( r > -1.0f ) {
			*exitDistance = r;

			if( result->t > r || result->t < -1.0f ) {
				result->t = r;
				result->nodeIndex = nodeIndex;
				result->faceIndex = (uint) kdNodeData3[faceIndex + i + 9];
			}
		}
	}

	result->position = ( result->t > -1.0f )
	                 ? (*origin) + result->t * (*dir)
	                 : result->position;
}


/**
 * Check all faces of a leaf node for intersections with the given ray to test if it can reach a light source.
 * If a hit is found between origin and light source, the function returns immediately.
 * @param  {const int}             nodeIndex
 * @param  {const int}             faceIndex
 * @param  {const float4*}         origin
 * @param  {const float4*}         dir
 * @param  {const global float*} scVertices
 * @param  {const global uint*}  scFaces
 * @param  {const global int*}   kdNodeData3
 * @param  {const float}           entryDistance
 * @param  {float*}                exitDistance
 * @return {bool}                                True, if a hit is detected between origin and light, false otherwise.
 */
bool checkFacesForShadow(
	const int nodeIndex, const int faceIndex, const float4* origin, const float4* dir,
	const int numFaces, const global float* kdNodeData3,
	const float entryDistance, float* exitDistance
) {
	float4 a = (float4)( 0.0f );
	float4 b = (float4)( 0.0f );
	float4 c = (float4)( 0.0f );
	float4 prefetch_a = (float4)( 0.0f );
	float4 prefetch_b = (float4)( 0.0f );
	float4 prefetch_c = (float4)( 0.0f );
	float r;

	uint j = faceIndex;

	prefetch_a.x = kdNodeData3[j];
	prefetch_a.y = kdNodeData3[j + 1];
	prefetch_a.z = kdNodeData3[j + 2];
	prefetch_b.x = kdNodeData3[j + 3];
	prefetch_b.y = kdNodeData3[j + 4];
	prefetch_b.z = kdNodeData3[j + 5];
	prefetch_c.x = kdNodeData3[j + 6];
	prefetch_c.y = kdNodeData3[j + 7];
	prefetch_c.z = kdNodeData3[j + 8];

	for( uint i = 0; i < numFaces; i += 10 ) {
		a = prefetch_a;
		b = prefetch_b;
		c = prefetch_c;

		j = faceIndex + i + 10;
		prefetch_a.x = kdNodeData3[j];
		prefetch_a.y = kdNodeData3[j + 1];
		prefetch_a.z = kdNodeData3[j + 2];
		prefetch_b.x = kdNodeData3[j + 3];
		prefetch_b.y = kdNodeData3[j + 4];
		prefetch_b.z = kdNodeData3[j + 5];
		prefetch_c.x = kdNodeData3[j + 6];
		prefetch_c.y = kdNodeData3[j + 7];
		prefetch_c.z = kdNodeData3[j + 8];

		r = checkFaceIntersection(
			*origin, *dir, a, b, c,
			entryDistance, *exitDistance
		);

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
 * @param {const global float*} kdNodeData1
 * @param {const global int*}   kdNodeData2
 * @param {const float4*}         hitNear
 */
void goToLeafNode(
	int* nodeIndex, const global float4* kdNodeSplits, const global int* kdNodeData2,
	const float4* hitNear
) {
	int left = kdNodeData2[(*nodeIndex) * 6];
	int right = kdNodeData2[(*nodeIndex) * 6 + 1];
	int axis = kdNodeData2[(*nodeIndex) * 6 + 2];
	const float hitPos[3] = { hitNear->x, hitNear->y, hitNear->z };
	float4 pos;

	while( left >= 0 && right >= 0 ) {
		pos = kdNodeSplits[(*nodeIndex)];
		*nodeIndex = ( hitPos[axis] < ( (float*) &pos )[axis] ) ? left : right;
		left = kdNodeData2[(*nodeIndex) * 6];
		right = kdNodeData2[(*nodeIndex) * 6 + 1];
		axis = MOD_3[axis + 1];
	}
}


/**
 * Set the values for the bounding box of the given node.
 * @param {const int}             nodeIndex
 * @param {const global float*} kdNodeData1
 * @param {float*}                bbMin
 * @param {float*}                bbMax
 */
inline void updateBoundingBox( const int nodeIndex, const global float4* kdNodeBB, float* bbMin, float* bbMax ) {
	float4 bbMin4 = kdNodeBB[nodeIndex * 2];
	float4 bbMax4 = kdNodeBB[nodeIndex * 2 + 1];

	bbMin[0] = bbMin4.x;
	bbMin[1] = bbMin4.y;
	bbMin[2] = bbMin4.z;
	bbMax[0] = bbMax4.x;
	bbMax[1] = bbMax4.y;
	bbMax[2] = bbMax4.z;
}


/**
 * Find the closest hit of the ray with a surface.
 * Uses stackless kd-tree traversal.
 * @param {const float4*}         origin
 * @param {const float4*}         dir
 * @param {const uint}            startNode
 * @param {const global float*} scVertices
 * @param {const global uint*}  scFaces
 * @param {const global float*} kdNodeData1
 * @param {const global int*}   kdNodeData2
 * @param {const global int*}   kdNodeData3
 * @param {const global int*}   kdNodeRopes
 * @param {hit_t*}                result
 */
inline void traverseKdTree(
	const float4* origin, const float4* dir, const int startNode,
	const global float4* kdNodeSplits, const global float4* kdNodeBB,
	const global int* kdNodeData2, const global float* kdNodeData3,
	const global int* kdNodeRopes, hit_t* result, const int bounce, const int kdRoot,
	const float initEntryDistance, const float initExitDistance
) {
	int nodeIndex = startNode;
	float entryDistance = initEntryDistance;
	float exitDistance = initExitDistance;

	float4 hitNear;
	float bbMax[3], bbMin[3];
	int exitRope, faceIndex, numFaces, ropeIndex;
	int i = 0; // TODO: remove

	while( nodeIndex >= 0 && entryDistance < exitDistance ) {
		if( i++ > 1000 ) return; // TODO: remove

		// Find a leaf node for this ray
		hitNear = (*origin) + entryDistance * (*dir);
		goToLeafNode( &nodeIndex, kdNodeSplits, kdNodeData2, &hitNear );

		// At a leaf node now, check triangle faces
		faceIndex = kdNodeData2[nodeIndex * 6 + 3];
		numFaces = kdNodeData2[nodeIndex * 6 + 4];
		// Follow the rope
		ropeIndex = kdNodeData2[nodeIndex * 6 + 5];

		checkFaces(
			nodeIndex, faceIndex, origin, dir, numFaces,
			kdNodeData3, entryDistance, &exitDistance, result
		);

		// Exit leaf node
		updateBoundingBox( nodeIndex, kdNodeBB, bbMin, bbMax );
		updateEntryDistanceAndExitRope( origin, dir, bbMin, bbMax, &entryDistance, &exitRope );

		if( entryDistance >= exitDistance ) {
			break;
		}

		nodeIndex = kdNodeRopes[ropeIndex + exitRope];
	}

	if( result->t > -1.0f ) {
		nodeIndex = kdRoot;
		hitNear = result->position;
		goToLeafNode( &nodeIndex, kdNodeSplits, kdNodeData2, &hitNear );
		result->nodeIndex = nodeIndex;
	}
}


/**
 * Test if from the current hit location there is an unobstracted direct path to the light source.
 * @param  {const float4*}         origin
 * @param  {const float4*}         toLight
 * @param  {const uint}            startNode
 * @param  {const global float*} scVertices
 * @param  {const global uint*}  scFaces
 * @param  {const global float*} kdNodeData1
 * @param  {const global int*}   kdNodeData2
 * @param  {const global int*}   kdNodeData3
 * @param  {const global int*}   kdNodeRopes
 * @return {bool}
 */
inline bool shadowTest(
	const float4* origin, const float4* dir, const uint startNode,
	const global float4* kdNodeSplits, const global float4* kdNodeBB,
	const global int* kdNodeData2, const global float* kdNodeData3,
	const global int* kdNodeRopes, const int kdRoot,
	const float initEntryDistance, const float initExitDistance
) {
	int nodeIndex = startNode;
	float entryDistance = initEntryDistance;
	float exitDistance = initExitDistance;

	float4 hitNear;
	float bbMax[3], bbMin[3];
	int exitRope, faceIndex, numFaces, ropeIndex;
	int i = 0; // TODO: remove

	while( nodeIndex >= 0 && entryDistance < exitDistance ) {
		if( i++ > 1000 ) return false; // TODO: remove

		// Find a leaf node for this ray
		hitNear = (*origin) + entryDistance * (*dir);
		goToLeafNode( &nodeIndex, kdNodeSplits, kdNodeData2, &hitNear );

		// At a leaf node now, check triangle faces
		faceIndex = kdNodeData2[nodeIndex * 6 + 3];
		numFaces = kdNodeData2[nodeIndex * 6 + 4];
		// Follow the rope
		ropeIndex = kdNodeData2[nodeIndex * 6 + 5];

		if( checkFacesForShadow(
			nodeIndex, faceIndex, origin, dir, numFaces,
			kdNodeData3, entryDistance, &exitDistance
		) ) {
			return true;
		}

		// Exit leaf node
		updateBoundingBox( nodeIndex, kdNodeBB, bbMin, bbMax );
		updateEntryDistanceAndExitRope( origin, dir, bbMin, bbMax, &entryDistance, &exitRope );

		if( entryDistance > 1.0f ) {
			return false;
		}

		nodeIndex = kdNodeRopes[ropeIndex + exitRope];
	}

	return false;
}
