/**
 * New direction for (perfectly) diffuse surfaces.
 * (Well, depening on the given parameters.)
 * @param  nl   Normal (unit vector).
 * @param  phi
 * @param  sina
 * @param  cosa
 * @return
 */
float4 jitter(
	const float4 nl, const float phi, const float sina, const float cosa
) {
	const float4 u = fast_normalize( cross( nl.yzxw, nl ) );
	const float4 v = fast_normalize( cross( nl, u ) );

	return fast_normalize( fast_normalize( u * native_cos( phi ) + v * native_sin( phi ) ) * sina + nl * cosa );
}


/**
 * MACRO: Reflect a ray.
 * @param  {float4} dir    Direction of ray.
 * @param  {float4} normal Normal of surface.
 * @return {float4}        Reflected ray.
 */
#define reflect( dir, normal ) ( ( dir ) - 2.0f * dot( ( normal ).xyz, ( dir ).xyz ) * ( normal ) )


inline bool extendDepth( material* mtl, float* seed ) {
	#if BRDF == 2
		return ( fmax( mtl->nu, mtl->nv ) >= 50.0f );
	#else
		return ( mtl->rough < rand( seed ) );
	#endif
}


/**
 * Get the a new direction for a ray hitting a transparent surface (glass etc.).
 * @param  {const ray4*}     currentRay The current ray.
 * @param  {const material*} mtl        Material of the hit surface.
 * @param  {float*}          seed       Seed for the random number generator.
 * @return {float4}                     A new direction for the ray.
 */
float4 refract( const ray4* currentRay, const material* mtl, float* seed ) {
	const bool into = ( dot( currentRay->normal.xyz, currentRay->dir.xyz ) < 0.0f );
	const float4 nl = into ? currentRay->normal : -currentRay->normal;

	const float m1 = into ? NI_AIR : mtl->Ni;
	const float m2 = into ? mtl->Ni : NI_AIR;
	const float m = native_divide( m1, m2 );

	const float cosI = -dot( currentRay->dir.xyz, nl.xyz );
	const float sinT2 = m * m * ( 1.0f - cosI * cosI );

	// Critical angle. Total internal reflection.
	if( sinT2 > 1.0f ) {
		return reflect( currentRay->dir, nl );
	}

	const float cosT = native_sqrt( 1.0f - sinT2 );
	const float4 tDir = m * currentRay->dir + ( m * cosI - cosT ) * nl;


	// Reflectance and transmission

	float r0 = native_divide( m1 - m2, m1 + m2 );
	r0 *= r0;
	const float c = ( m1 > m2 ) ? native_sqrt( 1.0f - sinT2 ) : cosI;
	const float reflectance = fresnel( c, r0 );
	// transmission = 1.0f - reflectance

	return ( reflectance < rand( seed ) ) ? tDir : reflect( currentRay->dir, nl );
}


/**
 * Calculate the new ray depending on the current one and the hit surface.
 * @param  {const ray4}     currentRay  The current ray
 * @param  {const material} mtl         Material of the hit surface.
 * @param  {float*}         seed        Seed for the random number generator.
 * @param  {bool*}          addDepth    Flag.
 * @return {ray4}                       The new ray.
 */
