/**
 * Based on: "An Efficient and Robust Ray–Box Intersection Algorithm", Williams et al.
 * @param  {const ray4*}   ray
 * @param  {const float3*} invDir
 * @param  {const float4*} bbMin
 * @param  {const float4*} bbMax
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
 * Intersection of ray with sphere.
 * @param  {const ray4*}  ray
 * @param  {const float3} pos   Center of the sphere.
 * @param  {const float}  r     Radius of the sphere.
 * @param  {float*}       tNear
 * @param  {float*}       tFar
 * @return {const bool}
 */
const bool intersectSphere(
	ray4* ray, const float3 pos, const float r,
	float* tNear, float* tFar
) {
	float t0, t1; // solutions for t if the ray intersects

	// geometric solution
	float3 L = pos - ray->origin;
	float tca = dot( L, ray->dir );

	if( tca < 0.0f ) {
		return false;
	}

	float d2 = dot( L, L ) - tca * tca;

	if( d2 > r ) {
		return false;
	}

	float thc = native_sqrt( r - d2 );
	t0 = tca - thc;
	t1 = tca + thc;

	if( t0 > t1 ) {
		swap( &t0, &t1 );
	}

	if( t0 < 0.0f ) {
		t0 = t1; // if t0 is negative, let's use t1 instead

		if( t0 < 0.0f ) {
			return false; // both t0 and t1 are negative
		}
	}

	*tNear = t0;
	*tFar = t1;

	return true;
}


/**
 * Find intersection of a triangle and a ray. (No tessellation.)
 * After Möller and Trumbore.
 * @param  {const float3}         a
 * @param  {const float3}         b
 * @param  {const float3}         c
 * @param  {const uint4}          fn
 * @param  {global const float4*} normals
 * @param  {const ray4*}          ray
 * @param  {float*}               t
 * @return {float3}
 */
float3 flatTriAndRayIntersect(
	const float3 a, const float3 b, const float3 c,
	const uint4 fn, global const float4* normals,
	const ray4* ray, float* t
) {
	const float3 edge1 = b - a;
	const float3 edge2 = c - a;
	const float3 tVec = ray->origin - a;
	const float3 pVec = cross( ray->dir, edge2 );
	const float3 qVec = cross( tVec, edge1 );
	const float invDet = native_recip( dot( edge1, pVec ) );

	*t = dot( edge2, qVec ) * invDet;

	if( *t >= ray->t || *t < EPSILON5 ) {
		*t = INFINITY;
		return (float3)( 0.0f );
	}

	const float u = dot( tVec, pVec ) * invDet;
	const float v = dot( ray->dir, qVec ) * invDet;

	if( u + v > 1.0f || fmin( u, v ) < 0.0f ) {
		*t = INFINITY;
		return (float3)( 0.0f );
	}

	return fast_normalize( cross( edge1, edge2 ) );

	// const float3 an = normals[fn.x].xyz;
	// const float3 bn = normals[fn.y].xyz;
	// const float3 cn = normals[fn.z].xyz;

	// return getTriangleNormal( an, bn, cn, 1.0f - u - v, u, v );
}


/**
 * Face intersection test after Möller and Trumbore.
 * @param  {const Scene*} scene
 * @param  {const ray4*}  ray
 * @param  {const int}    fIndex
 * @param  {float*}       t
 * @param  {const float}  tNear
 * @param  {const float}  tFar
 * @return {float3}
 */
float3 checkFaceIntersection(
	const Scene* scene, const ray4* ray, const int fIndex, float* t,
	const float tNear, const float tFar
) {
	const face_t f = scene->faces[fIndex];
	const float3 a = scene->vertices[f.vertices.x].xyz;
	const float3 b = scene->vertices[f.vertices.y].xyz;
	const float3 c = scene->vertices[f.vertices.z].xyz;

	#if PHONGTESS == 1

		const float3 an = scene->normals[f.normals.x].xyz;
		const float3 bn = scene->normals[f.normals.y].xyz;
		const float3 cn = scene->normals[f.normals.z].xyz;
		const int3 cmp = ( an == bn ) + ( bn == cn );

		// Comparing vectors in OpenCL: 0/false/not equal; -1/true/equal
		if( cmp.x + cmp.y + cmp.z == -6 )

	#endif

	{
		return flatTriAndRayIntersect( a, b, c, f.normals, scene->normals, ray, t );
	}


	// Phong Tessellation
	// Based on: "Direct Ray Tracing of Phong Tessellation" by Shinji Ogaki, Yusuke Tokuyoshi
	#if PHONGTESS == 1

		return phongTessTriAndRayIntersect( a, b, c, an, bn, cn, ray, t, tNear, tFar );

	#endif
}