#FILE:pt_header.cl:FILE#
#FILE:pt_utils.cl:FILE#
#FILE:pt_spectral.cl:FILE#
#FILE:pt_brdf.cl:FILE#
#FILE:pt_traversal.cl:FILE#



/**
 * Generate the initial ray into the scene.
 * @param  {const int2}          pos
 * @param  {const float4}        initRayParts Diverse parameters that are needed to calculate the ray.
 * @param  {const global float*} eyeIn        Camera eye position.
 * @param  {float*}              seed         Seed for the random number generator.
 * @return {ray4}
 */
ray4 initRay( const float pxDim, const global float* eyeIn, float* seed ) {
	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };

	const float4 eye = { eyeIn[0], eyeIn[1], eyeIn[2], 0.0f };
	const float4 w = { eyeIn[3], eyeIn[4], eyeIn[5], 0.0f };
	const float4 u = { eyeIn[6], eyeIn[7], eyeIn[8], 0.0f };
	const float4 v = { eyeIn[9], eyeIn[10], eyeIn[11], 0.0f };

	const float4 initialRay = w + pxDim * 0.5f *
			(
				u - IMG_WIDTH * u + 2.0f * pos.x * u +
				v - IMG_HEIGHT * v + 2.0f * pos.y * v
			);

	ray4 ray;
	ray.t = -2.0f;
	ray.origin = eye;
	ray.dir = fast_normalize( initialRay );

	const float rnd = rand( seed );
	const float4 aaDir = jitter( ray.dir, PI_X2 * rand( seed ), native_sqrt( rnd ), native_sqrt( 1.0f - rnd ) );
	ray.dir = fast_normalize( ray.dir +	aaDir * ANTI_ALIASING );

	return ray;
}


/**
 * Write the final color to the output image.
 * @param {const int2}           pos         Pixel coordinate in the image to read from/write to.
 * @param {read_only image2d_t}  imageIn     The previously generated image.
 * @param {write_only image2d_t} imageOut    Output.
 * @param {const float}          pixelWeight Mixing weight of the new color with the old one.
 * @param {float[40]}            spdLight    Spectral power distribution reaching this pixel.
 * @param {float}                focus       Value <t> of the ray.
 */
void setColors(
	read_only image2d_t imageIn, write_only image2d_t imageOut,
	const float pixelWeight, float spdLight[40], float focus
) {
	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };
	const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
	const float4 imagePixel = read_imagef( imageIn, sampler, pos );
	const float4 accumulatedColor = spectrumToRGB( spdLight );
	const float4 color = mix(
		clamp( accumulatedColor, 0.0f, 1.0f ),
		imagePixel, pixelWeight
	);
	color.w = focus;

	write_imagef( imageOut, pos, color );
}


/**
 *
 * @param ray
 * @param newRay
 * @param mtl
 * @param lightRay
 * @param lightMTL
 * @param specPowerDists
 * @param spd
 * @param spdTotal
 * @param maxValSpd
 */
