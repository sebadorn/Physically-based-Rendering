/**
 *
 * @param  mtl
 * @param  seed
 * @return
 */
inline bool extendDepth( material* mtl, float* seed ) {
	#if BRDF == 2
		return ( fmax( mtl->nu, mtl->nv ) >= 50.0f );
	#else
		return ( mtl->rough < rand( seed ) );
	#endif
}


/**
 * Calculate the new ray depending on the current one and the hit surface.
 * @param  {const ray4}     currentRay  The current ray
 * @param  {const material} mtl         Material of the hit surface.
 * @param  {float*}         seed        Seed for the random number generator.
 * @param  {bool*}          addDepth    Flag.
 * @return {ray4}                       The new ray.
 */
ray4 getNewRay(
	const ray4* ray, const material* mtl, float* seed, bool* addDepth
) {
	ray4 newRay;
	newRay.t = -2.0f;
	newRay.origin = fma( ray->t, ray->dir, ray->origin );

	// Transparency and refraction
	bool doTransRefr = ( mtl->d < 1.0f && mtl->d <= rand( seed ) );

	*addDepth = ( *addDepth || doTransRefr );

	if( doTransRefr ) {
		newRay.dir = refract( ray, mtl, seed );

		return newRay;
	}

	// BRDF: Not much of any.
	// Supports specular, diffuse, and glossy surfaces.
	#if BRDF == 0

		newRay.dir = newRayNoBRDF( ray, mtl, seed );

	// BRDF: Schlick.
	// Supports specular, diffuse, glossy, and anisotropic surfaces.
	#elif BRDF == 1

		newRay.dir = newRaySchlick( ray, mtl, seed );

	// BRDF: Shirley-Ashikhmin.
	// Supports specular, diffuse, glossy, and anisotropic surfaces.
	#elif BRDF == 2

		newRay.dir = newRayShirleyAshikhmin( ray, mtl, seed );

	#endif

	newRay.origin += ray->normal * EPSILON;

	return newRay;
}


/**
 * Face intersection test after Möller and Trumbore.
 * @param  {const ray4*}   ray
 * @param  {const face_t}  face
 * @param  {float4*}       tuv
 * @param  {const float}   tNear
 * @param  {const float}   tFar
 */
void checkFaceIntersection(
	const ray4* ray, const face_t face, float3* tuv, const float tNear, const float tFar
) {
	const float3 edge1 = face.b.xyz - face.a.xyz;
	const float3 edge2 = face.c.xyz - face.a.xyz;
	const float3 tVec = ray->origin.xyz - face.a.xyz;
	const float3 pVec = cross( ray->dir.xyz, edge2 );
	const float3 qVec = cross( tVec, edge1 );
	const float invDet = native_recip( dot( edge1, pVec ) );

	tuv->x = dot( edge2, qVec ) * invDet;

	if( tuv->x < EPSILON || fmax( tuv->x - tFar, tNear - tuv->x ) > EPSILON ) {
		tuv->x = -2.0f;
		return;
	}

	tuv->y = dot( tVec, pVec ) * invDet;
	tuv->z = dot( ray->dir.xyz, qVec ) * invDet;
	tuv->x = ( fmin( tuv->y, tuv->z ) < 0.0f || tuv->y > 1.0f || tuv->y + tuv->z > 1.0f ) ? -2.0f : tuv->x;
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
	ray4* ray, const bvhNode* node, const global face_t* faces, float tNear, float tFar,
	constant const material* materials
) {
	#define IS_SAME_SIDE ( dot( normal.xyz, -ray->dir.xyz ) >= 0.0f )
	#define IS_TRANSPARENT ( materials[(uint) f.a.w].d < 1.0f )

	float3 tuv;
	float tFar_reset = tFar;

	// At most 4 faces in a leaf node.
	// Unrolled the loop as optimization.


	// Face 1

	checkFaceIntersection( ray, faces[node->faces.x], &tuv, tNear, tFar );

	if( tuv.x > -1.0f ) {
		tFar = tuv.x;

		if( ray->t > tuv.x || ray->t < -1.0f ) {
			face_t f = faces[node->faces.x];
			float4 normal = getTriangleNormal( f, tuv );

			if( IS_SAME_SIDE || IS_TRANSPARENT ) {
				ray->normal = normal;
				ray->normal.w = (float) node->faces.x;
				ray->t = tuv.x;
				tFar_reset = tFar;
			}
			else {
				tFar = tFar_reset;
			}
		}
	}


	// Face 2

	if( node->faces.y == -1 ) {
		return;
	}

	checkFaceIntersection( ray, faces[node->faces.y], &tuv, tNear, tFar );

	if( tuv.x > -1.0f ) {
		tFar = tuv.x;

		if( ray->t > tuv.x || ray->t < -1.0f ) {
			face_t f = faces[node->faces.y];
			float4 normal = getTriangleNormal( f, tuv );

			if( IS_SAME_SIDE || IS_TRANSPARENT ) {
				ray->normal = normal;
				ray->normal.w = (float) node->faces.y;
				ray->t = tuv.x;
				tFar_reset = tFar;
			}
			else {
				tFar = tFar_reset;
			}
		}
	}


	// Face 3

	if( node->faces.z == -1 ) {
		return;
	}

	checkFaceIntersection( ray, faces[node->faces.z], &tuv, tNear, tFar );

	if( tuv.x > -1.0f ) {
		tFar = tuv.x;

		if( ray->t > tuv.x || ray->t < -1.0f ) {
			face_t f = faces[node->faces.z];
			float4 normal = getTriangleNormal( f, tuv );

			if( IS_SAME_SIDE || IS_TRANSPARENT ) {
				ray->normal = normal;
				ray->normal.w = (float) node->faces.z;
				ray->t = tuv.x;
				tFar_reset = tFar;
			}
			else {
				tFar = tFar_reset;
			}
		}
	}


	// Face 4

	if( node->faces.w == -1 ) {
		return;
	}

	checkFaceIntersection( ray, faces[node->faces.w], &tuv, tNear, tFar );

	if( tuv.x > -1.0f ) {
		tFar = tuv.x;

		if( ray->t > tuv.x || ray->t < -1.0f ) {
			face_t f = faces[node->faces.w];
			float4 normal = getTriangleNormal( f, tuv );

			if( IS_SAME_SIDE || IS_TRANSPARENT ) {
				ray->normal = normal;
				ray->normal.w = (float) node->faces.w;
				ray->t = tuv.x;
				tFar_reset = tFar;
			}
			else {
				tFar = tFar_reset;
			}
		}
	}

	#undef IS_SAME_SIDE
	#undef IS_TRANSPARENT
}


