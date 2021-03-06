/**
 * MACRO: Get half-vector.
 * @param  {const float3} v
 * @param  {const float3} w
 * @return {float3}         Half-vector of v and w.
 */
#define bisect( v, w ) ( fast_normalize( ( v ) + ( w ) ) )


constant uint MOD_3[6] = { 0, 1, 2, 0, 1, 2 };


// // PCG - random number generator

// typedef struct {
// 	ulong state;
// 	ulong inc;
// } pcg32_random_t;

// uint pcg32_random_r( pcg32_random_t* rng ) {
// 	ulong oldstate = rng->state;

// 	// Advance internal state
// 	rng->state = oldstate * 6364136223846793005 + ( rng->inc | 1 );

// 	// Calculate output function (XSH RR), uses old state for max ILP
// 	uint xorshifted = ( ( oldstate >> 18u ) ^ oldstate ) >> 27u;
// 	uint rot = oldstate >> 59u;

// 	return ( xorshifted >> rot ) | ( xorshifted << ( (-rot) & 31 ) );
// }


/**
 *
 * @param  {float*} seed
 * @return {float}       A random number [0, 1].
 */
inline float rand( float* seed ) {
	float i;
	*seed += 1.0f;

	return fract( native_sin( *seed ) * 43758.5453123f, &i );
}


/**
 * Fresnel factor.
 * @param  {const float} u
 * @param  {const float} c Reflection factor.
 * @return {float}
 */
inline float fresnel( const float u, const float c ) {
	const float v = 1.0f - u;
	return c + ( 1.0f - c ) * v * v * v * v * v;
}


/**
 * Fresnel factor.
 * @param  {const float} u
 * @param  {const float} c Reflection factor.
 * @return {float}
 */
inline float4 fresnel4( const float u, const float4 c ) {
	const float v = 1.0f - u;
	return c + ( 1.0f - c ) * v * v * v * v * v;
}


/**
 * Swap two float values.
 * @param {float*} a
 * @param {float*} b
 */
inline void swap( float* a, float* b ) {
	const float tmp = *a;
	*a = *b;
	*b = tmp;
}


/**
 *
 * @param  {const material*} mtl
 * @param  {float*}          seed
 * @return {bool}
 */
inline bool extendDepth( const material* mtl, float* seed ) {
	#if BRDF == 1
		// TODO: Use rand() in some way instead of this fixed threshold value.
		return ( fmax( mtl->data.s2, mtl->data.s3 ) >= 50.0f );
	#else
		return ( mtl->data.s3 < rand( seed ) );
	#endif
}


/**
 * Solve a cubic function: a0*x^3 + a1*x^2 + a2*x^1 + a3 = 0
 * @param  {const float} a0
 * @param  {const float} a1
 * @param  {const float} a2
 * @param  {const float} a3
 * @param  {float[3]}    x  Output. Found real solutions.
 * @return {char}           Number of found real solutions.
 */
