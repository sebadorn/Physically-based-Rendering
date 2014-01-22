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
	const int numFaces, const global uint* kdFaces,
	const global float4* scVertices, const global uint4* scFaces,
	const float entryDistance, float* exitDistance, const float boxExitLimit
) {
	float4 a, b, c;
	float r;
	uint j;

	for( uint i = 0; i < numFaces; i++ ) {
		j = kdFaces[faceIndex + i];

		a = scVertices[scFaces[j].x];
		b = scVertices[scFaces[j].y];
		c = scVertices[scFaces[j].z];

		r = checkFaceIntersection(
			ray->origin, ray->dir, a, b, c,
			entryDistance, fmin( *exitDistance, boxExitLimit )
		);

		if( r > -1.0f ) {
			*exitDistance = r;

			if( ray->t > r || ray->nodeIndex < 0 ) {
				ray->t = r;
				ray->nodeIndex = nodeIndex;
				ray->faceIndex = j;
			}
		}
	}
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
 * @param {const int}           kdRoot
 * @param {const float}         entryDistance
 * @param {const float}         exitDistance
 */
void traverseKdTree(
	ray4* ray, int nodeIndex, const global kdNonLeaf* kdNonLeaves,
	const global kdLeaf* kdLeaves, const global uint* kdFaces,
	const global float4* scVertices, const global uint4* scFaces,
	float entryDistance, float exitDistance
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
			ray, nodeIndex, ropes.s6, ropes.s7, kdFaces,
			scVertices, scFaces, entryDistance, &exitDistance, boxExitLimit
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
