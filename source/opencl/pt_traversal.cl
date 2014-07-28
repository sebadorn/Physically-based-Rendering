/**
 *
 * @param  {const material*} mtl
 * @param  {float*}          seed
 * @return {bool}
 */
inline bool extendDepth( const material* mtl, float* seed ) {
	#if BRDF == 1
		// TODO: Use rand() in some way instead of this fixed threshold value.
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
	// newRay.origin += ray->normal * EPSILON7;

	// Transparency and refraction
	bool doTransRefr = ( mtl->d < 1.0f && mtl->d <= rand( seed ) );

	*addDepth = ( *addDepth || doTransRefr );

	if( doTransRefr ) {
		newRay.dir = refract( ray, mtl, seed );
	}
	else {

		#if BRDF == 0

			// BRDF: Schlick.
			// Supports specular, diffuse, glossy, and anisotropic surfaces.
			newRay.dir = newRaySchlick( ray, mtl, seed );

		#elif BRDF == 1

			// BRDF: Shirley-Ashikhmin.
			// Supports specular, diffuse, glossy, and anisotropic surfaces.
			newRay.dir = newRayShirleyAshikhmin( ray, mtl, seed );

		#endif

	}

	return newRay;
}


/**
 * Phong tessellation of a given barycentric point.
 * @param  {const float3} P1
 * @param  {const float3} P2
 * @param  {const float3} P3
 * @param  {const float3} N1
 * @param  {const float3} N2
 * @param  {const float3} N3
 * @param  {const float}  u
 * @param  {const float}  v
 * @param  {const float}  w
 * @return {float3}          Phong tessellated point.
 */
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


/**
 * Get the best axis of the ray direction in order to calculate the t factor later on.
 * Meaning: Get the axis with the biggest coordinate.
 * @param  {const float3} rd Ray direction.
 * @return {char}            Index of the axis (x: 0, y: 1, z: 2)
 */
inline char getBestRayDomain( const float3 rd ) {
	const float3 d = fabs( rd );
	char domain = ( d.y > d.z ) ? 1 : 2;

	if( d.x > d.y ) {
		domain = ( d.x > d.z ) ? 0 : 2;
	}

	return domain;
}


/**
 * Find intersection of a triangle and a ray. (No tessellation.)
 * @param  {const face_t*} face
 * @param  {const ray4*}   ray
 * @param  {float3*}       tuv
 * @param  {const float}   tNear
 * @param  {const float}   tFar
 * @return {float4}
 */
float4 flatTriAndRayIntersect(
	const face_t* face, const ray4* ray, float3* tuv, const float tNear, const float tFar
) {
	const float3 edge1 = face->b.xyz - face->a.xyz;
	const float3 edge2 = face->c.xyz - face->a.xyz;
	const float3 tVec = ray->origin.xyz - face->a.xyz;
	const float3 pVec = cross( ray->dir.xyz, edge2 );
	const float3 qVec = cross( tVec, edge1 );
	const float invDet = native_recip( dot( edge1, pVec ) );

	tuv->x = dot( edge2, qVec ) * invDet;

	if( tuv->x < EPSILON5 || fmax( tuv->x - tFar, tNear - tuv->x ) > EPSILON5 ) {
		tuv->x = INFINITY;
		return (float4)( 0.0f );
	}

	tuv->y = dot( tVec, pVec ) * invDet;
	tuv->z = dot( ray->dir.xyz, qVec ) * invDet;
	tuv->x = ( fmin( tuv->y, tuv->z ) < 0.0f || tuv->y > 1.0f || tuv->y + tuv->z > 1.0f ) ? INFINITY : tuv->x;

	return getTriangleNormal( face, 1.0f - tuv->y - tuv->z, tuv->y, tuv->z );
}


/**
 * Find intersection of a triangle and a ray. (Phong tessellation.)
 * @param  {const face_t*} face
 * @param  {const ray4*}   ray
 * @param  {float3*}       tuv
 * @param  {const float}   tNear
 * @param  {const float}   tFar
 * @return {float4}
 */
