// Traversal for the acceleration structure.
// Type: Bounding Volume Hierarchy (BVH)

#define CALL_TRAVERSE         traverseStackless( bvh, &ray, faces, vertices, normals );
#define CALL_TRAVERSE_SHADOWS traverse_shadows( bvh, &lightRay, faces, vertices, normals );


/**
 * Test faces of the given node for intersections with the given ray.
 * @param {ray4*}                ray
 * @param {const bvhNode*}       node
 * @param {const global face_t*} faces
 * @param {const float tNear}    tNear
 * @param {float tFar}           tFar
 */
void intersectFaces(
	ray4* ray, const bvhNode* node,
	const global face_t* faces,
	const global float4* vertices,
	const global float4* normals,
	const float tNear, float tFar
) {
	float3 tuv;

	// First face. Always exists in a leaf node.
	face_t face = faces[node->faces.x];

	float3 a = vertices[face.vertices.x].xyz;
	float3 b = vertices[face.vertices.y].xyz;
	float3 c = vertices[face.vertices.z].xyz;

	float3 normal = checkFaceIntersection( ray, a, b, c, face.normals, normals, &tuv, tNear, tFar );

	if( tuv.x < INFINITY ) {
		tFar = tuv.x;

		if( ray->t > tuv.x ) {
			ray->normal = normal;
			ray->hitFace = node->faces.x;
			ray->t = tuv.x;
		}
	}

	// Second face, if existing.
	if( node->faces.y == -1 ) {
		return;
	}

	face = faces[node->faces.y];

	a = vertices[face.vertices.x].xyz;
	b = vertices[face.vertices.y].xyz;
	c = vertices[face.vertices.z].xyz;

	normal = checkFaceIntersection( ray, a, b, c, face.normals, normals, &tuv, tNear, tFar );

	if( tuv.x < INFINITY ) {
		tFar = tuv.x;

		if( ray->t > tuv.x ) {
			ray->normal = normal;
			ray->hitFace = node->faces.y;
			ray->t = tuv.x;
		}
	}

	// Third face, if existing.
	if( node->faces.z == -1 ) {
		return;
	}

	face = faces[node->faces.z];

	a = vertices[face.vertices.x].xyz;
	b = vertices[face.vertices.y].xyz;
	c = vertices[face.vertices.z].xyz;

	normal = checkFaceIntersection( ray, a, b, c, face.normals, normals, &tuv, tNear, tFar );

	if( tuv.x < INFINITY ) {
		tFar = tuv.x;

		if( ray->t > tuv.x ) {
			ray->normal = normal;
			ray->hitFace = node->faces.z;
			ray->t = tuv.x;
		}
	}
}


/**
 * Traverse the BVH and test the faces against the given ray.
 * @param {global const bvhNode*} bvh
 * @param {ray4*}                 ray
 * @param {global const face_t*}  faces
 */
void traverse(
	global const bvhNode* bvh,
	ray4* ray,
	global const face_t* faces,
	const global float4* vertices,
	const global float4* normals
) {
	int bvhStack[BVH_STACKSIZE];
	int stackIndex = 0;
	bvhStack[stackIndex] = 0; // Node 0 is always the BVH root node

	const float3 invDir = native_recip( ray->dir );

	while( stackIndex >= 0 ) {
		bvhNode node = bvh[bvhStack[stackIndex--]];
		float tNearL = 0.0f;
		float tFarL = INFINITY;

		// Is a leaf node
		if( node.faces.x >= 0 ) {
			if(
				intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) &&
				ray->t > tNearL
			) {
				intersectFaces( ray, &node, faces, vertices, normals, tNearL, tFarL );
			}

			continue;
		}


		// The node that is pushed on the stack first will be evaluated last.
		// We want the left node to be tested first, so we push it last.

		int leftChildIndex = (int) node.bbMin.w;
		int rightChildIndex = (int) node.bbMax.w;

		float tNearR = 0.0f;
		float tFarR = INFINITY;

		// Right child node
		node = bvh[rightChildIndex];

		if(
			intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNearR, &tFarR ) &&
			ray->t > tNearR
		) {
			bvhStack[++stackIndex] = rightChildIndex;
		}

		// Left child node
		node = bvh[leftChildIndex];

		if(
			intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) &&
			ray->t > tNearL
		) {
			bvhStack[++stackIndex] = leftChildIndex;
		}
	}
}


/**
 * Traverse the BVH without using a stack and test the faces against the given ray.
 * @param {global const bvhNode*} bvh
 * @param {ray4*}                 ray
 * @param {global const face_t*}  faces
 */
void traverseStackless(
	global const bvhNode* bvh,
	ray4* ray,
	global const face_t* faces,
	global const float4* vertices,
	global const float4* normals
) {
	const float3 invDir = native_recip( ray->dir );
	int index = 0;

	do {
		const bvhNode node = bvh[index];

		float tNear = 0.0f;
		float tFar = INFINITY;

		bool isNodeHit = (
			intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNear, &tFar ) &&
			ray->t > tNear
		);

		// In case of no hit: Go right or up.
		index = node.faces.w;

		if( !isNodeHit ) {
			continue;
		}

		// Not a leaf node, progress further down to the left.
		if( node.faces.x == -1 ) {
			index = (int) node.bbMin.w;
		}
		// Node is leaf node.
		else {
			intersectFaces( ray, &node, faces, vertices, normals, tNear, tFar );
		}
	} while( index > 0 );
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
	ray4* ray,
	const global face_t* faces,
	const global float4* vertices,
	const global float4* normals
) {
	bool addLeftToStack, addRightToStack, rightThenLeft;
	float tFarL, tFarR, tNearL, tNearR;

	uint bvhStack[BVH_STACKSIZE];
	int stackIndex = 0;
	bvhStack[stackIndex] = 0; // Node 0 is always the BVH root node

	const float3 invDir = native_recip( ray->dir );

	while( stackIndex >= 0 ) {
		bvhNode node = bvh[bvhStack[stackIndex--]];
		tNearL = 0.0f;
		tFarL = INFINITY;

		// Is a leaf node with faces
		if( node.faces.x >= 0 ) {
			if( intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) ) {
				intersectFaces( ray, &node, faces, vertices, normals, tNearL, tFarL );

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
