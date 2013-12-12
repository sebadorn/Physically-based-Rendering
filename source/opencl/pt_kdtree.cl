/**
 * Check all faces of a leaf node for intersections with the given ray.
 * @param {const int}             nodeIndex
 * @param {const int}             faceIndex
 * @param {const float4*}         origin
 * @param {const float4*}         dir
 * @param {const global float*}   scVertices
 * @param {const global uint*}    scFaces
 * @param {const global int*}     kdNodeFaces
 * @param {const float}           entryDistance
 * @param {float*}                exitDistance
 * @param {hit_t*}                result
 */
void checkFaces(
	const int nodeIndex, const int faceIndex, const float4* origin, const float4* dir,
	const int numFaces, const global float* kdNodeFaces,
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

	prefetch_a.x = kdNodeFaces[j];
	prefetch_a.y = kdNodeFaces[j + 1];
	prefetch_a.z = kdNodeFaces[j + 2];
	prefetch_b.x = kdNodeFaces[j + 3];
	prefetch_b.y = kdNodeFaces[j + 4];
	prefetch_b.z = kdNodeFaces[j + 5];
	prefetch_c.x = kdNodeFaces[j + 6];
	prefetch_c.y = kdNodeFaces[j + 7];
	prefetch_c.z = kdNodeFaces[j + 8];

	for( uint i = 0; i < numFaces; i += 10 ) {
		j = faceIndex + i + 10;
		a = prefetch_a;
		b = prefetch_b;
		c = prefetch_c;

		prefetch_a.x = kdNodeFaces[j];
		prefetch_a.y = kdNodeFaces[j + 1];
		prefetch_a.z = kdNodeFaces[j + 2];
		prefetch_b.x = kdNodeFaces[j + 3];
		prefetch_b.y = kdNodeFaces[j + 4];
		prefetch_b.z = kdNodeFaces[j + 5];
		prefetch_c.x = kdNodeFaces[j + 6];
		prefetch_c.y = kdNodeFaces[j + 7];
		prefetch_c.z = kdNodeFaces[j + 8];

		r = checkFaceIntersection(
			*origin, *dir, a, b, c,
			entryDistance, *exitDistance
		);

		if( r > -1.0f ) {
			*exitDistance = r;

			if( result->t > r || result->t < -1.0f ) {
				result->t = r;
				result->nodeIndex = nodeIndex;
				result->faceIndex = (uint) kdNodeFaces[j - 1];
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
 * @param  {const global int*}   kdNodeFaces
 * @param  {const float}           entryDistance
 * @param  {float*}                exitDistance
 * @return {bool}                                True, if a hit is detected between origin and light, false otherwise.
 */
bool checkFacesForShadow(
	const int nodeIndex, const int faceIndex, const float4* origin, const float4* dir,
	const int numFaces, const global float* kdNodeFaces,
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

	prefetch_a.x = kdNodeFaces[j];
	prefetch_a.y = kdNodeFaces[j + 1];
	prefetch_a.z = kdNodeFaces[j + 2];
	prefetch_b.x = kdNodeFaces[j + 3];
	prefetch_b.y = kdNodeFaces[j + 4];
	prefetch_b.z = kdNodeFaces[j + 5];
	prefetch_c.x = kdNodeFaces[j + 6];
	prefetch_c.y = kdNodeFaces[j + 7];
	prefetch_c.z = kdNodeFaces[j + 8];

	for( uint i = 0; i < numFaces; i += 10 ) {
		j = faceIndex + i + 10;
		a = prefetch_a;
		b = prefetch_b;
		c = prefetch_c;

		prefetch_a.x = kdNodeFaces[j];
		prefetch_a.y = kdNodeFaces[j + 1];
		prefetch_a.z = kdNodeFaces[j + 2];
		prefetch_b.x = kdNodeFaces[j + 3];
		prefetch_b.y = kdNodeFaces[j + 4];
		prefetch_b.z = kdNodeFaces[j + 5];
		prefetch_c.x = kdNodeFaces[j + 6];
		prefetch_c.y = kdNodeFaces[j + 7];
		prefetch_c.z = kdNodeFaces[j + 8];

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
 * @param {const global float*}   kdNodeSplits
 * @param {const global int*}     kdNodeMeta
 * @param {const float4*}         hitNear
 */
void goToLeafNode(
	int* nodeIndex, const global float* kdNodeSplits, const global int* kdNodeMeta,
	const float* hitNear
) {
	int left = kdNodeMeta[(*nodeIndex) * 6];
	int right = kdNodeMeta[(*nodeIndex) * 6 + 1];
	int axis = kdNodeMeta[(*nodeIndex) * 6 + 2];

	while( left >= 0 && right >= 0 ) {
		*nodeIndex = ( hitNear[axis] < kdNodeSplits[(*nodeIndex) * 3 + axis] ) ? left : right;

		left = kdNodeMeta[(*nodeIndex) * 6];
		right = kdNodeMeta[(*nodeIndex) * 6 + 1];
		axis = MOD_3[axis + 1];
	}
}


/**
 * Set the values for the bounding box of the given node.
 * @param {const int}             nodeIndex
 * @param {const global float*} kdNodeSplits
 * @param {float*}                bbMin
 * @param {float*}                bbMax
 */
inline void updateBoundingBox( const int nodeIndex, const global float* kdNodeBB, float* bbMin, float* bbMax ) {
	bbMin[0] = kdNodeBB[nodeIndex * 6];
	bbMin[1] = kdNodeBB[nodeIndex * 6 + 1];
	bbMin[2] = kdNodeBB[nodeIndex * 6 + 2];
	bbMax[0] = kdNodeBB[nodeIndex * 6 + 3];
	bbMax[1] = kdNodeBB[nodeIndex * 6 + 4];
	bbMax[2] = kdNodeBB[nodeIndex * 6 + 5];
}


/**
 * Find the closest hit of the ray with a surface.
 * Uses stackless kd-tree traversal.
 * @param {const float4*}       origin
 * @param {const float4*}       dir
 * @param {int}                 nodeIndex
 * @param {const global float*} kdNodeSplits
 * @param {const global float*} kdNodeBB
 * @param {const global int*}   kdNodeMeta
 * @param {const global float*} kdNodeFaces
 * @param {const global int*}   kdNodeRopes
 * @param {hit_t*}              result
 * @param {const int}           bounce
 * @param {const int}           kdRoot
 * @param {const float}         entryDistance
 * @param {const float}         exitDistance
 */
void traverseKdTree(
	const float4* origin, const float4* dir, int nodeIndex,
	const global float* kdNodeSplits, const global float* kdNodeBB,
	const global int* kdNodeMeta, const global float* kdNodeFaces,
	const global int* kdNodeRopes, hit_t* result, const int bounce, const int kdRoot,
	float entryDistance, float exitDistance
) {
	float4 hitNear;
	float bbMax[3], bbMin[3];
	int exitRope, faceIndex, numFaces, ropeIndex;

	while( nodeIndex >= 0 && entryDistance < exitDistance ) {
		// Find a leaf node that the ray hits
		hitNear = fma( entryDistance, *dir, *origin );
		goToLeafNode( &nodeIndex, kdNodeSplits, kdNodeMeta, (float*) &hitNear );

		// Check all faces of the leaf node
		faceIndex = kdNodeMeta[nodeIndex * 6 + 3];
		numFaces = kdNodeMeta[nodeIndex * 6 + 4];
		ropeIndex = kdNodeMeta[nodeIndex * 6 + 5];

		checkFaces(
			nodeIndex, faceIndex, origin, dir, numFaces,
			kdNodeFaces, entryDistance, &exitDistance, result
		);

		// Exit leaf node
		updateBoundingBox( nodeIndex, kdNodeBB, bbMin, bbMax );
		updateEntryDistanceAndExitRope( origin, dir, bbMin, bbMax, &entryDistance, &exitRope );

		nodeIndex = kdNodeRopes[ropeIndex + exitRope];
	}

	if( result->t > -1.0f ) {
		nodeIndex = kdRoot;
		hitNear = result->position;
		goToLeafNode( &nodeIndex, kdNodeSplits, kdNodeMeta, &hitNear );
		result->nodeIndex = nodeIndex;
	}
}


/**
 * Test if from the current hit location there is an unobstracted direct path to the light source.
 * @param  {const float4*}       origin
 * @param  {const float4*}       dir
 * @param  {int}                 nodeIndex
 * @param  {const global float*} kdNodeSplits
 * @param  {const global float*} kdNodeBB
 * @param  {const global int*}   kdNodeMeta
 * @param  {const global float*} kdNodeFaces
 * @param  {const global int*}   kdNodeRopes
 * @param  {const int}           kdRoot
 * @param  {const float}         exitDistance
 * @return {bool}
 */
bool shadowTestIntersection(
	const float4* origin, const float4* dir, int nodeIndex,
	const global float* kdNodeSplits, const global float* kdNodeBB,
	const global int* kdNodeMeta, const global float* kdNodeFaces,
	const global int* kdNodeRopes, const int kdRoot,
	float exitDistance
) {
	float entryDistance = 0.0f;

	float4 hitNear;
	float bbMax[3], bbMin[3];
	int exitRope, faceIndex, numFaces, ropeIndex;

	while( nodeIndex >= 0 && entryDistance < exitDistance ) {
		// Find a leaf node that the ray hits
		hitNear = fma( entryDistance, *dir, *origin );
		goToLeafNode( &nodeIndex, kdNodeSplits, kdNodeMeta, (float*) &hitNear );

		// Check all faces of the leaf node
		faceIndex = kdNodeMeta[nodeIndex * 6 + 3];
		numFaces = kdNodeMeta[nodeIndex * 6 + 4];
		ropeIndex = kdNodeMeta[nodeIndex * 6 + 5];

		if( checkFacesForShadow(
			nodeIndex, faceIndex, origin, dir, numFaces,
			kdNodeFaces, entryDistance, &exitDistance
		) ) {
			return true;
		}

		// Exit leaf node
		updateBoundingBox( nodeIndex, kdNodeBB, bbMin, bbMax );
		updateEntryDistanceAndExitRope( origin, dir, bbMin, bbMax, &entryDistance, &exitRope );

		nodeIndex = kdNodeRopes[ropeIndex + exitRope];
	}

	return false;
}