void updateSPD(
	const ray4* ray, const ray4* newRay, const material* mtl,
	const ray4* lightRay, const int lightRaySource,
	constant const float* specPowerDists, float* spd, float* spdTotal, float* maxValSpd
) {
	#define COLOR_DIFF ( specPowerDists[index0 + i] )
	#define COLOR_SPEC ( specPowerDists[index1 + i] )

	const uint index0 = mtl->spd.x * SPEC;
	const uint index1 = mtl->spd.y * SPEC;


	// BRDF: Self-made something to account for some basic effects
	#if BRDF == 0

		float brdf, u;

		#if IMPLICIT == 1
			if( lightRaySource >= 0 ) {
				brdf = lambert( ray->normal, lightRay->dir );

				for( int i = 0; i < SPEC; i++ ) {
					spdTotal[i] += spd[i] * specPowerDists[lightRaySource + i] *
					               COLOR_DIFF * clamp( fresnel( u, COLOR_SPEC ) * brdf, 0.0f, 1.0f ) *
					               mtl->d + ( 1.0f - mtl->d ) );
				}
			}
		#endif

		u = 1.0f;
		brdf = lambert( ray->normal, newRay->dir ) * mtl->rough + ( 1.0f - mtl->rough );
		brdf = brdf * mtl->d + ( 1.0f - mtl->d );

		for( int i = 0; i < SPEC; i++ ) {
			spd[i] *= COLOR_DIFF * clamp( fresnel( u, COLOR_SPEC ) * brdf, 0.0f, 1.0f );
			*maxValSpd = fmax( spd[i], *maxValSpd );
		}

	// BRDF: Schlick
	#elif BRDF == 1

		float brdf, pdf, u;

		#if IMPLICIT == 1
			if( lightRaySource >= 0 ) {
				brdf = brdfSchlick( mtl, ray, lightRay, &( ray->normal ), &u, &pdf );
				brdf *= lambert( ray->normal, lightRay->dir );
				brdf = native_divide( brdf, pdf );

				for( int i = 0; i < SPEC; i++ ) {
					spdTotal[i] += spd[i] * specPowerDists[lightRaySource + i] *
					               COLOR_DIFF * ( fresnel( u, COLOR_SPEC ) * brdf *
					               mtl->d + ( 1.0f - mtl->d ) );
				}
			}
		#endif

		brdf = brdfSchlick( mtl, ray, newRay, &( ray->normal ), &u, &pdf );
		brdf *= lambert( ray->normal, newRay->dir );
		brdf = native_divide( brdf, pdf );

		for( int i = 0; i < SPEC; i++ ) {
			spd[i] *= COLOR_DIFF * ( clamp( fresnel( u, COLOR_SPEC ) * brdf, 0.0f, 1.0f ) * mtl->d + ( 1.0f - mtl->d ) );
			*maxValSpd = fmax( spd[i], *maxValSpd );
		}

	// BRDF: Shirley/Ashikhmin
	#elif BRDF == 2

		float brdf_d, brdf_s, brdfDiff, brdfSpec, pdf;
		float cosLaw, dotHK1;

		#if IMPLICIT == 1
			if( lightRaySource >= 0 ) {
				brdfShirleyAshikhmin(
					mtl->nu, mtl->nv, mtl->Rs, mtl->Rd,
					ray, lightRay, &( ray->normal ), &brdfSpec, &brdfDiff, &dotHK1, &pdf
				);

				cosLaw = lambert( ray->normal, lightRay->dir );
				brdfSpec = native_divide( brdfSpec, pdf ) * cosLaw;
				brdfDiff = native_divide( brdfDiff, pdf ) * cosLaw;

				for( int i = 0; i < SPEC; i++ ) {
					brdf_s = brdfSpec * fresnel( dotHK1, mtl->Rs * COLOR_SPEC );
					brdf_d = brdfDiff * COLOR_DIFF * ( 1.0f - mtl->Rs * COLOR_SPEC );

					spdTotal[i] += spd[i] * specPowerDists[lightRaySource + i] *
					               clamp( brdf_s + brdf_d, 0.0f, 1.0f ) *
					               mtl->d + ( 1.0f - mtl->d );
				}
			}
		#endif

		brdfShirleyAshikhmin(
			mtl->nu, mtl->nv, mtl->Rs, mtl->Rd,
			ray, newRay, &( ray->normal ), &brdfSpec, &brdfDiff, &dotHK1, &pdf
		);

		cosLaw = lambert( ray->normal, newRay->dir );
		brdfSpec = native_divide( brdfSpec, pdf ) * cosLaw;
		brdfDiff = native_divide( brdfDiff, pdf ) * cosLaw;

		for( int i = 0; i < SPEC; i++ ) {
			brdf_s = brdfSpec * fresnel( dotHK1, mtl->Rs * COLOR_SPEC );
			brdf_d = brdfDiff * COLOR_DIFF * ( 1.0f - mtl->Rs * COLOR_SPEC );

			spd[i] *= clamp( brdf_s + brdf_d, 0.0f, 1.0f ) * mtl->d + ( 1.0f - mtl->d );
			*maxValSpd = fmax( spd[i], *maxValSpd );
		}

	#endif

	#undef COLOR_DIFF
	#undef COLOR_SPEC
}



// KERNELS


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
	float seed, float pixelWeight, const float pxDim,
	global const float* eyeIn, global const face_t* faces, global const bvhNode* bvh,
	constant const material* materials, constant const float* specPowerDists,
	read_only image2d_t imageIn, write_only image2d_t imageOut
) {
	float spd[SPEC], spdTotal[SPEC];
	setArray( spdTotal, 0.0f );

	float focus;
	bool addDepth;


	for( uint sample = 0; sample < SAMPLES; sample++ ) {
		setArray( spd, 1.0f );
		int light = -1;
		ray4 ray = initRay( pxDim, eyeIn, &seed );
		float maxValSpd = 0.0f;
		int depthAdded = 0;

		for( uint depth = 0; depth < MAX_DEPTH + depthAdded; depth++ ) {
			traverseBVH( bvh, &ray, faces, materials );

			if( ray.t < 0.0f ) {
				light = SKY_LIGHT * SPEC;
				break;
			}

			focus = ( depth == 0 ) ? ray.t : focus;

			material mtl = materials[(uint) faces[(uint) ray.normal.w].a.w];
			ray.normal.w = 0.0f;

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
			lightRay.t = -2.0f;

			#if IMPLICIT == 1
				if( mtl.d > 0.0f ) {
					lightRay.origin = fma( ray.t, ray.dir, ray.origin ) + ray.normal * EPSILON;
					const float rnd2 = rand( &seed );
					lightRay.dir = jitter( ray.normal, PI_X2 * rand( &seed ), native_sqrt( rnd2 ), native_sqrt( 1.0f - rnd2 ) );

					traverseBVH( bvh, &lightRay, faces, materials );

					if( lightRay.t < 0.0f ) {
						lightRaySource = SKY_LIGHT * SPEC;
					}
					else if( lightRay.t > -1.0f ) {
						material lightMTL = materials[(uint) faces[(uint) lightRay.normal.w].a.w];
						lightRaySource = ( lightMTL.light == 1 ) ? lightMTL.spd.x : -1;
					}

					lightRay.normal.w = 0.0f;
				}
			#endif

			// New direction of the ray (bouncing of the hit surface)
			ray4 newRay = getNewRay( &ray, &mtl, &seed, &addDepth );

			updateSPD(
				&ray, &newRay, &mtl, &lightRay, lightRaySource,
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

	for( int i = 0; SAMPLES > 1 && i < SPEC; i++ ) {
		spdTotal[i] = native_divide( spdTotal[i], (float) SAMPLES );
	}

	setColors( imageIn, imageOut, pixelWeight, spdTotal, focus );
}
