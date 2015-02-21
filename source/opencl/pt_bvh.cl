// Traversal for the acceleration structure.
// Type: Bounding Volume Hierarchy (BVH)

#define CALL_TRAVERSE         traverse( bvhNodes, bvhLeaves, &ray, faces );
#define CALL_TRAVERSE_SHADOWS traverse_shadows( bvhNodes, bvhLeaves, &lightRay, faces );


/**
 * Test faces of the given node for intersections with the given ray.
 * @param {ray4*}                ray
 * @param {const bvhNode*}       node
 * @param {const global face_t*} faces
 * @param {const float tNear}    tNear
 * @param {float tFar}           tFar
 */
void intersectFaces(
	ray4* ray, const int4 nodeFaces, const global face_t* faces,
	const float tNear, float tFar
) {
	float3 tuv;
	float4 normal = checkFaceIntersection( ray, faces[nodeFaces.x], &tuv, tNear, tFar );

	if( tuv.x < INFINITY ) {
		tFar = tuv.x;

		if( ray->t > tuv.x ) {
			ray->normal = normal;
			ray->normal.w = (float) nodeFaces.x;
			ray->t = tuv.x;
		}
	}

	if( nodeFaces.y == -1 ) {
		return;
	}

	normal = checkFaceIntersection( ray, faces[nodeFaces.y], &tuv, tNear, tFar );

	if( tuv.x < INFINITY ) {
		tFar = tuv.x;

		if( ray->t > tuv.x ) {
			ray->normal = normal;
			ray->normal.w = (float) nodeFaces.y;
			ray->t = tuv.x;
		}
	}

	if( nodeFaces.z == -1 ) {
		return;
	}

	normal = checkFaceIntersection( ray, faces[nodeFaces.z], &tuv, tNear, tFar );

	if( tuv.x < INFINITY ) {
		tFar = tuv.x;

		if( ray->t > tuv.x ) {
			ray->normal = normal;
			ray->normal.w = (float) nodeFaces.z;
			ray->t = tuv.x;
		}
	}

	if( nodeFaces.w == -1 ) {
		return;
	}

	normal = checkFaceIntersection( ray, faces[nodeFaces.w], &tuv, tNear, tFar );

	if( tuv.x < INFINITY ) {
		tFar = tuv.x;

		if( ray->t > tuv.x ) {
			ray->normal = normal;
			ray->normal.w = (float) nodeFaces.w;
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
	global const bvhNode* bvhNodes, global const bvhLeaf* bvhLeaves,
	ray4* ray, global const face_t* faces
) {
	int bvhStack[BVH_STACKSIZE];
	int stackIndex = 0;
	bvhStack[stackIndex] = 0; // Node 0 is always the BVH root node

	const float3 invDir = native_recip( ray->dir.xyz );

	while( stackIndex >= 0 ) {
		bvhNode node = bvhNodes[bvhStack[stackIndex--]];
		float tNear = 0.0f;
		float tFar = INFINITY;

		// Check if node is hit by ray
		if(
			!intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNear, &tFar ) ||
			ray->t <= tNear
		) {
			// Not hit, continue with next one on the stack.
			continue;
		}


		int leftChildIndex = (int) node.bbMin.w;

		// Left child node is a leaf
		if( leftChildIndex < 0 ) {
			int leafIndex = ( leftChildIndex * -1 ) - 1;
			bvhLeaf leaf = bvhLeaves[leafIndex];

			if(
				intersectBox( ray, &invDir, leaf.bbMin, leaf.bbMax, &tNear, &tFar ) &&
				ray->t > tNear
			) {
				intersectFaces( ray, leaf.faces, faces, tNear, tFar );
			}
		}
		// Left child is not a leaf
		else if( leftChildIndex > 0 ) {
			bvhStack[++stackIndex] = leftChildIndex - 1;
		}
		// Otherwise there is no left child node


		tNear = 0.0f;
		tFar = INFINITY;
		int rightChildIndex = (int) node.bbMax.w;

		// Right child node is a leaf
		if( rightChildIndex < 0 ) {
			int leafIndex = ( rightChildIndex * -1 ) - 1;
			bvhLeaf leaf = bvhLeaves[leafIndex];

			if(
				intersectBox( ray, &invDir, leaf.bbMin, leaf.bbMax, &tNear, &tFar ) &&
				ray->t > tNear
			) {
				intersectFaces( ray, leaf.faces, faces, tNear, tFar );
			}
		}
		// Right child node is not a leaf
		else if( rightChildIndex > 0 ) {
			bvhStack[++stackIndex] = rightChildIndex - 1;
		}
		// Otherwise there is no right child node
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
	const global bvhNode* bvhNodes, const global bvhLeaf* bvhLeaves,
	ray4* ray, const global face_t* faces
) {
	// bool addLeftToStack, addRightToStack, rightThenLeft;
	float tFarL, tFarR, tNearL, tNearR;

	int bvhStack[BVH_STACKSIZE];
	int stackIndex = 0;
	bvhStack[stackIndex] = 0; // Node 0 is always the BVH root node

	const float3 invDir = native_recip( ray->dir.xyz );

	while( stackIndex >= 0 ) {
		bvhNode node = bvhNodes[bvhStack[stackIndex--]];
		tNearL = 0.0f;
		tFarL = INFINITY;

		// Check if node is hit by ray
		if( !intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) ) {
			// Not hit, continue with next one on the stack.
			continue;
		}

		// Is left child node a leaf node?
		if( node.bbMin.w < 0.0f ) {
			int leafIndex = ( ( (int) node.bbMin.w ) * -1 ) - 1;
			bvhLeaf leaf = bvhLeaves[leafIndex];

			if( intersectBox( ray, &invDir, leaf.bbMin, leaf.bbMax, &tNearL, &tFarL ) ) {
				intersectFaces( ray, leaf.faces, faces, tNearL, tFarL );

				if( ray->t < INFINITY ) {
					break;
				}
			}
		}
		else {
			bvhStack[++stackIndex] = (int) node.bbMin.w - 1;
		}

		// Is right child node a leaf node?
		if( node.bbMax.w < 0.0f ) {
			int leafIndex = ( ( (int) node.bbMax.w ) * -1 ) - 1;
			bvhLeaf leaf = bvhLeaves[leafIndex];

			if( intersectBox( ray, &invDir, leaf.bbMin, leaf.bbMax, &tNearL, &tFarL ) ) {
				intersectFaces( ray, leaf.faces, faces, tNearL, tFarL );

				if( ray->t < INFINITY ) {
					break;
				}
			}
		}
		else {
			bvhStack[++stackIndex] = (int) node.bbMax.w - 1;
		}

		// // Is a leaf node with faces
		// if( node.bbMin.w < 0.0f && node.bbMax.w < 0.0f ) {
		// 	if( intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) ) {
		// 		intersectFaces( ray, node.faces, faces, tNearL, tFarL );

		// 		if( ray->t < INFINITY ) {
		// 			break;
		// 		}
		// 	}

		// 	continue;
		// }

		// // Add child nodes to stack, if hit by ray

		// bvhNode childNode = bvh[(int) node.bbMin.w];

		// bool addLeftToStack = intersectBox( ray, &invDir, childNode.bbMin, childNode.bbMax, &tNearL, &tFarL );

		// float tNearR = 0.0f;
		// float tFarR = INFINITY;
		// childNode = bvh[(int) node.bbMax.w];

		// bool addRightToStack = intersectBox( ray, &invDir, childNode.bbMin, childNode.bbMax, &tNearR, &tFarR );


		// // The node that is pushed on the stack first will be evaluated last.
		// // So the nearer one should be pushed last, because it will be popped first then.
		// rightThenLeft = ( tNearR > tNearL );

		// if( rightThenLeft && addRightToStack) {
		// 	bvhStack[++stackIndex] = (int) node.bbMax.w;
		// }
		// if( rightThenLeft && addLeftToStack) {
		// 	bvhStack[++stackIndex] = (int) node.bbMin.w;
		// }

		// if( !rightThenLeft && addLeftToStack) {
		// 	bvhStack[++stackIndex] = (int) node.bbMin.w;
		// }
		// if( !rightThenLeft && addRightToStack) {
		// 	bvhStack[++stackIndex] = (int) node.bbMax.w;
		// }
	}
}