char solveCubic( const float a0, const float a1, const float a2, const float a3, float x[3] ) {
	#define THIRD 0.3333333333f
	#define THIRD_HALF 0.1666666666f
	float w, p, q, dis, phi;

	if( fabs( a0 ) > 0.0f ) {
		// cubic problem
		w = native_divide( a1, a0 ) * THIRD;
		p = native_divide( a2, a0 ) * THIRD - w * w;
		p = p * p * p;
		q = 0.5f * native_divide( a2 * w - a3, a0 ) - w * w * w;
		dis = q * q + p;

		if( dis < 0.0f ) {
			// three real solutions
			phi = acos( clamp( native_divide( q, native_sqrt( -p ) ), -1.0f, 1.0f ) );
			p = 2.0f * pow( -p, THIRD_HALF );

			const float u[3] = {
				p * native_cos( phi * THIRD ) - w,
				p * native_cos( ( phi + 2.0f * M_PI ) * THIRD ) - w,
				p * native_cos( ( phi + 4.0f * M_PI ) * THIRD ) - w
			};

			x[0] = fmin( u[0], fmin( u[1], u[2] ) );
			x[1] = fmax( fmin( u[0], u[1] ), fmax( fmin( u[0], u[2] ), fmin( u[1], u[2] ) ) );
			x[2] = fmax( u[0], fmax( u[1], u[2] ) );
			// Minimize rounding errors through a Newton iteration
			x[0] -= native_divide(
				a3 + x[0] * ( a2 + x[0] * ( a1 + x[0] * a0 ) ),
				a2 + x[0] * ( 2.0f * a1 + x[0] * 3.0f * a0 )
			);
			x[1] -= native_divide(
				a3 + x[1] * ( a2 + x[1] * ( a1 + x[1] * a0 ) ),
				a2 + x[1] * ( 2.0f * a1 + x[1] * 3.0f * a0 )
			);
			x[2] -= native_divide(
				a3 + x[2] * ( a2 + x[2] * ( a1 + x[2] * a0 ) ),
				a2 + x[2] * ( 2.0f * a1 + x[2] * 3.0f * a0 )
			);

			return 3;
		}
		else {
			// only one real solution!
			dis = native_sqrt( dis );
			x[0] = cbrt( q + dis ) + cbrt( q - dis ) - w;
			// Newton iteration
			x[0] -= native_divide(
				a3 + x[0] * ( a2 + x[0] * ( a1 + x[0] * a0 ) ),
				a2 + x[0] * ( 2.0f * a1 + x[0] * 3.0f * a0 )
			);

			return 1;
		}
	}
	else if( fabs( a1 ) > 0.0f ) {
		// quadratic problem
		p = 0.5f * native_divide( a2, a1 );
		dis = p * p - native_divide( a3, a1 );

		if( dis >= 0.0f ) {
			const float dis_sqrt = native_sqrt( dis );

			// 2 real solutions
			x[0] = -p - dis_sqrt;
			x[1] = -p + dis_sqrt;
			// Newton iteration
			x[0] -= native_divide(
				a3 + x[0] * ( a2 + x[0] * a1 ),
				a2 + x[0] * 2.0f * a1
			);
			x[1] -= native_divide(
				a3 + x[1] * ( a2 + x[1] * a1 ),
				a2 + x[1] * 2.0f * a1
			);

			return 2;
		}
	}
	else if( fabs( a2 ) > 0.0f ) {
		// linear equation
		x[0] = native_divide( -a3, a2 );

		return 1;
	}

	return 0;

	#undef THIRD
	#undef THIRD_HALF
}


/**
 * Get two planes from a ray that have the ray as intersection.
 * The planes are described in the Hesse normal form.
 * @param  {const ray4*} ray The ray.
 * @return {rayPlanes}       The planes describing the ray.
 */
rayPlanes getPlanesFromRay( const ray4* ray ) {
	rayPlanes rp;

	rp.n1 = fast_normalize( cross( ray->origin, ray->dir ) );
	rp.n2 = fast_normalize( cross( rp.n1, ray->dir ) );

	rp.o1 = dot( rp.n1, ray->origin );
	rp.o2 = dot( rp.n2, ray->origin );

	return rp;
}


/**
 * MACRO: Calculate the normal at the ray-triangle intersection.
 * @param  {const float3} an
 * @param  {const float3} bn
 * @param  {const float3} cn
 * @param  {const float}  u
 * @param  {const float}  v
 * @param  {const float}  w
 * @return {float3}
 */
#define getTriangleNormal( an, bn, cn, u, v, w ) fast_normalize( ( an ) * ( u ) + ( bn ) * ( v ) + ( cn ) * ( w ) );


/**
 *
 * @param  {const float}  u
 * @param  {const float}  v
 * @param  {const float}  w
 * @param  {const float3} C12
 * @param  {const float3} C23
 * @param  {const float3} C31
 * @param  {const float3} E23
 * @param  {const float3} E31
 * @return {float3}
 */