ray4 getNewRay(
	const ray4 currentRay, const material* mtl, float* seed, bool* addDepth
) {
	ray4 newRay;
	newRay.t = -2.0f;
	newRay.origin = fma( currentRay.t, currentRay.dir, currentRay.origin );

	#define DIR ( currentRay.dir )
	#define NORMAL ( currentRay.normal )
	#define ISOTROPY ( mtl->scratch.w )
	#define ROUGHNESS ( mtl->rough )


	// Transparency and refraction
	bool doTransRefr = ( mtl->d < 1.0f && mtl->d <= rand( seed ) );

	*addDepth = ( *addDepth || doTransRefr );

	if( doTransRefr ) {
		newRay.dir = refract( &currentRay, mtl, seed );
		newRay.dir = fast_normalize( newRay.dir );

		return newRay;
	}


	// BRDF: Not much of any. Supports specular, diffuse, and glossy surfaces.
	#if BRDF == 0

		const float rnd2 = rand( seed );
		newRay.dir = reflect( DIR, NORMAL ) * ( 1.0f - ROUGHNESS ) +
		             jitter( NORMAL, PI_X2 * rand( seed ), native_sqrt( rnd2 ), native_sqrt( 1.0f - rnd2 ) ) * ROUGHNESS;
		newRay.dir = fast_normalize( newRay.dir );

		return newRay;

	// BRDF: Schlick. Supports specular, diffuse, glossy, and anisotropic surfaces.
	// FIXME: Looks wrong.
	#elif BRDF == 1

		float a = rand( seed );
		float b = rand( seed );
		float iso2 = ISOTROPY * ISOTROPY;
		float alpha = acos( native_sqrt( native_divide( a, ROUGHNESS - a * ROUGHNESS + a ) ) );
		float phi;

		if( ROUGHNESS > 0.5f ) {
			newRay.dir = jitter( NORMAL, PI_X2 * a * 2.0f, native_sqrt( b ), native_sqrt( 1.0f - b ) );

			return newRay;
		}

		if( b < 0.25f ) {
			b = 1.0f - 4.0f * ( 0.25f - b );
			const float b2 = b * b;
			phi = M_PI_2 * native_sqrt( native_divide( iso2 * b2, 1.0f - b2 + b2 * iso2 ) );
		}
		else if( b < 0.5f ) {
			b = 1.0f - 4.0f * ( 0.5f - b );
			const float b2 = b * b;
			phi = M_PI_2 * native_sqrt( native_divide( iso2 * b2, 1.0f - b2 + b2 * iso2 ) );
			phi = M_PI - phi;
		}
		else if( b < 0.75f ) {
			b = 1.0f - 4.0f * ( 0.75f - b );
			const float b2 = b * b;
			phi = M_PI_2 * native_sqrt( native_divide( iso2 * b2, 1.0f - b2 + b2 * iso2 ) );
			phi = M_PI + phi;
		}
		else {
			b = 1.0f - 4.0f * ( 1.0f - b );
			const float b2 = b * b;
			phi = M_PI_2 * native_sqrt( native_divide( iso2 * b2, 1.0f - b2 + b2 * iso2 ) );
			phi = 2.0f * M_PI - phi;
		}

		if( ISOTROPY < 1.0f ) {
			phi += M_PI_2;
		}

		float4 H = jitter( NORMAL, phi, native_sin( alpha ), native_cos( alpha ) );
		newRay.dir = reflect( DIR, H );

		// const float4 specDir = jitter( reflect( DIR, NORMAL ), phi, native_sin( alpha ), native_cos( alpha ) );
		// const float4 diffDir = jitter( NORMAL, 2.0f * M_PI * a, native_sqrt( b ), native_sqrt( 1.0f - b ) );

		// newRay.dir = specDir;// * ( 1.0f - ROUGHNESS ) + diffDir * ROUGHNESS;

		// const float4 h = jitter( NORMAL, phi, native_sin( alpha ), native_cos( alpha ) );
		// float t = dot( h.xyz, NORMAL.xyz );
		// float vOut = dot( -DIR.xyz, NORMAL.xyz );
		// float vIn = fmax( dot( newRay.dir.xyz, NORMAL.xyz ), 0.0f );
		// const float u = fmax( dot( h.xyz, -DIR.xyz ), 0.0f );
		// const float brdf = D( t, vOut, vIn, 0.0f, ROUGHNESS, ISOTROPY ) * lambert( NORMAL, newRay.dir );

		return newRay;

	#elif BRDF == 2

		// Just do it perfectly specular at such a high and identical lobe values
		if( mtl->nu == mtl->nv && mtl->nu >= 100000.0f ) {
			newRay.dir = reflect( DIR, NORMAL );

			return newRay;
		}

		float a = rand( seed );
		float b = rand( seed );
		float phi_flip;
		float phi_flipf;

		if( a < 0.25f ) {
			a = 1.0f - 4.0f * ( 0.25f - a );
			phi_flip = 0.0f;
			phi_flipf = 1.0f;
		}
		else if( a < 0.5f ) {
			a = 1.0f - 4.0f * ( 0.5f - a );
			phi_flip = M_PI;
			phi_flipf = -1.0f;
		}
		else if( a < 0.75f ) {
			a = 1.0f - 4.0f * ( 0.75f - a );
			phi_flip = M_PI;
			phi_flipf = 1.0f;
		}
		else {
			a = 1.0f - 4.0f * ( 1.0f - a );
			phi_flip = 2.0f * M_PI;
			phi_flipf = -1.0f;
		}

		float phi = atan(
			native_sqrt( native_divide( mtl->nu + 1.0f, mtl->nv + 1.0f ) ) *
			native_tan( M_PI * a * 0.5f )
		);
		float phi_full = phi_flip + phi_flipf * phi;

		float cosphi = native_cos( phi );
		float sinphi = native_sin( phi );
		float theta_e = native_recip( mtl->nu * cosphi * cosphi + mtl->nv * sinphi * sinphi + 1.0f );
		float theta = acos( pow( 1.0f - b, theta_e ) );

		float4 h = jitter( NORMAL, phi_full, native_sin( theta ), native_cos( theta ) );
		float4 spec = reflect( DIR, h );
		float4 diff = jitter( NORMAL, 2.0f * M_PI * rand( seed ), native_sqrt( b ), native_sqrt( 1.0f - b ) );

		newRay.dir = spec;

		// New ray direction points under the hemisphere.
		// Use a cosine-weighted sample instead.
		if( dot( spec.xyz, NORMAL.xyz ) < 0.0f ) {
			newRay.dir = diff;
		}

		// float ph = native_sqrt( ( mtl->nu + 1.0f ) * ( mtl->nv + 1.0f ) ) * 0.5f * M_1_PI;
		// ph *= pow( dot( NORMAL.xyz, h.xyz ), mtl->nu * cosphi * cosphi + mtl->nv * sinphi * sinphi );
		// float pk2 = ph * native_recip( 4.0f * dot( spec.xyz, h.xyz ) );

		return newRay;

	#endif

	#undef DIR
	#undef ISOTROPY
	#undef NORMAL
	#undef ROUGHNESS
}


