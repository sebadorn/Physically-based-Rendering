/**
 *
 * @param  mtl
 * @param  seed
 * @return
 */
inline bool extendDepth( material* mtl, float* seed ) {
	#if BRDF == 2
		return ( fmax( mtl->nu, mtl->nv ) >= 50.0f );
	#else
		return ( mtl->rough < rand( seed ) );
	#endif
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
	const ray4* ray, const material* mtl, float* seed, bool* addDepth
) {
	ray4 newRay;
	newRay.t = -2.0f;
	// newRay.origin = fma( ray->t, ray->dir, ray->origin );
	newRay.origin = ray->hit;

	// Transparency and refraction
	bool doTransRefr = ( mtl->d < 1.0f && mtl->d <= rand( seed ) );

	*addDepth = ( *addDepth || doTransRefr );

	if( doTransRefr ) {
		newRay.dir = refract( ray, mtl, seed );
	}
	else {
		// BRDF: Not much of any.
		// Supports specular, diffuse, and glossy surfaces.
		#if BRDF == 0
			newRay.dir = newRayNoBRDF( ray, mtl, seed );
		// BRDF: Schlick.
		// Supports specular, diffuse, glossy, and anisotropic surfaces.
		#elif BRDF == 1
			newRay.dir = newRaySchlick( ray, mtl, seed );
		// BRDF: Shirley-Ashikhmin.
		// Supports specular, diffuse, glossy, and anisotropic surfaces.
		#elif BRDF == 2
			newRay.dir = newRayShirleyAshikhmin( ray, mtl, seed );
		#endif
	}

	return newRay;
}


float3 phongTessellation(
	float3 P1, float3 P2, float3 P3,
	float3 N1, float3 N2, float3 N3,
	float u, float v, float w
) {
	float3 pBary = P1*u + P2*v + P3*w;
	float3 projA = projectOnPlane( pBary, P1, N1 );
	float3 projB = projectOnPlane( pBary, P2, N2 );
	float3 projC = projectOnPlane( pBary, P3, N3 );

	float3 pTessellated = u*u*projA + v*v*projB + w*w*projC +
	                      u*v*( projectOnPlane( projB, P1, N1 ) + projectOnPlane( projA, P2, N2 ) ) +
	                      v*w*( projectOnPlane( projC, P2, N2 ) + projectOnPlane( projB, P3, N3 ) ) +
	                      w*u*( projectOnPlane( projA, P3, N3 ) + projectOnPlane( projC, P1, N1 ) );

	return pTessellated;
}


/**
 * Face intersection test after Möller and Trumbore.
 * @param  {const ray4*}   ray
 * @param  {const face_t}  face
 * @param  {float4*}       tuv
 * @param  {const float}   tNear
 * @param  {const float}   tFar
 */
