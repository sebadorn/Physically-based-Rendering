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

	for( uint i = faceIndex; i < faceIndex + numFaces; i++ ) {
		j = kdFaces[i];

		checkFaceIntersection( ray, faces[j], &tuv, entryDistance, *exitDistance );

		if( tuv.x > -1.0f ) {
			*exitDistance = tuv.x;

			if( ray->t > tuv.x || ray->t < 0.0f ) {
				ray->normal = getTriangleNormal( faces[j], tuv );
				ray->normal.w = (float) j;
				ray->t = tuv.x;
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
		// float4 n = prevRay.normal;
		// float4 nl = ( dot( n.xyz, prevRay.dir.xyz ) < 0.0f ) ? n : -n;

		// newRay.dir = reflect( prevRay.dir, n );
		// bool into = dot( n, nl ) > 0.0f;

		// float nc = NI_AIR;
		// float nt = mtl.Ni;
		// float nnt = into ? nc / nt : nt / nc;
		// float ddn = dot( prevRay.dir.xyz, nl.xyz );

		// float cos2t = 1.0f - nnt * nnt * ( 1.0f - ddn * ddn );

		// // Reflection or refraction (otherwise it's just reflection)
		// if( cos2t >= 0.0f ) {
		// 	float4 tdir = fast_normalize(
		// 		prevRay.dir * nnt - n * ( into ? 1.0f : -1.0f ) * ( ddn * nnt + native_sqrt( cos2t ) )
		// 	);
		// 	float a = nt - nc;
		// 	float b = nt + nc;
		// 	float R0 = a * a / ( b * b );
		// 	float c = 1.0f - ( into ? -ddn : dot( tdir.xyz, n.xyz ) );
		// 	float Re = R0 + ( 1.0f - R0 ) * pown( c, 5 );
		// 	float Tr = 1.0f - Re;
		// 	float P = 0.25f + 0.5f * Re;
		// 	float RP = Re / P;
		// 	float TP = Tr / ( 1.0f - P );

		// 	if( rand( seed ) >= P ) {
		// 		newRay.dir = tdir;
		// 	}
		// }


		float4 n = prevRay.normal;
		float4 nl = dot( n.xyz, prevRay.dir.xyz ) < 0.0f ? -n : n;
		bool into = !( n.x != nl.x && n.y != nl.y && n.z != nl.z );

		float m1 = into ? NI_AIR : mtl.Ni;
		float m2 = into ? mtl.Ni : NI_AIR;
		float m = m1 / m2;

		float iThetaCos = -dot( prevRay.dir.xyz, nl.xyz );
		float cosISq = iThetaCos * iThetaCos;
		float sinSq = m * m * ( 1.0f - cosISq );

		float4 tDir = m * prevRay.dir + ( m * iThetaCos - native_sqrt( 1.0f - sinSq ) ) * nl;

		// Critical angle. Total internal reflection.
		if( sinSq > 1.0f ) {
			newRay.dir = reflect( prevRay.dir, n );
		}
		else {
			float r0 = native_divide( m1 - m2, m1 + m2 );
			r0 *= r0;
			// float ui = 1.0f - iThetaCos;
			// float ut = 1.0f - native_sqrt( 1.0f - sinSq );
			// float refl = r0;
			// refl += ( m1 <= m2 )
			//       ? ( 1.0f - r0 ) * ui * ui * ui * ui * ui
			//       : ( 1.0f - r0 ) * ut * ut * ut * ut * ut;
			float c = ( m1 > m2 ) ? native_sqrt( 1.0f - sinSq ) : iThetaCos;
			float x = 1.0f - c;
			float refl = r0 + ( 1.0f - r0 ) * x * x * x * x * x;
			float trans = 1.0f - refl;

			// if( refl < rand( seed ) ) {
				newRay.dir = -tDir;
			// }
			// else {
			// 	newRay.dir = reflect( prevRay.dir, n );
			// }
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
		          ? -( nodeIndex + 1 )
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
	const global bvhNode* bvh, ray4* ray, const global kdNonLeaf* kdNonLeaves,
	const global kdLeaf* kdLeaves, const global uint* kdFaces, const global face_t* faces
) {
	bvhNode node;

	bool addLeftToStack, addRightToStack, rightThenLeft;
	float tFarL, tFarR, tNearL, tNearR;

	int leftChildIndex, rightChildIndex;
	int stackIndex = 0;
	int bvhStack[BVH_STACKSIZE];
	bvhStack[stackIndex] = 0; // Node 0 is always the BVH root node

	ray4 rayBest = *ray;
	ray4 rayCp = *ray;


	while( stackIndex >= 0 ) {
		node = bvh[bvhStack[stackIndex--]];

		leftChildIndex = (int) node.bbMin.w;
		rightChildIndex = (int) node.bbMax.w;

		tNearL = -2.0f;
		tFarL = FLT_MAX;

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

		tNearR = -2.0f;
		tFarR = FLT_MAX;
		addLeftToStack = false;
		addRightToStack = false;

		// Not a leaf, add child nodes to stack, if hit by ray
		if( leftChildIndex > 0 ) {
			node = bvh[leftChildIndex - 1];

			if(
				intersectBoundingBox( ray, node.bbMin, node.bbMax, &tNearL, &tFarL ) &&
				( rayBest.t < -1.0f || rayBest.t > tNearL )
			) {
				addLeftToStack = true;
			}
		}
		if( rightChildIndex > 0 ) {
			node = bvh[rightChildIndex - 1];

			if(
				intersectBoundingBox( ray, node.bbMin, node.bbMax, &tNearR, &tFarR ) &&
				( rayBest.t < -1.0f || rayBest.t > tNearR )
			) {
				addRightToStack = true;
			}
		}

		// The node that is pushed on the stack first will be evaluated last.
		// So the nearer one should be pushed last, because it will be popped first then.
		rightThenLeft = ( tNearR > tNearL );

		if( rightThenLeft && addRightToStack) {
			bvhStack[++stackIndex] = rightChildIndex - 1;
		}
		if( rightThenLeft && addLeftToStack) {
			bvhStack[++stackIndex] = leftChildIndex - 1;
		}

		if( !rightThenLeft && addLeftToStack) {
			bvhStack[++stackIndex] = leftChildIndex - 1;
		}
		if( !rightThenLeft && addRightToStack) {
			bvhStack[++stackIndex] = rightChildIndex - 1;
		}
	}

	*ray = rayBest;
}
