/**
 * Face intersection test after Möller and Trumbore.
 * @param  {const ray4*}   ray
 * @param  {const face_t}  face
 * @param  {float2*}       uv
 * @param  {const float}   tNear
 * @param  {const float}   tFar
 */
void checkFaceIntersection(
	const ray4* ray, const face_t face, float4* tuv, const float tNear, const float tFar
) {
	// const float4 edge1 = face.b - face.a;
	// const float4 edge2 = face.c - face.a;

	// const float4 tVec = ray->origin - face.a;
	// const float4 pVec = cross( ray->dir, edge2 );
	// const float4 qVec = cross( tVec, edge1 );

	// const float det = dot( edge1, pVec );
	// const float invDet = native_recip( det );

	// tuv->x = dot( edge2, qVec ) * invDet;

	// #define MT_FINAL_T_TEST fmax( tuv->x - tFar, tNear - tuv->x ) > EPSILON || tuv->x < EPSILON

	// #ifdef BACKFACE_CULLING
	// 	tuv->y = dot( tVec, pVec );
	// 	tuv->z = dot( ray->dir, qVec );
	// 	tuv->x = ( fmin( tuv->y, tuv->z ) < 0.0f || tuv->y > det || tuv->y + tuv->z > det || MT_FINAL_T_TEST ) ? -2.0f : tuv->x;
	// #else
	// 	tuv->y = dot( tVec, pVec ) * invDet;
	// 	tuv->z = dot( ray->dir, qVec ) * invDet;
	// 	tuv->x = ( fmin( tuv->y, tuv->z ) < 0.0f || tuv->y > 1.0f || tuv->y + tuv->z > 1.0f || MT_FINAL_T_TEST ) ? -2.0f : tuv->x;
	// #endif

	// #undef MT_FINAL_T_TEST


	// Alternate version:

	const float3 e1 = face.b.xyz - face.a.xyz;
	const float3 e2 = face.c.xyz - face.a.xyz;
	const float3 normal = fast_normalize( cross( e1, e2 ) );

	tuv->x = native_divide(
		-dot( normal, ( ray->origin.xyz - face.a.xyz ) ),
		dot( normal, ray->dir.xyz )
	);

	if( fmax( tuv->x - tFar, tNear - tuv->x ) > EPSILON || tuv->x < EPSILON ) {
		tuv->x = -2.0f;
		return;
	}

	const float uu = dot( e1, e1 );
	const float uv = dot( e1, e2 );
	const float vv = dot( e2, e2 );

	const float3 w = fma( ray->dir.xyz, tuv->x, ray->origin.xyz ) - face.a.xyz;
	const float wu = dot( w, e1 );
	const float wv = dot( w, e2 );
	const float inverseD = native_recip( uv * uv - uu * vv );

	tuv->y = ( uv * wv - vv * wu ) * inverseD;
	tuv->z = ( uv * wu - uu * wv ) * inverseD;
	tuv->x = ( fmin( tuv->y, tuv->z ) < 0.0f || tuv->y > 1.0f || tuv->y + tuv->z > 1.0f ) ? -2.0f : tuv->x;
}


// /**
//  * Check all faces of a leaf node for intersections with the given ray.
//  */
// void checkFaces(
// 	ray4* ray, const int faceIndex, const int numFaces,
// 	const global uint* kdFaces, const global face_t* faces,
// 	const float entryDistance, float* exitDistance
// ) {
// 	uint j;
// 	float4 tuv;

// 	for( uint i = faceIndex; i < faceIndex + numFaces; i++ ) {
// 		j = kdFaces[i];

// 		checkFaceIntersection( ray, faces[j], &tuv, entryDistance, *exitDistance );

// 		if( tuv.x > -1.0f ) {
// 			*exitDistance = tuv.x;

// 			if( ray->t > tuv.x || ray->t < 0.0f ) {
// 				ray->normal = getTriangleNormal( faces[j], tuv );
// 				ray->normal.w = (float) j;
// 				ray->t = tuv.x;
// 			}
// 		}
// 	}
// }


/**
 *
 * @param  prevRay
 * @param  mtl
 * @param  seed
 * @param  ignoreColor
 * @param  addDepth
 * @return
 */
