#FILE:pt_header.cl:FILE#
#FILE:pt_utils.cl:FILE#


#FILE:pt_spectral_precalc.cl:FILE#
#FILE:pt_brdf.cl:FILE#
#FILE:pt_phongtess.cl:FILE#
#FILE:pt_intersect.cl:FILE#


#if ACCEL_STRUCT == 0
	#FILE:pt_bvh.cl:FILE#
#endif



/**
 * Generate the initial ray into the scene.
 * @param  {const float}  pxDim   Pixel width and height.
 * @param  {const camera} cam     The camera model.
 * @param  {float*}       seed    Seed for the random number generator.
 * @param  {float}        tFocus  Focus distance for the image.
 * @param  {float}        tObject Distance to the object for this ray.
 * @return {ray4}                 The ray including adjustments for anti-aliasing and depth-of-field.
 */
ray4 initRay(
	const float pxDim, const camera cam, float* seed, float tFocus, float tObject
) {
	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };

	const float3 initialRay = cam.w + pxDim * 0.5f * (
		cam.u - IMG_WIDTH * cam.u + 2.0f * pos.x * cam.u +
		cam.v - IMG_HEIGHT * cam.v + 2.0f * pos.y * cam.v
	);

	ray4 ray;
	ray.t = INFINITY;
	ray.origin = cam.eye;
	ray.dir = fast_normalize( initialRay );

	antiAliasing( &ray, pxDim, seed );
	depthOfField( &ray, &cam, tObject, tFocus, seed );

	return ray;
}


/**
 * Get the t factor for the hit object of this ray and
 * the center ray of the previous frame.
 * @param  {read_only image2d_t} imageIn
 * @return {float2}
 */
float2 getPreviousFocus( read_only image2d_t imageIn ) {
	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };
	const int2 posCenter = { (int) IMG_WIDTH * 0.5f, (int) IMG_HEIGHT * 0.5f };
	const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
	const float4 thisRayPixel = read_imagef( imageIn, sampler, pos );
	const float4 centerRayPixel = read_imagef( imageIn, sampler, posCenter );

	return (float2)( thisRayPixel.w, centerRayPixel.w ); // tObject, tFocus
}


/**
 * Write color to the debug image.
 * @param {write_only image2d_t} imageDebug
 * @param {const float4}         color
 */
void writeDebugImage( write_only image2d_t imageDebug, float4 color ) {
	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };
	color.x /= 1082.0f; // number of faces in the test model
	color.y /= 1265.0f; // number of BVH nodes in the test model
	write_imagef( imageDebug, pos, color );
}


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
void updateColor(
	const ray4* ray, const ray4* newRay, const material* mtl,
	const ray4* lightRay, int lightRaySource, uint* secondaryPaths,
	constant const float* specPowerDists, float* spd, float* spdTotal, float* maxValSpd
) {
	#define COLOR_DIFF ( specPowerDists[index0 + i] )
	#define COLOR_SPEC ( specPowerDists[index1 + i] )

	const uint index0 = mtl->spd.x * SPEC;
	const uint index1 = mtl->spd.y * SPEC;

	lightRaySource *= SPEC;


	// BRDF: Schlick
	#if BRDF == 0

		float brdf, pdf, u;

		#if SHADOW_RAYS == 1

			if( lightRaySource >= 0 ) {
				brdf = brdfSchlick( mtl, ray, lightRay, &( ray->normal ), &u, &pdf );

				if( fabs( pdf ) > 0.00001f ) {
					brdf *= lambert( ray->normal, lightRay->dir );
					brdf = native_divide( brdf, pdf );

					for( int i = 0; i < SPEC; i++ ) {
						spdTotal[i] += spd[i] * specPowerDists[lightRaySource + i] *
						               COLOR_DIFF * ( fresnel( u, COLOR_SPEC ) * brdf *
						               mtl->data.s0 + ( 1.0f - mtl->data.s0 ) );
					}

					*secondaryPaths += 1;
				}
			}

		#endif

		brdf = brdfSchlick( mtl, ray, newRay, &( ray->normal ), &u, &pdf );
		brdf *= lambert( ray->normal, newRay->dir );
		brdf = native_divide( brdf, pdf );

		for( int i = 0; i < SPEC; i++ ) {
			spd[i] *= COLOR_DIFF * ( fresnel( u, COLOR_SPEC ) * brdf * mtl->data.s0 + ( 1.0f - mtl->data.s0 ) );
			*maxValSpd = fmax( spd[i], *maxValSpd );
		}

	// BRDF: Shirley/Ashikhmin
	#elif BRDF == 1

		float brdf_d, brdf_s, brdfDiff, brdfSpec, pdf;
		float dotHK1;

		#if SHADOW_RAYS == 1

			if( lightRaySource >= 0 ) {
				brdfShirleyAshikhmin(
					mtl->data.s2, mtl->data.s3, mtl->data.s4, mtl->data.s5,
					ray, lightRay, &( ray->normal ), &brdfSpec, &brdfDiff, &dotHK1, &pdf
				);

				if( fabs( pdf ) > 0.00001f ) {
					brdfSpec = native_divide( brdfSpec, pdf );
					brdfDiff = native_divide( brdfDiff, pdf );

					for( int i = 0; i < SPEC; i++ ) {
						brdf_s = brdfSpec * COLOR_SPEC * fresnel( dotHK1, mtl->data.s4 );
						brdf_d = brdfDiff * COLOR_DIFF * ( 1.0f - mtl->data.s4 );

						spdTotal[i] += spd[i] * specPowerDists[lightRaySource + i] *
						               ( brdf_s + brdf_d ) *
						               mtl->data.s0 + ( 1.0f - mtl->data.s0 );
					}

					*secondaryPaths += 1;
				}
			}

		#endif

		brdfShirleyAshikhmin(
			mtl->data.s2, mtl->data.s3, mtl->data.s4, mtl->data.s5,
			ray, newRay, &( ray->normal ), &brdfSpec, &brdfDiff, &dotHK1, &pdf
		);

		brdfSpec = native_divide( brdfSpec, pdf );
		brdfDiff = native_divide( brdfDiff, pdf );

		for( int i = 0; i < SPEC; i++ ) {
			brdf_s = brdfSpec * COLOR_SPEC * fresnel( dotHK1, mtl->data.s4 );
			brdf_d = brdfDiff * COLOR_DIFF * ( 1.0f - mtl->data.s4 );

			spd[i] *= ( brdf_s + brdf_d ) * mtl->data.s0 + ( 1.0f - mtl->data.s0 );
			*maxValSpd = fmax( spd[i], *maxValSpd );
		}

	#endif

	#undef COLOR_DIFF
	#undef COLOR_SPEC
}