float4 checkFaceIntersection(
	ray4* ray, const face_t face, float3* tuv, const float tNear, const float tFar
) {
	int4 cmpAB = ( face.an == face.bn );
	int4 cmpBC = ( face.bn == face.cn );

	if( cmpAB.x && cmpAB.y && cmpAB.z && cmpBC.x && cmpBC.y && cmpBC.z ) {
		const float3 edge1 = face.b.xyz - face.a.xyz;
		const float3 edge2 = face.c.xyz - face.a.xyz;
		const float3 tVec = ray->origin.xyz - face.a.xyz;
		const float3 pVec = cross( ray->dir.xyz, edge2 );
		const float3 qVec = cross( tVec, edge1 );
		const float invDet = native_recip( dot( edge1, pVec ) );

		tuv->x = dot( edge2, qVec ) * invDet;

		if( tuv->x < EPSILON || fmax( tuv->x - tFar, tNear - tuv->x ) > EPSILON ) {
			tuv->x = -2.0f;
			return (float4)( 0.0f );
		}

		tuv->y = dot( tVec, pVec ) * invDet;
		tuv->z = dot( ray->dir.xyz, qVec ) * invDet;
		tuv->x = ( fmin( tuv->y, tuv->z ) < 0.0f || tuv->y > 1.0f || tuv->y + tuv->z > 1.0f ) ? -2.0f : tuv->x;

		return fma( tuv->x, ray->dir, ray->origin );
	}


	// Phong Tessellation
	// Based on: "Direct Ray Tracing of Phong Tessellation" by Shinji Ogaki, Yusuke Tokuyoshi

	tuv->x = -2.0f;
	#define PT_ALPHA 1.0f

	float3 P1 = face.a.xyz;
	float3 P2 = face.b.xyz;
	float3 P3 = face.c.xyz;
	float3 N1 = face.an.xyz;
	float3 N2 = face.bn.xyz;
	float3 N3 = face.cn.xyz;

	float3 E12 = P2 - P1;
	float3 E23 = P3 - P2;
	float3 E31 = P1 - P3;

	float3 C31 = PT_ALPHA * ( dot( N1, E31 ) * N1 - dot( N3, E31 ) * N3 );
	float3 C23 = PT_ALPHA * ( dot( N3, E23 ) * N3 - dot( N2, E23 ) * N2 );
	float3 C12 = PT_ALPHA * ( dot( N2, E12 ) * N2 - dot( N1, E12 ) * N1 );

	// Planes of which the intersection is the ray
	// Hesse normal form
	float3 D1 = fast_normalize( cross( ray->origin.xyz, ray->dir.xyz ) );
	float3 D2 = fast_normalize( cross( D1, ray->dir.xyz ) );

	if( dot( ray->origin.xyz, D1 ) < 0.0f ) {
		D1 = -D1;
	}
	if( dot( ray->origin.xyz, D2 ) < 0.0f ) {
		D2 = -D2;
	}

	float O1 = -dot( ray->origin.xyz, D1 );
	float O2 = -dot( ray->origin.xyz, D2 );

	float a = dot( -D1, C31 );
	float b = dot( -D1, C23 );
	float c = dot( D1, P3 ) + O1;
	float d = dot( D1, C12 - C23 - C31 );
	float e = dot( D1, C31 + E31 );
	float f = dot( D1, C23 - E23 );
	float l = dot( -D2, C31 );
	float m = dot( -D2, C23 );
	float n = dot( D2, P3 ) + O2;
	float o = dot( D2, C12 - C23 - C31 );
	float p = dot( D2, C31 + E31 );
	float q = dot( D2, C23 - E23 );


	// Solve cubic

	float a0 = ( l*m*n + 2.0f*o*p*q ) - ( l*q*q + m*p*p + n*o*o );
	float a1 = ( a*m*n + l*b*n + l*m*c + 2.0f*( d*p*q + o*e*q + o*p*f ) ) -
	           ( a*q*q + b*p*p + c*o*o + 2.0f*( l*f*q + m*e*p + n*d*o ) );
	float a2 = ( a*b*n + a*m*c + l*b*c + 2.0f*( o*e*f + d*e*q + d*p*f ) ) -
	           ( l*f*f + m*e*e + n*d*d + 2.0f*( a*f*q + b*e*p + c*d*o ) );
	float a3 = ( a*b*c + 2.0f*d*e*f ) - ( a*f*f + b*e*e + c*d*d );

	float xs[3];
	int numCubicRoots = solveCubic( a0, a1, a2, a3, xs );

	if( 0 == numCubicRoots ) {
		return (float4)( 0.0f );
	}

	float x;
	float determinant = INFINITY;
	float A, B, C, D, E, F;

	for( int i = 0; i < numCubicRoots; i++ ) {
		A = a * xs[i] + l;
		B = b * xs[i] + m;
		D = d * xs[i] + o;

		if( determinant > D*D - A*B ) {
			determinant = D*D - A*B;
			x = xs[i];
		}
	}


	A = a * x + l;
	B = b * x + m;
	D = d * x + o;

	float u, v, w;

	if( 0.0f < determinant ) {
		C = c * x + n;
		E = e * x + p;
		F = f * x + q;

		if( fabs( A ) < fabs( B ) ) {
			A = native_divide( A, B );
			C = native_divide( C, B );
			D = native_divide( D, B );
			E = native_divide( E, B );
			F = native_divide( F, B );
			// B = 1.0f;

			float sqrtA = native_sqrt( D*D - A );
			float sqrtC = native_sqrt( F*F - C );
			float la1 = D + sqrtA;
			float la2 = D - sqrtA;
			float lc1 = F + sqrtC;
			float lc2 = F - sqrtC;

			if( fabs( 2.0f*E - la1*lc1 - la2*lc2 ) < fabs( 2.0f*E - la1*lc2 - la2*lc1 ) ) {
				float tmp = lc1;
				lc1 = lc2;
				lc2 = tmp;
			}

			for( int loop = 0; loop < 2; loop++ ) {
				float g = ( 0 == loop ) ? -la1 : -la2;
				float h = ( 0 == loop ) ? -lc1 : -lc2;

				// Solve quadratic function: c0*u*u + c1*u + c2 = 0
				float c0 = a + g*( 2.0f*d + b*g );
				float c1 = 2.0f*( h*( d + b*g ) + e + f*g );
				float c2 = h*( b*h + 2.0f*f ) + c;

				if( 0.0f < c0 && ( ( 0.0f < c1 && 0.0f < c2 ) || ( 0.0f > c2 && 0.0f > c0 + c1 + c2 ) ) ) {
					continue;
				}
				if( 0.0f > c0 && ( ( 0.0f > c1 && 0.0f > c2 ) || ( 0.0f < c2 && 0.0f < c0 + c1 + c2 ) ) ) {
					continue;
				}

				if( 0.0000001f > fabs( c0 ) ) {
					if( 0.0000001f < fabs( c1 ) ) {
						u = native_divide( -c2, c1 );
						v = g*u + h;
						w = 1.0f - u - v;

						if( 0.0f <= u && 0.0 <= v && 0.0 <= w ) {
							float3 pTessellated = phongTessellation( P1, P2, P3, N1, N2, N3, u, v, w );
							float3 test = pTessellated - ray->origin.xyz;

							if( test.x / ray->dir.x >= 0.0f && test.y / ray->dir.y >= 0.0f && test.z / ray->dir.z >= 0.0f ) {
								tuv->x = fast_length( test );
								tuv->y = u;
								tuv->z = v;

								return (float4)( pTessellated, 0.0f );
							}
						}
					}
				}
				else {
					float tmp0, tmp1;
					float discriminant = c1 * c1 - 4.0f * c0 * c2;

					if( 0.0f <= discriminant ) {
						solveQuadratic( c0, c1, c2, &tmp0, &tmp1 );

						// Line 1
						u = tmp0;
						v = g * u + h;
						w = 1.0f - u - v;

						if( 0.0f <= u && 0.0f <= v && 0.0f <= w ) {
							float3 pTessellated = phongTessellation( P1, P2, P3, N1, N2, N3, u, v, w );
							float3 test = pTessellated - ray->origin.xyz;

							if( test.x / ray->dir.x >= 0.0f && test.y / ray->dir.y >= 0.0f && test.z / ray->dir.z >= 0.0f ) {
								tuv->x = fast_length( test );
								tuv->y = u;
								tuv->z = v;

								return (float4)( pTessellated, 0.0f );
							}
						}

						// Line 2
						u = tmp1;
						v = g * u + h;
						w = 1.0f - u - v;

						if( 0.0f <= u && 0.0f <= v && 0.0f <= w ) {
							float3 pTessellated = phongTessellation( P1, P2, P3, N1, N2, N3, u, v, w );
							float3 test = pTessellated - ray->origin.xyz;

							if( test.x / ray->dir.x >= 0.0f && test.y / ray->dir.y >= 0.0f && test.z / ray->dir.z >= 0.0f ) {
								tuv->x = fast_length( test );
								tuv->y = u;
								tuv->z = v;

								return (float4)( pTessellated, 0.0f );
							}
						}
					}
				}
			}
		}
		else {
			B = native_divide( B, A );
			C = native_divide( C, A );
			D = native_divide( D, A );
			E = native_divide( E, A );
			F = native_divide( F, A );
			// A = 1.0f;

			float sqrtB = native_sqrt( D * D - B );
			float sqrtC = native_sqrt( E * E - C );
			float lb1 = D + sqrtB;
			float lb2 = D - sqrtB;
			float lc1 = E + sqrtC;
			float lc2 = E - sqrtC;

			if( fabs( 2.0f*F - lb1*lc1 - lb2*lc2 ) < fabs( 2.0f*F - lb1*lc2 - lb2*lc1 ) ) {
				float tmp = lc1;
				lc1 = lc2;
				lc2 = tmp;
			}

			for( int loop = 0; loop < 2; loop++ ) {
				float g = ( 0 == loop ) ? -lb1 : -lb2;
				float h = ( 0 == loop ) ? -lc1 : -lc2;

				float c0 = b + g*( 2.0f*d + a*g );
				float c1 = 2.0f*( h*( d + a*g ) + f + e*g );
				float c2 = h*( a*h + 2.0f*e ) + c;

				if( 0.0f < c0 && ( ( 0.0f < c1 && 0.0f < c2 ) || ( 0.0f > c2 && 0.0f > c0 + c1 + c2 ) ) ) {
					continue;
				}
				if( 0.0f > c0 && ( ( 0.0f > c1 && 0.0f > c2 ) || ( 0.0f < c2 && 0.0f < c0 + c1 + c2 ) ) ) {
					continue;
				}

				if( 0.0000001f > fabs( c0 ) ) {
					if( 0.0000001f < fabs( c1 ) ) {
						v = native_divide( -c2, c1 );
						u = g*v + h;
						w = 1.0f - u - v;

						if( 0.0f <= u && 0.0 <= v && 0.0 <= w ) {
							float3 pTessellated = phongTessellation( P1, P2, P3, N1, N2, N3, u, v, w );
							float3 test = pTessellated - ray->origin.xyz;

							if( test.x / ray->dir.x >= 0.0f && test.y / ray->dir.y >= 0.0f && test.z / ray->dir.z >= 0.0f ) {
								tuv->x = fast_length( test );
								tuv->y = u;
								tuv->z = v;

								return (float4)( pTessellated, 0.0f );
							}
						}
					}
				}
				else {
					float tmp0, tmp1;
					float discriminant = c1 * c1 - 4.0f * c0 * c2;

					if( 0.0f <= discriminant ) {
						solveQuadratic( c0, c1, c2, &tmp0, &tmp1 );

						// Line 1
						v = tmp0;
						u = g * v + h;
						w = 1.0f - u - v;

						if( 0.0f <= u && 0.0f <= v && 0.0f <= w ) {
							float3 pTessellated = phongTessellation( P1, P2, P3, N1, N2, N3, u, v, w );
							float3 test = pTessellated - ray->origin.xyz;

							if( test.x / ray->dir.x >= 0.0f && test.y / ray->dir.y >= 0.0f && test.z / ray->dir.z >= 0.0f ) {
								tuv->x = fast_length( test );
								tuv->y = u;
								tuv->z = v;

								return (float4)( pTessellated, 0.0f );
							}
						}

						// Line 2
						v = tmp1;
						u = g * v + h;
						w = 1.0f - u - v;

						if( 0.0f <= u && 0.0f <= v && 0.0f <= w ) {
							float3 pTessellated = phongTessellation( P1, P2, P3, N1, N2, N3, u, v, w );
							float3 test = pTessellated - ray->origin.xyz;

							if( test.x / ray->dir.x >= 0.0f && test.y / ray->dir.y >= 0.0f && test.z / ray->dir.z >= 0.0f ) {
								tuv->x = fast_length( test );
								tuv->y = u;
								tuv->z = v;

								return (float4)( pTessellated, 0.0f );
							}
						}
					}
				}
			}
		}
	}

	return (float4)( 0.0f );


	// float a4 = a*b*o*o + a*a*m*m + d*d*l*m + b*b*l*l - a*d*m*o - b*d*l*o - 2.0f*a*b*l*m;
	// float a3 = b*e*o*o + d*d*m*p - a*d*m*q - b*d*l*q - b*d*o*p - a*f*m*o - d*e*m*o - b*f*l*o +
	//            2.0f*( a*e*m*m + b*b*l*p + a*b*o*q + d*f*l*m - a*b*m*p - b*e*l*m );
	// float a2 = a*b*q*q + f*f*l*m + b*c*o*o + d*d*m*n + b*b*p*p + e*e*m*m - b*f*o*p - b*d*n*o -
	//            e*f*m*o - c*d*m*o - b*d*p*q - a*f*m*q - d*e*m*q - b*f*l*q +
	//            2.0f*( b*b*l*n + a*c*m*m + b*e*o*q + d*f*m*p - b*e*m*p - a*b*m*n - b*c*l*m );
	// float a1 = b*e*q*q + f*f*m*p - b*f*p*q - b*d*n*q - e*f*m*q - c*d*m*q - b*f*n*o - c*f*m*o +
	//            2.0f*( c*e*m*m + b*b*n*p + b*c*o*q + d*f*m*n - b*c*m*p - b*e*m*n );
	// float a0 = b*c*q*q + b*b*n*n + f*f*m*n + c*c*m*m - b*f*n*q - c*f*m*q - 2.0f*b*c*m*n;

	// if( a4 == 0.0f ) {
	// 	return (float4)( 0.0f );
	// }


	// // Solve quartic function: 0 = a4 * u^4 + a3 * u^3 + a2 * u^2 + a1 * u + a0
	// float delta0 = a2*a2 - 3.0f*a3*a1 + 12.0f*a4*a0;
	// float delta1 = 2.0f*a2*a2*a2 - 9.0f*a3*a2*a1 + 27.0f*a3*a3*a0 + 27.0f*a4*a1*a1 - 72.0f*a4*a2*a0;
	// float delta = native_divide( delta1*delta1 - 4.0f*delta0*delta0*delta0, -27.0f );

	// float pq_p = native_divide( 8.0f*a4*a2 - 3.0f*a3*a3, 8.0f*a4*a4 );
	// float pq_q = native_divide( a3*a3*a3 - 4.0f*a4*a3*a2 + 8.0f*a4*a4*a1, 8.0f*a4*a4*a4 );

	// float Q, S;
	// if( delta > 0.0f ) {
	// 	float phi = acos( native_divide( delta1, 2.0f*native_sqrt( delta0*delta0*delta0 ) ) );
	// 	S = 0.5f * native_sqrt(
	// 		-native_divide( 2.0f*pq_p, 3.0f ) + native_divide( 2.0f, 3.0f*a4 ) * native_sqrt( delta0 ) *
	// 		native_cos( native_divide( phi, 3.0f ) )
	// 	);
	// }
	// else {
	// 	if( delta != 0.0f && delta0 == 0.0f ) {
	// 		Q = cbrt( delta1 );
	// 	}
	// 	else {
	// 		Q = cbrt( 0.5f * ( delta1 + native_sqrt( delta1*delta1 - 4.0f*delta0*delta0*delta0 ) ) );
	// 	}
	// 	S = 0.5f * native_sqrt(
	// 		-native_divide( 2.0f * pq_p, 3.0f ) +
	// 		native_recip( 3.0f * a4 ) * ( Q + native_divide( delta0, Q ) )
	// 	);
	// }

	// if( S == 0.0f ) {
	// 	return (float4)( 0.0f );
	// }

	// float part_1 = -native_divide( a3, 4.0f*a4 );
	// float sqrt_part12 = -4.0f*S*S - 2.0f*pq_p + native_divide( pq_q, S );
	// float sqrt_part34 = -4.0f*S*S - 2.0f*pq_p - native_divide( pq_q, S );
	// float part12_2 = 0.5f * native_sqrt( sqrt_part12 );
	// float part34_2 = 0.5f * native_sqrt( sqrt_part34 );

	// float u1 = part_1 - S + part12_2;
	// float u2 = part_1 - S - part12_2;
	// float u3 = part_1 + S + part34_2;
	// float u4 = part_1 + S - part34_2;

	// // float u = x1;
	// // if( sqrt_part12 < 0.0f || u < 0.0f || u > 1.0f ) { u = x2; }
	// // if( sqrt_part12 < 0.0f || u < 0.0f || u > 1.0f ) { u = x3; }
	// // if( sqrt_part34 < 0.0f || u < 0.0f || u > 1.0f ) { u = x4; }
	// // if( sqrt_part34 < 0.0f || u < 0.0f || u > 1.0f ) { return (float4)( 0.0f ); }

	// float v1 = -1.0f, v2 = -1.0f, v3 = -1.0f, v4 = -1.0f, v5 = -1.0f, v6 = -1.0f, v7 = -1.0f, v8 = -1.0f;
	// if( sqrt_part12 >= 0.0f && u1 >= 0.0f && u1 <= 1.0f ) {
	// 	v1 = pq( a, b, c, d, e, f, u1, true );
	// 	v2 = pq( a, b, c, d, e, f, u1, false );
	// }
	// if( sqrt_part12 >= 0.0f && u2 >= 0.0f && u2 <= 1.0f ) {
	// 	v3 = pq( a, b, c, d, e, f, u2, true );
	// 	v4 = pq( a, b, c, d, e, f, u2, false );
	// }
	// if( sqrt_part34 >= 0.0f && u3 >= 0.0f && u3 <= 1.0f ) {
	// 	v5 = pq( a, b, c, d, e, f, u3, true );
	// 	v6 = pq( a, b, c, d, e, f, u3, false );
	// }
	// if( sqrt_part34 >= 0.0f && u4 >= 0.0f && u4 <= 1.0f ) {
	// 	v7 = pq( a, b, c, d, e, f, u4, true );
	// 	v8 = pq( a, b, c, d, e, f, u4, false );
	// }

	// float u, v;
	// if( v1 >= 0.0f && u1 + v1 <= 1.0f ) {
	// 	u = u1;
	// 	v = v1;
	// }
	// else if( v2 >= 0.0f && u1 + v2 <= 1.0f ) {
	// 	u = u1;
	// 	v = v2;
	// }
	// else if( v3 >= 0.0f && u2 + v3 <= 1.0f ) {
	// 	u = u2;
	// 	v = v3;
	// }
	// else if( v4 >= 0.0f && u2 + v4 <= 1.0f ) {
	// 	u = u2;
	// 	v = v4;
	// }
	// else if( v5 >= 0.0f && u3 + v5 <= 1.0f ) {
	// 	u = u3;
	// 	v = v5;
	// }
	// else if( v6 >= 0.0f && u3 + v6 <= 1.0f ) {
	// 	u = u3;
	// 	v = v6;
	// }
	// else if( v7 >= 0.0f && u4 + v7 <= 1.0f ) {
	// 	u = u4;
	// 	v = v7;
	// }
	// else if( v8 >= 0.0f && u4 + v8 <= 1.0f ) {
	// 	u = u4;
	// 	v = v8;
	// }
	// else {
	// 	return (float4)( 0.0f );
	// }

	// Solve quadratic function
	// pq_p = native_divide( d*u + f, b );
	// pq_q = native_divide( a*u*u + c + e*u, b );

	// float pq_p_half = 0.5f * pq_p;
	// float pq_sqrt = native_sqrt( pq_p_half*pq_p_half - pq_q );
	// x1 = -pq_p_half + pq_sqrt;
	// x2 = -pq_p_half - pq_sqrt;

	// float v = x1;
	// if( v < 0.0f || v > 1.0f ) { v = x2; }
	// if( v < 0.0f || v > 1.0f ) { return (float4)( 0.0f ); }

	// if( u + v > 1.0f ) { return (float4)( 0.0f ); }

	// tuv->y = u;
	// tuv->z = v;
	// float w = 1.0f - u - v;


	// // Get the point
	// float3 pBary = P1*u + P2*v + P3*w;
	// float3 projA = projectOnPlane( pBary, P1, N1 );
	// float3 projB = projectOnPlane( pBary, P2, N2 );
	// float3 projC = projectOnPlane( pBary, P3, N3 );

	// float3 pTessellated = u*u*projA + v*v*projB + w*w*projC +
	//                       u*v*( projectOnPlane( projB, P1, N1 ) + projectOnPlane( projA, P2, N2 ) ) +
	//                       v*w*( projectOnPlane( projC, P2, N2 ) + projectOnPlane( projB, P3, N3 ) ) +
	//                       w*u*( projectOnPlane( projA, P3, N3 ) + projectOnPlane( projC, P1, N1 ) );

	// tuv->x = fast_length( pTessellated - ray->origin.xyz );
	// return (float4)( pTessellated, 0.0f );

	#undef PT_ALPHA
}