ray4 getNewRay( ray4 prevRay, material mtl, float* seed, bool* ignoreColor, bool* addDepth ) {
	ray4 newRay;
	newRay.t = -2.0f;
	newRay.origin = fma( prevRay.t, prevRay.dir, prevRay.origin );

	*addDepth = false;
	*ignoreColor = false;

	// Transparency and refraction
	if( mtl.d < 1.0f && mtl.d <= rand( seed ) ) {
		newRay.dir = refract( &prevRay, &mtl, seed );

		*addDepth = true;
		*ignoreColor = true;
	}
	// Specular
	else if( mtl.illum == 3 ) {
		newRay.dir = reflect( prevRay.dir, prevRay.normal );
		newRay.dir += ( mtl.gloss > 0.0f )
		            ? uniformlyRandomVector( seed ) * mtl.gloss
		            : (float4)( 0.0f );
		newRay.dir = fast_normalize( newRay.dir );

		*addDepth = true;
		*ignoreColor = true;
	}
	// Diffuse
	else {
		// newRay.dir = cosineWeightedDirection( seed, prevRay.normal );
		float rnd1 = rand( seed );
		float rnd2 = rand( seed );
		newRay.dir = jitter(
			prevRay.normal, 2.0f * M_PI * rnd1,
			sqrt( rnd2 ), sqrt( 1.0f - rnd2 )
		);
	}

	return newRay;
}


// /**
//  * Traverse down the kd-tree to find a leaf node the given ray intersects.
//  * @param  {int}                     nodeIndex
//  * @param  {const global kdNonLeaf*} kdNonLeaves
//  * @param  {const float4}            hitNear
//  * @return {int}
//  */
// int goToLeafNode( uint nodeIndex, const global kdNonLeaf* kdNonLeaves, const float4 hitNear ) {
// 	float4 split;
// 	int4 children;

// 	int axis = kdNonLeaves[nodeIndex].split.w;
// 	float* hitPos = (float*) &hitNear;
// 	float* splitPos;
// 	bool isOnLeft;

// 	while( true ) {
// 		children = kdNonLeaves[nodeIndex].children;
// 		split = kdNonLeaves[nodeIndex].split;
// 		splitPos = (float*) &split;

// 		isOnLeft = ( hitPos[axis] < splitPos[axis] );
// 		nodeIndex = isOnLeft ? children.x : children.y;

// 		if( ( isOnLeft && children.z ) || ( !isOnLeft && children.w ) ) {
// 			return nodeIndex;
// 		}

// 		axis = MOD_3[axis + 1];
// 	}

// 	return -1;
// }


// /**
//  * Find the closest hit of the ray with a surface.
//  * Uses stackless kd-tree traversal.
//  */
// void traverseKdTree(
// 	ray4* ray, const global kdNonLeaf* kdNonLeaves,
// 	const global kdLeaf* kdLeaves, const global uint* kdFaces, const global face_t* faces,
// 	float tNear, float tFar, uint kdRoot
// ) {
// 	kdLeaf currentNode;
// 	int8 ropes;
// 	int exitRope;
// 	int nodeIndex = goToLeafNode( kdRoot, kdNonLeaves, fma( tNear, ray->dir, ray->origin ) );

// 	while( nodeIndex >= 0 && tNear < tFar ) {
// 		currentNode = kdLeaves[nodeIndex];
// 		ropes = currentNode.ropes;

// 		checkFaces( ray, ropes.s6, ropes.s7, kdFaces, faces, tNear, &tFar );

// 		// Exit leaf node
// 		updateEntryDistanceAndExitRope(
// 			ray, currentNode.bbMin, currentNode.bbMax, &tNear, &exitRope
// 		);

// 		if( tNear >= tFar ) {
// 			break;
// 		}

// 		nodeIndex = ( (int*) &ropes )[exitRope];
// 		nodeIndex = ( nodeIndex < 1 )
// 		          ? -( nodeIndex + 1 )
// 		          : goToLeafNode( nodeIndex - 1, kdNonLeaves, fma( tNear, ray->dir, ray->origin ) );
// 	}
// }



