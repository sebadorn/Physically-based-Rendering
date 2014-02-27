/**
 *
 * @param  {float*} seed
 * @return {float}
 */
inline float rand( float* seed ) {
	float i;
	*seed += 1.0f;

	return fract( native_sin( *seed ) * 43758.5453123f, &i );
}


/**
 * http://www.rorydriscoll.com/2009/01/07/better-sampling/ (Not 1:1 used here.)
 * @param  {const float}  seed
 * @param  {const float4} normal
 * @return {float4}
 */
float4 cosineWeightedDirection( float* seed, const float4 normal ) {
	const float u = rand( seed );
	const float r = native_sqrt( u );
	const float angle = PI_X2 * rand( seed );

	const float4 sdir = ( fabs( normal.x ) < 0.5f )
	                  ? (float4)( 0.0f, normal.z, -normal.y, 0.0f )
	                  : (float4)( -normal.z, 0.0f, normal.x, 0.0f );
	const float4 tdir = cross( normal, sdir );

	return r * native_cos( angle ) * sdir + r * native_sin( angle ) * tdir + native_sqrt( 1.0f - u ) * normal;
}


/**
 *
 * @param  {const face_t} face
 * @param  {const float4} tuv
 * @return {float4}
 */
inline float4 getTriangleNormal( const face_t face, const float4 tuv ) {
	const float w = 1.0f - tuv.y - tuv.z;

	return fast_normalize( w * face.an + tuv.y * face.bn + tuv.z * face.cn );
}


/**
 * New direction for (perfectly) diffuse surfaces.
 * @param  d    Normal (unit vector).
 * @param  phi
 * @param  sina
 * @param  cosa
 * @return
 */
float4 jitter( float4 d, float phi, float sina, float cosa ) {
	const float4 u = fast_normalize( cross( d.yzxw, d ) );
	const float4 v = cross( d, u );

	return fast_normalize(
		( u * native_cos( phi ) + v * native_sin( phi ) ) * sina + d * cosa
	);
}


/**
 * MACRO: Reflect a ray.
 * @param  {float4} dir    Direction of ray.
 * @param  {float4} normal Normal of surface.
 * @return {float4}        Reflected ray.
 */
#define reflect( dir, normal ) ( ( dir ) - 2.0f * dot( ( normal ).xyz, ( dir ).xyz ) * ( normal ) )


/**
 *
 * @param  prevRay
 * @param  mtl
 * @param  seed
 * @return
 */
float4 refract( ray4* prevRay, material* mtl, float* seed ) {
	bool into = ( dot( prevRay->normal.xyz, prevRay->dir.xyz ) < 0.0f );
	float4 nl = into ? prevRay->normal : -prevRay->normal;

	float m1 = into ? NI_AIR : mtl->Ni;
	float m2 = into ? mtl->Ni : NI_AIR;
	float m = native_divide( m1, m2 );

	float cosI = -dot( prevRay->dir.xyz, nl.xyz );
	float sinSq = m * m * ( 1.0f - cosI * cosI );

	// Critical angle. Total internal reflection.
	if( sinSq > 1.0f ) {
		return reflect( prevRay->dir, nl );
	}

	// No TIR, proceed.
	float4 tDir = m * prevRay->dir + ( m * cosI - native_sqrt( 1.0f - sinSq ) ) * nl;

	float r0 = native_divide( m1 - m2, m1 + m2 );
	r0 *= r0;
	float c = ( m1 > m2 ) ? native_sqrt( 1.0f - sinSq ) : cosI;
	float x = 1.0f - c;
	float refl = r0 + ( 1.0f - r0 ) * x * x * x * x * x;
	// float trans = 1.0f - refl;

	return ( refl < rand( seed ) ) ? tDir : reflect( prevRay->dir, nl );
}


/**
 * Get a vector in a random direction.
 * @param  {const float} seed Seed for the random value.
 * @return {float4}           Random vector.
 */
inline float4 uniformlyRandomDirection( float* seed ) {
	const float v = rand( seed );
	const float u = rand( seed );
	const float z = 1.0f - 2.0f * u;
	const float r = native_sqrt( 1.0f - z * z );
	const float angle = PI_X2 * v;

	return (float4)( r * native_cos( angle ), r * native_sin( angle ), z, 0.0f );
}


/**
 * MACRO: Get a vector in a random direction.
 * @param  {float*} seed Seed for the random value.
 * @return {float4}      Random vector.
 */
#define uniformlyRandomVector( seed ) ( uniformlyRandomDirection( ( seed ) ) * native_sqrt( rand( ( seed ) ) ) )


/**
 * Source: http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-7-intersecting-simple-shapes/ray-box-intersection/
 * Which is based on: "An Efficient and Robust Ray–Box Intersection Algorithm", Williams et al.
 * @param  {const float4*} origin
 * @param  {const float4*} dir
 * @param  {const float*}  bbMin
 * @param  {const float*}  bbMax
 * @param  {float*}        tNear
 * @param  {float*}        tFar
 * @return {bool}
 */
const bool intersectBoundingBox(
	const ray4* ray, const float4 bbMin, const float4 bbMax, float* tNear, float* tFar
) {
	const float3 invDir = native_recip( ray->dir.xyz );
	const float3 t1 = ( bbMin.xyz - ray->origin.xyz ) * invDir;
	float3 tMax = ( bbMax.xyz - ray->origin.xyz ) * invDir;
	const float3 tMin = fmin( t1, tMax );
	tMax = fmax( t1, tMax );

	*tNear = fmax( fmax( tMin.x, tMin.y ), tMin.z );
	*tFar = fmin( fmin( tMax.x, tMax.y ), fmin( tMax.z, *tFar ) );

	return ( *tNear <= *tFar );
}


