#FILE:pt_header.cl:FILE#

#FILE:pt_utils.cl:FILE#

#FILE:pt_kdtree.cl:FILE#



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
 * @param kdRoot
 * @param timeSinceStart
 */
int findPath(
	float4* origin, float4* dir, float4* normal,
	const global float4* scNormals, const global uint* scFacesVN,
	const global float4* lights,
	const global kdNonLeaf* kdNonLeaves, const global kdLeaf* kdLeaves,
	const global float* kdNodeFaces,
	const int startNode, const int kdRoot, const float timeSinceStart, const int bounce,
	const float initEntryDistance, const float initExitDistance
) {
	hit_t hit;
	hit.t = -2.0f;
	hit.position = (float4)( 0.0f, 0.0f, 0.0f, -2.0f );
	hit.nodeIndex = -1;

	traverseKdTree(
		origin, dir, startNode, kdNonLeaves, kdLeaves, kdNodeFaces,
		&hit, bounce, kdRoot, initEntryDistance, initExitDistance
	);

	if( hit.t > -1.0f ) {
		// Normal of hit position
		const uint f = hit.faceIndex * 3;
		*normal = scNormals[scFacesVN[f]];

		// New ray
		*dir = cosineWeightedDirection( timeSinceStart + hit.t, *normal );
		*dir = fast_normalize( *dir ); // WARNING: fast_

		hit.position.w = (float) hit.nodeIndex;
		normal->w = hit.faceIndex;
	}

	*origin = hit.position;

	return hit.nodeIndex;
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
	const uint2 offset, const float4 initRayParts,
	const global float* eyeIn,
	global float4* origins, global float4* dirs,
	const float timeSinceStart
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

	dirs[workIndex] = fast_normalize( initialRay ); // WARNING: fast_
	origins[workIndex] = eye;
}


/**
 * KERNEL.
 * Search the kd-tree to find the closest intersection of the ray with a surface.
 * @param {const uint2}          offset
 * @param {const float}          timeSinceStart
 * @param {const global float4*} lights
 * @param {global float4*}       origins
 * @param {global float4*}       dirs
 * @param {const global float4*} scNormals
 * @param {const global uint*}   scFacesVN
 * @param {const global float*}  kdNodeSplits
 * @param {const global float*}  kdNodeBB
 * @param {const global int*}    kdNodeMeta
 * @param {const global int*}    kdNodeFaces
 * @param {const global int*}    kdNodeRopes
 * @param {const int}            kdRoot
 * @param {global float4*}       hits
 * @param {global float4*}       hitNormals
 */