void intersectFaces( ray4* ray, const sphereNode* node, const global face_t* faces, float tNear, float tFar ) {
	int testFaces[4];
	testFaces[0] = node->faces.x;
	testFaces[1] = node->faces.y;
	testFaces[2] = node->faces.z;
	testFaces[3] = node->faces.w;

	face_t theFace;
	float4 tuv;

	for( uint i = 0; i < 4; i++ ) {
		if( testFaces[i] == -1 ) {
			break;
		}

		theFace = faces[testFaces[i]];
		checkFaceIntersection( ray, theFace, &tuv, tNear, tFar );

		if( tuv.x > -1.0f ) {
			tFar = tuv.x;

			if( ray->t > tuv.x || ray->t < 0.0f ) {
				ray->normal = getTriangleNormal( theFace, tuv );
				ray->normal.w = (float) testFaces[i];
				ray->t = tuv.x;
			}
		}
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
void traverseBVH( const global sphereNode* bvh, ray4* ray, const global face_t* faces ) {
	sphereNode node, nodeLeft, nodeRight;

	bool addLeftToStack, addRightToStack, rightThenLeft;
	float tFarL, tFarR, tNearL, tNearR;

	uint bvhStack[BVH_STACKSIZE];
	int stackIndex = 0;
	bvhStack[stackIndex] = 0; // Node 0 is always the BVH root node

	ray4 rayBest = *ray;
	ray4 rayCp = *ray;

	while( stackIndex >= 0 ) {
		node = bvh[bvhStack[stackIndex--]];
		tNearL = -2.0f;
		tFarL = FLT_MAX;

		// Is a leaf node with faces
		if( node.faces.x > -1 ) {
			if(
				intersectBoundingBox( ray, node.bbMin, node.bbMax, &tNearL, &tFarL ) &&
				( rayBest.t < -1.0f || rayBest.t > tNearL )
			) {
				rayCp = *ray;
				intersectFaces( &rayCp, &node, faces, tNearL, tFarL );
				rayBest = ( rayBest.t < 0.0f || ( rayCp.t > -1.0f && rayCp.t < rayBest.t ) ) ? rayCp : rayBest;
			}

			continue;
		}

		tNearR = -2.0f;
		tFarR = FLT_MAX;
		addLeftToStack = false;
		addRightToStack = false;

		// Add child nodes to stack, if hit by ray
		if( node.leftChild > 0 ) {
			nodeLeft = bvh[node.leftChild];
			float4 center = ( nodeLeft.bbMin + nodeLeft.bbMax ) / 2.0f;
			float4 r = nodeLeft.bbMin + center;
			float radius = fast_length( r );

			if(
				// intersectBoundingBox( ray, nodeLeft.bbMin, nodeLeft.bbMax, &tNearL, &tFarL ) &&
				intersectSphere( ray, center, radius, &tNearL, &tFarL ) &&
				( rayBest.t < -1.0f || rayBest.t > tNearL )
			) {
				addLeftToStack = true;
			}
		}

		if( node.rightChild > 0 ) {
			nodeRight = bvh[node.rightChild];
			float4 center = ( nodeRight.bbMin + nodeRight.bbMax ) / 2.0f;
			float4 r = nodeRight.bbMin + center;
			float radius = fast_length( r );

			if(
				// intersectBoundingBox( ray, nodeRight.bbMin, nodeRight.bbMax, &tNearR, &tFarR ) &&
				intersectSphere( ray, center, radius, &tNearR, &tFarR ) &&
				( rayBest.t < -1.0f || rayBest.t > tNearR )
			) {
				addRightToStack = true;
			}
		}

		// The node that is pushed on the stack first will be evaluated last.
		// So the nearer one should be pushed last, because it will be popped first then.
		rightThenLeft = ( tNearR > tNearL );

		if( rightThenLeft && addRightToStack) {
			bvhStack[++stackIndex] = node.rightChild;
		}
		if( rightThenLeft && addLeftToStack) {
			bvhStack[++stackIndex] = node.leftChild;
		}

		if( !rightThenLeft && addLeftToStack) {
			bvhStack[++stackIndex] = node.leftChild;
		}
		if( !rightThenLeft && addRightToStack) {
			bvhStack[++stackIndex] = node.rightChild;
		}
	}

	*ray = rayBest;
}
