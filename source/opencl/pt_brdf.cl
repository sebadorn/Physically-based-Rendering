#define DIR ( ray->dir )
#define N ( ray->normal )


#if BRDF == 0


	/**
	 * Zenith angle.
	 * @param  {const float} t
	 * @param  {const float} r Roughness factor (0: perfect specular, 1: perfect diffuse).
	 * @return {float}
	 */
	inline float Z( const float t, const float r ) {
		const float x = 1.0f + r * t * t - t * t;
		return ( x == 0.0f ) ? 0.0f : native_divide( r, x * x );
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
		const float x = p2 - p2 * w2 + w2;
		return ( x == 0.0f ) ? 0.0f : native_sqrt( native_divide( p, x ) );
	}


	/**
	 * Smith factor.
	 * @param  {const float} v
	 * @param  {const float} r Roughness factor (0: perfect specular, 1: perfect diffuse)
	 * @return {float}
	 */
	inline float G( const float v, const float r ) {
		const float x = r - r * v + v;
		return ( x == 0.0f ) ? 0.0f : native_divide( v, x );
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
		const float r, const float p
	) {
		return Z( t, r ) * A( w, p );
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
		const float r, const float p
	) {
		const float gp = G( vOut, r ) * G( vIn, r );
		const float obstructed = gp * Z( t, r ) * A( w, p );
		const float reemission = 1.0f - gp;

		return obstructed + reemission;
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

		const float lam = a * M_1_PI;
		const float ani = ( b == 0.0f || d == 0.0f )
		                ? 0.0f
		                : native_divide( b, d ) * B2( t, vOut, vIn, w, r, p );
		const float fres = ( vIn == 0.0f )
		                 ? 0.0f
		                 : native_divide( c, vIn );

		return lam + ani + fres;
	}


	/**
	 *
	 * @param  {const material*} mtl
	 * @param  {const ray4*}     rayLightOut
	 * @param  {const ray4*}     rayLightIn
	 * @param  {const float4*}   normal
	 * @param  {float*}          u
	 * @param  {float*}          pdf
	 * @return {float}
	 */
	float brdfSchlick(
		const material* mtl, const ray4* rayLightOut, const ray4* rayLightIn,
		const float4* normal, float* u, float* pdf
	) {
		#define V_IN ( rayLightIn->dir )
		#define V_OUT ( -rayLightOut->dir )

		const float3 un = fast_normalize( cross( normal->yzx, normal->xyz ) );

		const float3 h = bisect( V_OUT.xyz, V_IN.xyz );
		const float t = dot( h, normal->xyz );
		const float vIn = dot( V_IN.xyz, normal->xyz );
		const float vOut = dot( V_OUT.xyz, normal->xyz );
		// const float w = dot( un.xyz, projection( h, normal->xyz ) );
		const float3 hp = fast_normalize( cross( cross( h, normal->xyz ), normal->xyz ) );
		const float w = dot( un.xyz, hp );

		*u = dot( h, V_OUT.xyz );
		*pdf = native_divide( t, 4.0f * M_PI * dot( V_OUT.xyz, h ) );

		return D( t, vOut, vIn, w, mtl->rough, mtl->p );

		#undef V_IN
		#undef V_OUT
	}


	/**
	 *
	 * @param  {const ray4*}     ray
	 * @param  {const material*} mtl
	 * @param  {float*}          seed
	 * @return {float4}
	 */
	float4 newRaySchlick( const ray4* ray, const material* mtl, float* seed ) {
		float4 newRay;

		if( mtl->rough == 0.0f ) {
			return reflect( DIR, N );
		}

		float a = rand( seed );
		float b = rand( seed );
		float iso2 = mtl->p * mtl->p;
		float alpha = acos( native_sqrt( native_divide( a, mtl->rough - a * mtl->rough + a ) ) );
		float phi;

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

		if( mtl->p < 1.0f ) {
			phi += M_PI_2;
		}

		float4 H = jitter( N, phi, native_sin( alpha ), native_cos( alpha ) );
		newRay = reflect( DIR, H );

		if( dot( newRay.xyz, N.xyz ) <= 0.0f ) {
			newRay = jitter( N, PI_X2 * rand( seed ) * 2.0f, native_sqrt( a ), native_sqrt( 1.0f - a ) );
		}

		return newRay;
	}


