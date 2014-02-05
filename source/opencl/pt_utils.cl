float rand( float* seed ) {
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
	const float v = rand( seed );
	const float r = native_sqrt( u );
	const float angle = PI_X2 * v;

	const float4 sdir = ( fabs( normal.x ) < 0.5f )
	                  ? (float4)( 0.0f, normal.z, -normal.y, 0.0f )
	                  : (float4)( -normal.z, 0.0f, normal.x, 0.0f );
	const float4 tdir = cross( normal, sdir );

	return r * native_cos( angle ) * sdir + r * native_sin( angle ) * tdir + native_sqrt( 1.0f - u ) * normal;
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
	float4 u = fast_normalize( cross( (float4)( d.y, d.z, d.x, 0.0f ), d ) );
	float4 v = cross( d, u );

	return fast_normalize(
		( u * native_cos( phi ) + v * native_sin( phi ) ) * sina + d * cosa
	);
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
inline float4 uniformlyRandomDirection( float* seed ) {
	const float v = rand( seed );
	const float u = rand( seed );
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
inline float4 uniformlyRandomVector( float* seed ) {
	return uniformlyRandomDirection( seed ) * native_sqrt( rand( seed ) );
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


// /**
//  *
//  * @param  t
//  * @param  r Roughness factor (0: perfect specular, 1: perfect diffuse).
//  * @return
//  */
// inline float Z( float t, float r ) {
// 	return r / pown( 1.0f + r * t * t - t * t, 2 );
// }


// /**
//  *
//  * @param  w
//  * @param  p Isotropy factor (0: perfect anisotropic, 1: perfect isotropic).
//  * @return
//  */
// inline float A( float w, float p ) {
// 	return sqrt( p / ( p * p - p * p * w * w + w * w ) );
// }


// /**
//  *
//  * @param  v
//  * @param  r Roughness factor (0: perfect specular, 1: perfect diffuse)
//  * @return
//  */
// inline float G( float v, float r ) {
// 	return v / ( r - r * v + v );
// }


// /**
//  *
//  * @param  t
//  * @param  vOut
//  * @param  vIn
//  * @param  w
//  * @param  r   Roughness factor (0: perfect specular, 1: perfect diffuse)
//  * @param  p   Isotropy factor (0: perfect anisotropic, 1: perfect isotropic).
//  * @return
//  */
// inline float B( float t, float vOut, float vIn, float w, float r, float p ) {
// 	float gp = G( vOut, r ) * G( vIn, r );
// 	float d = 4.0f * M_PI * vOut * vIn;
// 	float part1 = gp * Z( t, r ) * A( w, p ) / d;
// 	float part2 = ( 1.0f - gp ) / d;

// 	return part1 + part2;
// }


// /**
//  * Directional factor.
//  * @param  t
//  * @param  v
//  * @param  vIn
//  * @param  w
//  * @param  r
//  * @param  p
//  * @return
//  */
// inline float D( float t, float vOut, float vIn, float w, float r, float p ) {
// 	float b = 4.0f * r * ( 1.0f - r );
// 	float a = ( r < 0.5f ) ? 0.0f : 1.0f - b;
// 	float c = ( r < 0.5f ) ? 1.0f - b : 0.0f;

// 	return ( a / M_PI + b / ( 4.0f * M_PI * vOut * vIn ) * B( t, vOut, vIn, w, r, p ) + c / vIn );
// 	// TODO: ( c / vIn ) probably not correct
// }


// /**
//  * Spectral factor.
//  * @param  u
//  * @param  c Reflection factor.
//  * @return
//  */
// inline float S( float u, float c ) {
// 	return c + ( 1.0f - c ) * pown( 1.0f - u, 5 );
// }


// inline float4 projection( float4 h, float4 n ) {
// 	return dot( h, n ) / pown( fast_length( n ), 2 ) * n;
// }


// inline void getValuesBRDF(
// 	float4 V_in, float4 V_out, float4 N, float4 T,
// 	float4* H, float* t, float* v, float* vIn, float* w
// ) {
// 	*H = fast_normalize( V_out + V_in );
// 	*t = fmax( dot( *H, N ), 0.0f );
// 	*v = fmax( dot( V_out, N ), 0.0f );
// 	*vIn = fmax( dot( V_in, N ), 0.0f );
// 	*w = dot( T, projection( *H, N ) );
// }


// inline pathPoint getDefaultPathPoint() {
// 	pathPoint p;
// 	p.spdMaterial = -1;
// 	p.spdLight = -1;
// 	p.D = 0.0f;
// 	p.u = 1.0f;
// 	p.cosLaw = 0.0f;
// 	p.D_light = 0.0f;
// 	p.u_light = 1.0f;
// 	p.cosLaw_light = 0.0f;

// 	return p;
// }


/**
 * Apply the cosine law for light sources.
 * @param  {float4} n Normal of the surface the light hits.
 * @param  {float4} l Normalized direction to the light source.
 * @return {float}
 */
inline float cosineLaw( float4 n, float4 l ) {
	return fmax( dot( n, l ), 0.0f );
}


ray4 getNewRay( ray4 prevRay, float4 normal, material mtl, float* seed ) {
	ray4 newRay;
	newRay.t = -2.0f;
	newRay.nodeIndex = -1;
	newRay.origin = fma( prevRay.t, prevRay.dir, prevRay.origin );

	// Transparency and refraction
	if( mtl.d < 1.0f && mtl.d <= rand( seed ) ) {
		newRay.dir = reflect( prevRay.dir, normal );

		float a = dot( normal, prevRay.dir );
		float ddn = fabs( a );
		float nnt = ( a > 0.0f ) ? mtl.Ni / NI_AIR : NI_AIR / mtl.Ni;
		float cos2t = 1.0f - nnt * nnt * ( 1.0f - ddn * ddn );

		if( cos2t > 0.0f ) {
			float4 tdir = fast_normalize( newRay.dir * nnt + sign( a ) * normal * ( ddn * nnt + sqrt( cos2t ) ) );
			float R0 = ( mtl.Ni - NI_AIR ) * ( mtl.Ni - NI_AIR ) / ( ( mtl.Ni + NI_AIR ) * ( mtl.Ni + NI_AIR ) );
			float c = 1.0f - mix( ddn, dot( tdir, normal ), (float) a > 0.0f );
			float Re = R0 + ( 1.0f - R0 ) * pown( c, 5 );
			float P = 0.25f + 0.5f * Re;
			float RP = Re / P;
			float TP = ( 1.0f - Re ) / ( 1.0f - P );

			if( rand( seed ) >= P ) {
				newRay.dir = tdir;
			}
		}
	}
	// Specular
	else if( mtl.illum == 3 ) {
		newRay.dir = reflect( prevRay.dir, normal );
		newRay.dir += ( mtl.gloss > 0.0f )
		            ? uniformlyRandomVector( seed ) * mtl.gloss
		            : (float4)( 0.0f );
		newRay.dir = fast_normalize( newRay.dir );
	}
	// Diffuse
	else {
		// newRay.dir = cosineWeightedDirection( seed, normal );
		float rnd1 = rand( seed );
		float rnd2 = rand( seed );
		newRay.dir = jitter(
			normal, 2.0f * M_PI * rnd1,
			sqrt( rnd2 ), sqrt( 1.0f - rnd2 )
		);
	}

	return newRay;
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
