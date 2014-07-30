// Traversal for the acceleration structure.
// Type: Combination of kD-tree (each object) and BVH (objects in the scene).

#define CALL_TRAVERSE         traverse( bvh, kdNonLeaves, kdLeaves, kdFaces, &ray, faces );
#define CALL_TRAVERSE_SHADOWS traverse_shadows( bvh, kdNonLeaves, kdLeaves, kdFaces, &lightRay, faces );


/**
 * Check all faces of a leaf node for intersections with the given ray.
 */
void checkFaces(
	ray4* ray, const int faceIndex, const int numFaces,
	const global uint* kdFaces, const global face_t* faces,
	const float entryDistance, float* exitDistance
) {
	for( uint i = faceIndex; i < faceIndex + numFaces; i++ ) {
		float3 tuv;
		uint j = kdFaces[i];

		float4 normal = checkFaceIntersection( ray, faces[j], &tuv, entryDistance, *exitDistance );

		if( tuv.x < INFINITY ) {
			*exitDistance = tuv.x;

			if( ray->t > tuv.x ) {
				ray->normal = normal;
				ray->normal.w = (float) j;
				ray->t = tuv.x;
			}
		}
	}
}


/**
 * Traverse down the kD-tree to find a leaf node the given ray intersects.
 * @param  {int}                     nodeIndex
 * @param  {const global kdNonLeaf*} kdNonLeaves
 * @param  {const float3}            hitNear
 * @return {int}
 */
int goToLeafNode( uint nodeIndex, const global kdNonLeaf* kdNonLeaves, const float3 hitNear ) {
	float4 split;
	int4 children;

	int axis = kdNonLeaves[nodeIndex].split.w;
	float* hitPos = (float*) &hitNear;
	float* splitPos;
	bool isOnLeft;

	while( true ) {
		children = kdNonLeaves[nodeIndex].children;
		split = kdNonLeaves[nodeIndex].split;
		splitPos = (float*) &split;

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
* Source: http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-7-intersecting-simple-shapes/ray-box-intersection/
* Which is based on: "An Efficient and Robust Ray–Box Intersection Algorithm", Williams et al.
* @param {const float4*} origin
* @param {const float4*} dir
* @param {const float*} bbMin
* @param {const float*} bbMax
* @param {float*} tFar
* @param {int*} exitRope
*/
void updateEntryDistanceAndExitRope(
	const ray4* ray, const float4 bbMin, const float4 bbMax, float* tFar, int* exitRope
) {
	const float4 invDir = native_recip( ray->dir );
	const bool signX = signbit( invDir.x );
	const bool signY = signbit( invDir.y );
	const bool signZ = signbit( invDir.z );

	float4 t1 = native_divide( bbMin - ray->origin, ray->dir );
	float4 tMax = native_divide( bbMax - ray->origin, ray->dir );
	tMax = fmax( t1, tMax );

	*tFar = fmin( fmin( tMax.x, tMax.y ), tMax.z );
	*exitRope = ( *tFar == tMax.y ) ? 3 - signY : 1 - signX;
	*exitRope = ( *tFar == tMax.z ) ? 5 - signZ : *exitRope;
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
	int nodeIndex = goToLeafNode( kdRoot, kdNonLeaves, ray->origin.xyz + tNear * ray->dir.xyz );

	while( nodeIndex >= 0 && tNear < tFar ) {
		currentNode = kdLeaves[nodeIndex];
		ropes = currentNode.ropes;

		checkFaces( ray, ropes.s6, ropes.s7, kdFaces, faces, tNear, &tFar );

		// Exit leaf node
		updateEntryDistanceAndExitRope(
			ray, currentNode.bbMin, currentNode.bbMax, &tNear, &exitRope
		);

		if( tNear >= tFar ) {
			break;
		}

		nodeIndex = ( (int*) &ropes )[exitRope];
		nodeIndex = ( nodeIndex < 1 )
		          ? -( nodeIndex + 1 )
		          : goToLeafNode( nodeIndex - 1, kdNonLeaves, ray->origin.xyz + tNear * ray->dir.xyz );
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
void traverse(
	const global bvhNode* bvh, const global kdNonLeaf* kdNonLeaves,
	const global kdLeaf* kdLeaves, const global uint* kdFaces,
	ray4* ray, const global face_t* faces
) {
	int leftChildIndex, rightChildIndex;
	uint bvhStack[BVH_STACKSIZE];
	int stackIndex = 0;
	bvhStack[stackIndex] = 0; // Node 0 is always the BVH root node

	const float3 invDir = native_recip( ray->dir.xyz );
	float tBest = INFINITY;

	while( stackIndex >= 0 ) {
		bvhNode node = bvh[bvhStack[stackIndex--]];
		float tNearL = 0.0f;
		float tFarL = INFINITY;

		leftChildIndex = (int) node.bbMin.w;


		// Is a leaf node and contains a kD-tree
		if( leftChildIndex < 0 ) {
			if(
				intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) &&
				tBest > tNearL
			) {
				traverseKdTree(
					ray, kdNonLeaves, kdLeaves, kdFaces, faces, tNearL, tFarL, -( leftChildIndex + 1 )
				);
				tBest = fmin( ray->t, tBest );
				ray->t = tBest;
			}

			continue;
		}

		// Add child nodes to stack, if hit by ray

		bvhNode childNode = bvh[leftChildIndex - 1];

		bool addLeftToStack = (
			intersectBox( ray, &invDir, childNode.bbMin, childNode.bbMax, &tNearL, &tFarL ) &&
			tBest > tNearL
		);

		float tNearR = 0.0f;
		float tFarR = INFINITY;
		childNode = bvh[(int) node.bbMax.w - 1];

		bool addRightToStack = (
			intersectBox( ray, &invDir, childNode.bbMin, childNode.bbMax, &tNearR, &tFarR ) &&
			tBest > tNearR
		);


		// The node that is pushed on the stack first will be evaluated last.
		// So the nearer one should be pushed last, because it will be popped first then.
		const bool rightThenLeft = ( tNearR > tNearL );

		if( rightThenLeft && addRightToStack ) {
			bvhStack[++stackIndex] = (int) node.bbMax.w - 1;
		}
		if( rightThenLeft && addLeftToStack ) {
			bvhStack[++stackIndex] = leftChildIndex - 1;
		}

		if( !rightThenLeft && addLeftToStack ) {
			bvhStack[++stackIndex] = leftChildIndex - 1;
		}
		if( !rightThenLeft && addRightToStack ) {
			bvhStack[++stackIndex] = (int) node.bbMax.w - 1;
		}
	}
}