float4 phongTessTriAndRayIntersect(
	const face_t* face, const ray4* ray, float3* tuv, const float tNear, const float tFar
) {
	#define P1 ( face->a.xyz )
	#define P2 ( face->b.xyz )
	#define P3 ( face->c.xyz )
	#define N1 ( face->an.xyz )
	#define N2 ( face->bn.xyz )
	#define N3 ( face->cn.xyz )

	float4 normal = (float4)( 0.0f );
	tuv->x = INFINITY;

	const float3 E01 = P2 - P1;
	const float3 E12 = P3 - P2;
	const float3 E20 = P1 - P3;

	const float3 C1 = PhongTess_ALPHA * ( dot( N2, E01 ) * N2 - dot( N1, E01 ) * N1 );
	const float3 C2 = PhongTess_ALPHA * ( dot( N3, E12 ) * N3 - dot( N2, E12 ) * N2 );
	const float3 C3 = PhongTess_ALPHA * ( dot( N1, E20 ) * N1 - dot( N3, E20 ) * N3 );

	float a, b, c, d, e, f, l, m, n, o, p, q;

	{
		const rayPlanes rp = getPlanesFromRay( ray );

		a = dot( -rp.n1, C3 );
		b = dot( -rp.n1, C2 );
		c = dot( rp.n1, P3 ) - rp.o1;
		d = dot( rp.n1, C1 - C2 - C3 ) * 0.5f;
		e = dot( rp.n1, C3 + E20 ) * 0.5f;
		f = dot( rp.n1, C2 - E12 ) * 0.5f;
		l = dot( -rp.n2, C3 );
		m = dot( -rp.n2, C2 );
		n = dot( rp.n2, P3 ) - rp.o2;
		o = dot( rp.n2, C1 - C2 - C3 ) * 0.5f;
		p = dot( rp.n2, C3 + E20 ) * 0.5f;
		q = dot( rp.n2, C2 - E12 ) * 0.5f;
	}


	// Solve cubic

	float xs[3] = { -1.0f, -1.0f, -1.0f };
	char numCubicRoots = 0;

	{
		const float a3 = ( l*m*n + 2.0f*o*p*q ) - ( l*q*q + m*p*p + n*o*o );
		const float a2 = ( a*m*n + l*b*n + l*m*c + 2.0f*( d*p*q + o*e*q + o*p*f ) ) -
		                 ( a*q*q + b*p*p + c*o*o + 2.0f*( l*f*q + m*e*p + n*d*o ) );
		const float a1 = ( a*b*n + a*m*c + l*b*c + 2.0f*( o*e*f + d*e*q + d*p*f ) ) -
		                 ( l*f*f + m*e*e + n*d*d + 2.0f*( a*f*q + b*e*p + c*d*o ) );
		const float a0 = ( a*b*c + 2.0f*d*e*f ) - ( a*f*f + b*e*e + c*d*d );

		numCubicRoots = solveCubic( a0, a1, a2, a3, xs );
	}

	if( 0 == numCubicRoots ) {
		return normal;
	}

	float x = 0.0f;
	float determinant = INFINITY;
	float mA, mB, mC, mD, mE, mF;

	for( char i = 0; i < numCubicRoots; i++ ) {
		mA = a * xs[i] + l;
		mB = b * xs[i] + m;
		mD = d * xs[i] + o;
		const float tmp = mD * mD - mA * mB;

		x = ( determinant > tmp ) ? xs[i] : x;
		determinant = fmin( determinant, tmp );
	}

	if( 0.0f >= determinant ) {
		return normal;
	}


	const char domain = getBestRayDomain( ray->dir.xyz );

	mA = a * x + l;
	mB = b * x + m;
	mC = c * x + n;
	mD = d * x + o;
	mE = e * x + p;
	mF = f * x + q;

	const bool AlessB = fabs( mA ) < fabs( mB );

	const float mBorA = AlessB ? mB : mA;
	mA = native_divide( mA, mBorA );
	mB = native_divide( mB, mBorA );
	mC = native_divide( mC, mBorA );
	mD = native_divide( mD, mBorA );
	mE = native_divide( mE, mBorA );
	mF = native_divide( mF, mBorA );

	const float mAorB = AlessB ? mA : mB;
	const float mEorF = AlessB ? 2.0f * mE : 2.0f * mF;
	const float mForE = AlessB ? mF : mE;
	const float ab = AlessB ? a : b;
	const float ba = AlessB ? b : a;
	const float ef = AlessB ? e : f;
	const float fe = AlessB ? f : e;

	const float sqrtAorB = native_sqrt( mD * mD - mAorB );
	const float sqrtC = native_sqrt( mForE * mForE - mC );
	const float lab1 = mD + sqrtAorB;
	const float lab2 = mD - sqrtAorB;
	float lc1 = mForE + sqrtC;
	float lc2 = mForE - sqrtC;

	if( fabs( mEorF - lab1 * lc1 - lab2 * lc2 ) < fabs( mEorF - lab1 * lc2 - lab2 * lc1 ) ) {
		swap( &lc1, &lc2 );
	}

	#pragma unroll(2)
	for( char loop = 0; loop < 2; loop++ ) {
		const float g = ( 0 == loop ) ? -lab1 : -lab2;
		const float h = ( 0 == loop ) ? -lc1 : -lc2;

		// Solve quadratic function: c0*u*u + c1*u + c2 = 0
		const float c0 = ab + g * ( 2.0f * d + ba * g );
		const float c1 = 2.0f * ( h * ( d + ba * g ) + ef + fe * g );
		const float c2 = h * ( ba * h + 2.0f * fe ) + c;
		const char numResults = solveCubic( 0.0f, c0, c1, c2, xs );

		for( char i = 0; i < numResults; i++ ) {
			float u = xs[i];
			float v = g * u + h;
			const float w = 1.0f - u - v;

			if( u < 0.0f || v < 0.0f || w < 0.0f ) {
				continue;
			}

			if( !AlessB ) {
				swap( &u, &v );
			}

			const float3 pTessellated = phongTessellation( P1, P2, P3, N1, N2, N3, u, v, w ) - ray->origin.xyz;
			const float t = native_divide(
				( (float*) &pTessellated )[domain],
				( (float*) &(ray->dir) )[domain]
			);

			// tuv->x -- best hit in this AABB so far
			// ray->t -- best hit found in other AABBs so far
			// tFar   -- far limit of this AABB
			// tNear  -- near limit of this AABB
			if( t >= fabs( tNear ) && t <= fmin( tuv->x, fmin( ray->t, tFar ) ) ) {
				tuv->x = t;
				tuv->y = u;
				tuv->z = v;
				normal = getPhongTessNormal( face, ray->dir.xyz, u, v, w, C1, C2, C3, E12, E20 );
			}
		}
	}

	return normal;

	#undef P1
	#undef P2
	#undef P3
	#undef N1
	#undef N2
	#undef N3
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
	#if PhongTess == 1

		const int3 cmp = ( face.an.xyz == face.bn.xyz ) + ( face.bn.xyz == face.cn.xyz );

		// Comparing vectors in OpenCL: 0/false/not equal; -1/true/equal
		if( cmp.x + cmp.y + cmp.z == -6 )

	#endif

	{
		return flatTriAndRayIntersect( &face, ray, tuv, tNear, tFar );
	}


	// Phong Tessellation
	// Based on: "Direct Ray Tracing of Phong Tessellation" by Shinji Ogaki, Yusuke Tokuyoshi
	#if PhongTess == 1

		return phongTessTriAndRayIntersect( &face, ray, tuv, tNear, tFar );

	#endif
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
	ray4* ray, const bvhNode* node, global const uint* bvhFaces, const global face_t* faces, const float tNear, float tFar
) {
	// const int faceIndices[4] = { nodeFaces.x, nodeFaces.y, nodeFaces.z, nodeFaces.w };

	// #pragma unroll(4)
	for( char i = 0; i < node->facesInterval.y; i++ ) {
		float3 tuv;
		float4 normal = checkFaceIntersection( ray, faces[bvhFaces[node->facesInterval.x + i]], &tuv, tNear, tFar );

		if( tuv.x < INFINITY ) {
			tFar = tuv.x;

			if( ray->t > tuv.x ) {
				ray->normal = normal;
				ray->normal.w = (float) bvhFaces[node->facesInterval.x + i];
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
	const ray4* ray, const float3* invDir, const float4 bbMin, const float4 bbMax,
	float* tNear, float* tFar
) {
	const float3 t1 = ( bbMin.xyz - ray->origin.xyz ) * (*invDir);
	float3 tMax = ( bbMax.xyz - ray->origin.xyz ) * (*invDir);
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

	return ( fmax( *tNear, *tFar ) > EPSILON5 );
}


/**
 * Traverse the BVH and test the faces against the given ray.
 * @param {global const bvhNode*} bvh
 * @param {global const int4*}    bvhFaces
 * @param {ray4*}                 ray
 * @param {global const face_t*}  faces
 */
void traverseBVH(
	global const bvhNode* bvh, global const uint* bvhFaces,
	ray4* ray, global const face_t* faces
) {
	uint bvhStack[BVH_STACKSIZE];
	int stackIndex = 0;
	bvhStack[stackIndex] = 0; // Node 0 is always the BVH root node

	const float3 invDir = native_recip( ray->dir.xyz );
	float tBest = INFINITY;

	while( stackIndex >= 0 ) {
		bvhNode node = bvh[bvhStack[stackIndex--]];
		float tNearL = 0.0f;
		float tFarL = INFINITY;

		// Is a leaf node
		if( node.bbMin.w < 0.0f ) {
			if(
				intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) &&
				tBest > tNearL
			) {
				intersectFaces( ray, &node, bvhFaces, faces, tNearL, tFarL );
				tBest = fmin( ray->t, tBest );
			}

			continue;
		}


		// Add child nodes to stack, if hit by ray

		bvhNode childNode = bvh[(int) node.bbMin.w];

		bool addLeftToStack = (
			intersectBox( ray, &invDir, childNode.bbMin, childNode.bbMax, &tNearL, &tFarL ) &&
			tBest > tNearL
		);

		float tNearR = 0.0f;
		float tFarR = INFINITY;
		childNode = bvh[(int) node.bbMax.w];

		bool addRightToStack = (
			intersectBox( ray, &invDir, childNode.bbMin, childNode.bbMax, &tNearR, &tFarR ) &&
			tBest > tNearR
		);


		// The node that is pushed on the stack first will be evaluated last.
		// So the nearer one should be pushed last, because it will be popped first then.
		const bool rightThenLeft = ( tNearR > tNearL );

		if( rightThenLeft && addRightToStack ) {
			bvhStack[++stackIndex] = (uint) node.bbMax.w;
		}
		if( rightThenLeft && addLeftToStack ) {
			bvhStack[++stackIndex] = (uint) node.bbMin.w;
		}

		if( !rightThenLeft && addLeftToStack ) {
			bvhStack[++stackIndex] = (uint) node.bbMin.w;
		}
		if( !rightThenLeft && addRightToStack ) {
			bvhStack[++stackIndex] = (uint) node.bbMax.w;
		}
	}

	ray->t = fmin( ray->t, tBest );
}


/**
 * Traverse the BVH and test the faces against the given ray.
 * This version is for the shadow ray test, so it only checks IF there
 * is an intersection and terminates on the first hit.
 * @param {const global bvhNode*} bvh
 * @param {ray4*}                 ray
 * @param {const global face_t*}  faces
 */
void traverseBVH_shadowRay(
	const global bvhNode* bvh, const global uint* bvhFaces,
	ray4* ray, const global face_t* faces
) {
	bool addLeftToStack, addRightToStack, rightThenLeft;
	float tFarL, tFarR, tNearL, tNearR;

	uint bvhStack[BVH_STACKSIZE];
	int stackIndex = 0;
	bvhStack[stackIndex] = 0; // Node 0 is always the BVH root node

	const float3 invDir = native_recip( ray->dir.xyz );

	while( stackIndex >= 0 ) {
		bvhNode node = bvh[bvhStack[stackIndex--]];
		tNearL = 0.0f;
		tFarL = INFINITY;

		// Is a leaf node with faces
		if( node.bbMin.w < 0.0f && node.bbMax.w < 0.0f ) {
			if( intersectBox( ray, &invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) ) {
				intersectFaces( ray, &node, bvhFaces, faces, tNearL, tFarL );

				if( ray->t < INFINITY ) {
					break;
				}
			}

			continue;
		}

		// Add child nodes to stack, if hit by ray

		bvhNode childNode = bvh[(int) node.bbMin.w];

		bool addLeftToStack = intersectBox( ray, &invDir, childNode.bbMin, childNode.bbMax, &tNearL, &tFarL );

		float tNearR = 0.0f;
		float tFarR = INFINITY;
		childNode = bvh[(int) node.bbMax.w];

		bool addRightToStack = intersectBox( ray, &invDir, childNode.bbMin, childNode.bbMax, &tNearR, &tFarR );


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
}
