#FILE:pt_header.cl:FILE#

#FILE:pt_utils.cl:FILE#

#FILE:pt_kdtree.cl:FILE#

#FILE:pt_spectral.cl:FILE#



/**
 *
 * @param origin
 * @param dir
 * @param normal
 * @param scVertices
 * @param scFaces
 * @param scNormals
 * @param scFacesVN
 * @param lights
 * @param kdNodeData1
 * @param kdNodeMeta
 * @param kdNodeFaces
 * @param kdNodeRopes
 * @param timeSinceStart
 */
ray4 findPath(
	ray4* ray, const global float4* scVertices, const global uint4* scFaces,
	const global float4* scNormals, const global uint* scFacesVN,
	int startNode, const global kdNonLeaf* kdNonLeaves,
	const global kdLeaf* kdLeaves, const global uint* kdFaces,
	const float timeSinceStart, const int bounce,
	const float entryDistance, const float exitDistance,
	const global int* faceToMaterial, const global material* materials
) {
	traverseKdTree(
		ray, startNode, kdNonLeaves, kdLeaves, kdFaces,
		scVertices, scFaces, bounce, entryDistance, exitDistance
	);

	ray4 newRay;
	newRay.t = -2.0f;
	newRay.nodeIndex = -1;
	newRay.faceIndex = -1;

	if( ray->nodeIndex >= 0 ) {
		const uint f = ray->faceIndex;
		ray->normal = scNormals[scFacesVN[f * 3]];

		if( bounce < BOUNCES - 1 ) {
			newRay.origin = fma( ray->t, ray->dir, ray->origin );

			material mtl = materials[faceToMaterial[f]];

			if( mtl.d < 1.0f ) {
				newRay.dir = ray->dir;
			}
			else {
				newRay.dir = ( mtl.illum == 3 )
				           ? fast_normalize( reflect( ray->dir, ray->normal ) + uniformlyRandomVector( timeSinceStart + ray->t ) * mtl.gloss )
				           : fast_normalize( cosineWeightedDirection( timeSinceStart + ray->t, ray->normal ) );
			}

			if( mtl.light == 1 ) {
				newRay.nodeIndex = -2;
			}
		}
	}

	return newRay;
}


/**
 *
 * @param  {float4} light
 * @param  {float4} hit
 * @param  {ray4}   ray
 * @param  {float}  Ns
 * @return {float}
 */
float calcSpecularHighlight( float4 light, float4 hit, ray4 ray, float Ns ) {
	float specularHighlight;

	#if SPECULAR_HIGHLIGHT == 0

		// Disabled
		specularHighlight = 0.0f;

	#elif SPECULAR_HIGHLIGHT == 1

		// Phong
		float4 reflectedLight = fast_normalize( reflect( light - hit, ray.normal ) );
		specularHighlight = fmax( 0.0f, dot( reflectedLight, fast_normalize( hit - ray.origin ) ) );
		specularHighlight = pow( specularHighlight, Ns );

	#elif SPECULAR_HIGHLIGHT == 2

		// Blinn-Phong
		float4 H = fast_normalize( ( light - hit ) + ( ray.origin - hit ) );
		specularHighlight = fmax( 0.0f, dot( ray.normal, H ) );
		specularHighlight = pow( specularHighlight, Ns );

	#elif SPECULAR_HIGHLIGHT == 3

		// Gauss
		float4 H = fast_normalize( ( light - hit ) + ( ray.origin - hit ) );
		float angle = acos( clamp( dot( ray.normal, H ), 0.0f, 1.0f ) );
		float exponent = angle * Ns;
		exponent = -( exponent * exponent );
		specularHighlight = exp( exponent );

	#endif

	return specularHighlight;
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
	const uint2 offset, const float timeSinceStart,
	const float4 initRayParts, const global float* eyeIn,
	global ray4* rays
) {
	const int2 pos = { offset.x + get_global_id( 0 ), offset.y + get_global_id( 1 ) };

	float4 eye = { eyeIn[0], eyeIn[1], eyeIn[2], 0.0f };
	const float4 w = { eyeIn[3], eyeIn[4], eyeIn[5], 0.0f };
	const float4 u = { eyeIn[6], eyeIn[7], eyeIn[8], 0.0f };
	const float4 v = { eyeIn[9], eyeIn[10], eyeIn[11], 0.0f };

	#ifdef ANTI_ALIASING
		eye += uniformlyRandomVector( timeSinceStart ) * 0.005f;
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
	ray.dir = fast_normalize( initialRay );

	rays[workIndex * BOUNCES] = ray;
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
	const uint2 offset, const float timeSinceStart,
	const global float4* scVertices, const global uint4* scFaces,
	const global float4* scNormals, const global uint* scFacesVN,
	const global kdNonLeaf* kdNonLeaves, const global kdLeaf* kdLeaves,
	const global uint* kdFaces, const float8 kdRootBB,
	global ray4* rays, const global int* faceToMaterial, const global material* materials
) {
	const int2 pos = {
		offset.x + get_global_id( 0 ),
		offset.y + get_global_id( 1 )
	};
	const uint workIndex = ( pos.x + pos.y * IMG_WIDTH ) * BOUNCES;

	const float4 bbMinRoot = { kdRootBB.s0, kdRootBB.s1, kdRootBB.s2, 0.0f };
	const float4 bbMaxRoot = { kdRootBB.s4, kdRootBB.s5, kdRootBB.s6, 0.0f };

	ray4 newRay;
	ray4 ray = rays[workIndex];
	ray.t = -2.0f;
	ray.nodeIndex = -1;
	ray.faceIndex = -1;
	ray.shadow = 1.0f;

	float entryDistance = 0.0f;
	float exitDistance = FLT_MAX;

	if( !intersectBoundingBox( ray.origin, ray.dir, bbMinRoot, bbMaxRoot, &entryDistance, &exitDistance ) ) {
		rays[workIndex] = ray;
		return;
	}

	int startNode = goToLeafNode( 0, kdNonLeaves, fma( entryDistance, ray.dir, ray.origin ) );

	for( int bounce = 0; bounce < BOUNCES; bounce++ ) {
		newRay = findPath(
			&ray, scVertices, scFaces, scNormals, scFacesVN,
			startNode, kdNonLeaves, kdLeaves, kdFaces,
			timeSinceStart + (float) bounce, bounce, entryDistance, exitDistance,
			faceToMaterial, materials
		);

		rays[workIndex + bounce] = ray;

		if( ray.nodeIndex < 0 || newRay.nodeIndex == -2 ) {
			break;
		}

		startNode = ray.nodeIndex;
		ray = newRay;
		entryDistance = 0.0f;
		exitDistance = FLT_MAX;
	}
}