/**
 * Face intersection test after Möller and Trumbore.
 * @param  {const ray4*}   ray
 * @param  {const face_t}  face
 * @param  {float4*}       tuv
 * @param  {const float}   tNear
 * @param  {const float}   tFar
 */
void checkFaceIntersection(
	const ray4* ray, const face_t face, float3* tuv, const float tNear, const float tFar
) {
	const float3 edge1 = face.b.xyz - face.a.xyz;
	const float3 edge2 = face.c.xyz - face.a.xyz;
	const float3 tVec = ray->origin.xyz - face.a.xyz;
	const float3 pVec = cross( ray->dir.xyz, edge2 );
	const float3 qVec = cross( tVec, edge1 );
	const float invDet = native_recip( dot( edge1, pVec ) );

	tuv->x = dot( edge2, qVec ) * invDet;

	if( tuv->x < EPSILON || fmax( tuv->x - tFar, tNear - tuv->x ) > EPSILON ) {
		tuv->x = -2.0f;
		return;
	}

	tuv->y = dot( tVec, pVec ) * invDet;
	tuv->z = dot( ray->dir.xyz, qVec ) * invDet;
	tuv->x = ( fmin( tuv->y, tuv->z ) < 0.0f || tuv->y > 1.0f || tuv->y + tuv->z > 1.0f ) ? -2.0f : tuv->x;
}


/**
 * Test faces of the given node for intersections with the given ray.
 * @param {ray4*}                ray
 * @param {const bvhNode*}       node
 * @param {const global face_t*} faces
 * @param {float tNear}          tNear
 * @param {float tFar}           tFar
 */
void intersectFaces(
	ray4* ray, const bvhNode* node, const global face_t* faces, float tNear, float tFar
) {
	float3 tuv;

	// At most 4 faces in a leaf node.
	// Unrolled the loop as optimization.


	// Face 1

	checkFaceIntersection( ray, faces[node->faces.x], &tuv, tNear, tFar );

	if( tuv.x > -1.0f ) {
		tFar = tuv.x;

		if( ray->t > tuv.x || ray->t < -1.0f ) {
			ray->normal = getTriangleNormal( faces[node->faces.x], tuv );
			ray->normal.w = (float) node->faces.x;
			ray->t = tuv.x;
		}
	}


	// Face 2

	if( node->faces.y == -1 ) {
		return;
	}

	checkFaceIntersection( ray, faces[node->faces.y], &tuv, tNear, tFar );

	if( tuv.x > -1.0f ) {
		tFar = tuv.x;

		if( ray->t > tuv.x || ray->t < -1.0f ) {
			ray->normal = getTriangleNormal( faces[node->faces.y], tuv );
			ray->normal.w = (float) node->faces.y;
			ray->t = tuv.x;
		}
	}


	// Face 3

	if( node->faces.z == -1 ) {
		return;
	}

	checkFaceIntersection( ray, faces[node->faces.z], &tuv, tNear, tFar );

	if( tuv.x > -1.0f ) {
		tFar = tuv.x;

		if( ray->t > tuv.x || ray->t < -1.0f ) {
			ray->normal = getTriangleNormal( faces[node->faces.z], tuv );
			ray->normal.w = (float) node->faces.z;
			ray->t = tuv.x;
		}
	}


	// Face 4

	if( node->faces.w == -1 ) {
		return;
	}

	checkFaceIntersection( ray, faces[node->faces.w], &tuv, tNear, tFar );

	if( tuv.x > -1.0f ) {
		tFar = tuv.x;

		if( ray->t > tuv.x || ray->t < -1.0f ) {
			ray->normal = getTriangleNormal( faces[node->faces.w], tuv );
			ray->normal.w = (float) node->faces.w;
			ray->t = tuv.x;
		}
	}
}


