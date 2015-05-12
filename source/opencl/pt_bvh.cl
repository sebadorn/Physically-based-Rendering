// Traversal for the acceleration structure.
// Type: Bounding Volume Hierarchy (BVH)

#define CALL_TRAVERSE         traverse( &scene, bvh, &ray );
#define CALL_TRAVERSE_SHADOWS traverseShadows( &scene, &lightRay );


/**
 * Test face for intersections with the given ray and update the ray.
 * @param {const Scene*}      scene
 * @param {ray4*}             ray
 * @param {const int}         faceIndex
 * @param {float3*}           tuv
 * @param {const float tNear} tNear
 * @param {float tFar}        tFar
 */
void intersectFace( const Scene* scene, ray4* ray, const int faceIndex, float* t, const float tNear, float tFar ) {
	const float3 normal = checkFaceIntersection( scene, ray, faceIndex, t, tNear, tFar );

	if( ray->t > *t ) {
		ray->normal = normal;
		ray->hitFace = faceIndex;
		ray->t = *t;
	}

	scene->debugColor.x += 1.0f;
}


/**
 * Test faces of the given node for intersections with the given ray.
 * @param {const Scene*}      scene
 * @param {ray4*}             ray
 * @param {const bvhNode*}    node
 * @param {const float tNear} tNear
 * @param {float tFar}        tFar
 */
void intersectFaces( const Scene* scene, ray4* ray, const bvhNode* node, const float tNear, float tFar ) {
	float t = INFINITY;

	intersectFace( scene, ray, node->bbMin.w, &t, tNear, tFar );

	// Second face, if existing.
	if( node->bbMax.w == -1 ) {
		return;
	}

	intersectFace( scene, ray, node->bbMax.w, &t, tNear, tFar );
}


/**
 * Traverse the BVH without using a stack and test the faces against the given ray.
 * @param {const Scene*} scene
 * @param {ray4*}        ray
 */
void traverse( const Scene* scene, read_only image2d_t bvh, ray4* ray ) {
	const float3 invDir = native_recip( ray->dir );
	int index = 1; // Skip the root node (0) and start with the left child node.

	const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

	do {
		scene->debugColor.y += 1.0f;
		// const bvhNode node = scene->bvh[index];

		int2 pos;
		pos.x = ( index * 2 ) % BVH_TEX_DIM;
		pos.y = (int) floor( (float) ( index * 2 ) / BVH_TEX_DIM );

		bvhNode node;
		node.bbMin = read_imagef( bvh, sampler, pos );

		pos.x = ( index * 2 + 1 ) % BVH_TEX_DIM;
		pos.y = (int) floor( (float) ( index * 2 + 1 ) / BVH_TEX_DIM );

		node.bbMax = read_imagef( bvh, sampler, pos );

		int currentIndex = index;

		// To save memory, we interpret <node.bbMax.w> depending on the situation:
		// - For a leaf node <node.bbMax.w> is a face index.
		// - Otherwise it is the index of the next node to visit.
		// <node.bbMin.w> is used as face index, too. If it is -1.0f the node is NOT a leaf node.
		//
		// If a node has a left child, it will always be next in memory (index + 1).
		// Also, if a node is a leaf node, the next node to visit (a right sibling or
		// right child of a distinct parent) will also be next in memory (index + 1).
		index = ( node.bbMin.w == -1.0f ) ? (int) node.bbMax.w : currentIndex + 1;

		float tNear = 0.0f;
		float tFar = INFINITY;

		bool isNodeHit = (
			intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNear, &tFar ) &&
			tFar > EPSILON5 && ray->t > tNear
		);

		if( !isNodeHit ) {
			continue;
		}

		index = currentIndex + 1;

		// Node is leaf node. Test faces.
		if( node.bbMin.w != -1.0f ) {
			intersectFaces( scene, ray, &node, tNear, tFar );
		}
	} while( index > 0 && index < BVH_NUM_NODES );
}


/**
 * Traverse the BVH and test the faces against the given ray.
 * This version is for the shadow ray test, so it only checks IF there
 * is an intersection and terminates on the first hit.
 * @param {const Scene*} scene
 * @param {ray4*}        ray
 */
void traverseShadows( const Scene* scene, ray4* ray ) {
	const float3 invDir = native_recip( ray->dir );
	int index = 1;

	do {
		const bvhNode node;// = scene->bvh[index];

		int currentIndex = index;

		// @see traverse() for an explanation.
		index = ( node.bbMin.w == -1.0f ) ? (int) node.bbMax.w : currentIndex + 1;

		float tNear = 0.0f;
		float tFar = INFINITY;

		bool isNodeHit = (
			intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNear, &tFar ) &&
			tFar > EPSILON5
		);

		if( !isNodeHit ) {
			continue;
		}

		index = currentIndex + 1;

		// Node is leaf node. Test faces-
		if( node.bbMin.w != -1.0f ) {
			intersectFaces( scene, ray, &node, tNear, tFar );
			index = currentIndex + 1;

			// It's enough to know that something blocks the way. It doesn't matter what or where.
			// TODO: It *does* matter what and where, if the material has transparency.
			if( ray->t < INFINITY ) {
				break;
			}
		}
	} while( index > 0 && index < BVH_NUM_NODES );
}
