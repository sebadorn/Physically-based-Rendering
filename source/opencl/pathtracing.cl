#FILE:pt_header.cl:FILE#
#FILE:pt_utils.cl:FILE#


#if USE_SPECTRAL == 0

	#FILE:pt_rgb.cl:FILE#
	#define SETCOLORS setColors( imageIn, imageOut, pixelWeight, finalColor, focus );

#elif USE_SPECTRAL == 1

	#FILE:pt_spectral_precalc.cl:FILE#
	#define SETCOLORS setColors( imageIn, imageOut, pixelWeight, spdTotal, focus );

#endif


#FILE:pt_brdf.cl:FILE#
#FILE:pt_phongtess.cl:FILE#
#FILE:pt_intersect.cl:FILE#


#if ACCEL_STRUCT == 0
	#FILE:pt_bvh.cl:FILE#
#elif ACCEL_STRUCT == 1
	#FILE:pt_kdtree.cl:FILE#
#endif



/**
 * Generate the initial ray into the scene.
 * @param  {const int2}          pos
 * @param  {const float4}        initRayParts Diverse parameters that are needed to calculate the ray.
 * @param  {const global float*} eyeIn        Camera eye position.
 * @param  {float*}              seed         Seed for the random number generator.
 * @return {ray4}
 */
ray4 initRay(
	const float pxDim, const global float* eyeIn, float* seed,
	const float tFocus, const float tObject
) {
	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };

	// Camera position and vectors for the image projection plane.
	const float3 eye = { eyeIn[0], eyeIn[1], eyeIn[2] };
	const float3 w = { eyeIn[3], eyeIn[4], eyeIn[5] };
	const float3 u = { eyeIn[6], eyeIn[7], eyeIn[8] };
	const float3 v = { eyeIn[9], eyeIn[10], eyeIn[11] };

	const float3 initialRay = w + pxDim * 0.5f * (
		u - IMG_WIDTH * u + 2.0f * pos.x * u +
		v - IMG_HEIGHT * v + 2.0f * pos.y * v
	);

	ray4 ray;
	ray.t = INFINITY;
	ray.origin = eye;
	ray.dir = fast_normalize( initialRay );

	// Anti-Aliasing by slightly jittering the ray.
	if( ANTI_ALIASING > 0.0f ) {
		const float rnd = rand( seed );
		const float3 aaDir = jitter( ray.dir, PI_X2 * rand( seed ), native_sqrt( rnd ), native_sqrt( 1.0f - rnd ) );
		ray.dir = fast_normalize( ray.dir + aaDir * pxDim * ANTI_ALIASING );
	}

	// Depth-of-Field with a thin lense model.
	if( tObject > 0.0f ) {
		// Distance to focal plane.
		float3 planeNormal = -w;
		float3 pointOnPlane = ray.origin + planeNormal * tFocus;
		float tPlane = -dot( ray.origin, planeNormal + pointOnPlane ) / dot( ray.dir, planeNormal );

		// Thin lense settings.
		const float focalLength = 0.035f;
		const float aperture = focalLength / 0.5f;

		// Circle of confusion diameter on our image projection plane.
		float tmp1 = focalLength * ( tObject - tPlane );
		float tmp2 = tObject * ( tPlane - focalLength );
		float cocLength = fabs( aperture * native_divide( tmp1, tmp2 ) );

		// Choose a random point inside the circle of confusion.
		float rand1 = rand( seed );
		float rand2 = rand( seed );
		float radius = rand1 * cocLength * 0.5f;
		float angle = PI_X2 * rand2;
		float x = radius * native_cos( angle );
		float y = radius * native_sin( angle );
		float3 cocPoint = ray.origin + x * u + y * v;

		// Set the new ray direction.
		float3 hitFocalPlane = fma( tPlane, ray.dir, ray.origin );
		ray.dir = fast_normalize( hitFocalPlane - cocPoint );
	}

	return ray;
}