/**
 * Based on: "An Efficient and Robust Ray–Box Intersection Algorithm", Williams et al.
 * @param  {const ray4*}   ray
 * @param  {const float*}  bbMin
 * @param  {const float*}  bbMax
 * @param  {float*}        tNear
 * @param  {float*}        tFar
 * @return {const bool}          True, if ray intersects box, false otherwise.
 */
const bool intersectBox(
	const ray4* ray, const float3 invDir, const float4 bbMin, const float4 bbMax, float* tNear, float* tFar
) {
	const float3 t1 = ( bbMin.xyz - ray->origin.xyz ) * invDir;
	float3 tMax = ( bbMax.xyz - ray->origin.xyz ) * invDir;
	const float3 tMin = fmin( t1, tMax );
	tMax = fmax( t1, tMax );

	*tNear = fmax( fmax( tMin.x, tMin.y ), tMin.z );
	*tFar = fmin( fmin( tMax.x, tMax.y ), fmin( tMax.z, *tFar ) );

	return ( *tNear <= *tFar );
}


/**
 * Calculate intersection of a ray with a sphere.
 * @param  {const ray4*}  ray          The ray.
 * @param  {const float4} sphereCenter Center of the sphere.
 * @param  {const float}  radius       Radius of the sphere.
 * @param  {float*}       tNear        <t> near of the intersection (ray entering the sphere).
 * @param  {float*}       tFar         <t> far of the intersection (ray leaving the sphere).
 * @return {const bool}                True, if ray intersects sphere, false otherwise.
 */
const bool intersectSphere(
	const ray4* ray, const float4 sphereCenter, const float radius, float* tNear, float* tFar
) {
	const float3 op = sphereCenter.xyz - ray->origin.xyz;
	const float b = dot( op, ray->dir.xyz );
	float det = b * b - dot( op, op ) + radius * radius;

	if( det < 0.0f ) {
		return false;
	}

	det = native_sqrt( det );
	*tNear = b - det;
	*tFar = b + det;

	return ( fmax( *tNear, *tFar ) > EPSILON );
}


/**
 * Traverse the BVH and test the kD-trees against the given ray.
 * @param {const global bvhNode*} bvh
 * @param {ray4*}                 ray
 * @param {const global face_t*}  faces
 */
void traverseBVH(
	const global bvhNode* bvh, ray4* ray, const global face_t* faces,
	constant const material* materials
) {
	bool addLeftToStack, addRightToStack, rightThenLeft;
	float tFarL, tFarR, tNearL, tNearR;

	uint bvhStack[BVH_STACKSIZE];
	int stackIndex = 0;
	bvhStack[stackIndex] = 0; // Node 0 is always the BVH root node

	ray4 rayBest = *ray;
	const float3 invDir = native_recip( ray->dir.xyz );

	rayBest.t = FLT_MAX;

	while( stackIndex >= 0 ) {
		bvhNode node = bvh[bvhStack[stackIndex--]];
		tNearL = -2.0f;
		tFarL = FLT_MAX;

		// Is a leaf node with faces
		if( node.faces.x > -1 ) {
			if(
				intersectBox( ray, invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) &&
				rayBest.t > tNearL
			) {
				intersectFaces( ray, &node, faces, tNearL, tFarL, materials );
				rayBest = ( ray->t > -1.0f && ray->t < rayBest.t ) ? *ray : rayBest;
			}

			continue;
		}

		tNearR = -2.0f;
		tFarR = FLT_MAX;
		addLeftToStack = false;
		addRightToStack = false;

		#define BVH_LEFTCHILD bvh[(int) node.bbMin.w]
		#define BVH_RIGHTCHILD bvh[(int) node.bbMax.w]

		// Add child nodes to stack, if hit by ray
		if(
			node.bbMin.w > 0.0f &&
			intersectBox( ray, invDir, BVH_LEFTCHILD.bbMin, BVH_LEFTCHILD.bbMax, &tNearL, &tFarL ) &&
			rayBest.t > tNearL
		) {
			addLeftToStack = true;
		}

		if(
			node.bbMax.w > 0.0f &&
			intersectBox( ray, invDir, BVH_RIGHTCHILD.bbMin, BVH_RIGHTCHILD.bbMax, &tNearR, &tFarR ) &&
			rayBest.t > tNearR
		) {
			addRightToStack = true;
		}

		#undef BVH_LEFTCHILD
		#undef BVH_RIGHTCHILD


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

	*ray = ( rayBest.t != FLT_MAX ) ? rayBest : *ray;
}
