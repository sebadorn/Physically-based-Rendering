/**
 * Get a random value.
 * @param  {const float4} scale
 * @param  {const float}  seed  Seed for the random number.
 * @return {float}              A random number.
 */
inline float random( const float4 scale, const float seed ) {
	float i;
	return fract( native_sin( dot( seed, scale ) ) * 43758.5453f + seed, &i );
}


/**
 * http://www.rorydriscoll.com/2009/01/07/better-sampling/ (Not 1:1 used here.)
 * @param  {const float}  seed
 * @param  {const float4} normal
 * @return {float4}
 */
float4 cosineWeightedDirection( const float seed, const float4 normal ) {
	const float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 0.0f ), seed );
	const float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 0.0f ), seed );
	const float r = native_sqrt( u );
	const float angle = PI_X2 * v;

	const float4 sdir = ( fabs( normal.x ) < 0.5f )
	                  ? (float4)( 0.0f, normal.z, -normal.y, 0.0f )
	                  : (float4)( -normal.z, 0.0f, normal.x, 0.0f );
	const float4 tdir = cross( normal, sdir );

	return r * native_cos( angle ) * sdir + r * native_sin( angle ) * tdir + native_sqrt( 1.0f - u ) * normal;
}


/**
 * Reflect a ray.
 * @param  {float4} dir    Direction of ray.
 * @param  {float4} normal Normal of surface.
 * @return {float4}        Reflected ray.
 */
float4 reflect( float4 dir, float4 normal ) {
	return dir - 2.0f * dot( normal, dir ) * normal;
}


/**
 * Get a vector in a random direction.
 * @param  {const float} seed Seed for the random value.
 * @return {float4}           Random vector.
 */
inline float4 uniformlyRandomDirection( const float seed ) {
	const float v = random( (float4)( 12.9898f, 78.233f, 151.7182f, 0.0f ), seed );
	const float u = random( (float4)( 63.7264f, 10.873f, 623.6736f, 0.0f ), seed );
	const float z = 1.0f - 2.0f * u;
	const float r = native_sqrt( 1.0f - z * z );
	const float angle = PI_X2 * v;

	return (float4)( r * native_cos( angle ), r * native_sin( angle ), z, 0.0f );
}


/**
 * Get a vector in a random direction.
 * @param  {const float}  seed Seed for the random value.
 * @return {float4}            Random vector.
 */
inline float4 uniformlyRandomVector( const float seed ) {
	const float rnd = random( (float4)( 36.7539f, 50.3658f, 306.2759f, 0.0f ), seed );

	return uniformlyRandomDirection( seed ) * native_sqrt( rnd );
}


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
bool intersectBoundingBox(
	const float4 origin, const float4 dir, const float4 bbMin, const float4 bbMax,
	float* tNear, float* tFar
) {
	float4 invDir = native_recip( dir );
	float4 t1 = ( bbMin - origin ) * invDir;
	float4 tMax = ( bbMax - origin ) * invDir;
	float4 tMin = fmin( t1, tMax );
	tMax = fmax( t1, tMax );

	*tNear = fmax( fmax( tMin.x, tMin.y ), tMin.z );
	*tFar = fmin( fmin( tMax.x, tMax.y ), fmin( tMax.z, *tFar ) );

	return ( *tNear <= *tFar );
}


/**
 * Basically a shortened version of udpateEntryDistanceAndExitRope().
 * @param  {const float4*} origin
 * @param  {const float4*} dir
 * @param  {const float*}  bbMin
 * @param  {const float*}  bbMax
 * @return {float}
 */
float getBoxExitLimit(
	const float4 origin, const float4 dir, const float4 bbMin, const float4 bbMax
) {
	float4 t1 = native_divide( bbMin - origin, dir );
	float4 tMax = native_divide( bbMax - origin, dir );
	tMax = fmax( t1, tMax );

	return fmin( fmin( tMax.x, tMax.y ), tMax.z );
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
	const float4 origin, const float4 dir, const float4 bbMin, const float4 bbMax,
	float* tFar, int* exitRope
) {
	const float4 invDir = native_recip( dir ); // WARNING: native_
	const bool signX = signbit( invDir.x );
	const bool signY = signbit( invDir.y );
	const bool signZ = signbit( invDir.z );

	float4 t1 = native_divide( bbMin - origin, dir );
	float4 tMax = native_divide( bbMax - origin, dir );
	tMax = fmax( t1, tMax );

	*tFar = fmin( fmin( tMax.x, tMax.y ), tMax.z );
	*exitRope = ( *tFar == tMax.y ) ? 3 - signY : 1 - signX;
	*exitRope = ( *tFar == tMax.z ) ? 5 - signZ : *exitRope;
}