#if USE_SPECTRAL == 0

	/**
	 * Update the accumulated RGB color according to the hit material and BRDF.
	 * @param {const ray4*}           ray
	 * @param {const ray4*}           newRay
	 * @param {const material*}       mtl
	 * @param {const ray4*}           lightRay
	 * @param {const int}             lightRaySource
	 * @param {uint*}                 secondaryPaths
	 * @param {float4*}               color
	 * @param {float4*}               finalColor
	 */
	void updateColor(
		const ray4* ray, const ray4* newRay, const material* mtl,
		const ray4* lightRay, const float4 lightRaySource, uint* secondaryPaths,
		float4* color, float4* finalColor
	) {
		// BRDF: Schlick
		#if BRDF == 0

			float brdf, pdf, u;

			#if IMPLICIT == 1

				if( lightRaySource.x >= 0 ) {
					brdf = brdfSchlick( mtl, ray, lightRay, &( ray->normal ), &u, &pdf );

					if( fabs( pdf ) > 0.00001f ) {
						brdf *= lambert( ray->normal, lightRay->dir );
						brdf = native_divide( brdf, pdf );

						*finalColor += *color * lightRaySource * mtl->rgbDiff *
							( fresnel4( u, mtl->rgbSpec ) * brdf * mtl->d + ( 1.0f - mtl->d ) );

						*secondaryPaths += 1;
					}
				}

			#endif

			brdf = brdfSchlick( mtl, ray, newRay, &( ray->normal ), &u, &pdf );
			brdf *= lambert( ray->normal, newRay->dir );
			brdf = native_divide( brdf, pdf );

			*color *= mtl->rgbDiff * ( fresnel4( u, mtl->rgbSpec ) * brdf * mtl->d + ( 1.0f - mtl->d ) );

		// BRDF: Shirley/Ashikhmin
		#elif BRDF == 1

			float brdfDiff, brdfSpec, pdf;
			float4 brdf_d, brdf_s;
			float dotHK1;

			#if IMPLICIT == 1

				if( lightRaySource.x >= 0 ) {
					brdfShirleyAshikhmin(
						mtl->nu, mtl->nv, mtl->Rs, mtl->Rd,
						ray, lightRay, &( ray->normal ), &brdfSpec, &brdfDiff, &dotHK1, &pdf
					);

					if( fabs( pdf ) > 0.00001f ) {
						brdfSpec = native_divide( brdfSpec, pdf );
						brdfDiff = native_divide( brdfDiff, pdf );

						brdf_s = brdfSpec * mtl->rgbSpec * fresnel( dotHK1, mtl->Rs );
						brdf_d = brdfDiff * mtl->rgbDiff * ( 1.0f - mtl->Rs );

						float4 brdfColor = ( brdf_s + brdf_d ) * mtl->d + ( 1.0f - mtl->d );
						float maxRGB = max( 1.0f, max( brdfColor.x, max( brdfColor.y, brdfColor.z ) ) );
						brdfColor /= maxRGB;

						*finalColor += clamp( brdfColor, 0.0f, 1.0f ) * lightRaySource * mtl->d + ( 1.0f - mtl->d );

						*secondaryPaths += 1;
					}
				}

			#endif

			brdfShirleyAshikhmin(
				mtl->nu, mtl->nv, mtl->Rs, mtl->Rd,
				ray, newRay, &( ray->normal ), &brdfSpec, &brdfDiff, &dotHK1, &pdf
			);

			brdfSpec = native_divide( brdfSpec, pdf );
			brdfDiff = native_divide( brdfDiff, pdf );

			brdf_s = brdfSpec * mtl->rgbSpec * fresnel( dotHK1, mtl->Rs );
			brdf_d = brdfDiff * mtl->rgbDiff * ( 1.0f - mtl->Rs );

			float4 brdfColor = ( brdf_s + brdf_d ) * mtl->d + ( 1.0f - mtl->d );
			float maxRGB = max( 1.0f, max( brdfColor.x, max( brdfColor.y, brdfColor.z ) ) );
			brdfColor /= maxRGB;

			*color *= clamp( brdfColor, 0.0f, 1.0f );

		#endif
	}

