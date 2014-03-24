#FILE:pt_header.cl:FILE#
#FILE:pt_utils.cl:FILE#
#FILE:pt_spectral.cl:FILE#
#FILE:pt_traversal.cl:FILE#



/**
 * Generate the initial ray into the scene.
 * @param  {const int2}          pos
 * @param  {const float4}        initRayParts Diverse parameters that are needed to calculate the ray.
 * @param  {const global float*} eyeIn        Camera eye position.
 * @param  {float*}              seed         Seed for the random number generator.
 * @return {ray4}
 */
ray4 initRay( const int2 pos, const float2 initRayParts, const global float* eyeIn, float* seed ) {
	const float4 eye = { eyeIn[0], eyeIn[1], eyeIn[2], 0.0f };
	const float4 w = { eyeIn[3], eyeIn[4], eyeIn[5], 0.0f };
	const float4 u = { eyeIn[6], eyeIn[7], eyeIn[8], 0.0f };
	const float4 v = { eyeIn[9], eyeIn[10], eyeIn[11], 0.0f };

	#define pxWidth initRayParts.x
	// #define pxHeight initRayParts.y
	#define pxHeight initRayParts.x

	const float4 initialRay = w
			- native_divide( IMG_WIDTH, 2.0f ) * pxWidth * u
			- native_divide( IMG_HEIGHT, 2.0f ) * pxHeight * v
			+ native_divide( pxWidth, 2.0f ) * u
			+ native_divide( pxHeight, 2.0f ) * v
			+ pos.x * pxWidth * u
			+ pos.y * pxHeight * v;

	#undef pxWidth
	#undef pxHeight

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
	const int2 pos, read_only image2d_t imageIn, write_only image2d_t imageOut,
	const float pixelWeight, float spdLight[40], float focus
) {
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
	const ray4 ray, const ray4 newRay, const material mtl,
	const ray4 lightRay, const material lightMTL,
	constant const float* specPowerDists, float* spd, float* spdTotal, float* maxValSpd
) {
	const uint index = mtl.spd * SPEC;
	const float cosLaw = lambert( ray.normal, newRay.dir );

	// BRDF: none
	#if BRDF == 0

		const float u = 1.0f;
		const float brdf = cosLaw;

		#if IMPLICIT == 1
			const float brdfLight = lambert( ray.normal, lightRay.dir );
		#endif

	// BRDF: Schlick
	#elif BRDF == 1

		float4 H;
		float t, v, vIn, vOut, w;

		const float4 groove = groove3D( mtl.scratch, ray.normal );
		getValuesBRDF( newRay.dir, -ray.dir, ray.normal, groove, &H, &t, &v, &vIn, &vOut, &w );
		const float u = fmax( dot( H.xyz, -ray.dir.xyz ), 0.0f );
		const float brdf = D( t, vOut, vIn, w, mtl.rough, mtl.scratch.w ) * cosLaw;

		#if IMPLICIT == 1
			getValuesBRDF( lightRay.dir, -ray.dir, ray.normal, (float4)( 0.0f ), &H, &t, &v, &vIn, &vOut, &w );
			const float uLight = fmax( dot( H.xyz, -ray.dir.xyz ), 0.0f );
			const float brdfLight = D( t, vOut, vIn, w, mtl.rough, mtl.scratch.w ) * lambert( ray.normal, lightRay.dir );
		#endif

	#endif

	#if IMPLICIT == 1
		if( lightRay.t > -1.0f && lightMTL.light == 1 ) {
			int indexLight = lightMTL.spd * SPEC;

			for( int i = 0; i < SPEC; i++ ) {
				spdTotal[i] += spd[i] * specPowerDists[indexLight + i] *
				               fresnel( uLight, specPowerDists[index + i] ) * brdfLight;
			}
		}
	#endif

	for( int i = 0; i < SPEC; i++ ) {
		spd[i] *= fresnel( u, specPowerDists[index + i] ) * brdf;
		*maxValSpd = fmax( spd[i], *maxValSpd );
	}
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
	float seed, float pixelWeight, const float2 initRayParts,
	global const float* eyeIn, global const face_t* faces, global const bvhNode* bvh,
	constant const material* materials, constant const float* specPowerDists,
	read_only image2d_t imageIn, write_only image2d_t imageOut
) {
	const int2 pos = { get_global_id( 0 ), get_global_id( 1 ) };

	float spd[SPEC], spdTotal[SPEC];
	setArray( spdTotal, 0.0f );

	float focus;
	bool addDepth, ignoreColor;


	for( uint sample = 0; sample < SAMPLES; sample++ ) {
		setArray( spd, 1.0f );
		int light = -1;
		ray4 ray = initRay( pos, initRayParts, eyeIn, &seed );
		float maxValSpd = 0.0f;
		int depthAdded = 0;

		for( uint depth = 0; depth < MAX_DEPTH + depthAdded; depth++ ) {
			traverseBVH( bvh, &ray, faces );

			if( ray.t < 0.0f ) {
				break;
			}

			focus = ( depth == 0 ) ? ray.t : focus;

			material mtl = materials[(uint) faces[(uint) ray.normal.w].a.w];
			ray.normal.w = 0.0f;

			// Implicit connection to a light found
			if( mtl.light == 1 ) {
				light = mtl.spd * SPEC;
				break;
			}

			// Last round, no need to calculate a new ray.
			// Unless we hit a material that extends the path.
			if( mtl.d == 1.0f && mtl.rough >= 0.4f && depth == MAX_DEPTH + depthAdded - 1 ) {
				break;
			}

			seed += ray.t;

			ray4 lightRay;
			lightRay.t = -2.0f;
			material lightMTL;

			#if IMPLICIT == 1
				if( mtl.rough > 0.01f ) {
					lightRay.origin = fma( ray.t, ray.dir, ray.origin );
					lightRay.dir = fast_normalize( (float4)( 1.5f - 3.0f * rand( &seed ), 2.0f, 4.0f - 5.0f * rand( &seed ), 0.0f ) - lightRay.origin );

					traverseBVH( bvh, &lightRay, faces );

					if( lightRay.t > -1.0f ) {
						lightMTL = materials[(uint) faces[(uint) lightRay.normal.w].a.w];
						lightRay.normal.w = 0.0f;
					}
				}
			#endif

			// New direction of the ray (bouncing of the hit surface)
			ray4 newRay = getNewRay( ray, &mtl, &seed, &ignoreColor, &addDepth );

			if( !ignoreColor ) {
				updateSPD( ray, newRay, mtl, lightRay, lightMTL, specPowerDists, spd, spdTotal, &maxValSpd );
			}

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

	setColors( pos, imageIn, imageOut, pixelWeight, spdTotal, focus );
}
