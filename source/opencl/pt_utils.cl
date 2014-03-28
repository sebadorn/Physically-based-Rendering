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
 * MACRO: Apply Lambert's cosine law for light sources.
 * @param  {float4} n Normal of the surface the light hits.
 * @param  {float4} l Normalized direction to the light source.
 * @return {float}
 */
#define lambert( n, l ) ( fmax( dot( ( n ).xyz, ( l ).xyz ), 0.0f ) )


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
 *
 * @param  {const face_t} face
 * @param  {const float3} tuv
 * @return {float4}
 */
inline float4 getTriangleNormal( const face_t face, const float3 tuv ) {
	const float w = 1.0f - tuv.y - tuv.z;

	return fast_normalize( w * face.an + tuv.y * face.bn + tuv.z * face.cn );
}


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
 * Initialize a float array with a default value.
 * @param {float*} arr  The array to initialize.
 * @param {float}  val  The default value to set.
 */
inline void setArray( float* arr, float val ) {
	for( int i = 0; i < SPEC; i++ ) {
		arr[i] = val;
	}
}



// ------------- //
// BRDF: Schlick //
// ------------- //


/**
 * Zenith angle.
 * @param  {const float} t
 * @param  {const float} r Roughness factor (0: perfect specular, 1: perfect diffuse).
 * @return {float}
 */
inline float Z( const float t, const float r ) {
	const float x = 1.0f + r * t * t - t * t;
	return native_divide( r, x * x );
}


/**
 * Azimuth angle.
 * @param  {const float} w
 * @param  {const float} p Isotropy factor (0: perfect anisotropic, 1: perfect isotropic).
 * @return {float}
 */
inline float A( const float w, const float p ) {
	const float p2 = p * p;
	const float w2 = w * w;
	return native_sqrt( native_divide( p, p2 - p2 * w2 + w2 ) );
}


/**
 * Smith factor.
 * @param  {const float} v
 * @param  {const float} r Roughness factor (0: perfect specular, 1: perfect diffuse)
 * @return {float}
 */
inline float G( const float v, const float r ) {
	return native_divide( v, r - r * v + v );
}


/**
 * Directional factor.
 * @param  {const float} t
 * @param  {const float} v
 * @param  {const float} vIn
 * @param  {const float} w
 * @param  {const float} r
 * @param  {const float} p
 * @return {float}
 */
inline float B1(
	const float t, const float vOut, const float vIn, const float w,
	const float r, const float p, const float d
) {
	const float drecip = native_recip( d );

	return Z( t, r ) * A( w, p ) * drecip;
}


/**
 * Directional factor.
 * @param  {const float} t
 * @param  {const float} v
 * @param  {const float} vIn
 * @param  {const float} w
 * @param  {const float} r
 * @param  {const float} p
 * @return {float}
 */
inline float B2(
	const float t, const float vOut, const float vIn, const float w,
	const float r, const float p, const float d
) {
	const float drecip = native_recip( d );
	const float gp = G( vOut, r ) * G( vIn, r );
	const float part1 = gp * Z( t, r ) * A( w, p );
	const float part2 = ( 1.0f - gp );

	return ( part1 + part2 ) * drecip;
}


/**
 * Directional factor.
 * @param  {const float} t
 * @param  {const float} v
 * @param  {const float} vIn
 * @param  {const float} w
 * @param  {const float} r
 * @param  {const float} p
 * @return {float}
 */
inline float D(
	const float t, const float vOut, const float vIn, const float w,
	const float r, const float p
) {
	const float b = 4.0f * r * ( 1.0f - r );
	const float a = ( r < 0.5f ) ? 0.0f : 1.0f - b;
	const float c = ( r < 0.5f ) ? 1.0f - b : 0.0f;

	const float d = 4.0f * M_PI * vOut * vIn;

	const float p0 = native_divide( a, M_PI );
	const float p1 = ( b == 0.0f || d == 0.0f )
	               ? 0.0f
	               : native_divide( b, d ) * B2( t, vOut, vIn, w, r, p, d );
	const float p2 = ( vIn == 0.0f )
	               ? 0.0f
	               : native_divide( c, vIn );

	return p0 + p1 + p2;
}


