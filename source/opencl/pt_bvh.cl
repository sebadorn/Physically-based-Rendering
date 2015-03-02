// Traversal for the acceleration structure.
// Type: Bounding Volume Hierarchy (BVH)

#define CALL_TRAVERSE         traverseStackless( bvh, &ray, faces, vertices, normals );
#define CALL_TRAVERSE_SHADOWS traverseShadowsStackless( bvh, &lightRay, faces, vertices, normals );


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
	face_t f;
	float3 tuv;
	float3 a, b, c;
	float3 normal;

	// First face. Always exists in a leaf node.
	f = faces[node->faces.x];
	a = vertices[f.vertices.x].xyz;
	b = vertices[f.vertices.y].xyz;
	c = vertices[f.vertices.z].xyz;

	normal = checkFaceIntersection( ray, a, b, c, f.normals, normals, &tuv, tNear, tFar );
	tFar = ( tuv.x < INFINITY ) ? tuv.x : tFar;

	if( ray->t > tuv.x ) {
		ray->normal = normal;
		ray->hitFace = node->faces.x;
		ray->t = tuv.x;
	}

	// Second face, if existing.
	if( node->faces.y == -1 ) {
		return;
	}

	f = faces[node->faces.y];
	a = vertices[f.vertices.x].xyz;
	b = vertices[f.vertices.y].xyz;
	c = vertices[f.vertices.z].xyz;

	normal = checkFaceIntersection( ray, a, b, c, f.normals, normals, &tuv, tNear, tFar );
	tFar = ( tuv.x < INFINITY ) ? tuv.x : tFar;

	if( ray->t > tuv.x ) {
		ray->normal = normal;
		ray->hitFace = node->faces.y;
		ray->t = tuv.x;
	}

	// Third face, if existing.
	if( node->faces.z == -1 ) {
		return;
	}

	f = faces[node->faces.z];
	a = vertices[f.vertices.x].xyz;
	b = vertices[f.vertices.y].xyz;
	c = vertices[f.vertices.z].xyz;

	normal = checkFaceIntersection( ray, a, b, c, f.normals, normals, &tuv, tNear, tFar );
	tFar = ( tuv.x < INFINITY ) ? tuv.x : tFar;

	if( ray->t > tuv.x ) {
		ray->normal = normal;
		ray->hitFace = node->faces.z;
		ray->t = tuv.x;
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
				tFarL > EPSILON5 && ray->t > tNearL
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
			tFarR > EPSILON5 && ray->t > tNearR
		) {
			bvhStack[++stackIndex] = rightChildIndex;
		}

		// Left child node
		node = bvh[leftChildIndex];

		if(
			intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) &&
			tFarL > EPSILON5 && ray->t > tNearL
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
			tFar > EPSILON5 && ray->t > tNear
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
void traverseShadows(
	const global bvhNode* bvh,
	ray4* ray,
	const global face_t* faces,
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
				tFarL > EPSILON5
			) {
				intersectFaces( ray, &node, faces, vertices, normals, tNearL, tFarL );

				// It's enough to know that something blocks the way. It doesn't matter what or where.
				// TODO: It *does* matter what and where, if the material has transparency.
				if( ray->t < INFINITY ) {
					break;
				}
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
			tFarR > EPSILON5
		) {
			bvhStack[++stackIndex] = rightChildIndex;
		}

		// Left child node
		node = bvh[leftChildIndex];

		if(
			intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) &&
			tFarL > EPSILON5
		) {
			bvhStack[++stackIndex] = leftChildIndex;
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
void traverseShadowsStackless(
	const global bvhNode* bvh,
	ray4* ray,
	const global face_t* faces,
	const global float4* vertices,
	const global float4* normals
) {
	const float3 invDir = native_recip( ray->dir );
	int index = 0;

	do {
		const bvhNode node = bvh[index];

		float tNear = 0.0f;
		float tFar = INFINITY;

		bool isNodeHit = (
			intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNear, &tFar ) &&
			tFar > EPSILON5
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

			// It's enough to know that something blocks the way. It doesn't matter what or where.
			// TODO: It *does* matter what and where, if the material has transparency.
			if( ray->t < INFINITY ) {
				break;
			}
		}
	} while( index > 0 );
}
