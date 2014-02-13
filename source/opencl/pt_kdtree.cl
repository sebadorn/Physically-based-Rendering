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
int goToLeafNode( uint nodeIndex, const global kdNonLeaf* kdNonLeaves, const float4 hitNear ) {
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
	float tNear, float tFar, uint kdRoot
) {
	kdLeaf currentNode;
	int8 ropes;
	int exitRope;
	float boxExitLimit;

	int nodeIndex = goToLeafNode( kdRoot, kdNonLeaves, fma( tNear, ray->dir, ray->origin ) );

	while( nodeIndex >= 0 && tNear < tFar ) {
		currentNode = kdLeaves[nodeIndex];
		ropes = currentNode.ropes;
		boxExitLimit = getBoxExitLimit( ray->origin, ray->dir, currentNode.bbMin, currentNode.bbMax );

		checkFaces(
			ray, nodeIndex, ropes.s6, ropes.s7, kdFaces,
			faces, tNear, &tFar, boxExitLimit
		);

		// Exit leaf node
		updateEntryDistanceAndExitRope(
			ray->origin, ray->dir, currentNode.bbMin, currentNode.bbMax,
			&tNear, &exitRope
		);

		nodeIndex = ( (int*) &ropes )[exitRope];
		nodeIndex = ( nodeIndex < 1 )
		          ? -nodeIndex - 1
		          : goToLeafNode( nodeIndex - 1, kdNonLeaves, fma( tNear, ray->dir, ray->origin ) );
	}
}


/**
 * Traverse the BVH and test the kD-trees against the given ray.
 * @param {const global bvhNode*}   bvh
 * @param {bvhNode}                 node        Root node of the BVH.
 * @param {ray4*}                   ray
 * @param {const global kdNonLeaf*} kdNonLeaves
 * @param {const global kdLeaf*}    kdLeaves
 * @param {const global uint*}      kdFaces
 * @param {const global face_t*}    faces
 */
void traverseBVH(
	const global bvhNode* bvh, bvhNode node, ray4* ray, const global kdNonLeaf* kdNonLeaves,
	const global kdLeaf* kdLeaves, const global uint* kdFaces, const global face_t* faces
) {
	bvhNode leftNode, rightNode;
	ray4 rayCp1, rayCp2;
	bool leftIsLeaf, rightIsLeaf;
	uint leftKdRoot, rightKdRoot;
	float tNear, tFar;

	int stackIndex = 0;
	bvhNode bvhStack[BVH_STACKSIZE];
	bvhStack[stackIndex] = node;

	while( stackIndex >= 0 ) {
		node = bvhStack[stackIndex];
		stackIndex--;

		if( node.rightChild >= 0 ) {
			rightNode = bvh[node.rightChild];
			rightIsLeaf = ( rightNode.bbMin.w == 1.0f );
			rightKdRoot = rightNode.bbMax.w;
			rightNode.bbMin.w = 0.0f;
			rightNode.bbMax.w = 0.0f;
		}

		if( node.leftChild >= 0 ) {
			leftNode = bvh[node.leftChild];
			leftIsLeaf = ( leftNode.bbMin.w == 1.0f );
			leftKdRoot = leftNode.bbMax.w;
			leftNode.bbMin.w = 0.0f;
			leftNode.bbMax.w = 0.0f;
		}

		rayCp1 = *ray;
		rayCp2 = *ray;
		tFar = FLT_MAX;

		if(
			node.rightChild >= 0 &&
			intersectBoundingBox( ray->origin, ray->dir, rightNode.bbMin, rightNode.bbMax, &tNear, &tFar ) &&
			( ray->t < -1.0f || ray->t >= tNear )
		) {
			if( rightIsLeaf ) {
				traverseKdTree( &rayCp2, kdNonLeaves, kdLeaves, kdFaces, faces, tNear, tFar, rightKdRoot );
			}
			else {
				stackIndex++;
				bvhStack[stackIndex] = rightNode;
			}
		}

		tFar = FLT_MAX;

		if(
			node.leftChild >= 0 &&
			intersectBoundingBox( ray->origin, ray->dir, leftNode.bbMin, leftNode.bbMax, &tNear, &tFar ) &&
			( ray->t < -1.0f || ray->t >= tNear )
		) {
			if( leftIsLeaf ) {
				traverseKdTree( &rayCp1, kdNonLeaves, kdLeaves, kdFaces, faces, tNear, tFar, leftKdRoot );
			}
			else {
				stackIndex++;
				bvhStack[stackIndex] = leftNode;
			}
		}

		if(
			( ray->nodeIndex < 0 && rayCp2.nodeIndex >= 0 ) ||
			( rayCp2.nodeIndex >= 0 && ray->t > rayCp2.t )
		) {
			*ray = rayCp2;
		}

		if(
			( ray->nodeIndex < 0 && rayCp1.nodeIndex >= 0 ) ||
			( rayCp1.nodeIndex >= 0 && ray->t > rayCp1.t )
		) {
			*ray = rayCp1;
		}
	}
}