const bool intersectSphere( const ray4* ray, const float4 pos, const float radius, float* tNear, float* tFar ) {
	float3 op = pos.xyz - ray->origin.xyz;
	float b = dot( op, ray->dir.xyz );
	float det = b * b - dot( op, op ) + radius * radius;

	if( det < 0.0f ) {
		return false;
	}

	det = native_sqrt( det );
	*tNear = b - det;
	*tFar = b + det;

	return ( *tNear > EPSILON || *tFar > EPSILON );
}


/**
 * Source: http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-7-intersecting-simple-shapes/ray-box-intersection/
 * Which is based on: "An Efficient and Robust Ray–Box Intersection Algorithm", Williams et al.
 * @param {const float4*} origin
 * @param {const float4*} dir
 * @param {const float*}  bbMin
 * @param {const float*}  bbMax
 * @param {float*}        tFar
 * @param {int*}          exitRope
 */
void updateEntryDistanceAndExitRope(
	const ray4* ray, const float4 bbMin, const float4 bbMax, float* tFar, int* exitRope
) {
	const float3 invDir = native_recip( ray->dir.xyz );
	const float3 t1 = native_divide( bbMin.xyz - ray->origin.xyz, ray->dir.xyz );
	float3 tMax = native_divide( bbMax.xyz - ray->origin.xyz, ray->dir.xyz );
	tMax = fmax( t1, tMax );

	*tFar = fmin( fmin( tMax.x, tMax.y ), tMax.z );
	*exitRope = ( *tFar == tMax.y ) ? 3 - signbit( invDir.y ) : 1 - signbit( invDir.x );
	*exitRope = ( *tFar == tMax.z ) ? 5 - signbit( invDir.z ) : *exitRope;
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


/**
 * MACRO: Apply the cosine law for light sources.
 * @param  {float4} n Normal of the surface the light hits.
 * @param  {float4} l Normalized direction to the light source.
 * @return {float}
 */
#define cosineLaw( n, l ) ( fmax( dot( ( n ).xyz, ( l ).xyz ), 0.0f ) )



// ---- //
// BRDF //
// ---- //


/**
 *
 * @param  t
 * @param  r Roughness factor (0: perfect specular, 1: perfect diffuse).
 * @return
 */
inline float Z( float t, float r ) {
	return r / pown( 1.0f + r * t * t - t * t, 2 );
}


/**
 *
 * @param  w
 * @param  p Isotropy factor (0: perfect anisotropic, 1: perfect isotropic).
 * @return
 */
inline float A( float w, float p ) {
	return sqrt( p / ( p * p - p * p * w * w + w * w ) );
}


/**
 *
 * @param  v
 * @param  r Roughness factor (0: perfect specular, 1: perfect diffuse)
 * @return
 */
inline float G( float v, float r ) {
	return v / ( r - r * v + v );
}


/**
 *
 * @param  t
 * @param  vOut
 * @param  vIn
 * @param  w
 * @param  r   Roughness factor (0: perfect specular, 1: perfect diffuse)
 * @param  p   Isotropy factor (0: perfect anisotropic, 1: perfect isotropic).
 * @return
 */
inline float B( float t, float vOut, float vIn, float w, float r, float p ) {
	float gp = G( vOut, r ) * G( vIn, r );
	float d = 4.0f * M_PI * vOut * vIn;
	float part1 = gp * Z( t, r ) * A( w, p ) / d;
	float part2 = ( 1.0f - gp ) / d;

	return part1 + part2;
}


/**
 * Directional factor.
 * @param  t
 * @param  v
 * @param  vIn
 * @param  w
 * @param  r
 * @param  p
 * @return
 */
inline float D( float t, float vOut, float vIn, float w, float r, float p ) {
	float b = 4.0f * r * ( 1.0f - r );
	float a = ( r < 0.5f ) ? 0.0f : 1.0f - b;
	float c = ( r < 0.5f ) ? 1.0f - b : 0.0f;

	return ( a / M_PI + b / ( 4.0f * M_PI * vOut * vIn ) * B( t, vOut, vIn, w, r, p ) + c / vIn );
	// TODO: ( c / vIn ) probably not correct
}


/**
 * Spectral factor.
 * @param  u
 * @param  c Reflection factor.
 * @return
 */
inline float S( float u, float c ) {
	return c + ( 1.0f - c ) * pown( 1.0f - u, 5 );
}


inline float4 projection( float4 h, float4 n ) {
	return dot( h, n ) / pown( fast_length( n ), 2 ) * n;
}


inline void getValuesBRDF(
	float4 V_in, float4 V_out, float4 N, float4 T,
	float4* H, float* t, float* v, float* vIn, float* vOut, float* w
) {
	T.w = 0.0f;
	*H = fast_normalize( V_out + V_in );
	*t = fmax( dot( *H, N ), 0.0f );
	*v = fmax( dot( V_out, N ), 0.0f );
	*vIn = fmax( dot( V_in, N ), 0.0f );
	*vOut = fmax( dot( V_out, N ), 0.0f );
	*w = dot( T, projection( *H, N ) );
}