#elif BRDF == 1


	/**
	 *
	 * @param  {const float}   nu
	 * @param  {const float}   nv
	 * @param  {const float}   Rs
	 * @param  {const float}   Rd
	 * @param  {const ray4*}   rayLightOut
	 * @param  {const ray4*}   rayLightIn
	 * @param  {const float4*} normal
	 * @param  {float*}        brdfSpec
	 * @param  {float*}        brdfDiff
	 * @param  {float*}        dotHK1
	 * @param  {float*}        pdf
	 * @return {float}
	 */
	float brdfShirleyAshikhmin(
		const float nu, const float nv, const float Rs, const float Rd,
		const ray4* rayLightOut, const ray4* rayLightIn, const float4* normal,
		float* brdfSpec, float* brdfDiff, float* dotHK1, float* pdf
	) {
		// Surface tangent vectors orthonormal to the surface normal
		const float3 un = fast_normalize( cross( normal->yzx, normal->xyz ) );
		const float3 vn = fast_normalize( cross( normal->xyz, un ) );

		const float3 k1 = rayLightIn->dir.xyz;   // to light
		const float3 k2 = -rayLightOut->dir.xyz; // to viewer
		const float3 h = bisect( k1, k2 );

		const float dotHU = dot( h, un );
		const float dotHV = dot( h, vn );
		const float dotHN = dot( h, normal->xyz );
		const float dotNK1 = dot( normal->xyz, k1 );
		const float dotNK2 = dot( normal->xyz, k2 );
		*dotHK1 = dot( h, k1 ); // Needed for the Fresnel term later

		// Specular
		float ps_e = nu * dotHU * dotHU + nv * dotHV * dotHV;
		ps_e = ( dotHN == 1.0f ) ? 0.0f : native_divide( ps_e, 1.0f - dotHN * dotHN );
		const float ps0 = native_sqrt( ( nu + 1.0f ) * ( nv + 1.0f ) ) * 0.125f * M_1_PI;
		const float ps1_num = pow( dotHN, ps_e );
		const float ps1 = native_divide( ps1_num, (*dotHK1) * fmax( dotNK1, dotNK2 ) );

		// Diffuse
		float pd = Rd * 0.38750768752f; // M_1_PI * native_divide( 28.0f, 23.0f );
		const float a = 1.0f - dotNK1 * 0.5f;
		const float b = 1.0f - dotNK2 * 0.5f;
		pd *= 1.0f - a * a * a * a * a;
		pd *= 1.0f - b * b * b * b * b;

		*brdfSpec = ps0 * ps1;
		*brdfDiff = pd;

		// Probability Distribution Function
		const float ph = ps0 * ps1_num;
		*pdf = native_divide( ph, (*dotHK1) );
	}


	/**
	 *
	 * @param  {const ray4*}     ray
	 * @param  {const material*} mtl
	 * @param  {float*}          seed
	 * @return {float4}
	 */
	float4 newRayShirleyAshikhmin( const ray4* ray, const material* mtl, float* seed ) {
		// // Just do it perfectly specular at such high and identical lobe values
		// if( mtl->nu == mtl->nv && mtl->nu >= 100000.0f ) {
		// 	return reflect( DIR, N );
		// }

		float a = rand( seed );
		const float b = rand( seed );
		float phi_flip = M_PI;
		float phi_flipf = 1.0f;
		float aMax = 1.0f;

		if( a < 0.25f ) {
			aMax = 0.25f;
			phi_flip = 0.0f;
		}
		else if( a < 0.5f ) {
			aMax = 0.5f;
			phi_flipf = -1.0f;
		}
		else if( a < 0.75f ) {
			aMax = 0.75f;
		}
		else {
			phi_flip = 2.0f * M_PI;
			phi_flipf = -1.0f;
		}

		a = 1.0f - 4.0f * ( aMax - a );

		const float phi = atan(
			native_sqrt( native_divide( mtl->nu + 1.0f, mtl->nv + 1.0f ) ) *
			native_tan( M_PI_2 * a )
		);
		const float phi_full = phi_flip + phi_flipf * phi;

		const float cosphi = native_cos( phi );
		const float sinphi = native_sin( phi );
		const float theta_e = native_recip( mtl->nu * cosphi * cosphi + mtl->nv * sinphi * sinphi + 1.0f );
		const float theta = acos( pow( 1.0f - b, theta_e ) );

		const float4 normal = ( mtl->d < 1.0f || dot( N.xyz, -DIR.xyz ) >= 0.0f ) ? N : -N;

		const float4 h = jitter( normal, phi_full, native_sin( theta ), native_cos( theta ) );
		const float4 spec = reflect( DIR, h );
		const float4 diff = jitter( normal, 2.0f * M_PI * rand( seed ), native_sqrt( b ), native_sqrt( 1.0f - b ) );

		// If new ray direction points under the hemisphere,
		// use a cosine-weighted sample instead.
		const float4 newRay = ( dot( spec.xyz, normal.xyz ) <= 0.0f ) ? diff : spec;

		return newRay;
	}


#endif

#undef DIR
#undef N


/**
 * Calculate the new ray depending on the current one and the hit surface.
 * @param  {const ray4}     currentRay  The current ray
 * @param  {const material} mtl         Material of the hit surface.
 * @param  {float*}         seed        Seed for the random number generator.
 * @param  {bool*}          addDepth    Flag.
 * @return {ray4}                       The new ray.
 */
ray4 getNewRay(
	const ray4* ray, const material* mtl, float* seed, bool* addDepth
) {
	ray4 newRay;
	newRay.t = INFINITY;
	newRay.origin = fma( ray->t, ray->dir, ray->origin );
	// newRay.origin += ray->normal * EPSILON7;

	// Transparency and refraction
	bool doTransRefr = ( mtl->d < 1.0f && mtl->d <= rand( seed ) );

	*addDepth = ( *addDepth || doTransRefr );

	if( doTransRefr ) {
		newRay.dir = refract( ray, mtl, seed );
	}
	else {

		#if BRDF == 0

			// BRDF: Schlick.
			// Supports specular, diffuse, glossy, and anisotropic surfaces.
			newRay.dir = newRaySchlick( ray, mtl, seed );

		#elif BRDF == 1

			// BRDF: Shirley-Ashikhmin.
			// Supports specular, diffuse, glossy, and anisotropic surfaces.
			newRay.dir = newRayShirleyAshikhmin( ray, mtl, seed );

		#endif

	}

	return newRay;
}
