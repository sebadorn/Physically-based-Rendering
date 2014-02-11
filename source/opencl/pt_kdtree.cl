/**
 * Check all faces of a leaf node for intersections with the given ray.
 */
void checkFaces(
	ray4* ray, const int nodeIndex, const int faceIndex,
	const int numFaces, const global uint* kdFaces, const global face_t* faces,
	const float entryDistance, float* exitDistance, const float boxExitLimit
) {
	float r = -2.0f;
	face_t face;
	uint j;

	for( uint i = 0; i < numFaces; i++ ) {
		j = kdFaces[faceIndex + i];
		face = faces[j];

		r = checkFaceIntersection(
			ray->origin, ray->dir, face.a, face.b, face.c,
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
int goToLeafNode(
	uint nodeIndex, const global kdNonLeaf* kdNonLeaves, const float4 hitNear
) {
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
 */
void traverseKdTree(
	ray4* ray, const global kdNonLeaf* kdNonLeaves,
	const global kdLeaf* kdLeaves, const global uint* kdFaces, const global face_t* faces,
	float entryDistance, uint kdRoot
) {
	kdLeaf currentNode;
	int8 ropes;
	int exitRope;
	float boxExitLimit;
	float exitDistance = FLT_MAX;

	int nodeIndex = goToLeafNode( kdRoot, kdNonLeaves, fma( entryDistance, ray->dir, ray->origin ) );

	while( nodeIndex >= 0 && entryDistance < exitDistance ) {
		currentNode = kdLeaves[nodeIndex];
		ropes = currentNode.ropes;
		boxExitLimit = getBoxExitLimit( ray->origin, ray->dir, currentNode.bbMin, currentNode.bbMax );

		checkFaces(
			ray, nodeIndex, ropes.s6, ropes.s7, kdFaces,
			faces, entryDistance, &exitDistance, boxExitLimit
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


void traverseBVH(
	const global bvhNode* bvh, bvhNode node, ray4* ray, const global kdNonLeaf* kdNonLeaves,
	const global kdLeaf* kdLeaves, const global uint* kdFaces, const global face_t* faces,
	float entryDistance
) {
	bvhNode leftNode, rightNode;
	bool isLeftLeaf, isRightLeaf;
	uint leftKdRoot, rightKdRoot;

	int stackIndex = 0;
	bvhNode bvhStack[20];
	bvhStack[stackIndex] = node;

	int i = 0;
	while( i++ < 20 && stackIndex >= 0 ) {
		node = bvhStack[stackIndex--];
		leftNode = bvh[node.leftChild];
		rightNode = bvh[node.rightChild];

		isLeftLeaf = ( leftNode.bbMin.w == 1.0f );
		isRightLeaf = ( rightNode.bbMin.w == 1.0f );

		leftKdRoot = leftNode.bbMax.w;
		rightKdRoot = rightNode.bbMax.w;

		leftNode.bbMin.w = 0.0f;
		rightNode.bbMin.w = 0.0f;
		leftNode.bbMax.w = 0.0f;
		rightNode.bbMax.w = 0.0f;

		ray4 rayCp1 = *ray;
		ray4 rayCp2 = *ray;

		if( intersectBB( ray->origin, ray->dir, leftNode.bbMin, leftNode.bbMax ) ) {
			if( isLeftLeaf ) {
				traverseKdTree(
					&rayCp1, kdNonLeaves, kdLeaves, kdFaces, faces, entryDistance, leftKdRoot
				);
			}
			else {
				bvhStack[stackIndex++] = leftNode;
			}
		}

		if( intersectBB( ray->origin, ray->dir, rightNode.bbMin, rightNode.bbMax ) ) {
			if( isRightLeaf ) {
				traverseKdTree(
					&rayCp2, kdNonLeaves, kdLeaves, kdFaces, faces, entryDistance, rightKdRoot
				);
			}
			else {
				bvhStack[stackIndex++] = rightNode;
			}
		}

		if(
			( ray->nodeIndex < 0 && rayCp1.nodeIndex >= 0 ) ||
			( rayCp1.nodeIndex >= 0 && ray->t > rayCp1.t )
		) {
			*ray = rayCp1;
		}
		if(
			( ray->nodeIndex < 0 && rayCp2.nodeIndex >= 0 ) ||
			( rayCp2.nodeIndex >= 0 && ray->t > rayCp2.t )
		) {
			*ray = rayCp2;
		}
	}
}