#elif USE_SPECTRAL == 1

	/**
	 * Update the spectral power distribution according to the hit material and BRDF.
	 * @param {const ray4*}           ray
	 * @param {const ray4*}           newRay
	 * @param {const material*}       mtl
	 * @param {const ray4*}           lightRay
	 * @param {const int}             lightRaySource
	 * @param {uint*}                 secondaryPaths
	 * @param {constant const float*} specPowerDists
	 * @param {float*}                spd
	 * @param {float*}                spdTotal
	 * @param {float*}                maxValSpd
	 */
	void updateSPD(
		const ray4* ray, const ray4* newRay, const material* mtl,
		const ray4* lightRay, const int lightRaySource, uint* secondaryPaths,
		constant const float* specPowerDists, float* spd, float* spdTotal, float* maxValSpd
	) {
		#define COLOR_DIFF ( specPowerDists[index0 + i] )
		#define COLOR_SPEC ( specPowerDists[index1 + i] )

		const uint index0 = mtl->spd.x * SPEC;
		const uint index1 = mtl->spd.y * SPEC;


		// BRDF: Schlick
		#if BRDF == 0

			float brdf, pdf, u;

			#if IMPLICIT == 1

				if( lightRaySource >= 0 ) {
					brdf = brdfSchlick( mtl, ray, lightRay, &( ray->normal ), &u, &pdf );

					if( fabs( pdf ) > 0.00001f ) {
						brdf *= lambert( ray->normal, lightRay->dir );
						brdf = native_divide( brdf, pdf );

						for( int i = 0; i < SPEC; i++ ) {
							spdTotal[i] += spd[i] * specPowerDists[lightRaySource + i] *
							               COLOR_DIFF * ( fresnel( u, COLOR_SPEC ) * brdf *
							               mtl->d + ( 1.0f - mtl->d ) );
						}

						*secondaryPaths += 1;
					}
				}

			#endif

			brdf = brdfSchlick( mtl, ray, newRay, &( ray->normal ), &u, &pdf );
			brdf *= lambert( ray->normal, newRay->dir );
			brdf = native_divide( brdf, pdf );

			for( int i = 0; i < SPEC; i++ ) {
				spd[i] *= COLOR_DIFF * ( fresnel( u, COLOR_SPEC ) * brdf * mtl->d + ( 1.0f - mtl->d ) );
				*maxValSpd = fmax( spd[i], *maxValSpd );
			}

		// BRDF: Shirley/Ashikhmin
		#elif BRDF == 1

			float brdf_d, brdf_s, brdfDiff, brdfSpec, pdf;
			float dotHK1;

			#if IMPLICIT == 1

				if( lightRaySource >= 0 ) {
					brdfShirleyAshikhmin(
						mtl->nu, mtl->nv, mtl->Rs, mtl->Rd,
						ray, lightRay, &( ray->normal ), &brdfSpec, &brdfDiff, &dotHK1, &pdf
					);

					if( fabs( pdf ) > 0.00001f ) {
						brdfSpec = native_divide( brdfSpec, pdf );
						brdfDiff = native_divide( brdfDiff, pdf );

						for( int i = 0; i < SPEC; i++ ) {
							brdf_s = brdfSpec * COLOR_SPEC * fresnel( dotHK1, mtl->Rs );
							brdf_d = brdfDiff * COLOR_DIFF * ( 1.0f - mtl->Rs );

							spdTotal[i] += spd[i] * specPowerDists[lightRaySource + i] *
							               ( brdf_s + brdf_d ) *
							               mtl->d + ( 1.0f - mtl->d );
						}

						*secondaryPaths += 1;
					}
				}

			#endif

			brdfShirleyAshikhmin(
				mtl->nu, mtl->nv, mtl->Rs, mtl->Rd,
				ray, newRay, &( ray->normal ), &brdfSpec, &brdfDiff, &dotHK1, &pdf
			);

			brdfSpec = native_divide( brdfSpec, pdf );
			brdfDiff = native_divide( brdfDiff, pdf );

			for( int i = 0; i < SPEC; i++ ) {
				brdf_s = brdfSpec * COLOR_SPEC * fresnel( dotHK1, mtl->Rs );
				brdf_d = brdfDiff * COLOR_DIFF * ( 1.0f - mtl->Rs );

				spd[i] *= ( brdf_s + brdf_d ) * mtl->d + ( 1.0f - mtl->d );
				*maxValSpd = fmax( spd[i], *maxValSpd );
			}

		#endif

		#undef COLOR_DIFF
		#undef COLOR_SPEC
	}

#endif


// KERNELS


