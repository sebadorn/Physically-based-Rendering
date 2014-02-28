#FILE:pt_header.cl:FILE#
#FILE:pt_utils.cl:FILE#
#FILE:pt_spectral.cl:FILE#
#FILE:pt_traversal.cl:FILE#



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
	const float4 imagePixel = read_imagef( imageIn, sampler, pos );
	const float4 accumulatedColor = spectrumToRGB( spdLight );
	const float4 color = mix(
		clamp( accumulatedColor, 0.0f, 1.0f ),
		imagePixel, pixelWeight
	);
	color.w = focus;

	write_imagef( imageOut, pos, color );
}



// KERNELS


/**
 * KERNEL.
 * Generate the initial rays into the scene.
 * @param {const uint2}         offset       Pixel offset for the current image tile.
 * @param {float}               seed         Seed for the random number generator.
 * @param {const float4}        initRayParts Diverse parameters that are needed to calculate the ray.
 * @param {const global float*} eyeIn        Camera eye position.
 * @param {global rayBase*}     rays         Output. Array for the created rays.
 */
kernel void initRays(
	const uint2 offset, float seed,
	const float4 initRayParts, const global float* eyeIn,
	global rayBase* rays
) {
	const int2 pos = { offset.x + get_global_id( 0 ), offset.y + get_global_id( 1 ) };

	float4 eye = { eyeIn[0], eyeIn[1], eyeIn[2], 0.0f };
	const float4 w = { eyeIn[3], eyeIn[4], eyeIn[5], 0.0f };
	const float4 u = { eyeIn[6], eyeIn[7], eyeIn[8], 0.0f };
	const float4 v = { eyeIn[9], eyeIn[10], eyeIn[11], 0.0f };

	#ifdef ANTI_ALIASING
		eye += uniformlyRandomVector( &seed ) * 0.005f;
	#endif

	#define pxWidthAndHeight initRayParts.x
	#define adjustW initRayParts.y
	#define adjustH initRayParts.z
	#define pxwhHalf initRayParts.w

	const uint workIndex = pos.x + pos.y * IMG_WIDTH;
	const float wgsHalfMulPxWH = WORKGROUPSIZE_HALF * pxWidthAndHeight;

	const float4 initialRay = w
			+ wgsHalfMulPxWH * ( v - u )
			+ pxwhHalf * ( u - v )
			+ ( pos.x - adjustW ) * pxWidthAndHeight * u
			- ( WORKGROUPSIZE - pos.y + adjustH ) * pxWidthAndHeight * v;

	rayBase ray;
	ray.origin = eye;
	ray.dir = fast_normalize( initialRay );

	rays[workIndex] = ray;
}


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
	const uint2 offset, float seed, float pixelWeight,
	const global face_t* faces, const global bvhNode* bvh,
	const global rayBase* initRays, const global material* materials,
	const global float* specPowerDists,
	read_only image2d_t imageIn, write_only image2d_t imageOut
) {
	const int2 pos = {
		offset.x + get_global_id( 0 ),
		offset.y + get_global_id( 1 )
	};
	const uint workIndex = pos.x + pos.y * IMG_WIDTH;

	float spd[SPEC], spdTotal[SPEC];
	setArray( spdTotal, 0.0f );

	ray4 firstRay;
	firstRay.origin = initRays[workIndex].origin;
	firstRay.dir = initRays[workIndex].dir;
	firstRay.t = -2.0f;

	ray4 newRay, ray;
	material mtl;

	float brdf, cosLaw, focus, maxValSpd;
	int depthAdded, light;
	uint index;
	bool addDepth, ignoreColor;

	for( uint sample = 0; sample < SAMPLES; sample++ ) {
		setArray( spd, 1.0f );
		light = -1;
		ray = firstRay;
		maxValSpd = 0.0f;
		depthAdded = 0;

		for( uint depth = 0; depth < MAX_DEPTH + depthAdded; depth++ ) {
			traverseBVH( bvh, &ray, faces );

			if( ray.t < 0.0f ) {
				break;
			}

			if( depth == 0 ) {
				focus = ray.t;
			}

			mtl = materials[(uint) faces[(uint) ray.normal.w].a.w];
			ray.normal.w = 0.0f;

			// Implicit connection to a light found
			if( mtl.light == 1 ) {
				light = mtl.spd * SPEC;
				break;
			}

			// Last round, no need to calculate a new ray.
			// Unless we hit a material that extends the path.
			if( mtl.d == 1.0f && mtl.illum != 3 && depth == MAX_DEPTH + depthAdded - 1 ) {
				break;
			}

			// New direction of the ray (bouncing of the hit surface)
			seed += ray.t;
			newRay = getNewRay( ray, mtl, &seed, &ignoreColor, &addDepth );

			if( !ignoreColor ) {
				index = mtl.spd * SPEC;
				cosLaw = cosineLaw( ray.normal, newRay.dir );

				// float4 H;
				// float t, v, vIn, vOut, w;
				// float r = 1.0f;
				// getValuesBRDF( newRay.dir, -ray.dir, ray.normal, mtl.scratch, &H, &t, &v, &vIn, &vOut, &w );
				// float u = fmax( dot( H, -ray.dir ), 0.0f );
				// brdf = D( t, vOut, vIn, w, r, mtl.scratch.w );
				brdf = 1.0f;

				for( int i = 0; i < SPEC; i++ ) {
					spd[i] *= specPowerDists[index + i] * brdf * cosLaw;
					maxValSpd = fmax( spd[i], maxValSpd );
				}
			}

			// Extend max path depth
			depthAdded += ( addDepth && depthAdded < MAX_ADDED_DEPTH );

			// Russian roulette termination
			if( depth > 2 && maxValSpd < rand( &seed ) ) {
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
		spdTotal[i] = native_divide( spdTotal[i], (float) SAMPLES );
	}


	setColors( pos, imageIn, imageOut, pixelWeight, spdTotal, focus );
}
