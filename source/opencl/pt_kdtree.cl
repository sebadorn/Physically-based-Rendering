/**
 * Face intersection test after Möller and Trumbore.
 * @param  {const float4*} origin
 * @param  {const float4*} dir
 * @param  {const float4*} a
 * @param  {const float4*} b
 * @param  {const float4*} c
 * @param  {const float}   tNear
 * @param  {const float}   tFar
 * @param  {hit_t*}        result
 * @return {float}
 */
inline float checkFaceIntersection(
	const ray4* ray, const float4 a, const float4 b, const float4 c, float2* uv,
	const float tNear, const float tFar
) {
	const float4 edge1 = b - a;
	const float4 edge2 = c - a;
	const float4 tVec = ray->origin - a;
	const float4 pVec = cross( ray->dir, edge2 );
	const float4 qVec = cross( tVec, edge1 );
	const float det = dot( edge1, pVec );
	const float invDet = native_recip( det );
	float t = dot( edge2, qVec ) * invDet;

	#define MT_FINAL_T_TEST fmax( t - tFar, tNear - t ) > EPSILON || t < EPSILON

	#ifdef BACKFACE_CULLING
		const float u = dot( tVec, pVec );
		const float v = dot( ray->dir, qVec );

		t = ( fmin( u, v ) < 0.0f || u > det || u + v > det || MT_FINAL_T_TEST ) ? -2.0f : t;
	#else
		const float u = dot( tVec, pVec ) * invDet;
		const float v = dot( ray->dir, qVec ) * invDet;

		t = ( fmin( u, v ) < 0.0f || u > 1.0f || u + v > 1.0f || MT_FINAL_T_TEST ) ? -2.0f : t;
	#endif

	uv->x = u;
	uv->y = v;

	return t;
}


/**
 * Check all faces of a leaf node for intersections with the given ray.
 */
void checkFaces(
	ray4* ray, const int faceIndex, const int numFaces,
	const global uint* kdFaces, const global face_t* faces,
	const float entryDistance, float* exitDistance, const float boxExitLimit
) {
	float r = -2.0f;
	face_t face;
	uint j;
	float2 uv;

	for( uint i = 0; i < numFaces; i++ ) {
		j = kdFaces[faceIndex + i];
		face = faces[j];

		r = checkFaceIntersection(
			ray, face.a, face.b, face.c, &uv,
			entryDistance, fmin( *exitDistance, boxExitLimit )
		);

		if( r > -1.0f ) {
			*exitDistance = r;

			if( ray->t > r || ray->t < 0.0f ) {
				ray->t = r;
				ray->faceIndex = j;
				ray->normal = getTriangleNormal( face, uv );
			}
		}
	}
}


/**
 *
 * @param  prevRay
 * @param  normal
 * @param  mtl
 * @param  seed
 * @param  addBounce
 * @return
 */
ray4 getNewRay( ray4 prevRay, material mtl, float* seed, bool* ignoreColor, bool* addBounce ) {
	ray4 newRay;
	newRay.t = -2.0f;
	newRay.origin = fma( prevRay.t, prevRay.dir, prevRay.origin ) + prevRay.normal * EPSILON;

	*addBounce = false;
	*ignoreColor = false;

	// Transparency and refraction
	if( mtl.d < 1.0f && mtl.d <= rand( seed ) ) {
		newRay.dir = reflect( prevRay.dir, prevRay.normal );

		float a = dot( prevRay.normal, prevRay.dir );
		float ddn = fabs( a );
		float nnt = ( a > 0.0f ) ? mtl.Ni / NI_AIR : NI_AIR / mtl.Ni;
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

		*addBounce = true;
		*ignoreColor = true;
	}
	// Specular
	else if( mtl.illum == 3 ) {
		newRay.dir = reflect( prevRay.dir, prevRay.normal );
		newRay.dir += ( mtl.gloss > 0.0f )
		            ? uniformlyRandomVector( seed ) * mtl.gloss
		            : (float4)( 0.0f );
		newRay.dir = fast_normalize( newRay.dir );

		*addBounce = true;
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
			ray, ropes.s6, ropes.s7, kdFaces,
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
	bool addLeftToStack, addRightToStack, leftIsLeaf, rightIsLeaf;
	uint leftKdRoot, rightKdRoot;
	float tFarL, tFarR, tNearL, tNearR;

	int stackIndex = 0;
	bvhNode bvhStack[BVH_STACKSIZE];
	bvhStack[stackIndex] = node;

	while( stackIndex >= 0 ) {
		node = bvhStack[stackIndex--];

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

		tNearR = -2.0f;
		tNearL = -2.0f;
		tFarR = FLT_MAX;
		tFarL = FLT_MAX;

		if( node.rightChild >= 0 ) {
			intersectBoundingBox( ray->origin, ray->dir, rightNode.bbMin, rightNode.bbMax, &tNearR, &tFarR );
		}
		if( node.leftChild >= 0 ) {
			intersectBoundingBox( ray->origin, ray->dir, leftNode.bbMin, leftNode.bbMax, &tNearL, &tFarL );
		}

		addRightToStack = node.rightChild >= 0 && !rightIsLeaf && ( ray->t < 0.0f || ray->t >= tNearR );
		addLeftToStack = node.leftChild >= 0 && !leftIsLeaf && ( ray->t < 0.0f || ray->t >= tNearL );

		// If the left node is closer to the ray origin, test it first.
		// By pushing the right node first on the stack and then the left one,
		// the left one will be popped first.
		if( tNearR > tNearL ) {
			if( addRightToStack ) {
				bvhStack[++stackIndex] = rightNode;
			}
			if( addLeftToStack ) {
				bvhStack[++stackIndex] = leftNode;
			}
		}
		else {
			if( addLeftToStack ) {
				bvhStack[++stackIndex] = leftNode;
			}
			if( addRightToStack ) {
				bvhStack[++stackIndex] = rightNode;
			}
		}

		rayCp1 = *ray;
		rayCp2 = *ray;

		if( node.rightChild >= 0 && rightIsLeaf ) {
			traverseKdTree( &rayCp2, kdNonLeaves, kdLeaves, kdFaces, faces, tNearR, tFarR, rightKdRoot );
		}
		if( node.leftChild >= 0 && leftIsLeaf ) {
			traverseKdTree( &rayCp1, kdNonLeaves, kdLeaves, kdFaces, faces, tNearL, tFarL, leftKdRoot );
		}

		if(
			( ray->t < 0.0f && rayCp2.t > -1.0f ) ||
			( rayCp2.t > -1.0f && ray->t > rayCp2.t )
		) {
			*ray = rayCp2;
		}

		if(
			( ray->t < 0.0f && rayCp1.t > -1.0f ) ||
			( rayCp1.t > -1.0f && ray->t > rayCp1.t )
		) {
			*ray = rayCp1;
		}
	}
}
