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
	newRay.t = INFINITY;
	newRay.origin = fma( ray->t, ray->dir, ray->origin );
	// newRay.origin += ray->normal * EPSILON;

	// Transparency and refraction
	bool doTransRefr = ( mtl->d < 1.0f && mtl->d <= rand( seed ) );

	*addDepth = ( *addDepth || doTransRefr );

	if( doTransRefr ) {
		newRay.dir = refract( ray, mtl, seed );
	}
	else {
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
	}

	return newRay;
}


float3 phongTessellation(
	const float3 P1, const float3 P2, const float3 P3,
	const float3 N1, const float3 N2, const float3 N3,
	const float u, const float v, const float w
) {
	const float3 pBary = P1 * u + P2 * v + P3 * w;
	const float3 pTessellated =
			u * projectOnPlane( pBary, P1, N1 ) +
			v * projectOnPlane( pBary, P2, N2 ) +
			w * projectOnPlane( pBary, P3, N3 );

	return ( 1.0f - PhongTess_ALPHA ) * pBary + PhongTess_ALPHA * pTessellated;
}


inline char getBestRayDomain( float3 d ) {
	char domain = ( fabs( d.y ) > fabs( d.z ) ) ? 1 : 2;

	if( fabs( d.x ) > fabs( d.y ) ) {
		domain = ( fabs( d.x ) > fabs( d.z ) ) ? 0 : 2;
	}

	return domain;
}


float4 flatTriangleAndRayIntersection(
	const face_t* face, const ray4* ray, float3* tuv, const float tNear, const float tFar
) {
	const float3 edge1 = face->b.xyz - face->a.xyz;
	const float3 edge2 = face->c.xyz - face->a.xyz;
	const float3 tVec = ray->origin.xyz - face->a.xyz;
	const float3 pVec = cross( ray->dir.xyz, edge2 );
	const float3 qVec = cross( tVec, edge1 );
	const float invDet = native_recip( dot( edge1, pVec ) );

	tuv->x = dot( edge2, qVec ) * invDet;

	if( tuv->x < EPSILON || fmax( tuv->x - tFar, tNear - tuv->x ) > EPSILON ) {
		tuv->x = INFINITY;
		return (float4)( 0.0f );
	}

	tuv->y = dot( tVec, pVec ) * invDet;
	tuv->z = dot( ray->dir.xyz, qVec ) * invDet;
	tuv->x = ( fmin( tuv->y, tuv->z ) < 0.0f || tuv->y > 1.0f || tuv->y + tuv->z > 1.0f ) ? INFINITY : tuv->x;

	return getTriangleNormal( face, 1.0f - tuv->y - tuv->z, tuv->y, tuv->z );
}


/**
 * Face intersection test after Möller and Trumbore.
 * @param  {const ray4*}   ray
 * @param  {const face_t}  face
 * @param  {float4*}       tuv
 * @param  {const float}   tNear
 * @param  {const float}   tFar
 */
