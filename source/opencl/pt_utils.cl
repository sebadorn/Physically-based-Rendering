/**
 * MACRO: Get half-vector.
 * @param  {const float3} v
 * @param  {const float3} w
 * @return {float3}         Half-vector of v and w.
 */
#define bisect( v, w ) ( fast_normalize( ( v ) + ( w ) ) )


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


// float pq( float a, float b, float c, float d, float e, float f, float u, bool getFirst ) {
// 	float pq_p = native_divide( d*u + f, b );
// 	float pq_q = native_divide( a*u*u + c + e*u, b );

// 	float pq_p_half = 0.5f * pq_p;
// 	float pq_sqrt = native_sqrt( pq_p_half*pq_p_half - pq_q );
// 	pq_sqrt *= getFirst ? 1.0f : -1.0f;

// 	return -pq_p_half + pq_sqrt;
// }


void solveQuadratic( float c0, float c1, float c2, float* x0, float* x1 ) {
	float p = native_divide( c1, c2 );
	float q = native_divide( c0, c2 );
	float p_half = 0.5f * p;
	float tmp = native_sqrt( p_half * p_half - q );

	*x0 = p_half + tmp;
	*x1 = p_half - tmp;
}


int solveCubic( float a, float b, float c, float d, float x[3] ) {
	b /= a;
	c /= a;
	d /= a;
	float q = native_divide( 3.0f*c - b*b, 9.0f );
	float r = native_divide( 9.0f*b*c - 27.0f*d - 2.0f*b*b*b, 54.0f );
	float discr = q*q*q + r*r;
	float term1 = native_divide( b, 3.0f );

	int results_real = 0;

	if( discr > 0.0f ) {
		float s = r + native_sqrt( discr );
		s = ( s < 0.0f ) ? -cbrt( -s ) : cbrt( s );
		float t = r - native_sqrt( discr );
		t = ( t < 0.0f ) ? -cbrt( -t ) : cbrt( t );
		x[0] = -term1 + s + t;
		// term1 += ( s + t ) * 0.5f;
		// x[1] = -term1;
		// x[2] = x[1];
		// term1 = native_sqrt( 3.0f ) * ( s - t ) * 0.5f;
		// x2_im = term1;
		// x3_im = -term1;

		results_real = 1;
	}
	else if( fabs( discr ) < 0.0000001f ) {
		float r13 = ( r < 0.0f ) ? -cbrt( -r ) : cbrt( r );
		x[0] = -term1 + 2.0f*r13;
		x[1] = -term1 - r13;
		// x[2] = x[1];

		results_real = 2;
	}
	else {
		q = -q;
		float tmp1 = q*q*q;
		tmp1 = acos( native_divide( r, native_sqrt( tmp1 ) ) );
		float r13 = 2.0f * native_sqrt( q );
		x[0] = -term1 + r13 * native_cos( native_divide( tmp1, 3.0f ) );
		x[1] = -term1 + r13 * native_cos( native_divide( tmp1 + 2.0f*M_PI, 3.0f ) );
		x[2] = -term1 + r13 * native_cos( native_divide( tmp1 + 4.0f*M_PI, 3.0f ) );

		results_real = 3;
	}

	return results_real;
}


/**
 *
 * @param  {const face_t} face
 * @param  {const float3} tuv
 * @return {float4}
 */
inline float4 getTriangleNormal( const face_t face, const float3 tuv ) {
	return fast_normalize( face.an + tuv.y * ( face.bn - face.an ) + tuv.z * ( face.cn - face.an ) );
}


/**
 * New direction for (perfectly) diffuse surfaces.
 * (Well, depening on the given parameters.)
 * @param  {const float4} nl   Normal (unit vector).
 * @param  {const float}  phi
 * @param  {const float}  sina
 * @param  {const float}  cosa
 * @return {float4}
 */
float4 jitter(
	const float4 nl, const float phi, const float sina, const float cosa
) {
	const float4 u = fast_normalize( cross( nl.yzxw, nl ) );
	const float4 v = fast_normalize( cross( nl, u ) );

	return fast_normalize(
		fast_normalize(
			u * native_cos( phi ) +
			v * native_sin( phi )
		) * sina + nl * cosa
	);
}


/**
 * Project a point on a plane.
 * @param  {float3} q Point to project.
 * @param  {float3} p Point of plane.
 * @param  {float3} n Normal of plane.
 * @return {float3}   Projected point.
 */
#define projectOnPlane( q, p, n ) ( ( q ) - cross( dot( ( ( q ) - ( p ) ), ( n ) ), ( n ) ) )


/**
 * MACRO: Apply Lambert's cosine law for light sources.
 * @param  {float4} n Normal of the surface the light hits.
 * @param  {float4} l Normalized direction to the light source.
 * @return {float}
 */
#define lambert( n, l ) ( fmax( dot( ( n ).xyz, ( l ).xyz ), 0.0f ) )


/**
 * MACRO: Projection of h orthogonal n, with n being a unit vector.
 * @param  {const float3} h
 * @param  {const float3} n
 * @return {float3}         Projection of h onto n.
 */
#define projection( h, n ) ( dot( ( h ).xyz, ( n ).xyz ) * ( n ) )


/**
 *
 * @param  {float*} seed
 * @return {float}
 */
inline const float rand( float* seed ) {
	float i;
	*seed += 1.0f;

	return fract( native_sin( *seed ) * 43758.5453123f, &i );
}


// #define RAND_A 16807.0f
// #define RAND_M 2147483647.0f
// #define RAND_MRECIP 4.656612875245797e-10

// // Park-Miller RNG
// inline const float rand( int* seed ) {
// 	const float temp = (*seed) * RAND_A;
// 	*seed = (int)( temp - RAND_M * floor( temp * RAND_MRECIP ) );

// 	return (*seed) * RAND_MRECIP;
// }


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
float4 refract( const ray4* ray, const material* mtl, float* seed ) {
	#define DIR ( ray->dir )
	#define N ( ray->normal )

	const bool into = ( dot( N.xyz, DIR.xyz ) < 0.0f );
	const float4 nl = into ? N : -N;

	const float m1 = into ? NI_AIR : mtl->Ni;
	const float m2 = into ? mtl->Ni : NI_AIR;
	const float m = native_divide( m1, m2 );

	const float cosI = -dot( DIR.xyz, nl.xyz );
	const float sinT2 = m * m * ( 1.0f - cosI * cosI );

	// Critical angle. Total internal reflection.
	if( sinT2 > 1.0f ) {
		return reflect( DIR, nl );
	}


	// Reflectance and transmission

	const float r0 = native_divide( m1 - m2, m1 + m2 );
	const float c = ( m1 > m2 ) ? native_sqrt( 1.0f - sinT2 ) : cosI;
	const float reflectance = fresnel( c, r0 * r0 );
	// transmission = 1.0f - reflectance

	#define COS_T ( native_sqrt( 1.0f - sinT2 ) )
	#define DO_REFRACT ( reflectance < rand( seed ) )

	const float4 newRay = DO_REFRACT
	                    ? m * DIR + ( m * cosI - COS_T ) * nl
	                    : reflect( DIR, nl );

	return fast_normalize( newRay );

	#undef DIR
	#undef N
	#undef COS_T
	#undef DO_REFRACT
}


/**
 * Initialize a float array with a default value.
 * @param {float*} arr  The array to initialize.
 * @param {float}  val  The default value to set.
 */
inline void setArray( float* arr, float val ) {
	for( int i = 0; i < SPEC; i++ ) {
		arr[i] = val;
	}
}
