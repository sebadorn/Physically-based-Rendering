float4 groove3D( float4 scr, const float4 n ) {
	scr = (float4)( scr.x, 0.0f, scr.y, 0.0f );
	const float4 n2D = { 0.0f, 0.0f, 1.0f, 0.0f };

	const float4 r = cross( n2D, n );
	const float c = dot( n2D.xyz, n.xyz );
	const float s = native_sqrt( 1.0f - c * c );
	const float ci = 1.0f - c;

	float4 T;
	T.x = ( r.x * r.x * ci + c ) * scr.x +
	      ( r.x * r.y * ci - r.z * s ) * scr.y +
	      ( r.x * r.z * ci + r.y * s ) * scr.z;
	T.y = ( r.y * r.x * ci + r.z * s ) * scr.x +
	      ( r.y * r.y * ci + c ) * scr.y +
	      ( r.y * r.z * ci - r.x * s ) * scr.z;
	T.z = ( r.z * r.x * ci - r.y * s ) * scr.x +
	      ( r.z * r.y * ci + r.x * s ) * scr.y +
	      ( r.z * r.z * ci + c ) * scr.z;

	return fast_normalize( T );
}


/**
 * New direction for (perfectly) diffuse surfaces.
 * (Well, depening on the given parameters.)
 * @param  nl   Normal (unit vector).
 * @param  phi
 * @param  sina
 * @param  cosa
 * @return
 */
float4 jitter(
	const float4 nl, const float phi, const float sina, const float cosa
) {
	const float4 u = fast_normalize( cross( nl.yzxw, nl ) );
	const float4 v = fast_normalize( cross( nl, u ) );

	return ( u * native_cos( phi ) + v * native_sin( phi ) ) * sina + nl * cosa;
}


/**
 * MACRO: Reflect a ray.
 * @param  {float4} dir    Direction of ray.
 * @param  {float4} normal Normal of surface.
 * @return {float4}        Reflected ray.
 */
#define reflect( dir, normal ) ( ( dir ) - 2.0f * dot( ( normal ).xyz, ( dir ).xyz ) * ( normal ) )


/**
 * Get the a new direction for a ray hitting a transparent surface (glass etc.).
 * @param  {const ray4*}     currentRay The current ray.
 * @param  {const material*} mtl        Material of the hit surface.
 * @param  {float*}          seed       Seed for the random number generator.
 * @return {float4}                     A new direction for the ray.
 */
float4 refract( const ray4* currentRay, const material* mtl, float* seed ) {
	const bool into = ( dot( currentRay->normal.xyz, currentRay->dir.xyz ) < 0.0f );
	const float4 nl = into ? currentRay->normal : -currentRay->normal;

	const float m1 = into ? NI_AIR : mtl->Ni;
	const float m2 = into ? mtl->Ni : NI_AIR;
	const float m = native_divide( m1, m2 );

	const float cosI = -dot( currentRay->dir.xyz, nl.xyz );
	const float sinT2 = m * m * ( 1.0f - cosI * cosI );

	// Critical angle. Total internal reflection.
	if( sinT2 > 1.0f ) {
		return reflect( currentRay->dir, nl );
	}

	const float cosT = native_sqrt( 1.0f - sinT2 );
	const float4 tDir = m * currentRay->dir + ( m * cosI - cosT ) * nl;


	// Reflectance and transmission

	float r0 = native_divide( m1 - m2, m1 + m2 );
	r0 *= r0;
	const float c = ( m1 > m2 ) ? native_sqrt( 1.0f - sinT2 ) : cosI;
	const float reflectance = fresnel( c, r0 );
	// transmission = 1.0f - reflectance

	return ( reflectance < rand( seed ) ) ? tDir : reflect( currentRay->dir, nl );
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
	const ray4 currentRay, const material* mtl, float* seed, bool* ignoreColor, bool* addDepth
) {
	ray4 newRay;
	newRay.t = -2.0f;
	newRay.origin = fma( currentRay.t, currentRay.dir, currentRay.origin );

	#define DIR ( currentRay.dir )
	#define ISOTROPY ( mtl->scratch.w )
	#define NORMAL ( currentRay.normal )
	#define ROUGHNESS ( mtl->rough )


	// BRDF: Not much of any. Supports specular, diffuse, and glossy surfaces.
	#if BRDF == 0

		float rnd2 = rand( seed );
		newRay.dir = reflect( DIR, NORMAL ) * ( 1.0f - ROUGHNESS ) +
		             jitter( NORMAL, PI_X2 * rand( seed ), native_sqrt( rnd2 ), native_sqrt( 1.0f - rnd2 ) ) * ROUGHNESS;

	// BRDF: Schlick. Supports specular, diffuse, glossy, and anisotropic surfaces.
	// Downside: Really dark images.
	#elif BRDF == 1

		const float a = rand( seed );
		const float b = rand( seed );
		const float alpha = acos( native_sqrt( native_divide( a, ROUGHNESS - a * ROUGHNESS + a ) ) );
		const float phi = PI_X2 * native_sqrt( native_divide(
			ISOTROPY * ISOTROPY * b * b,
			1.0f - b * b + b * b * ISOTROPY * ISOTROPY
		) );

		// We could just use the jitter instead of NORMAL for perfect specular materials,
		// but this way we get a little better/smoother result for perfect mirrors.
		const float4 rn = ( ROUGHNESS == 0.0f )
		                ? NORMAL
		                : jitter( NORMAL, phi, native_sin( alpha ), native_cos( alpha ) );

		newRay.dir = reflect( DIR, rn );

	#endif


	// Transparency and refraction
	bool doTransRefr = ( mtl->d < 1.0f && mtl->d <= rand( seed ) );
	newRay.dir = doTransRefr ? refract( &currentRay, mtl, seed ) : newRay.dir;


	newRay.dir = fast_normalize( newRay.dir );

	*addDepth = ( ROUGHNESS < rand( seed ) || doTransRefr );
	*ignoreColor = doTransRefr;

	return newRay;

	#undef DIR
	#undef ISOTROPY
	#undef NORMAL
	#undef ROUGHNESS
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
	ray4* ray, const bvhNode* node, const global face_t* faces, float tNear, float tFar
) {
	float3 tuv;

	// At most 4 faces in a leaf node.
	// Unrolled the loop as optimization.


	// Face 1

	checkFaceIntersection( ray, faces[node->faces.x], &tuv, tNear, tFar );

	if( tuv.x > -1.0f ) {
		tFar = tuv.x;

		if( ray->t > tuv.x || ray->t < -1.0f ) {
			ray->normal = getTriangleNormal( faces[node->faces.x], tuv );
			ray->normal.w = (float) node->faces.x;
			ray->t = tuv.x;
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
			ray->normal = getTriangleNormal( faces[node->faces.y], tuv );
			ray->normal.w = (float) node->faces.y;
			ray->t = tuv.x;
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
			ray->normal = getTriangleNormal( faces[node->faces.z], tuv );
			ray->normal.w = (float) node->faces.z;
			ray->t = tuv.x;
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
			ray->normal = getTriangleNormal( faces[node->faces.w], tuv );
			ray->normal.w = (float) node->faces.w;
			ray->t = tuv.x;
		}
	}
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
void traverseBVH( const global bvhNode* bvh, ray4* ray, const global face_t* faces ) {
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
				intersectFaces( ray, &node, faces, tNearL, tFarL );
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