float4 checkFaceIntersection(
	const ray4* ray, const face_t face, float3* tuv, const float tNear, const float tFar
) {
	#define P1 ( face.a.xyz )
	#define P2 ( face.b.xyz )
	#define P3 ( face.c.xyz )
	#define N1 ( face.an.xyz )
	#define N2 ( face.bn.xyz )
	#define N3 ( face.cn.xyz )

	#if PhongTess == 1

		const int3 cmp = ( N1 == N2 ) + ( N2 == N3 );

		// Comparing vectors in OpenCL: 0/false/not equal; -1/true/equal
		if( cmp.x + cmp.y + cmp.z == -6 )

	#endif

	{
		return flatTriangleAndRayIntersection( &face, ray, tuv, tNear, tFar );
	}


	// Phong Tessellation
	// Based on: "Direct Ray Tracing of Phong Tessellation" by Shinji Ogaki, Yusuke Tokuyoshi
	#if PhongTess == 1

		float4 normal = (float4)( 0.0f );
		tuv->x = INFINITY;

		float3 E01 = P2 - P1;
		float3 E12 = P3 - P2;
		float3 E20 = P1 - P3;

		float3 C1 = PhongTess_ALPHA * ( dot( N2, E01 ) * N2 - dot( N1, E01 ) * N1 );
		float3 C2 = PhongTess_ALPHA * ( dot( N3, E12 ) * N3 - dot( N2, E12 ) * N2 );
		float3 C3 = PhongTess_ALPHA * ( dot( N1, E20 ) * N1 - dot( N3, E20 ) * N3 );


		// Planes of which the intersection is the ray
		// Hesse normal form

		float3 D1 = { 1.0f, 2.0f, 0.0f };
		D1.z = native_divide( -D1.x * ray->dir.x - D1.y * ray->dir.y, ray->dir.z );
		D1 = fast_normalize( D1 );

		float3 D2 = cross( D1, ray->dir.xyz );
		D2 = fast_normalize( D2 );

		#define O1 ( dot( D1, ray->origin.xyz ) )
		#define O2 ( dot( D2, ray->origin.xyz ) )

		const float a = dot( -D1, C3 );
		const float b = dot( -D1, C2 );
		const float c = dot( D1, P3 ) - O1;
		const float d = dot( D1, C1 - C2 - C3 ) * 0.5f;
		const float e = dot( D1, C3 + E20 ) * 0.5f;
		const float f = dot( D1, C2 - E12 ) * 0.5f;
		const float l = dot( -D2, C3 );
		const float m = dot( -D2, C2 );
		const float n = dot( D2, P3 ) - O2;
		const float o = dot( D2, C1 - C2 - C3 ) * 0.5f;
		const float p = dot( D2, C3 + E20 ) * 0.5f;
		const float q = dot( D2, C2 - E12 ) * 0.5f;

		#undef O1
		#undef O2


		// Solve cubic

		const float a3 = ( l*m*n + 2.0f*o*p*q ) - ( l*q*q + m*p*p + n*o*o );
		const float a2 = ( a*m*n + l*b*n + l*m*c + 2.0f*( d*p*q + o*e*q + o*p*f ) ) -
		                 ( a*q*q + b*p*p + c*o*o + 2.0f*( l*f*q + m*e*p + n*d*o ) );
		const float a1 = ( a*b*n + a*m*c + l*b*c + 2.0f*( o*e*f + d*e*q + d*p*f ) ) -
		                 ( l*f*f + m*e*e + n*d*d + 2.0f*( a*f*q + b*e*p + c*d*o ) );
		const float a0 = ( a*b*c + 2.0f*d*e*f ) - ( a*f*f + b*e*e + c*d*d );

		float xs[3] = { -1.0f, -1.0f, -1.0f };
		const char numCubicRoots = solveCubic( a0, a1, a2, a3, xs );

		if( 0 == numCubicRoots ) {
			return normal;
		}

		float x, t, u, v, w;
		float determinant = INFINITY;
		float mA, mB, mC, mD, mE, mF;

		for( char i = 0; i < numCubicRoots; i++ ) {
			mA = fma( a, xs[i], l );
			mB = fma( b, xs[i], m );
			mD = fma( d, xs[i], o );

			if( determinant > mD * mD - mA * mB ) {
				determinant = mD * mD - mA * mB;
				x = xs[i];
			}
		}

		if( 0.0f >= determinant ) {
			return normal;
		}


		const char domain = getBestRayDomain( ray->dir.xyz );

		mA = fma( a, x, l );
		mB = fma( b, x, m );
		mC = fma( c, x, n );
		mD = fma( d, x, o );
		mE = fma( e, x, p );
		mF = fma( f, x, q );

		#define IS_T_VALID ( t > EPSILON && t < tuv->x && t < ray->t )

		if( fabs( mA ) < fabs( mB ) ) {
			mA = native_divide( mA, mB );
			mC = native_divide( mC, mB );
			mD = native_divide( mD, mB );
			mE = native_divide( mE, mB );
			mF = native_divide( mF, mB );

			const float sqrtA = native_sqrt( mD * mD - mA );
			const float sqrtC = native_sqrt( mF * mF - mC );
			float la1 = mD + sqrtA;
			float la2 = mD - sqrtA;
			float lc1 = mF + sqrtC;
			float lc2 = mF - sqrtC;

			if( fabs( 2.0f * mE - la1 * lc1 - la2 * lc2 ) < fabs( 2.0f * mE - la1 * lc2 - la2 * lc1 ) ) {
				swap( &lc1, &lc2 );
			}

			for( int loop = 0; loop < 2; loop++ ) {
				const float g = ( 0 == loop ) ? -la1 : -la2;
				const float h = ( 0 == loop ) ? -lc1 : -lc2;

				// Solve quadratic function: c0*u*u + c1*u + c2 = 0
				const float c0 = a + g * ( 2.0f * d + b * g );
				const float c1 = 2.0f * ( h * ( d + b * g ) + e + f * g );
				const float c2 = h * ( b * h + 2.0f * f ) + c;
				const char numResults = solveCubic( 0.0f, c0, c1, c2, xs );

				for( char i = 0; i < numResults; i++ ) {
					u = xs[i];
					v = fma( g, u, h );
					w = 1.0f - u - v;

					if( 0.0f <= u && 0.0f <= v && 0.0f <= w ) {
						float3 pTessellated = phongTessellation( P1, P2, P3, N1, N2, N3, u, v, w ) - ray->origin.xyz;
						t = native_divide(
							( (float*) &pTessellated )[domain],
							( (float*) &(ray->dir) )[domain]
						);

						if( IS_T_VALID ) {
							tuv->x = t;
							tuv->y = u;
							tuv->z = v;

							const float3 ns = getTriangleNormalS( u, v, w, C1, C2, C3, E12, E20 );
							const float4 np = getTriangleNormal( &face, u, v, w );
							const float3 r = getTriangleReflectionVec( ray->dir.xyz, np.xyz );
							normal = ( dot( ns, r ) < 0.0f ) ? (float4)( ns, 0.0f ) : np;
						}
					}
				}
			}
		}
		else {
			mB = native_divide( mB, mA );
			mC = native_divide( mC, mA );
			mD = native_divide( mD, mA );
			mE = native_divide( mE, mA );
			mF = native_divide( mF, mA );

			const float sqrtB = native_sqrt( mD * mD - mB );
			const float sqrtC = native_sqrt( mE * mE - mC );
			float lb1 = mD + sqrtB;
			float lb2 = mD - sqrtB;
			float lc1 = mE + sqrtC;
			float lc2 = mE - sqrtC;

			if( fabs( 2.0f * mF - lb1 * lc1 - lb2 * lc2 ) < fabs( 2.0f * mF - lb1 * lc2 - lb2 * lc1 ) ) {
				swap( &lc1, &lc2 );
			}

			for( int loop = 0; loop < 2; loop++ ) {
				const float g = ( 0 == loop ) ? -lb1 : -lb2;
				const float h = ( 0 == loop ) ? -lc1 : -lc2;

				// Solve quadratic function: c0*u*u + c1*u + c2 = 0
				const float c0 = b + g * ( 2.0f * d + a * g );
				const float c1 = 2.0f * ( h * ( d + a * g ) + f + e * g );
				const float c2 = h * ( a * h + 2.0f * e ) + c;
				const char numResults = solveCubic( 0.0f, c0, c1, c2, xs );

				for( char i = 0; i < numResults; i++ ) {
					v = xs[i];
					u = fma( g, v, h );
					w = 1.0f - u - v;

					if( 0.0f <= u && 0.0f <= v && 0.0f <= w ) {
						float3 pTessellated = phongTessellation( P1, P2, P3, N1, N2, N3, u, v, w ) - ray->origin.xyz;
						t = native_divide(
							( (float*) &pTessellated )[domain],
							( (float*) &(ray->dir) )[domain]
						);

						if( IS_T_VALID ) {
							tuv->x = t;
							tuv->y = u;
							tuv->z = v;

							const float3 ns = getTriangleNormalS( u, v, w, C1, C2, C3, E12, E20 );
							const float4 np = getTriangleNormal( &face, u, v, w );
							const float3 r = getTriangleReflectionVec( ray->dir.xyz, np.xyz );
							normal = ( dot( ns, r ) < 0.0f ) ? (float4)( ns, 0.0f ) : np;
						}
					}
				}
			}
		}

		return normal;

		#undef IS_T_VALID

	#endif

	#undef P1
	#undef P2
	#undef P3
	#undef N1
	#undef N2
	#undef N3
}