/**
 * Test, if a point is inside a given axis aligned bounding box.
 * @param  {const float*}  bbMin
 * @param  {const float*}  bbMax
 * @param  {const float4*} p
 * @return {bool}
 */
inline bool isInsideBoundingBox( const float* bbMin, const float* bbMax, const float4* p ) {
	return (
		p->x >= bbMin[0] && p->y >= bbMin[1] && p->z >= bbMin[2] &&
		p->x <= bbMax[0] && p->y <= bbMax[1] && p->z <= bbMax[2]
	);
}


/**
 * Face intersection test after Möller and Trumbore.
 * @param  {const float4*} origin
 * @param  {const float4*} dir
 * @param  {const float4*} a
 * @param  {const float4*} b
 * @param  {const float4*} c
 * @param  {const float}   tNear
 * @param  {const float}   tFar
 * @param  {hit_t*}        result
 * @return {float}
 */
inline float checkFaceIntersection(
	const float4 origin, const float4 dir, const float4 a, const float4 b, const float4 c,
	const float tNear, const float tFar
) {
	const float4 edge1 = b - a;
	const float4 edge2 = c - a;
	const float4 tVec = origin - a;
	const float4 pVec = cross( dir, edge2 );
	const float4 qVec = cross( tVec, edge1 );
	const float det = dot( edge1, pVec );
	const float invDet = native_recip( det );
	float t = dot( edge2, qVec ) * invDet;

	#define MT_FINAL_T_TEST fmax( t - tFar, tNear - t ) > EPSILON || t < EPSILON

	#ifdef BACKFACE_CULLING
		const float u = dot( tVec, pVec );
		const float v = dot( dir, qVec );

		t = ( fmin( u, v ) < 0.0f || u > det || u + v > det || MT_FINAL_T_TEST ) ? -2.0f : t;
	#else
		const float u = dot( tVec, pVec ) * invDet;
		const float v = dot( dir, qVec ) * invDet;

		t = ( fmin( u, v ) < 0.0f || u > 1.0f || u + v > 1.0f || MT_FINAL_T_TEST ) ? -2.0f : t;
	#endif

	return t;
}


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
return 1.0f;
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
	float4* H, float* t, float* v, float* vIn, float* w
) {
	*H = fast_normalize( V_out + V_in );
	*t = fmax( dot( *H, N ), 0.0f );
	*v = fmax( dot( V_out, N ), 0.0f );
	*vIn = fmax( dot( V_in, N ), 0.0f );
	*w = dot( T, projection( *H, N ) );
}


inline float4 getT( float4 N ) {
	float4 T = (float4)( 0.0f );

	T.x = ( N.x == 0.0f ) ? 1.0f : 0.0f;
	T.y = ( N.y == 0.0f ) ? 1.0f : 0.0f;
	T.z = ( N.z == 0.0f ) ? 1.0f : 0.0f;

	if( T.x == 0.0f && T.y == 0.0f ) {
		T.x = -N.y;
		T.y = N.x;
		T = fast_normalize( T );
	}

	return T;
}


inline pathPoint getDefaultPathPoint() {
	pathPoint p;
	p.spdMaterial = -1;
	p.spdLight = -1;
	p.D = 0.0f;
	p.u = 1.0f;
	p.cosLaw = 0.0f;
	p.D_light = 0.0f;
	p.u_light = 1.0f;
	p.cosLaw_light = 0.0f;

	return p;
}


/**
 * Apply the cosine law for light sources.
 * @param  {float4} n Normal of the surface the light hits.
 * @param  {float4} l Normalized direction to the light source.
 * @return {float}
 */
inline float cosineLaw( float4 n, float4 l ) {
	return fmax( dot( n, l ), 0.0f );
}


inline void getRayToLight( ray4* explicitRay, ray4 ray, float4 lightTarget ) {
	explicitRay->nodeIndex = -1;
	explicitRay->faceIndex = -1;
	explicitRay->t = -2.0f;
	explicitRay->origin = fma( ray.t, ray.dir, ray.origin );
	explicitRay->dir = fast_normalize( lightTarget - explicitRay->origin );
}