/**
 * Shoot shadow rays to the light sources.
 * @param {Scene*}                scene
 * @param {global const light_t*} lights
 * @param {ray4*}                 ray
 * @param {ray4*}                 lightRay
 * @param {float4*}               lightRaySource
 */
void shadowRayTest( Scene* scene, ray4* ray, ray4* lightRay, float4* lightRaySource ) {
	lightRay->origin = fma( ray->t, ray->dir, ray->origin ) + ray->normal * EPSILON5;
	lightRay->dir = fast_normalize( scene->lights[0].pos.xyz - lightRay->origin );

	CALL_TRAVERSE_SHADOWS

	if( lightRay->t == INFINITY ) {
		*lightRaySource = scene->lights[0].rgb;
	}
}



// KERNELS


/**
 * KERNEL.
 * Do the path tracing and calculate the final color for the pixel.
 */
kernel void pathTracing(
	// changing values
	float seed,
	const float pixelWeight,

	// view
	const float pxDim,
	const camera cam,

	// acceleration structure
	#if ACCEL_STRUCT == 0
		global const bvhNode* bvh,
	#endif

	// geometry and color related
	global const face_t* faces,
	global const float4* vertices,
	global const float4* normals,
	global const material* materials,
	global const light_t* lights,
	constant const float* specPowerDists,

	// old and new frame
	read_only image2d_t imageIn,
	write_only image2d_t imageOut,

	write_only image2d_t imageDebug
) {
	float spd[SPEC], spdTotal[SPEC];
	setArray( spdTotal, 0.0f );

	#if ACCEL_STRUCT == 0
		Scene scene = { bvh, lights, faces, vertices, normals, (float4)( 0.0f ) };
	#endif

	float focus = 0.0f;
	const float2 prevFocus = getPreviousFocus( imageIn );

	bool addDepth;
	uint secondaryPaths = 1; // Start at 1 instead of 0, because we are going to divide through it.

	for( uint sample = 0; sample < SAMPLES; sample++ ) {
		setArray( spd, 1.0f );
		int light = -1;
		float maxValColor = 0.0f;

		ray4 ray = initRay( pxDim, cam, &seed, prevFocus.y, prevFocus.x );
		int depthAdded = 0;

		for( uint depth = 0; depth < MAX_DEPTH + depthAdded; depth++ ) {
			CALL_TRAVERSE

			focus = ( sample + depth == 0 ) ? ray.t : focus;

			if( ray.t == INFINITY ) {
				light = SKY_LIGHT;
				break;
			}

			material mtl = materials[scene.faces[ray.hitFace].vertices.w];

			// Last round, no need to calculate a new ray.
			// Unless we hit a material that extends the path.
			addDepth = extendDepth( &mtl, &seed );

			if( mtl.data.s0 == 1.0f && !addDepth && depth == MAX_DEPTH + depthAdded - 1 ) {
				break;
			}

			seed += ray.t;

			int lightRaySource = -1;
			ray4 lightRay;
			lightRay.t = INFINITY;

			#if SHADOW_RAYS == 1
				if( mtl.data.s0 > 0.0f ) {
					shadowRayTest( &scene, &ray, &lightRay, &lightRaySource );
				}
			#endif

			// New direction of the ray (bouncing of the hit surface)
			ray4 newRay = getNewRay( &ray, &mtl, &seed, &addDepth );

			updateColor(
				&ray, &newRay, &mtl, &lightRay, lightRaySource, &secondaryPaths,
				specPowerDists, spd, spdTotal, &maxValColor
			);

			// Extend max path depth
			depthAdded += ( addDepth && depthAdded < MAX_ADDED_DEPTH );

			// Russian roulette termination
			if( russianRoulette( depth, depthAdded, maxValColor, &seed ) ) {
				break;
			}

			ray = newRay;
		} // end bounces

		if( light >= 0 ) {
			light *= SPEC;

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

	setColors( imageIn, imageOut, pixelWeight, spdTotal, focus );
	writeDebugImage( imageDebug, scene.debugColor );
}