/**
 * Based on: "An Efficient and Robust Ray–Box Intersection Algorithm", Williams et al.
 * @param  {const ray4*}   ray
 * @param  {const float*}  bbMin
 * @param  {const float*}  bbMax
 * @param  {float*}        tNear
 * @param  {float*}        tFar
 * @return {const bool}          True, if ray intersects box, false otherwise.
 */
const bool intersectBox(
	const ray4* ray, const float3 invDir, const float4 bbMin, const float4 bbMax, float* tNear, float* tFar
) {
	const float3 t1 = ( bbMin.xyz - ray->origin.xyz ) * invDir;
	float3 tMax = ( bbMax.xyz - ray->origin.xyz ) * invDir;
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
	const ray4* ray, const float4 sphereCenter, const float radius, float* tNear, float* tFar
) {
	const float3 op = sphereCenter.xyz - ray->origin.xyz;
	const float b = dot( op, ray->dir.xyz );
	float det = b * b - dot( op, op ) + radius * radius;

	if( det < 0.0f ) {
		return false;
	}

	det = native_sqrt( det );
	*tNear = b - det;
	*tFar = b + det;

	return ( fmax( *tNear, *tFar ) > EPSILON );
}


/**
 * Traverse the BVH and test the kD-trees against the given ray.
 * @param {const global bvhNode*} bvh
 * @param {ray4*}                 ray
 * @param {const global face_t*}  faces
 */
void traverseBVH( const global bvhNode* bvh, ray4* ray, const global face_t* faces ) {
	bool addLeftToStack, addRightToStack, rightThenLeft;
	float tFarL, tFarR, tNearL, tNearR;

	uint bvhStack[BVH_STACKSIZE];
	int stackIndex = 0;
	bvhStack[stackIndex] = 0; // Node 0 is always the BVH root node

	ray4 rayBest = *ray;
	const float3 invDir = native_recip( ray->dir.xyz );

	rayBest.t = FLT_MAX;

	while( stackIndex >= 0 ) {
		bvhNode node = bvh[bvhStack[stackIndex--]];
		tNearL = -2.0f;
		tFarL = FLT_MAX;

		// Is a leaf node with faces
		if( node.faces.x > -1 ) {
			if(
				intersectBox( ray, invDir, node.bbMin, node.bbMax, &tNearL, &tFarL ) &&
				rayBest.t > tNearL
			) {
				intersectFaces( ray, &node, faces, tNearL, tFarL );
				rayBest = ( ray->t > -1.0f && ray->t < rayBest.t ) ? *ray : rayBest;
			}

			continue;
		}

		tNearR = -2.0f;
		tFarR = FLT_MAX;
		addLeftToStack = false;
		addRightToStack = false;

		#define BVH_LEFTCHILD bvh[(int) node.bbMin.w]
		#define BVH_RIGHTCHILD bvh[(int) node.bbMax.w]

		// Add child nodes to stack, if hit by ray
		if(
			node.bbMin.w > 0.0f &&
			intersectBox( ray, invDir, BVH_LEFTCHILD.bbMin, BVH_LEFTCHILD.bbMax, &tNearL, &tFarL ) &&
			rayBest.t > tNearL
		) {
			addLeftToStack = true;
		}

		if(
			node.bbMax.w > 0.0f &&
			intersectBox( ray, invDir, BVH_RIGHTCHILD.bbMin, BVH_RIGHTCHILD.bbMax, &tNearR, &tFarR ) &&
			rayBest.t > tNearR
		) {
			addRightToStack = true;
		}

		#undef BVH_LEFTCHILD
		#undef BVH_RIGHTCHILD


		// The node that is pushed on the stack first will be evaluated last.
		// So the nearer one should be pushed last, because it will be popped first then.
		rightThenLeft = ( tNearR > tNearL );

		if( rightThenLeft && addRightToStack) {
			bvhStack[++stackIndex] = (int) node.bbMax.w;
		}
		if( rightThenLeft && addLeftToStack) {
			bvhStack[++stackIndex] = (int) node.bbMin.w;
		}

		if( !rightThenLeft && addLeftToStack) {
			bvhStack[++stackIndex] = (int) node.bbMin.w;
		}
		if( !rightThenLeft && addRightToStack) {
			bvhStack[++stackIndex] = (int) node.bbMax.w;
		}
	}

	*ray = ( rayBest.t != FLT_MAX ) ? rayBest : *ray;
}
