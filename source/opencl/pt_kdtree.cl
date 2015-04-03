// Traversal for the acceleration structure.
// Type: kd-tree

#define CALL_TRAVERSE         traverse( &scene, &ray );
#define CALL_TRAVERSE_SHADOWS traverse_shadows( &scene, &lightRay );


/**
 * Check all faces of a leaf node for intersections with the given ray.
 */
void checkFaces(
	const Scene* scene, ray4* ray,
	const int faceIndex, const int numFaces,
	const float tNear, float tFar
) {
	for( uint i = faceIndex; i < faceIndex + numFaces; i++ ) {
		float3 tuv;
		const uint j = scene->kdFaces[i];

		float3 normal = checkFaceIntersection( scene, ray, j, &tuv, tNear, tFar );

		if( tuv.x < INFINITY ) {
			tFar = tuv.x;

			if( ray->t > tuv.x ) {
				ray->normal = normal;
				ray->hitFace = j;
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
int goToLeafNode( uint nodeIndex, const global kdNonLeaf* kdNonLeaves, float3 hitNear ) {
	const float* hitPos = (float*) &hitNear;
	bool isOnLeft;

	while( true ) {
		const short axis = kdNonLeaves[nodeIndex].axis;
		const int4 children = kdNonLeaves[nodeIndex].children;
		const float split = kdNonLeaves[nodeIndex].split;

		isOnLeft = ( hitPos[axis] <= split );
		nodeIndex = isOnLeft ? children.x : children.y;

		if( ( isOnLeft && children.z ) || ( !isOnLeft && children.w ) ) {
			return nodeIndex;
		}
	}

	return -1;
}


/**
* Source: http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-7-intersecting-simple-shapes/ray-box-intersection/
* Which is based on: "An Efficient and Robust Ray–Box Intersection Algorithm", Williams et al.
* @param {const float4*} origin
* @param {const float4*} dir
* @param {const float*}  bbMin
* @param {const float*}  bbMax
* @param {float*}        tFar
* @param {int*}          exitRope
*/
void getEntryDistanceAndExitRope(
	const ray4* ray, const float3* invDir, const float4 bbMin, const float4 bbMax,
	float* tFar, int* exitRope
) {
	const bool signX = signbit( invDir->x );
	const bool signY = signbit( invDir->y );
	const bool signZ = signbit( invDir->z );

	const float3 t1 = ( bbMin.xyz - ray->origin ) * (*invDir);
	float3 tMax = ( bbMax.xyz - ray->origin ) * (*invDir);
	tMax = fmax( t1, tMax );

	*tFar = fmin( fmin( tMax.x, tMax.y ), tMax.z );
	*exitRope = ( *tFar == tMax.y ) ? 3 - signY : 1 - signX;
	*exitRope = ( *tFar == tMax.z ) ? 5 - signZ : *exitRope;
}


/**
 * Find the closest hit of the ray with a surface.
 * Uses stackless kd-tree traversal.
 * @param {const Scene*} scene
 * @param {ray4*}        ray
 */
void traverse( const Scene* scene, ray4* ray ) {
	float tNear = 0.0f;
	float tFar = INFINITY;
	int exitRope;

	const float3 invDir = native_recip( ray->dir );

	if( !intersectBox( ray, &invDir, scene->kdRootNode.bbMin, scene->kdRootNode.bbMax, &tNear, &tFar ) ) {
		return;
	}

	int nodeIndex = goToLeafNode( 0, scene->kdNonLeaves, fma( ray->dir, tNear, ray->origin ) );

	while( nodeIndex >= 0 && tNear <= tFar ) {
		const kdLeaf currentNode = scene->kdLeaves[nodeIndex];
		const int8 ropes = currentNode.ropes;

		checkFaces( scene, ray, ropes.s6, ropes.s7, tNear, tFar );

		// Exit leaf node
		getEntryDistanceAndExitRope( ray, &invDir, currentNode.bbMin, currentNode.bbMax, &tNear, &exitRope );

		if( tNear > tFar ) {
			break;
		}

		nodeIndex = ( (int*) &ropes )[exitRope];
		nodeIndex = ( nodeIndex < 1 )
		          ? -( nodeIndex + 1 )
		          : goToLeafNode( nodeIndex - 1, scene->kdNonLeaves, fma( ray->dir, tNear, ray->origin ) );
	}
}
