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



// ---- //
// BRDF //
// ---- //


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
	const float p1 = ( b == 0.0f ) ? 0.0f : native_divide( b, d ) * B2( t, vOut, vIn, w, r, p, d );
	const float p2 = ( vIn == 0.0f ) ? 0.0f : native_divide( c, vIn );

	return p0 + p1 + p2;
}


/**
 * MACRO: Projection of h orthogonal n, with n being a unit vector.
 * @param  {const float3} h
 * @param  {const float3} n
 * @return {float3}         Projection of h onto n.
 */
#define projection( h, n ) ( dot( ( h ).xyz, ( n ).xyz ) * ( n ) )


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
	float4* H, float* t, float* v, float* vIn, float* vOut, float* w
) {
	*H = fast_normalize( V_out + V_in );
	*t = dot( H->xyz, N.xyz );
	*v = dot( V_out.xyz, N.xyz );
	*vIn = dot( V_in.xyz, N.xyz );
	*vOut = dot( V_out.xyz, N.xyz );
	*w = dot( T.xyz, projection( H->xyz, N.xyz ) );
}
