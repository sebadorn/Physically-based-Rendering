#define DIR ( ray->dir )
#define N ( ray->normal )
#define ISOTROPY ( mtl->scratch.w )

#if BRDF == 0


	float4 newRayNoBRDF( const ray4* ray, const material* mtl, float* seed ) {
		const float a = rand( seed );

		const float4 spec = reflect( DIR, N );
		const float4 diff = jitter( N, PI_X2 * rand( seed ), native_sqrt( a ), native_sqrt( 1.0f - a ) );
		const float4 newRay = spec * ( 1.0f - mtl->rough ) + diff * mtl->rough;

		return fast_normalize( newRay );
	}


#elif BRDF == 1


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
	 * @param  mtl
	 * @param  rayLightOut
	 * @param  rayLightIn
	 * @param  normal
	 * @param  u
	 * @return
	 */
	float brdfSchlick(
		const material* mtl, const ray4* rayLightOut, const ray4* rayLightIn,
		const float4* normal, float* u
	) {
		#define V_IN ( rayLightIn->dir )
		#define V_OUT ( -rayLightOut->dir )

		// const float4 un = groove3D( mtl->scratch, *normal );
		const float3 un = fast_normalize( cross( normal->yzx, normal->xyz ) );

		const float3 H = bisect( V_OUT.xyz, V_IN.xyz );
		const float t = dot( H, normal->xyz );
		const float vIn = dot( V_IN.xyz, normal->xyz );
		const float vOut = dot( V_OUT.xyz, normal->xyz );
		const float w = dot( un.xyz, projection( H, normal->xyz ) );

		*u = dot( H, V_OUT.xyz );
		return D( t, vOut, vIn, w, mtl->rough, ISOTROPY );

		#undef V_IN
		#undef V_OUT
	}


	float4 newRaySchlick( const ray4* ray, const material* mtl, float* seed ) {
		float4 newRay;

		if( mtl->rough == 0.0f ) {
			return reflect( DIR, N );
		}

		float a = rand( seed );
		float b = rand( seed );
		float iso2 = ISOTROPY * ISOTROPY;
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

		if( ISOTROPY < 1.0f ) {
			phi += M_PI_2;
		}

		float4 H = jitter( N, phi, native_sin( alpha ), native_cos( alpha ) );
		newRay = reflect( DIR, H );

		if( dot( newRay.xyz, N.xyz ) <= 0.0f ) {
			newRay = jitter( N, PI_X2 * rand( seed ) * 2.0f, native_sqrt( a ), native_sqrt( 1.0f - a ) );
		}

		return newRay;
	}


#elif BRDF == 2


	float brdfShirleyAshikhmin(
		const float nu, const float nv, const float Rs, const float Rd,
		const ray4* rayLightOut, const ray4* rayLightIn, const float4* normal,
		float* brdfSpec, float* brdfDiff, float* dotHK1
	) {
		// Surface tangent vectors orthonormal to the surface normal
		float3 un = fast_normalize( cross( normal->yzx, normal->xyz ) );
		float3 vn = fast_normalize( cross( normal->xyz, un ) );

		float3 k1 = rayLightIn->dir.xyz;   // to light
		float3 k2 = -rayLightOut->dir.xyz; // to viewer
		float3 h = bisect( k1, k2 );

		// Pre-compute
		float dotHU = dot( h, un );
		float dotHV = dot( h, vn );
		float dotHN = dot( h, normal->xyz );
		float dotNK1 = dot( normal->xyz, k1 );
		float dotNK2 = dot( normal->xyz, k2 );
		*dotHK1 = dot( h, k1 );

		// Specular
		float ps_e = native_divide(
			nu * dotHU * dotHU + nv * dotHV * dotHV,
			1.0f - dotHN * dotHN
		);
		float ps = native_sqrt( ( nu + 1.0f ) * ( nv + 1.0f ) ) * 0.125f * M_1_PI;
		ps *= native_divide(
			pow( dotHN, ps_e ),
			(*dotHK1) * fmax( dotNK1, dotNK2 )
		);
		// ps *= fresnel( dotHK1, Rs );

		// Diffuse
		float pd = Rd * M_1_PI * native_divide( 28.0f, 23.0f );// * ( 1.0f - Rs );
		float a = 1.0f - dotNK1 * 0.5f;
		float b = 1.0f - dotNK2 * 0.5f;
		pd *= 1.0f - a * a * a * a * a;
		pd *= 1.0f - b * b * b * b * b;

		*brdfSpec = ps;
		*brdfDiff = pd;
	}


	float4 newRayShirleyAshikhmin( const ray4* ray, const material* mtl, float* seed ) {
		float4 newRay;

		// Just do it perfectly specular at such high and identical lobe values
		if( mtl->nu == mtl->nv && mtl->nu >= 100000.0f ) {
			return reflect( DIR, N );
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

		float4 h = jitter( N, phi_full, native_sin( theta ), native_cos( theta ) );
		float4 spec = reflect( DIR, h );
		float4 diff = jitter( N, 2.0f * M_PI * rand( seed ), native_sqrt( b ), native_sqrt( 1.0f - b ) );

		// If new ray direction points under the hemisphere,
		// use a cosine-weighted sample instead.
		newRay = ( dot( spec.xyz, N.xyz ) <= 0.0f ) ? diff : spec;

		// float ph = native_sqrt( ( mtl->nu + 1.0f ) * ( mtl->nv + 1.0f ) ) * 0.5f * M_1_PI;
		// ph *= pow( dot( N.xyz, h.xyz ), mtl->nu * cosphi * cosphi + mtl->nv * sinphi * sinphi );
		// float pk2 = ph * native_recip( 4.0f * dot( spec.xyz, h.xyz ) );

		return newRay;
	}


#endif

#undef DIR
#undef N
#undef ISOTROPY
