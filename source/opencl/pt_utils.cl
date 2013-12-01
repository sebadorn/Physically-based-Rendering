/**
 * Get a random value.
 * @param  {float4} scale
 * @param  {float}  seed  Seed for the random number.
 * @return {float}        A random number.
 */
inline float random( float4 scale, float seed ) {
	float i;
	return fract( sin( dot( seed, scale ) ) * 43758.5453f + seed, &i );
}


// http://www.rorydriscoll.com/2009/01/07/better-sampling/
inline float4 cosineWeightedDirection( float seed, float4 normal ) {
	float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 0.0f ), seed );
	float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 0.0f ), seed );
	float r = sqrt( u );
	float angle = PI_X2 * v;

	// compute basis from normal
	float4 sdir, tdir;
	if( fabs( normal.x ) < 0.5f ) {
		sdir = cross( normal, (float4)( 1.0f, 0.0f, 0.0f, 0.0f ) );
	} else {
		sdir = cross( normal, (float4)( 0.0f, 1.0f, 0.0f, 0.0f ) );
	}
	tdir = cross( normal, sdir );

	return r * cos( angle ) * sdir + r * sin( angle ) * tdir + sqrt( 1.0f - u ) * normal;
}


/**
 * Get a vector in a random direction.
 * @param  {float}  seed Seed for the random value.
 * @return {float4}      Random vector.
 */
inline float4 uniformlyRandomDirection( float seed ) {
	float u = random( (float4)( 12.9898f, 78.233f, 151.7182f, 0.0f ), seed );
	float v = random( (float4)( 63.7264f, 10.873f, 623.6736f, 0.0f ), seed );
	float z = 1.0f - 2.0f * u;
	float r = sqrt( 1.0f - z * z );
	float angle = PI_X2 * v;

	return (float4)( r * cos( angle ), r * sin( angle), z, 0.0f );
}


/**
 * Get a vector in a random direction.
 * @param  {float}  seed Seed for the random value.
 * @return {float4}      Random vector.
 */
inline float4 uniformlyRandomVector( float seed ) {
	float rnd = random( (float4)( 36.7539f, 50.3658f, 306.2759f, 0.0f ), seed );

	return uniformlyRandomDirection( seed ) * sqrt( rnd );
}


/**
 * Source: http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-7-intersecting-simple-shapes/ray-box-intersection/
 * Source: "An Efficient and Robust Ray–Box Intersection Algorithm", Williams et al.
 */
bool intersectBoundingBox(
	float4* origin, float4* direction, float* bbMin, float* bbMax,
	float* tNear, float* tFar, int* exitRope
) {
	float invDir[3] = {
		1.0f / direction->x,
		1.0f / direction->y,
		1.0f / direction->z
	};
	float bounds[2][3] = {
		bbMin[0], bbMin[1], bbMin[2],
		bbMax[0], bbMax[1], bbMax[2]
	};
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	bool signX = invDir[0] < 0.0f;
	bool signY = invDir[1] < 0.0f;
	bool signZ = invDir[2] < 0.0f;

	// X
	tmin = ( bounds[signX][0] - origin->x ) * invDir[0];
	tmax = ( bounds[1 - signX][0] - origin->x ) * invDir[0];
	// Y
	tymin = ( bounds[signY][1] - origin->y ) * invDir[1];
	tymax = ( bounds[1 - signY][1] - origin->y ) * invDir[1];

	if( tmin > tymax || tymin > tmax ) {
		return false;
	}

	// X vs. Y
	*exitRope = ( tymax < tmax ) ? 3 - signY : 1 - signX;
	tmin = fmax( tymin, tmin );
	tmax = fmin( tymax, tmax );
	// Z
	tzmin = ( bounds[signZ][2] - origin->z ) * invDir[2];
	tzmax = ( bounds[1 - signZ][2] - origin->z ) * invDir[2];

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
 * Test, if a point is inside a given axis aligned bounding box.
 * @param  bbMin [description]
 * @param  bbMax [description]
 * @param  p     [description]
 * @return       [description]
 */
inline bool isInsideBoundingBox( float* bbMin, float* bbMax, float4 p ) {
	return (
		p.x >= bbMin[0] && p.y >= bbMin[1] && p.z >= bbMin[2] &&
		p.x <= bbMax[0] && p.y <= bbMax[1] && p.z <= bbMax[2]
	);
}



/**
 * Face intersection test after Möller and Trumbore.
 * @param  origin [description]
 * @param  dir    [description]
 * @param  a      [description]
 * @param  b      [description]
 * @param  c      [description]
 * @param  tNear  [description]
 * @param  tFar   [description]
 * @param  result [description]
 * @return        [description]
 */
float checkFaceIntersection(
	float4* origin, float4* dir, float4 a, float4 b, float4 c,
	float tNear, float tFar, hit_t* result
) {
	float4 edge1 = b - a;
	float4 edge2 = c - a;
	float4 pVec = cross( *dir, edge2 );
	float det = dot( edge1, pVec );

#ifdef BACKFACE_CULLING

	if( det < EPSILON ) {
		return -2.0f;
	}

	float4 tVec = (*origin) - a;
	float u = dot( tVec, pVec );

	if( u < 0.0f || u > det ) {
		return -2.0f;
	}

	float4 qVec = cross( tVec, edge1 );
	float v = dot( *dir, qVec );

	if( v < 0.0f || u + v > det ) {
		return -2.0f;
	}

	float t = dot( edge2, qVec );
	float invDet = 1.0f / det;
	t *= invDet;

#else

	if( fabs( det ) < EPSILON ) {
		return -2.0f;
	}

	float4 tVec = (*origin) - a;
	float u = dot( tVec, pVec ) / det;

	if( u < 0.0f || u > 1.0f ) {
		return -2.0f;
	}

	float4 qVec = cross( tVec, edge1 );
	float v = dot( *dir, qVec ) / det;

	if( v < 0.0f || u + v > 1.0f ) {
		return -2.0f;
	}

	float t = dot( edge2, qVec ) / det;

#endif

	if( t < EPSILON || t < tNear - EPSILON || t > tFar + EPSILON ) {
		return -2.0f;
	}

	result->position = (*origin) + t * (*dir);
	result->distance = length( result->position - (*origin) );

	return t;
}
