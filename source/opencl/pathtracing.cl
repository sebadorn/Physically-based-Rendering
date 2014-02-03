#FILE:pt_header.cl:FILE#
#FILE:pt_utils.cl:FILE#
#FILE:pt_spectral.cl:FILE#
#FILE:pt_kdtree.cl:FILE#



/**
 *
 * @param pos
 * @param imageIn
 * @param imageOut
 * @param pixelWeight
 * @param spdLight
 */
void setColors(
	const int2 pos, read_only image2d_t imageIn, write_only image2d_t imageOut,
	const float pixelWeight, float spdLight[40]
) {
	const float4 imagePixel = read_imagef( imageIn, sampler, pos );
	const float4 accumulatedColor = spectrumToRGB( spdLight );
	const float4 color = mix(
		clamp( accumulatedColor, 0.0f, 1.0f ),
		imagePixel, pixelWeight
	);

	write_imagef( imageOut, pos, color );
}



// KERNELS


/**
 * KERNEL.
 * Generate the initial rays into the scene.
 * @param {const uint2}         offset
 * @param {const float4}        initRayParts
 * @param {const global float*} eyeIn          Camera eye position.
 * @param {global float4*}      origins        Output. The origin of each ray. (The camera eye.)
 * @param {global float4*}      dirs           Output. The generated rays for each pixel.
 * @param {const float}         timeSinceStart
 */
kernel void initRays(
	const uint2 offset, float seed,
	const float4 initRayParts, const global float* eyeIn,
	global ray4* rays
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

	ray4 ray;
	ray.origin = eye;
	ray.t = -2.0f;
	ray.nodeIndex = -1;
	ray.nodeIndex = -1;
	ray.dir = fast_normalize( initialRay );

	rays[workIndex] = ray;
}


/**
 * KERNEL.
 * Search the kd-tree to find the closest intersection of the ray with a surface.
 * @param {const uint2}              offset
 * @param {const float}              timeSinceStart
 * @param {global float4*}           origins
 * @param {global float4*}           dirs
 * @param {const global float4*}     scNormals
 * @param {const global uint*}       scFacesVN
 * @param {const global kdNonLeaf*}  kdNonLeaves
 * @param {const global kdLeaf*}     kdLeaves
 * @param {const global float*}      kdNodeBB
 * @param {const global int*}        kdNodeFaces
 * @param {global float4*}           hits
 * @param {global float4*}           hitNormals
 */
kernel void pathTracing(
	const uint2 offset, float seed, float pixelWeight,
	const global float4* scVertices, const global uint4* scFaces,
	const global float4* scNormals, const global uint* scFacesVN,
	const global kdNonLeaf* kdNonLeaves, const global kdLeaf* kdLeaves,
	const global uint* kdFaces, const float8 kdRootBB,
	global ray4* rays, const global int* faceToMaterial, const global material* materials,
	const global float* specPowerDists,
	read_only image2d_t imageIn, write_only image2d_t imageOut
) {
	// local float specPowerDists[200];

	// if( get_local_id( 0 ) == 1 ) {
	// 	for( int i = 0; i < 200; i++ ) {
	// 		specPowerDists[i] = globalSpecPowerDists[i];
	// 	}
	// }
	// barrier( CLK_LOCAL_MEM_FENCE );

	const int2 pos = {
		offset.x + get_global_id( 0 ),
		offset.y + get_global_id( 1 )
	};
	const uint workIndex = pos.x + pos.y * IMG_WIDTH;

	const float4 bbMinRoot = { kdRootBB.s0, kdRootBB.s1, kdRootBB.s2, 0.0f };
	const float4 bbMaxRoot = { kdRootBB.s4, kdRootBB.s5, kdRootBB.s6, 0.0f };

	ray4 newRay, ray;
	ray4 firstRay = rays[workIndex];
	material mtl;

	float firstEntryDistance, entryDistance;
	float tFar = FLT_MAX;

	float spd[40], spdTotal[40];
	setArray40( spdTotal, 0.0f );

	if( !intersectBoundingBox( firstRay.origin, firstRay.dir, bbMinRoot, bbMaxRoot, &firstEntryDistance, &tFar ) ) {
		setColors( pos, imageIn, imageOut, pixelWeight, spdTotal );
		return;
	}


	float4 H, T;
	float cosLaw, roughness, t, v, vIn, w;
	float isotropy = 1.0f;

	int firstStartNode = goToLeafNode( 0, kdNonLeaves, fma( firstEntryDistance, firstRay.dir, firstRay.origin ) );
	int light, startNode;

	uint index;


	for( uint sample = 0; sample < SAMPLES; sample++ ) {
		setArray40( spd, 1.0f );
		light = -1;
		startNode = firstStartNode;
		entryDistance = firstEntryDistance;
		ray = firstRay;

		for( uint bounce = 0; bounce < BOUNCES; bounce++ ) {
			traverseKdTree(
				&ray, startNode, kdNonLeaves, kdLeaves, kdFaces,
				scVertices, scFaces, entryDistance, FLT_MAX
			);

			if( ray.nodeIndex < 0 ) {
				break;
			}

			ray.normal = scNormals[scFacesVN[ray.faceIndex * 3]];
			mtl = materials[faceToMaterial[ray.faceIndex]];

			// Implicit connection to a light found
			if( mtl.light == 1 ) {
				light = mtl.spd * 40;
				break;
			}

			if( bounce == BOUNCES - 1 ) {
				break;
			}

			// New direction of the ray (bouncing of the hit surface)
			seed += ray.t;
			newRay = getNewRay( ray, mtl, &seed );

			// Influence path towards light source
			if( bounce == BOUNCES - 2 ) {
				// float4 lightTarget = (float4)( -1.0f + rand( &seed ) * 2.0f, 1.995f, 0.0f, 0.0f ) - newRay.origin;
				// float cos_a_max = sqrt( 1.0f - clamp( 0.6f / dot( lightTarget, lightTarget ), 0.0f, 1.0f ) );
				// float cosa = mix( cos_a_max, 1.0f, rand( &seed ) );
				// newRay.dir = jitter( lightTarget, 2.0f * M_PI * rand( &seed ), sqrt( 1.0f - cosa * cosa ), cosa );

				// float4 lightTarget = (float4)( -0.5f + rand( &seed ) * 0.99f, 1.95f, -0.3f + rand( &seed ) * 0.59f, 0.0f ) - newRay.origin;
				// newRay.dir = fast_normalize( newRay.dir + lightTarget * rand( &seed ) * 10.0f );
			}

			cosLaw = cosineLaw( ray.normal, newRay.dir );
			index = mtl.spd * 40;

			for( int i = 0; i < 40; i++ ) {
				spd[i] *= specPowerDists[index + i] * cosLaw;
			}

			entryDistance = 0.0f;
			startNode = ray.nodeIndex;
			ray = newRay;

		} // end bounces

		if( light >= 0 ) {
			for( int i = 0; i < 40; i++ ) {
				spdTotal[i] += spd[i] * specPowerDists[light + i];
			}
		}

	} // end samples

	for( int i = 0; i < 40; i++ ) {
		spdTotal[i] = native_divide( spdTotal[i], (float) SAMPLES );
	}


	setColors( pos, imageIn, imageOut, pixelWeight, spdTotal );
}