/**
 *
 * @param offset
 * @param timeSinceStart
 * @param textureWeight
 * @param hits
 * @param normals
 * @param lights
 * @param materialToFace
 * @param diffuseColors
 * @param textureIn
 * @param textureOut
 */
kernel void setColors(
	const uint2 offset, const float timeSinceStart, float imageWeight,
	const global ray4* rays, const global int* faceToMaterial,
	const global material* materials, const global float* specPowerDists,
	read_only image2d_t imageIn, write_only image2d_t imageOut
) {
	const int2 pos = { offset.x + get_global_id( 0 ), offset.y + get_global_id( 1 ) };
	const uint workIndex = ( pos.x + pos.y * IMG_WIDTH ) * BOUNCES;
	const float4 imagePixel = read_imagef( imageIn, sampler, pos );

	float4 accumulatedColor = (float4)( 0.0f );
	ray4 ray;
	material mtl;

	// Spectral power distribution of the light
	float spdFactors[40];
	float spdLight[40];
	bool foundLight = false;

	for( int i = 0; i < 40; i++ ) {
		spdFactors[i] = 1.0f;
	}
	float brightness = 1.0f;

	for( int bounce = 0; bounce < BOUNCES; bounce++ ) {
		ray = rays[workIndex + bounce];

		if( ray.nodeIndex < 0 ) {
			break;
		}

		material mtl = materials[faceToMaterial[ray.faceIndex]];
		brightness *= mtl.b;

		if( mtl.light == 1 ) {
			foundLight = true;
			for( int i = 0; i < 40; i++ ) {
				spdLight[i] = specPowerDists[mtl.spdDiffuse * 40 + i];
			}
			break;
		}
		else {
			for( int i = 0; i < 40; i++ ) {
				spdFactors[i] *= specPowerDists[mtl.spdDiffuse * 40 + i];
			}
		}
	}

	if( foundLight ) {
		for( int i = 0; i < 40; i++ ) {
			spdLight[i] *= spdFactors[i];
		}

		accumulatedColor = brightness * spectrumToRGB( spdLight );
		// float lum = 0.2126f * accumulatedColor.x + 0.7152f * accumulatedColor.y + 0.0722f * accumulatedColor.z;
	}

	const float4 color = mix(
		clamp( accumulatedColor, 0.0f, 1.0f ),
		imagePixel, imageWeight
	);
	write_imagef( imageOut, pos, color );
}