#if USE_SPECTRAL == 1

	/**
	 * KERNEL.
	 * Do the path tracing and calculate the final color for the pixel.
	 * @param {const uint2}            offset
	 * @param {float}                  seed
	 * @param {float}                  pixelWeight
	 * @param {const global face_t*}   faces
	 * @param {const global bvhNode*}  bvh
	 * @param {const global rayBase*}  initRays
	 * @param {const global material*} materials
	 * @param {const global float*}    specPowerDists
	 * @param {read_only image2d_t}    imageIn
	 * @param {write_only image2d_t}   imageOut
	 */
	kernel void pathTracing(
		// changing values
		float seed,
		const float pixelWeight,
		const float4 sunPos,

		// view
		const float pxDim,
		global const float* eyeIn,

		// acceleration structure
		#if ACCEL_STRUCT == 0

			global const bvhNode* bvh,

		#elif ACCEL_STRUCT == 1

			global const bvhNode* bvh,
			global const kdNonLeaf* kdNonLeaves,
			global const kdLeaf* kdLeaves,
			global const uint* kdFaces,

		#endif

		// geometry and color related
		global const face_t* faces,
		global const float4* vertices,
		global const float4* normals,
		global const material* materials,
		constant const float* specPowerDists,

		// old and new frame
		read_only image2d_t imageIn,
		write_only image2d_t imageOut
	) {
		float spd[SPEC], spdTotal[SPEC];
		setArray( spdTotal, 0.0f );

		float focus;
		bool addDepth;
		uint secondaryPaths = 1; // Start at 1 instead of 0, because we are going to divide through it.

		for( uint sample = 0; sample < SAMPLES; sample++ ) {
			setArray( spd, 1.0f );
			int light = -1;
			ray4 ray = initRay( pxDim, eyeIn, &seed );
			float maxValSpd = 0.0f;
			int depthAdded = 0;

			for( uint depth = 0; depth < MAX_DEPTH + depthAdded; depth++ ) {
				CALL_TRAVERSE

				if( ray.t == INFINITY ) {
					light = SKY_LIGHT * SPEC;
					break;
				}

				focus = ( depth == 0 ) ? ray.t : focus;

				material mtl = materials[faces[ray.hitFace].material];

				// Implicit connection to a light found
				if( mtl.light == 1 ) {
					light = mtl.spd.x * SPEC;
					break;
				}

				// Last round, no need to calculate a new ray.
				// Unless we hit a material that extends the path.
				addDepth = extendDepth( &mtl, &seed );

				if( mtl.d == 1.0f && !addDepth && depth == MAX_DEPTH + depthAdded - 1 ) {
					break;
				}

				seed += ray.t;

				int lightRaySource = -1;
				ray4 lightRay;
				lightRay.t = INFINITY;

				#if IMPLICIT == 1

					if( mtl.d > 0.0f ) {
						lightRay.origin = fma( ray.t, ray.dir, ray.origin ) + ray.normal * EPSILON5;
						const float rnd2 = rand( &seed );
						lightRay.dir = fast_normalize( sunPos.xyz - lightRay.origin );

						CALL_TRAVERSE_SHADOWS

						if( lightRay.t == INFINITY ) {
							lightRaySource = SKY_LIGHT * SPEC;
						}
						else {
							material lightMTL = materials[faces[lightRay.hitFace].material];
							lightRaySource = ( lightMTL.light == 1 ) ? lightMTL.spd.x : -1;
						}
					}

				#endif

				// New direction of the ray (bouncing of the hit surface)
				ray4 newRay = getNewRay( &ray, &mtl, &seed, &addDepth );

				updateSPD(
					&ray, &newRay, &mtl, &lightRay, lightRaySource, &secondaryPaths,
					specPowerDists, spd, spdTotal, &maxValSpd
				);

				// Extend max path depth
				depthAdded += ( addDepth && depthAdded < MAX_ADDED_DEPTH );

				// Russian roulette termination
				if( depth > 2 + depthAdded && maxValSpd < rand( &seed ) ) {
					break;
				}

				ray = newRay;
			} // end bounces

			if( light >= 0 ) {
				for( int i = 0; i < SPEC; i++ ) {
					spdTotal[i] += spd[i] * specPowerDists[light + i];
				}
			}
		} // end samples

		for( int i = 0; i < SPEC; i++ ) {
			spdTotal[i] = native_divide( spdTotal[i], (float) secondaryPaths );
		}

		#if SAMPLES > 1

			for( int i = 0; i < SPEC; i++ ) {
				spdTotal[i] = native_divide( spdTotal[i], (float) SAMPLES );
			}

		#endif

		SETCOLORS
	}

