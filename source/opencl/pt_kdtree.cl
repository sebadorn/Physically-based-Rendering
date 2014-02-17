/**
 * Face intersection test after Möller and Trumbore.
 * @param  {const ray4*}   ray
 * @param  {const face_t}  face
 * @param  {float2*}       uv
 * @param  {const float}   tNear
 * @param  {const float}   tFar
 * @return {float}
 */
inline float checkFaceIntersection(
	const ray4* ray, const face_t face, float4* tuv, const float tNear, const float tFar
) {
	const float4 edge1 = face.b - face.a;
	const float4 edge2 = face.c - face.a;
	const float4 tVec = ray->origin - face.a;
	const float4 pVec = cross( ray->dir, edge2 );
	const float4 qVec = cross( tVec, edge1 );
	const float det = dot( edge1, pVec );
	const float invDet = native_recip( det );
	tuv->x = dot( edge2, qVec ) * invDet;

	#define MT_FINAL_T_TEST fmax( tuv->x - tFar, tNear - tuv->x ) > EPSILON || tuv->x < EPSILON

	#ifdef BACKFACE_CULLING
		const float u = dot( tVec, pVec );
		const float v = dot( ray->dir, qVec );

		tuv->x = ( fmin( u, v ) < 0.0f || u > det || u + v > det || MT_FINAL_T_TEST ) ? -2.0f : tuv->x;
	#else
		const float u = dot( tVec, pVec ) * invDet;
		const float v = dot( ray->dir, qVec ) * invDet;

		tuv->x = ( fmin( u, v ) < 0.0f || u > 1.0f || u + v > 1.0f || MT_FINAL_T_TEST ) ? -2.0f : tuv->x;
	#endif

	#undef MT_FINAL_T_TEST

	tuv->y = u;
	tuv->z = v;
}


/**
 * Check all faces of a leaf node for intersections with the given ray.
 */
void checkFaces(
	ray4* ray, const int faceIndex, const int numFaces,
	const global uint* kdFaces, const global face_t* faces,
	const float entryDistance, float* exitDistance
) {
	uint j;
	float4 tuv;
	tuv.x = -2.0f;

	for( uint i = faceIndex; i < faceIndex + numFaces; i++ ) {
		j = kdFaces[i];

		checkFaceIntersection( ray, faces[j], &tuv, entryDistance, *exitDistance );

		if( tuv.x > -1.0f ) {
			*exitDistance = tuv.x;

			if( ray->t > tuv.x || ray->t < 0.0f ) {
				ray->t = tuv.x;
				ray->normal = getTriangleNormal( faces[j], tuv );
				ray->normal.w = (float) j;
			}
		}
	}
}


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
	newRay.origin = fma( prevRay.t, prevRay.dir, prevRay.origin ) + prevRay.normal * EPSILON;

	*addDepth = false;
	*ignoreColor = false;

	// Transparency and refraction
	if( mtl.d < 1.0f && mtl.d <= rand( seed ) ) {
		newRay.dir = reflect( prevRay.dir, prevRay.normal );

		float a = dot( prevRay.normal, prevRay.dir );
		float ddn = fabs( a );
		float nnt = mix( NI_AIR / mtl.Ni, mtl.Ni / NI_AIR, (float) a > 0.0f );
		float cos2t = 1.0f - nnt * nnt * ( 1.0f - ddn * ddn );

		if( cos2t > 0.0f ) {
			float4 tdir = fast_normalize( newRay.dir * nnt + sign( a ) * prevRay.normal * ( ddn * nnt + sqrt( cos2t ) ) );
			float R0 = ( mtl.Ni - NI_AIR ) * ( mtl.Ni - NI_AIR ) / ( ( mtl.Ni + NI_AIR ) * ( mtl.Ni + NI_AIR ) );
			float c = 1.0f - mix( ddn, dot( tdir, prevRay.normal ), (float) a > 0.0f );
			float Re = R0 + ( 1.0f - R0 ) * pown( c, 5 );
			float P = 0.25f + 0.5f * Re;
			float RP = Re / P;
			float TP = ( 1.0f - Re ) / ( 1.0f - P );

			if( rand( seed ) >= P ) {
				newRay.dir = tdir;
			}
		}

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
	int nodeIndex = goToLeafNode( kdRoot, kdNonLeaves, fma( tNear, ray->dir, ray->origin ) );

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

	bool addLeftToStack, addRightToStack, rightThenLeft;
	float tFarL, tFarR, tNearL, tNearR;

	int leftChildIndex, rightChildIndex;
	int stackIndex = 0;
	bvhNode bvhStack[BVH_STACKSIZE];
	bvhStack[stackIndex] = node;

	ray4 rayBest = *ray;
	ray4 rayCp = *ray;


	while( stackIndex >= 0 ) {
		node = bvhStack[stackIndex--];

		leftChildIndex = (int) node.bbMin.w;
		rightChildIndex = (int) node.bbMax.w;

		tNearL = -2.0f;
		tFarL = FLT_MAX;
		tNearR = -2.0f;
		tFarR = FLT_MAX;

		// Is a leaf node and contains a kD-tree
		if( leftChildIndex < 0 ) {
			if(
				intersectBoundingBox( ray, node.bbMin, node.bbMax, &tNearL, &tFarL ) &&
				( rayBest.t < -1.0f || rayBest.t > tNearL )
			) {
				rayCp = *ray;
				traverseKdTree(
					&rayCp, kdNonLeaves, kdLeaves, kdFaces, faces, tNearL, tFarL, -( leftChildIndex + 1 )
				);
				rayBest = ( rayBest.t < 0.0f || ( rayCp.t > -1.0f && rayCp.t < rayBest.t ) ) ? rayCp : rayBest;
			}

			continue;
		}

		addLeftToStack = false;
		addRightToStack = false;

		// Not a leaf, add child nodes to stack, if hit by ray
		if( leftChildIndex > 0 ) {
			leftNode = bvh[leftChildIndex - 1];

			if(
				intersectBoundingBox( ray, leftNode.bbMin, leftNode.bbMax, &tNearL, &tFarL ) &&
				( rayBest.t < -1.0f || rayBest.t > tNearL )
			) {
				addLeftToStack = true;
			}
		}
		if( rightChildIndex > 0 ) {
			rightNode = bvh[rightChildIndex - 1];

			if(
				intersectBoundingBox( ray, rightNode.bbMin, rightNode.bbMax, &tNearR, &tFarR ) &&
				( rayBest.t < -1.0f || rayBest.t > tNearR )
			) {
				addRightToStack = true;
			}
		}

		// The node that is pushed on the stack first will be evaluated last.
		// So the nearer one should be pushed last, because it will be popped first then.
		rightThenLeft = ( tNearR > tNearL );

		if( rightThenLeft && addRightToStack) {
			bvhStack[++stackIndex] = rightNode;
		}
		if( rightThenLeft && addLeftToStack) {
			bvhStack[++stackIndex] = leftNode;
		}

		if( !rightThenLeft && addLeftToStack) {
			bvhStack[++stackIndex] = leftNode;
		}
		if( !rightThenLeft && addRightToStack) {
			bvhStack[++stackIndex] = rightNode;
		}
	}

	*ray = rayBest;
}
