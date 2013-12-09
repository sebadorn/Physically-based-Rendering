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
inline float4 cosineWeightedDirection( const float seed, const float4 normal ) {
	const float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 0.0f ), seed );
	const float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 0.0f ), seed );
	const float r = native_sqrt( u );
	const float angle = PI_X2 * v;

	// compute basis from normal
	const float4 tmp = ( fabs( normal.x ) < 0.5f )
	                 ? (float4)( 1.0f, 0.0f, 0.0f, 0.0f )
	                 : (float4)( 0.0f, 1.0f, 0.0f, 0.0f );
	const float4 sdir = cross( normal, tmp );
	const float4 tdir = cross( normal, sdir );

	return r * native_cos( angle ) * sdir + r * native_sin( angle ) * tdir + native_sqrt( 1.0f - u ) * normal;
}


/**
 * Get a vector in a random direction.
 * @param  {const float}  seed Seed for the random value.
 * @return {float4}            Random vector.
 */
inline float4 uniformlyRandomDirection( const float seed ) {
	const float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 0.0f ), seed );
	const float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 0.0f ), seed );
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
 * Source: "An Efficient and Robust Ray–Box Intersection Algorithm", Williams et al.
 * @param  {const float4*} origin
 * @param  {const float4*} direction
 * @param  {const float*}  bbMin
 * @param  {const float*}  bbMax
 * @param  {float*}        tNear
 * @param  {float*}        tFar
 * @param  {int*}          exitRope
 * @return {bool}
 */
bool intersectBoundingBox(
	const float4* origin, const float4* direction, const float* bbMin, const float* bbMax,
	float* tNear, float* tFar, int* exitRope
) {
	const float4 invDir = native_recip( *direction ); // WARNING: native_
	const float bounds[2][3] = {
		bbMin[0], bbMin[1], bbMin[2],
		bbMax[0], bbMax[1], bbMax[2]
	};
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	const bool signX = signbit( invDir.x );
	const bool signY = signbit( invDir.y );
	const bool signZ = signbit( invDir.z );

	// X
	tmin = ( bounds[signX][0] - origin->x ) * invDir.x;
	tmax = ( bounds[1 - signX][0] - origin->x ) * invDir.x;
	// Y
	tymin = ( bounds[signY][1] - origin->y ) * invDir.y;
	tymax = ( bounds[1 - signY][1] - origin->y ) * invDir.y;

	if( tmin > tymax || tymin > tmax ) {
		return false;
	}

	// X vs. Y
	*exitRope = ( tymax < tmax ) ? 3 - signY : 1 - signX;
	tmin = fmax( tymin, tmin );
	tmax = fmin( tymax, tmax );
	// Z
	tzmin = ( bounds[signZ][2] - origin->z ) * invDir.z;
	tzmax = ( bounds[1 - signZ][2] - origin->z ) * invDir.z;

	if( tmin > tzmax || tzmin > tmax ) {
		return false;
	}

	// Z vs. previous winner
	*exitRope = ( tzmax < tmax ) ? 5 - signZ : *exitRope;
	*tNear = fmax( tzmin, tmin );
	*tFar = fmin( tzmax, tmax );

	return true;
}


/**
 *
 * @param origin
 * @param direction
 * @param bbMin
 * @param bbMax
 * @param tFar
 * @param exitRope
 */
void updateEntryDistanceAndExitRope(
	const float4* origin, const float4* direction, const float* bbMin, const float* bbMax,
	float* tFar, int* exitRope
) {
	const float4 invDir = native_recip( *direction ); // WARNING: native_
	const float bounds[2][3] = {
		bbMin[0], bbMin[1], bbMin[2],
		bbMax[0], bbMax[1], bbMax[2]
	};
	float tmax, tymax, tzmax;
	const bool signX = signbit( invDir.x );
	const bool signY = signbit( invDir.y );
	const bool signZ = signbit( invDir.z );

	// X
	tmax = ( bounds[1 - signX][0] - origin->x ) * invDir.x;
	// Y
	tymax = ( bounds[1 - signY][1] - origin->y ) * invDir.y;

	// X vs. Y
	*exitRope = ( tymax < tmax ) ? 3 - signY : 1 - signX;
	tmax = fmin( tymax, tmax );
	// Z
	tzmax = ( bounds[1 - signZ][2] - origin->z ) * invDir.z;

	// Z vs. previous winner
	*exitRope = ( tzmax < tmax ) ? 5 - signZ : *exitRope;
	*tFar = fmin( tzmax, tmax );
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
	const float4* origin, const float4* dir, const float4* a, const float4* b, const float4* c,
	const float tNear, const float tFar
) {
	const float4 edge1 = (*b) - (*a);
	const float4 edge2 = (*c) - (*a);
	const float4 tVec = (*origin) - (*a);
	const float4 qVec = cross( tVec, edge1 );
	const float4 pVec = cross( *dir, edge2 );
	const float det = dot( edge1, pVec );
	const float invDet = native_recip( det ); // WARNING: native_

	#ifdef BACKFACE_CULLING
		const float u = dot( tVec, pVec );
		const float v = dot( *dir, qVec );

		if( u < 0.0f || u > det || v < 0.0f || u + v > det ) {
			return -2.0f;
		}
	#else
		const float u = dot( tVec, pVec ) * invDet;
		const float v = dot( *dir, qVec ) * invDet;

		if( u < 0.0f || u > 1.0f || v < 0.0f || u + v > 1.0f ) {
			return -2.0f;
		}
	#endif

	const float t = dot( edge2, qVec ) * invDet;

	if( t < EPSILON || t < tNear - EPSILON || t > tFar + EPSILON ) {
		return -2.0f;
	}

	return t;
}
