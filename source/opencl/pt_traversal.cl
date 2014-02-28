/**
 * Face intersection test after Möller and Trumbore.
 * @param  {const ray4*}   ray
 * @param  {const face_t}  face
 * @param  {float4*}       tuv
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
 * Calculate the new ray depending on the current one and the hit surface.
 * @param  {const ray4}     currentRay  The current ray
 * @param  {const material} mtl         Material of the hit surface.
 * @param  {float*}         seed        Seed for the random number generator.
 * @param  {bool*}          ignoreColor Flag.
 * @param  {bool*}          addDepth    Flag.
 * @return {ray4}                       The new ray.
 */
ray4 getNewRay(
	const ray4 currentRay, const material mtl, float* seed, bool* ignoreColor, bool* addDepth
) {
	ray4 newRay;
	newRay.t = -2.0f;
	newRay.origin = fma( currentRay.t, currentRay.dir, currentRay.origin );

	*addDepth = false;
	*ignoreColor = false;

	// Transparency and refraction
	if( mtl.d < 1.0f && mtl.d <= rand( seed ) ) {
		newRay.dir = refract( &currentRay, &mtl, seed );

		*addDepth = true;
		*ignoreColor = true;
	}
	// Specular
	else if( mtl.illum == 3 ) {
		newRay.dir = reflect( currentRay.dir, currentRay.normal );
		newRay.dir += ( mtl.gloss > 0.0f )
		            ? uniformlyRandomVector( seed ) * mtl.gloss
		            : (float4)( 0.0f );
		newRay.dir = fast_normalize( newRay.dir );

		*addDepth = true;
		*ignoreColor = true;
	}
	// Diffuse
	else {
		// newRay.dir = cosineWeightedDirection( seed, currentRay.normal );
		float rnd1 = rand( seed );
		float rnd2 = rand( seed );
		newRay.dir = jitter(
			currentRay.normal, 2.0f * M_PI * rnd1,
			sqrt( rnd2 ), sqrt( 1.0f - rnd2 )
		);
	}

	return newRay;
}


/**
 * Test faces of the given node for intersections with the given ray.
 * @param {ray4*}                ray
 * @param {const bvhNode*}       node
 * @param {const global face_t*} faces
 * @param {float tNear}          tNear
 * @param {float tFar}           tFar
 */
void intersectFaces(
	ray4* ray, const bvhNode* node, const global face_t* faces, float tNear, float tFar
) {
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
 * @param {const global bvhNode*} bvh
 * @param {ray4*}                 ray
 * @param {const global face_t*}  faces
 */
void traverseBVH( const global bvhNode* bvh, ray4* ray, const global face_t* faces ) {
	bvhNode node, nodeLeft, nodeRight;

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
				intersectBox( ray, node.bbMin, node.bbMax, &tNearL, &tFarL ) &&
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
		if( node.bbMin.w > 0.0f ) {
			nodeLeft = bvh[(int) node.bbMin.w];

			if(
				intersectBox( ray, nodeLeft.bbMin, nodeLeft.bbMax, &tNearL, &tFarL ) &&
				( rayBest.t < -1.0f || rayBest.t > tNearL )
			) {
				addLeftToStack = true;
			}
		}

		if( node.bbMax.w > 0.0f ) {
			nodeRight = bvh[(int) node.bbMax.w];

			if(
				intersectBox( ray, nodeRight.bbMin, nodeRight.bbMax, &tNearR, &tFarR ) &&
				( rayBest.t < -1.0f || rayBest.t > tNearR )
			) {
				addRightToStack = true;
			}
		}

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

	*ray = rayBest;
}
