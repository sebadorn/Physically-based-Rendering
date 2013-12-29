#FILE:pt_header.cl:FILE#

#FILE:pt_utils.cl:FILE#

#FILE:pt_kdtree.cl:FILE#

#FILE:pt_specrend.cl:FILE#



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
	ray4* ray, const global float4* scNormals,
	const global uint* scFacesVN, const global float4* lights,
	int startNode, const global kdNonLeaf* kdNonLeaves,
	const global kdLeaf* kdLeaves, const global float* kdNodeFaces,
	const float timeSinceStart, const int bounce,
	const float entryDistance, const float exitDistance
) {
	traverseKdTree(
		ray, startNode, kdNonLeaves, kdLeaves, kdNodeFaces,
		bounce, entryDistance, exitDistance
	);

	ray4 newRay;
	newRay.t = -2.0f;
	newRay.nodeIndex = -1;
	newRay.faceIndex = -1;

	if( ray->nodeIndex >= 0 ) {
		const uint f = ray->faceIndex * 3;
		ray->normal = scNormals[scFacesVN[f]];

		newRay.origin = fma( ray->t, ray->dir, ray->origin );
		newRay.dir = fast_normalize( reflect( ray->dir, ray->normal ) );
		// newRay.dir = fast_normalize( cosineWeightedDirection( timeSinceStart + ray->t, newRay.normal ) );
	}

	return newRay;
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
 * @param {const global float4*}     lights
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
	const uint2 offset, const float timeSinceStart, const global float4* lights,
	const global float4* scNormals, const global uint* scFacesVN,
	const global kdNonLeaf* kdNonLeaves, const global kdLeaf* kdLeaves,
	const float8 kdRootBB, const global float* kdNodeFaces,
	global ray4* rays
) {
	const int2 pos = {
		offset.x + get_global_id( 0 ),
		offset.y + get_global_id( 1 )
	};
	const uint workIndex = pos.x + pos.y * IMG_WIDTH;

	const float4 bbMinRoot = { kdRootBB.s0, kdRootBB.s1, kdRootBB.s2, 0.0f };
	const float4 bbMaxRoot = { kdRootBB.s4, kdRootBB.s5, kdRootBB.s6, 0.0f };

	ray4 newRay, ray;
	ray = rays[workIndex * BOUNCES];
	ray.t = -2.0f;
	ray.nodeIndex = -1;
	ray.faceIndex = -1;

	float entryDistance = 0.0f;
	float exitDistance = FLT_MAX;

	if( !intersectBoundingBox( ray.origin, ray.dir, bbMinRoot, bbMaxRoot, &entryDistance, &exitDistance ) ) {
		rays[workIndex * BOUNCES] = ray;
		return;
	}

	int startNode = goToLeafNode( 0, kdNonLeaves, fma( entryDistance, ray.dir, ray.origin ) );

	for( int bounce = 0; bounce < BOUNCES; bounce++ ) {
		newRay = findPath(
			&ray, scNormals, scFacesVN, lights,
			startNode, kdNonLeaves, kdLeaves, kdNodeFaces,
			timeSinceStart + bounce, bounce, entryDistance, exitDistance
		);

		rays[workIndex * BOUNCES + bounce] = ray;

		if( ray.nodeIndex < 0 ) {
			break;
		}

		startNode = ray.nodeIndex;
		ray = newRay;
		entryDistance = 0.0f;
		exitDistance = FLT_MAX;
	}
}


/**
 * KERNEL.
 * Test hit surfaces for being in a shadow.
 * @param {const uint2} offset
 * @param {const float} timeSinceStart
 * @param {const global float4*} lights
 * @param {const global float4*} kdNodeSplits
 * @param {const global float4*} kdNodeBB
 * @param {const gloabl int*} kdNodeMeta
 * @param {const global float*} kdNodeFaces
 * @param {const global int*} kdNodeRopes
 * @param {global ray4*} rays
 */