kernel void pathTracing(
	const uint2 offset, const float timeSinceStart,
	const global float4* lights,
	const global float4* origins, const global float4* dirs,
	const global float4* scNormals, const global uint* scFacesVN,
	const global kdNonLeaf* kdNonLeaves, const global kdLeaf* kdLeaves,
	const float8 kdRootBB,
	const global float* kdNodeFaces, const int kdRoot,
	global float4* hits, global float4* hitNormals
) {
	const int2 pos = { offset.x + get_global_id( 0 ), offset.y + get_global_id( 1 ) };
	const uint workIndex = pos.x + pos.y * IMG_WIDTH;

	const float4 bbMinRoot = { kdRootBB.s0, kdRootBB.s1, kdRootBB.s2, kdRootBB.s3 };
	const float4 bbMaxRoot = { kdRootBB.s4, kdRootBB.s5, kdRootBB.s6, kdRootBB.s7 };

	float4 origin = origins[workIndex];
	float4 dir = dirs[workIndex];
	float4 normal = (float4)( 0.0f );

	float entryDistance = 0.0f;
	float exitDistance = FLT_MAX;

	if( !intersectBoundingBox( &origin, &dir, bbMinRoot, bbMaxRoot, &entryDistance, &exitDistance ) ) {
		hits[workIndex * BOUNCES] = (float4)( 0.0f, 0.0f, 0.0f, -2.0f );
		return;
	}

	int startNode = goToLeafNode( kdRoot, kdNonLeaves, fma( entryDistance, dir, origin ) );

	for( int bounce = 0; bounce < BOUNCES; bounce++ ) {
		startNode = findPath(
			&origin, &dir, &normal, scNormals, scFacesVN, lights,
			kdNonLeaves, kdLeaves, kdNodeFaces,
			startNode, kdRoot, timeSinceStart + bounce, bounce, entryDistance, exitDistance
		);

		hits[workIndex * BOUNCES + bounce] = origin;
		hitNormals[workIndex * BOUNCES + bounce] = normal;

		if( startNode < 0 ) {
			break;
		}

		entryDistance = 0.0f;
		exitDistance = FLT_MAX;
	}

	// How to use a barrier:
	// barrier( CLK_LOCAL_MEM_FENCE );
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
 * @param {const int} kdRoot
 * @param {global float4*} hits
 * @param {const global float4* hitNormals}
 */
kernel void shadowTest(
	const uint2 offset, const float timeSinceStart, const global float4* lights,
	const global kdNonLeaf* kdNonLeaves, const global kdLeaf* kdLeaves,
	const float8 kdRootBB, const global float* kdNodeFaces,
	const int kdRoot,
	global float4* hits, const global float4* hitNormals
) {
	const int2 pos = { offset.x + get_global_id( 0 ), offset.y + get_global_id( 1 ) };
	const uint workIndex = pos.x + pos.y * IMG_WIDTH;

	const float4 bbMinRoot = { kdRootBB.s0, kdRootBB.s1, kdRootBB.s2, kdRootBB.s3 };
	const float4 bbMaxRoot = { kdRootBB.s4, kdRootBB.s5, kdRootBB.s6, kdRootBB.s7 };

	int exitRope;
	uint index, startNode;

	float4 normal, origin;
	const float4 light = lights[0];
	bool isInShadow;

	for( int bounce = 0; bounce < BOUNCES; bounce++ ) {
		index = workIndex * BOUNCES + bounce;
		origin = hits[index];
		normal = hitNormals[index];

		if( origin.w <= -1.0f ) {
			break;
		}

		startNode = (uint) origin.w;
		origin.w = 0.0f;
		normal.w = 0.0f;

		float4 newLight = light - origin + uniformlyRandomVector( timeSinceStart + fast_length( origin ) ) * 0.1f;
		float4 originForShadow = origin + normal * EPSILON;

		isInShadow = shadowTestIntersection(
			&originForShadow, &newLight, startNode, kdNonLeaves, kdLeaves, kdNodeFaces, kdRoot
		);

		origin.w = !isInShadow;
		hits[index] = origin;
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
	const global float4* hits, const global float4* normals,
	const global float4* lights,
	const global int* materialToFace, const global float4* diffuseColors,
	read_only image2d_t textureIn, write_only image2d_t textureOut
) {
	const int2 pos = { offset.x + get_global_id( 0 ), offset.y + get_global_id( 1 ) };

	float4 hit, normal, surfaceColor, toLight;
	float diffuse, luminosity, shadowIntensity, specularHighlight, toLightLength;
	int face, material;

	float4 accumulatedColor = (float4)( 0.0f );
	float4 colorMask = (float4)( 1.0f, 1.0f, 1.0f, 0.0f );

	const float4 light = lights[0];
	const float4 newLight = light + uniformlyRandomVector( timeSinceStart ) * 0.1f;

	const uint workIndex = ( pos.x + pos.y * IMG_WIDTH ) * BOUNCES;
	const float4 texturePixel = read_imagef( textureIn, sampler, pos );
	float4 prefetch_hit = hits[workIndex];

	for( int bounce = 0; bounce < BOUNCES; bounce++ ) {
		hit = prefetch_hit;

		if( hit.w < -1.0f ) {
			break;
		}

		normal = normals[workIndex + bounce];

		shadowIntensity = hit.w;
		hit.w = 0.0f;

		face = normal.w;
		material = materialToFace[face];
		normal.w = 0.0f;

		surfaceColor = diffuseColors[material];
		prefetch_hit = hits[workIndex + bounce + 1];

		// The farther away a shadow is, the more diffuse it becomes (penumbrae)
		toLight = newLight - hit;

		// Lambert factor (dot product is a cosine)
		// Source: http://learnopengles.com/android-lesson-two-ambient-and-diffuse-lighting/
		diffuse = fmax( dot( normal, fast_normalize( toLight ) ), 0.0f );
		toLightLength = fast_length( toLight );
		luminosity = native_recip( toLightLength * toLightLength );

		specularHighlight = 0.0f; // Disabled for now

		colorMask *= surfaceColor;
		accumulatedColor += colorMask * luminosity * diffuse * shadowIntensity;
		accumulatedColor += colorMask * specularHighlight * shadowIntensity;
	}

	const float4 color = mix( accumulatedColor, texturePixel, textureWeight );
	write_imagef( textureOut, pos, color );
}
