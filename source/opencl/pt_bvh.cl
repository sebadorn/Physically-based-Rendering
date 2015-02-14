// Traversal for the acceleration structure.
// Type: Bounding Volume Hierarchy (BVH)

#define CALL_TRAVERSE         traverse( bvh, &ray, faces );
#define CALL_TRAVERSE_SHADOWS traverse_shadows( bvh, &lightRay, faces );


/**
 * Test faces of the given node for intersections with the given ray.
 * @param {ray4*}                ray
 * @param {const bvhNode*}       node
 * @param {const global face_t*} faces
 * @param {const float tNear}    tNear
 * @param {float tFar}           tFar
 */
void intersectFaces(
	ray4* ray, const bvhNode* node, const global face_t* faces,
	const float tNear, float tFar
) {
	float3 tuv;
	float4 normal = checkFaceIntersection( ray, faces[node->faces.x], &tuv, tNear, tFar );

	if( tuv.x < INFINITY ) {
		tFar = tuv.x;

		if( ray->t > tuv.x ) {
			ray->normal = normal;
			ray->normal.w = (float) node->faces.x;
			ray->t = tuv.x;
		}
	}

	if( node->faces.y == -1 ) {
		return;
	}

	normal = checkFaceIntersection( ray, faces[node->faces.y], &tuv, tNear, tFar );

	if( tuv.x < INFINITY ) {
		tFar = tuv.x;

		if( ray->t > tuv.x ) {
			ray->normal = normal;
			ray->normal.w = (float) node->faces.y;
			ray->t = tuv.x;
		}
	}
}


/**
 * Traverse the BVH and test the faces against the given ray.
 * @param {global const bvhNode*} bvh
 * @param {global const int4*}    bvhFaces
 * @param {ray4*}                 ray
 * @param {global const face_t*}  faces
 */
void traverse(
	global const bvhNode* bvh,
	ray4* ray, global const face_t* faces
) {
	uint bvhStack[BVH_STACKSIZE];
	int stackIndex = 0;
	bvhStack[stackIndex] = 0; // Node 0 is always the BVH root node

	const float3 invDir = native_recip( ray->dir.xyz );

	while( stackIndex >= 0 ) {
		bvhNode node = bvh[bvhStack[stackIndex--]];
		float tNearL = 0.0f;
		float tFarL = INFINITY;

		// Is a leaf node
		if( node.bbMin.w < 0.0f ) {
			if(
				intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) &&
				ray->t > tNearL
			) {
				intersectFaces( ray, &node, faces, tNearL, tFarL );
			}

			continue;
		}


		// Add child nodes to stack, if hit by ray

		bvhNode childNode = bvh[(int) node.bbMin.w];

		bool addLeftToStack = (
			intersectBox( ray, &invDir, childNode.bbMin, childNode.bbMax, &tNearL, &tFarL ) &&
			ray->t > tNearL
		);

		float tNearR = 0.0f;
		float tFarR = INFINITY;
		childNode = bvh[(int) node.bbMax.w];

		bool addRightToStack = (
			intersectBox( ray, &invDir, childNode.bbMin, childNode.bbMax, &tNearR, &tFarR ) &&
			ray->t > tNearR
		);


		// The node that is pushed on the stack first will be evaluated last.
		// So the nearer one should be pushed last, because it will be popped first then.
		const bool rightThenLeft = ( tNearR > tNearL );

		if( rightThenLeft && addRightToStack ) {
			bvhStack[++stackIndex] = (uint) node.bbMax.w;
		}
		if( rightThenLeft && addLeftToStack ) {
			bvhStack[++stackIndex] = (uint) node.bbMin.w;
		}

		if( !rightThenLeft && addLeftToStack ) {
			bvhStack[++stackIndex] = (uint) node.bbMin.w;
		}
		if( !rightThenLeft && addRightToStack ) {
			bvhStack[++stackIndex] = (uint) node.bbMax.w;
		}
	}
}


/**
 * Traverse the BVH and test the faces against the given ray.
 * This version is for the shadow ray test, so it only checks IF there
 * is an intersection and terminates on the first hit.
 * @param {const global bvhNode*} bvh
 * @param {ray4*}                 ray
 * @param {const global face_t*}  faces
 */
void traverse_shadows(
	const global bvhNode* bvh,
	ray4* ray, const global face_t* faces
) {
	bool addLeftToStack, addRightToStack, rightThenLeft;
	float tFarL, tFarR, tNearL, tNearR;

	uint bvhStack[BVH_STACKSIZE];
	int stackIndex = 0;
	bvhStack[stackIndex] = 0; // Node 0 is always the BVH root node

	const float3 invDir = native_recip( ray->dir.xyz );

	while( stackIndex >= 0 ) {
		bvhNode node = bvh[bvhStack[stackIndex--]];
		tNearL = 0.0f;
		tFarL = INFINITY;

		// Is a leaf node with faces
		if( node.bbMin.w < 0.0f && node.bbMax.w < 0.0f ) {
			if( intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) ) {
				intersectFaces( ray, &node, faces, tNearL, tFarL );

				if( ray->t < INFINITY ) {
					break;
				}
			}

			continue;
		}

		// Add child nodes to stack, if hit by ray

		bvhNode childNode = bvh[(int) node.bbMin.w];

		bool addLeftToStack = intersectBox( ray, &invDir, childNode.bbMin, childNode.bbMax, &tNearL, &tFarL );

		float tNearR = 0.0f;
		float tFarR = INFINITY;
		childNode = bvh[(int) node.bbMax.w];

		bool addRightToStack = intersectBox( ray, &invDir, childNode.bbMin, childNode.bbMax, &tNearR, &tFarR );


		// The node that is pushed on the stack first will be evaluated last.
		// So the nearer one should be pushed last, because it will be popped first then.
		rightThenLeft = ( tNearR > tNearL );

		if( rightThenLeft && addRightToStack) {
			bvhStack[++stackIndex] = (int) node.bbMax.w;
		}
		if( rightThenLeft && addLeftToStack) {
			bvhStack[++stackIndex] = (int) node.bbMin.w;
		}

		if( !rightThenLeft && addLeftToStack) {
			bvhStack[++stackIndex] = (int) node.bbMin.w;
		}
		if( !rightThenLeft && addRightToStack) {
			bvhStack[++stackIndex] = (int) node.bbMax.w;
		}
	}
}