inline float3 getTriangleNormalS(
	const float u, const float v, const float w,
	const float3 C12, const float3 C23, const float3 C31, const float3 E23, const float3 E31
) {
	const float3 du = ( w - u ) * C31 + v * ( C12 - C23 ) + E31;
	const float3 dv = ( w - v ) * C23 + u * ( C12 - C31 ) - E23;

	return fast_normalize( cross( du, dv ) );
}


/**
 *
 * @param  {const float3} view
 * @param  {const float3} np
 * @return {float3}
 */
inline float3 getTriangleReflectionVec( const float3 view, const float3 np ) {
	return view - 2.0f * np * dot( view, np );
}


/**
 *
 * @param  {const face_t*} face
 * @param  {const float3}  rayDir
 * @param  {const float}   u
 * @param  {const float}   v
 * @param  {const float}   w
 * @param  {const float3}  C1
 * @param  {const float3}  C2
 * @param  {const float3}  C3
 * @param  {const float3}  E12
 * @param  {const float3}  E20
 * @return {float4}
 */
inline float3 getPhongTessNormal(
	const float3 an, const float3 bn, const float3 cn,
	const float3 rayDir,
	const float u, const float v, const float w,
	const float3 C1, const float3 C2, const float3 C3,
	const float3 E12, const float3 E20
) {
	const float3 ns = getTriangleNormalS( u, v, w, C1, C2, C3, E12, E20 );
	const float3 np = getTriangleNormal( an, bn, cn, u, v, w );
	const float3 r = getTriangleReflectionVec( rayDir, np.xyz );

	return ( dot( ns, r ) < 0.0f ) ? ns : np;
}


/**
 * New direction for (perfectly) diffuse surfaces.
 * (Well, depening on the given parameters.)
 * @param  {const float3} nl   Normal (unit vector).
 * @param  {const float}  phi
 * @param  {const float}  sina
 * @param  {const float}  cosa
 * @return {float3}
 */
float3 jitter(
	const float3 nl, const float phi, const float sina, const float cosa
) {
	const float3 u = fast_normalize( cross( nl.yzx, nl ) );
	const float3 v = fast_normalize( cross( nl, u ) );

	return fast_normalize(
		fast_normalize(
			u * native_cos( phi ) +
			v * native_sin( phi )
		) * sina + nl * cosa
	);
}


/**
 * Anti-Aliasing by slightly jittering the ray.
 * @param {ray4*}       ray   The ray.
 * @param {const float} pxDim Pixel width and height.
 * @param {float*}      seed  Seed for RNG.
 */
void antiAliasing( ray4* ray, const float pxDim, float* seed ) {
	const float rnd = rand( seed );
	const float3 aaDir = jitter(
		ray->dir,
		PI_X2 * rand( seed ),
		native_sqrt( rnd ),
		native_sqrt( 1.0f - rnd )
	);

	ray->dir = fast_normalize( ray->dir + aaDir * pxDim * ANTI_ALIASING );
}


/**
 * Depth of Field by using a thin lense model.
 * Depending on the focus, the ray origin and direction will be changed.
 * @param {ray4*}         ray     The ray.
 * @param {const camera*} cam     Camera model.
 * @param {float}         tObject Distance to the object for this ray.
 * @param {float}         tFocus  Focus distance for the image.
 * @param {float*}        seed    Seed for RNG.
 */
void depthOfField( ray4* ray, const camera* cam, float tObject, float tFocus, float* seed ) {
	if( tObject == INFINITY ) {
		tObject = 1000.0f;
	}
	if( tFocus == INFINITY ) {
		tFocus = 1000.0f;
	}

	// Depth-of-Field with a thin lense model.
	if( tObject > 0.0f ) {
		const float aperture = cam->lense.x / cam->lense.y; // aperture = focal length / aperture

		// Choose a random point inside the circle of confusion.
		const float radius = rand( seed ) * aperture * 0.5f;
		const float angle = PI_X2 * rand( seed );
		const float x = radius * native_cos( angle );
		const float y = radius * native_sin( angle );

		ray->origin = ray->origin + x * cam->u + y * cam->v;

		// Set the new ray direction.
		const float3 hitFocalPlane = fma( tFocus, ray->dir, cam->eye );
		ray->dir = fast_normalize( hitFocalPlane - ray->origin );
	}
}