/**
 * MACRO: Projection of h orthogonal n, with n being a unit vector.
 * @param  {const float3} h
 * @param  {const float3} n
 * @return {float3}         Projection of h onto n.
 */
#define projection( h, n ) ( dot( ( h ).xyz, ( n ).xyz ) * ( n ) )


#define bisect( v, w ) ( fast_normalize( ( v ) + ( w ) ) )


/**
 * Calculate the values needed for the BRDF.
 * @param {const float4} V_in
 * @param {const float4} V_out
 * @param {const float4} N
 * @param {const float4} T
 * @param {float4*}      H
 * @param {float*}       t
 * @param {float*}       v
 * @param {float*}       vIn
 * @param {float*}       vOut
 * @param {float*}       w
 */
inline void getValuesBRDF(
	const float4 V_in, const float4 V_out, const float4 N, const float4 T,
	float4* H, float* t, float* vIn, float* vOut, float* w
) {
	*H = bisect( V_out, V_in );
	*t = dot( H->xyz, N.xyz );
	*vIn = dot( V_in.xyz, N.xyz );
	*vOut = dot( V_out.xyz, N.xyz );
	*w = dot( T.xyz, projection( H->xyz, N.xyz ) );
}


#if BRDF == 1

	float brdfSchlick(
		const material* mtl, const ray4* rayLightOut, const ray4* rayLightIn,
		const float4* normal, float* u
	) {
		#define ISOTROPY mtl->scratch.w

		float4 H;
		float t, vIn, vOut, w;

		const float4 groove = groove3D( mtl->scratch, *normal );
		getValuesBRDF( rayLightIn->dir, -rayLightOut->dir, *normal, groove, &H, &t, &vIn, &vOut, &w );

		*u = fmax( dot( H.xyz, -rayLightOut->dir.xyz ), 0.0f );
		return D( t, vOut, vIn, w, mtl->rough, ISOTROPY ) * fresnel( *u, 1.0f );

		#undef ISOTROPY
	}

#elif BRDF == 2

	float brdfShirleyAshikhmin(
		const float nu, const float nv, const float Rs, const float Rd,
		const ray4* rayLightOut, const ray4* rayLightIn, const float4* normal,
		float* brdfSpec, float* brdfDiff, float* dotHK1
	) {
		// Surface tangent vectors orthonormal to the surface normal
		float4 un = fast_normalize( cross( normal->yzxw, *normal ) );
		float4 vn = fast_normalize( cross( *normal, un ) );

		float4 k1 = rayLightIn->dir;   // to light
		float4 k2 = -rayLightOut->dir; // to viewer
		float4 h = bisect( k1, k2 );

		// Pre-compute
		float dotHU = dot( h.xyz, un.xyz );
		float dotHV = dot( h.xyz, vn.xyz );
		float dotHN = dot( h.xyz, normal->xyz );
		*dotHK1 = dot( h.xyz, k1.xyz );
		float dotHK2 = dot( h.xyz, k2.xyz );
		float dotNK1 = dot( normal->xyz, k1.xyz );
		float dotNK2 = dot( normal->xyz, k2.xyz );

		// Specular
		float ps_e = native_divide( nu * dotHU * dotHU + nv * dotHV * dotHV, 1.0f - dotHN * dotHN );
		float ps = native_sqrt( ( nu + 1.0f ) * ( nv + 1.0f ) * 0.125f * M_1_PI );
		ps *= native_divide( pow( dotHN, ps_e ), (*dotHK1) * fmax( dotNK1, dotNK2 ) );
		// ps *= fresnel( dotHK1, Rs );

		// Diffuse
		float pd = native_divide( 28.0f, 23.0f ) * M_1_PI * Rd;// * ( 1.0f - Rs );
		float a = 1.0f - dotNK1 * 0.5f;
		float b = 1.0f - dotNK2 * 0.5f;
		pd *= 1.0f - a * a * a * a * a;
		pd *= 1.0f - b * b * b * b * b;

		*brdfSpec = ps;
		*brdfDiff = pd;
	}

#endif