kernel void shadowTest(
	const uint2 offset, const float timeSinceStart, const global float4* lights,
	const global kdNonLeaf* kdNonLeaves, const global kdLeaf* kdLeaves,
	const float8 kdRootBB, const global float* kdNodeFaces,
	global ray4* rays
) {
	const int2 pos = { offset.x + get_global_id( 0 ), offset.y + get_global_id( 1 ) };
	const uint workIndex = pos.x + pos.y * IMG_WIDTH;

	#ifdef SHADOWS
		const float4 bbMinRoot = { kdRootBB.s0, kdRootBB.s1, kdRootBB.s2, kdRootBB.s3 };
		const float4 bbMaxRoot = { kdRootBB.s4, kdRootBB.s5, kdRootBB.s6, kdRootBB.s7 };

		int exitRope;
		uint startNode;
		const float4 light = lights[0];
		bool isInShadow;
	#endif

	float4 hit;
	uint index = workIndex * BOUNCES;
	ray4 ray;

	for( int bounce = 0; bounce < BOUNCES; bounce++ ) {
		ray = rays[index + bounce];

		if( ray.nodeIndex < 0 ) {
			break;
		}

		#ifdef SHADOWS
			hit = fma( ray.t, ray.dir, ray.origin );

			ray4 shadowRay;
			shadowRay.nodeIndex = ray.nodeIndex;
			shadowRay.origin = hit + ray.normal * EPSILON;
			shadowRay.dir = light - hit + uniformlyRandomVector( timeSinceStart + fast_length( hit ) ) * 0.1f;

			isInShadow = shadowTestIntersection( &shadowRay, kdNonLeaves, kdLeaves, kdNodeFaces );

			ray.shadow = !isInShadow;
		#else
			ray.shadow = 1.0f;
		#endif

		rays[index + bounce] = ray;
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
	const uint2 offset, const float timeSinceStart, const float textureWeight,
	const global ray4* rays, const global float4* lights,
	const global int* faceToMaterial, const global float4* diffuseColors,
	read_only image2d_t textureIn, write_only image2d_t textureOut
) {
	const int2 pos = { offset.x + get_global_id( 0 ), offset.y + get_global_id( 1 ) };

	float4 hit, reflectedLight, surfaceColor, toLight;
	float diffuse, luminosity, shadowIntensity, specularHighlight, toLightLength;
	int face, material;

	float4 accumulatedColor = (float4)( 0.0f );
	float4 colorMask = (float4)( 1.0f, 1.0f, 1.0f, 0.0f );

	const float4 light = lights[0];
	const float4 newLight = light + uniformlyRandomVector( timeSinceStart ) * 0.1f;

	const uint workIndex = ( pos.x + pos.y * IMG_WIDTH ) * BOUNCES;
	const float4 texturePixel = read_imagef( textureIn, sampler, pos );
	ray4 ray;

	for( int bounce = 0; bounce < BOUNCES; bounce++ ) {
		ray = rays[workIndex + bounce];

		if( ray.nodeIndex < 0 ) {
			break;
		}

		hit = fma( ray.t, ray.dir, ray.origin );

		material = faceToMaterial[ray.faceIndex];
		surfaceColor = diffuseColors[material];
		// surfaceColor = wavelengthToRGB( 600.0f );

		// The farther away a shadow is, the more diffuse it becomes (penumbrae)
		toLight = newLight - hit;

		// Lambert factor (dot product is a cosine)
		// Source: http://learnopengles.com/android-lesson-two-ambient-and-diffuse-lighting/
		diffuse = fmax( dot( ray.normal, fast_normalize( toLight ) ), 0.0f );
		toLightLength = fast_length( toLight );
		luminosity = native_recip( toLightLength * toLightLength );

		reflectedLight = fast_normalize( reflect( toLight, ray.normal ) );
		specularHighlight = fmax( 0.0f, dot( reflectedLight, fast_normalize( hit - ray.origin ) ) );
		specularHighlight = 2.0f * pow( specularHighlight, 20.0f );

		colorMask *= surfaceColor;
		accumulatedColor += colorMask * luminosity * diffuse * ray.shadow;
		accumulatedColor += colorMask * specularHighlight * ray.shadow;
	}

	const float4 color = mix(
		clamp( accumulatedColor, 0.0f, 1.0f ),
		texturePixel, textureWeight
	);
	write_imagef( textureOut, pos, color );
}
