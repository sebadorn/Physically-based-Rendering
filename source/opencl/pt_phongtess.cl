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
