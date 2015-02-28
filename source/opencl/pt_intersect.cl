/**
 * Find intersection of a triangle and a ray. (No tessellation.)
 * @param  {const face_t*} face
 * @param  {const ray4*}   ray
 * @param  {float3*}       tuv
 * @param  {const float}   tNear
 * @param  {const float}   tFar
 * @return {float4}
 */
float3 flatTriAndRayIntersect(
	const float3 a, const float3 b, const float3 c,
	const uint4 fn, global const float4* normals,
	const ray4* ray,
	float3* tuv,
	const float tNear,
	const float tFar
) {
	const float3 edge1 = b - a;
	const float3 edge2 = c - a;
	const float3 tVec = ray->origin - a;
	const float3 pVec = cross( ray->dir, edge2 );
	const float3 qVec = cross( tVec, edge1 );
	const float invDet = native_recip( dot( edge1, pVec ) );

	tuv->x = dot( edge2, qVec ) * invDet;

	if( tuv->x < EPSILON5 || fmax( tuv->x - tFar, tNear - tuv->x ) > EPSILON5 ) {
		tuv->x = INFINITY;
		return (float3)( 0.0f );
	}

	tuv->y = dot( tVec, pVec ) * invDet;
	tuv->z = dot( ray->dir, qVec ) * invDet;
	tuv->x = ( fmin( tuv->y, tuv->z ) < 0.0f || tuv->y > 1.0f || tuv->y + tuv->z > 1.0f ) ? INFINITY : tuv->x;

	const float3 an = normals[fn.x].xyz;
	const float3 bn = normals[fn.y].xyz;
	const float3 cn = normals[fn.z].xyz;

	return getTriangleNormal( an, bn, cn, 1.0f - tuv->y - tuv->z, tuv->y, tuv->z );
}


/**
 * Face intersection test after Möller and Trumbore.
 * @param  {const ray4*}   ray
 * @param  {const face_t}  face
 * @param  {float4*}       tuv
 * @param  {const float}   tNear
 * @param  {const float}   tFar
 * @return {float3}
 */
float3 checkFaceIntersection(
	const ray4* ray,
	const float3 a, const float3 b, const float3 c,
	const uint4 fn, global const float4* normals,
	float3* tuv,
	const float tNear, const float tFar
) {
	#if PHONGTESS == 1

		const float3 an = normals[fn.x].xyz;
		const float3 bn = normals[fn.y].xyz;
		const float3 cn = normals[fn.z].xyz;
		const int3 cmp = ( an == bn ) + ( bn == cn );

		// Comparing vectors in OpenCL: 0/false/not equal; -1/true/equal
		if( cmp.x + cmp.y + cmp.z == -6 )

	#endif

	{
		return flatTriAndRayIntersect( a, b, c, fn, normals, ray, tuv, tNear, tFar );
	}


	// Phong Tessellation
	// Based on: "Direct Ray Tracing of Phong Tessellation" by Shinji Ogaki, Yusuke Tokuyoshi
	#if PHONGTESS == 1

		return phongTessTriAndRayIntersect( a, b, c, an, bn, cn, ray, tuv, tNear, tFar );

	#endif
}


/**
 * Based on: "An Efficient and Robust Ray–Box Intersection Algorithm", Williams et al.
 * @param  {const ray4*}   ray
 * @param  {const float4*} invDir
 * @param  {const float*}  bbMin
 * @param  {const float*}  bbMax
 * @param  {float*}        tNear
 * @param  {float*}        tFar
 * @return {const bool}          True, if ray intersects box, false otherwise.
 */
const bool intersectBox(
	const ray4* ray, const float3* invDir,
	const float4 bbMin, const float4 bbMax,
	float* tNear, float* tFar
) {
	const float3 t1 = ( bbMin.xyz - ray->origin ) * (*invDir);
	float3 tMax = ( bbMax.xyz - ray->origin ) * (*invDir);
	const float3 tMin = fmin( t1, tMax );
	tMax = fmax( t1, tMax );

	*tNear = fmax( fmax( tMin.x, tMin.y ), tMin.z );
	*tFar = fmin( fmin( tMax.x, tMax.y ), fmin( tMax.z, *tFar ) );

	return ( *tNear <= *tFar );
}


/**
 * Calculate intersection of a ray with a sphere.
 * @param  {const ray4*}  ray          The ray.
 * @param  {const float4} sphereCenter Center of the sphere.
 * @param  {const float}  radius       Radius of the sphere.
 * @param  {float*}       tNear        <t> near of the intersection (ray entering the sphere).
 * @param  {float*}       tFar         <t> far of the intersection (ray leaving the sphere).
 * @return {const bool}                True, if ray intersects sphere, false otherwise.
 */
const bool intersectSphere(
	const ray4* ray, const float3 sphereCenter, const float radius, float* tNear, float* tFar
) {
	const float3 op = sphereCenter - ray->origin;
	const float b = dot( op, ray->dir );
	float det = b * b - dot( op, op ) + radius * radius;

	if( det < 0.0f ) {
		return false;
	}

	det = native_sqrt( det );
	*tNear = b - det;
	*tFar = b + det;

	return ( fmax( *tNear, *tFar ) > 0.0f );
}