#elif USE_SPECTRAL == 0

	/**
	 * KERNEL.
	 * Do the path tracing and calculate the final color for the pixel.
	 * @param {const uint2}            offset
	 * @param {float}                  seed
	 * @param {float}                  pixelWeight
	 * @param {const global face_t*}   faces
	 * @param {const global bvhNode*}  bvh
	 * @param {const global rayBase*}  initRays
	 * @param {const global material*} materials
	 * @param {const global float*}    specPowerDists
	 * @param {read_only image2d_t}    imageIn
	 * @param {write_only image2d_t}   imageOut
	 */
	kernel void pathTracing(
		// changing values
		float seed,
		const float pixelWeight,
		const float4 sunPos,

		// view
		const float pxDim,
		global const float* eyeIn,

		// acceleration structure
		#if ACCEL_STRUCT == 0

			global const bvhNode* bvh,

		#elif ACCEL_STRUCT == 1

			const kdLeaf kdRootNode,
			global const kdNonLeaf* kdNonLeaves,
			global const kdLeaf* kdLeaves,
			global const uint* kdFaces,

		#endif

		// geometry and color related
		global const face_t* faces,
		global const float4* vertices,
		global const float4* normals,
		global const material* materials,

		// old and new frame
		read_only image2d_t imageIn,
		write_only image2d_t imageOut
	) {
		float4 finalColor = (float4)( 0.0f );

		#if ACCEL_STRUCT == 0
			Scene scene = { bvh, faces, vertices, normals };
		#elif ACCEL_STRUCT == 1
			Scene scene = { kdRootNode, kdNonLeaves, kdLeaves, kdFaces, faces, vertices, normals };
		#endif

		float focus = 0.0f;
		bool addDepth;
		uint secondaryPaths = 1; // Start at 1 instead of 0, because we are going to divide through it.


		const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };
		const int2 posCenter = { (int) IMG_WIDTH * 0.5f, (int) IMG_HEIGHT * 0.5f };
		const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
		const float4 thisRayPixel = read_imagef( imageIn, sampler, pos );
		const float4 centerRayPixel = read_imagef( imageIn, sampler, posCenter );


		for( uint sample = 0; sample < SAMPLES; sample++ ) {
			float4 color = (float4)( 1.0f );
			float4 light = (float4)( -1.0f );

			ray4 ray = initRay( pxDim, eyeIn, &seed, centerRayPixel.w, thisRayPixel.w );
			int depthAdded = 0;

			for( uint depth = 0; depth < MAX_DEPTH + depthAdded; depth++ ) {
				CALL_TRAVERSE

				if( ray.t == INFINITY ) {
					light = SKY_LIGHT;
					break;
				}

				focus = ( sample + depth == 0 ) ? ray.t : focus;
				material mtl = materials[faces[ray.hitFace].material];

				// Implicit connection to a light found
				if( mtl.light == 1 ) {
					light = mtl.rgbDiff;
					break;
				}

				// Last round, no need to calculate a new ray.
				// Unless we hit a material that extends the path.
				addDepth = extendDepth( &mtl, &seed );

				if( mtl.d == 1.0f && !addDepth && depth == MAX_DEPTH + depthAdded - 1 ) {
					break;
				}

				seed += ray.t;

				float4 lightRaySource = (float4)( -1.0f );
				ray4 lightRay;
				lightRay.t = INFINITY;

				#if IMPLICIT == 1

					if( mtl.d > 0.0f ) {
						lightRay.origin = fma( ray.t, ray.dir, ray.origin ) + ray.normal * EPSILON5;
						const float rnd2 = rand( &seed );
						lightRay.dir = fast_normalize( sunPos.xyz - lightRay.origin );

						CALL_TRAVERSE_SHADOWS

						if( lightRay.t == INFINITY ) {
							lightRaySource = SKY_LIGHT;
						}
						else {
							material lightMTL = materials[faces[lightRay.hitFace].material];
							lightRaySource = ( lightMTL.light == 1 ) ? lightMTL.rgbDiff : (float4)( -1.0f );
						}
					}

				#endif

				// New direction of the ray (bouncing of the hit surface)
				ray4 newRay = getNewRay( &ray, &mtl, &seed, &addDepth );

				updateColor(
					&ray, &newRay, &mtl, &lightRay, lightRaySource, &secondaryPaths, &color, &finalColor
				);

				// Extend max path depth
				depthAdded += ( addDepth && depthAdded < MAX_ADDED_DEPTH );

				// Russian roulette termination
				float maxValRGB = fmax( color.x, fmax( color.y, color.z ) );

				if( depth > 2 + depthAdded && maxValRGB < rand( &seed ) ) {
					break;
				}

				ray = newRay;
			} // end bounces

			if( light.x > -1.0f ) {
				color *= light;
				finalColor += color;
			}
		} // end samples

		finalColor /= (float) secondaryPaths;

		#if SAMPLES > 1
			finalColor /= (float) SAMPLES;
		#endif

		SETCOLORS
	}

#endif