/**
 * Test faces of the given node for intersections with the given ray.
 * @param {ray4*}                ray
 * @param {const bvhNode*}       node
 * @param {const global face_t*} faces
 * @param {const float tNear}    tNear
 * @param {float tFar}           tFar
 */
void intersectFaces(
	ray4* ray, const bvhNode* node, const global face_t* faces, const float tNear, float tFar
) {
	float3 tuv;
	const int faceIndices[4] = { node->faces.x, node->faces.y, node->faces.z, node->faces.w };

	for( int i = 0; i < 4; i++ ) {
		if( faceIndices[i] == -1 ) {
			break;
		}

		float4 normal = checkFaceIntersection( ray, faces[faceIndices[i]], &tuv, tNear, tFar );

		if( tuv.x < INFINITY ) {
			tFar = tuv.x;

			if( ray->t > tuv.x ) {
				ray->normal = normal;
				ray->normal.w = (float) faceIndices[i];
				ray->t = tuv.x;
			}
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
void traverseBVH(
	const global bvhNode* bvh, ray4* ray, const global face_t* faces
) {
	bool addLeftToStack, addRightToStack, rightThenLeft;
	float tFarL, tFarR, tNearL, tNearR;

	uint bvhStack[BVH_STACKSIZE];
	int stackIndex = 0;
	bvhStack[stackIndex] = 0; // Node 0 is always the BVH root node

	ray4 rayBest = *ray;
	const float3 invDir = native_recip( ray->dir.xyz );

	rayBest.t = INFINITY;

	while( stackIndex >= 0 ) {
		bvhNode node = bvh[bvhStack[stackIndex--]];
		tNearL = 0.0f;
		tFarL = INFINITY;

		// Is a leaf node with faces
		if( node.faces.x > -1 ) {
			if(
				intersectBox( ray, invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) &&
				rayBest.t > tNearL
			) {
				intersectFaces( ray, &node, faces, tNearL, tFarL );
				rayBest = ( ray->t < rayBest.t ) ? *ray : rayBest;
			}

			continue;
		}

		tNearR = 0.0f;
		tFarR = INFINITY;
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

	*ray = ( rayBest.t != INFINITY ) ? rayBest : *ray;
}