/**
 * Test faces of the given node for intersections with the given ray.
 * @param {ray4*}                ray
 * @param {const bvhNode*}       node
 * @param {const global face_t*} faces
 * @param {const float tNear}    tNear
 * @param {float tFar}           tFar
 */
void intersectFaces(
	ray4* ray, const bvhNode* node, const global face_t* faces, const float tNear, float tFar,
	constant const material* materials
) {
	#define IS_SAME_SIDE ( dot( normal.xyz, -ray->dir.xyz ) >= 0.0f )

	float3 tuv;
	// float tFar_reset;
	int faceIndices[4] = { node->faces.x, node->faces.y, node->faces.z, node->faces.w };

	for( int i = 0; i < 4; i++ ) {
		if( faceIndices[i] == -1 ) {
			break;
		}

		// tFar_reset = tFar;
		float4 hit = checkFaceIntersection( ray, faces[faceIndices[i]], &tuv, tNear, tFar );

		if( tuv.x > -1.0f ) {
			tFar = tuv.x;

			if( ray->t > tuv.x || ray->t < -1.0f ) {
				const float4 normal = getTriangleNormal( faces[faceIndices[i]], tuv );

				// if( IS_SAME_SIDE || materials[(uint) faces[faceIndices[i]].a.w].d < 1.0f ) {
					ray->normal = normal;
					ray->normal.w = (float) faceIndices[i];
					ray->t = tuv.x;
					ray->hit = hit;
				// }
				// else {
				// 	tFar = tFar_reset;
				// }
			}
		}
	}

	#undef IS_SAME_SIDE
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
void traverseBVH(
	const global bvhNode* bvh, ray4* ray, const global face_t* faces,
	constant const material* materials
) {
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
				intersectFaces( ray, &node, faces, tNearL, tFarL, materials );
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
