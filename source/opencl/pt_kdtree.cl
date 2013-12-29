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
	ray4* ray, const int nodeIndex, const int faceIndex,
	const int numFaces, const global float* kdNodeFaces,
	const float entryDistance, float* exitDistance, const float boxExitLimit
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

	for( uint i = 1; i <= numFaces; i++ ) {
		j = faceIndex + i * 10;
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
			ray->origin, ray->dir, a, b, c,
			entryDistance, fmin( *exitDistance, boxExitLimit )
		);

		if( r > -1.0f ) {
			*exitDistance = r;

			if( ray->t > r || ray->t < -1.0f ) {
				ray->t = r;
				ray->nodeIndex = nodeIndex;
				ray->faceIndex = kdNodeFaces[j - 1];
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
 * @param  {const global float*} scVertices
 * @param  {const global uint*}  scFaces
 * @param  {const global int*}   kdNodeFaces
 * @param  {const float}           entryDistance
 * @param  {float*}                exitDistance
 * @return {bool}                                True, if a hit is detected between origin and light, false otherwise.
 */
bool checkFacesForShadow(
	ray4* ray, const int nodeIndex, const int faceIndex,
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

	for( uint i = 0; i < numFaces; i++ ) {
		j = faceIndex + i * 10 + 10;
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
			ray->origin, ray->dir, a, b, c, entryDistance, *exitDistance
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
 * @param  {int}                     nodeIndex
 * @param  {const global kdNonLeaf*} kdNonLeaves
 * @param  {const float4}            hitNear
 * @return {int}
 */
int goToLeafNode( int nodeIndex, const global kdNonLeaf* kdNonLeaves, const float4 hitNear ) {
	float4 split;
	int4 children;

	int axis = kdNonLeaves[nodeIndex].split.w;
	float hitPos[3] = { hitNear.x, hitNear.y, hitNear.z };
	float splitPos[3];
	bool isOnLeft;

	while( true ) {
		children = kdNonLeaves[nodeIndex].children;
		split = kdNonLeaves[nodeIndex].split;

		splitPos[0] = split.x;
		splitPos[1] = split.y;
		splitPos[2] = split.z;

		isOnLeft = ( hitPos[axis] < splitPos[axis] );
		nodeIndex = isOnLeft ? children.x : children.y;

		if( ( isOnLeft && children.z ) || ( !isOnLeft && children.w ) ) {
			return nodeIndex;
		}

		axis = MOD_3[axis + 1];
	}

	return -1;
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
	ray4* ray, int nodeIndex,
	const global kdNonLeaf* kdNonLeaves,
	const global kdLeaf* kdLeaves, const global float* kdNodeFaces,
	const int bounce, float entryDistance, float exitDistance
) {
	kdLeaf currentNode;
	int8 ropes;
	int exitRope = 0;
	float boxExitLimit = FLT_MAX;

	while( nodeIndex >= 0 && entryDistance < exitDistance ) {
		currentNode = kdLeaves[nodeIndex];
		ropes = currentNode.ropes;
		boxExitLimit = getBoxExitLimit( ray->origin, ray->dir, currentNode.bbMin, currentNode.bbMax );

		checkFaces(
			ray, nodeIndex, ropes.s6, ropes.s7, kdNodeFaces,
			entryDistance, &exitDistance, boxExitLimit
		);

		// Exit leaf node
		updateEntryDistanceAndExitRope(
			ray->origin, ray->dir, currentNode.bbMin, currentNode.bbMax,
			&entryDistance, &exitRope
		);

		nodeIndex = ( (int*) &ropes )[exitRope];
		nodeIndex = ( nodeIndex < 1 )
		          ? -nodeIndex - 1
		          : goToLeafNode( nodeIndex - 1, kdNonLeaves, fma( entryDistance, ray->dir, ray->origin ) );
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
 * @param  {const float}         exitDistance
 * @return {bool}
 */
bool shadowTestIntersection(
	ray4* ray, const global kdNonLeaf* kdNonLeaves,
	const global kdLeaf* kdLeaves, const global float* kdNodeFaces
) {
	kdLeaf currentNode;
	int8 ropes;
	int exitRope;
	int nodeIndex = ray->nodeIndex;
	float entryDistance = 0.0f;
	float exitDistance = 1.0f;

	while( nodeIndex >= 0 && entryDistance < exitDistance ) {
		currentNode = kdLeaves[nodeIndex];
		ropes = currentNode.ropes;

		if( checkFacesForShadow(
			ray, nodeIndex, ropes.s6, ropes.s7, kdNodeFaces,
			entryDistance, &exitDistance
		) ) {
			return true;
		}

		// Exit leaf node
		updateEntryDistanceAndExitRope(
			ray->origin, ray->dir, currentNode.bbMin, currentNode.bbMax,
			&entryDistance, &exitRope
		);

		nodeIndex = ( (int*) &ropes )[exitRope];
		nodeIndex = ( nodeIndex < 1 )
		          ? -nodeIndex - 1
		          : goToLeafNode( nodeIndex - 1, kdNonLeaves, fma( entryDistance, ray->dir, ray->origin ) );
	}

	return false;
}