/**
 * Russian roulette termination. The longer a path and the less
 * energy it transports, the likelier it will be terminated.
 * @param  {const int}   depth       Current depth of path.
 * @param  {const int}   depthAdded  Number of path extensions so far.
 * @param  {const float} maxValColor Maximum found energy (either in an RGB value or the SPD).
 * @param  {float*}      seed        Seed for the RNG.
 * @return {bool}                    True, if path should be terminated, false otherwise.
 */
inline bool russianRoulette( const int depth, const int depthAdded, const float maxValColor, float* seed ) {
	return ( depth > 2 + depthAdded && maxValColor < rand( seed ) );
}


/**
 * Project a point on a plane.
 * @param  {float3} q Point to project.
 * @param  {float3} p Point of plane.
 * @param  {float3} n Normal of plane.
 * @return {float3}   Projected point.
 */
inline float3 projectOnPlane( const float3 q, const float3 p, const float3 n ) {
	return q - dot( q - p, n ) * n;
}


/**
 * MACRO: Apply Lambert's cosine law for light sources.
 * @param  {float3} n Normal of the surface the light hits.
 * @param  {float3} l Normalized direction to the light source.
 * @return {float}
 */
#define lambert( n, l ) ( fmax( dot( ( n ), ( l ) ), 0.0f ) )


/**
 * MACRO: Projection of h orthogonal n, with n being a unit vector.
 * @param  {const float3} h
 * @param  {const float3} n
 * @return {float3}         Projection of h onto n.
 */
#define projection( h, n ) ( dot( ( h ), ( n ) ) * ( n ) )


/**
 * MACRO: Reflect a ray.
 * @param  {float3} dir    Direction of ray.
 * @param  {float3} normal Normal of surface.
 * @return {float3}        Reflected ray.
 */
#define reflect( dir, normal ) ( ( dir ) - 2.0f * dot( ( normal ), ( dir ) ) * ( normal ) )


/**
 * Get the a new direction for a ray hitting a transparent surface (glass etc.).
 * @param  {const ray4*}     ray  The current ray.
 * @param  {const material*} mtl  Material of the hit surface.
 * @param  {float*}          seed Seed for the random number generator.
 * @return {float3}               A new direction for the ray.
 */
float3 refract( const ray4* ray, const material* mtl, float* seed ) {
	const bool into = ( dot( ray->normal, -ray->dir ) > 0.0f );
	const float3 nl = into ? ray->normal : -ray->normal;

	const float m1 = into ? NI_AIR : mtl->data.s1;
	const float m2 = into ? mtl->data.s1 : NI_AIR;
	const float m = native_divide( m1, m2 );

	const float cosI = -dot( nl, ray->dir );
	const float sinT2 = m * m * ( 1.0f - cosI * cosI );

	// Critical angle. Total internal reflection.
	if( sinT2 >= 1.0f ) {
		return reflect( ray->dir, nl );
	}

	// Reflectance and transmission

	const float sqrtCosT = native_sqrt( 1.0f - sinT2 );
	const float r0 = native_divide( m1 - m2, m1 + m2 );
	const float c = ( m1 > m2 ) ? sqrtCosT : cosI;
	const float reflectance = fresnel( c, r0 * r0 );
	// transmission = 1.0f - reflectance

	const float3 newDir = ( reflectance < rand( seed ) ) ?
	                      m * ray->dir + ( m * cosI - sqrtCosT ) * nl :
	                      reflect( ray->dir, nl );

	return newDir;
}
