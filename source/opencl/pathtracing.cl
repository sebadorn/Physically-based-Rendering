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
 * @param kdNodeData2
 * @param kdNodeData3
 * @param kdNodeRopes
 * @param kdRoot
 * @param timeSinceStart
 */
int findPath(
	float4* origin, float4* dir, float4* normal,
	const __global float4* scVertices, const __global uint4* scFaces,
	const __global float4* scNormals, const __global uint* scFacesVN,
	const __global float4* lights,
	const __global float4* kdNodeSplits, const __global float4* kdNodeBbMin, const __global float4* kdNodeBbMax,
	const __global int* kdNodeData2,
	const __global int* kdNodeData3, const __global int* kdNodeRopes,
	const int startNode, const int kdRoot, const float timeSinceStart, const int bounce,
	const float initEntryDistance, const float initExitDistance
) {
	hit_t hit;
	hit.t = -2.0f;
	hit.position = (float4)( 0.0f, 0.0f, 0.0f, -2.0f );
	hit.nodeIndex = -1;

	traverseKdTree(
		origin, dir, startNode, scVertices, scFaces,
		kdNodeSplits, kdNodeBbMin, kdNodeBbMax, kdNodeData2, kdNodeData3, kdNodeRopes,
		&hit, bounce, kdRoot, initEntryDistance, initExitDistance
	);

	if( hit.t > -1.0f ) {
		// Normal of hit position
		const uint f = hit.faceIndex * 3;
		*normal = scNormals[scFacesVN[f]];

		// New ray
		*dir = cosineWeightedDirection( timeSinceStart + hit.t, *normal );
		*dir = fast_normalize( *dir ); // WARNING: fast_

		#ifdef SHADOWS

			const float4 light = lights[0];
			const float4 newLight = light + uniformlyRandomVector( timeSinceStart + hit.t ) * 0.1f;
			float4 originForShadow = hit.position + (*normal) * EPSILON;
			float4 toLight = newLight - hit.position;

			bool isInShadow = shadowTest(
				&originForShadow, &toLight, hit.nodeIndex, scVertices, scFaces,
				kdNodeSplits, kdNodeBbMin, kdNodeBbMax, kdNodeData2, kdNodeData3, kdNodeRopes, kdRoot,
				initEntryDistance, initExitDistance
			);

			hit.position.w = !isInShadow;

		#else

			hit.position.w = 1.0f;

		#endif

		normal->w = hit.faceIndex;
	}

	*origin = hit.position;

	return hit.nodeIndex;
}



// KERNELS


/**
 * KERNEL.
 * Generate the initial rays into the scene.
 * @param {const uint}             offsetW
 * @param {const uint}             offsetH
 * @param {const float4}           initRayParts
 * @param {const __global float*}  eyeIn        Camera eye position.
 * @param {__global float4*}       origins      Output. The origin of each ray. (The camera eye.)
 * @param {__global float4*}       dirs         Output. The generated rays for each pixel.
 */
__kernel __attribute__( ( work_group_size_hint( WORKGROUPSIZE, WORKGROUPSIZE, 1 ) ) )
void initRays(
	const uint2 offset, const float4 initRayParts,
	const __global float* eyeIn,
	__global float4* origins, __global float4* dirs,
	const float timeSinceStart
) {
	const int2 pos = { offset.x + get_global_id( 0 ), offset.y + get_global_id( 1 ) };
	const uint workIndex = pos.x + pos.y * IMG_WIDTH;

	float4 eye = { eyeIn[0], eyeIn[1], eyeIn[2], 0.0f };
	float4 w = { eyeIn[3], eyeIn[4], eyeIn[5], 0.0f };
	float4 u = { eyeIn[6], eyeIn[7], eyeIn[8], 0.0f };
	float4 v = { eyeIn[9], eyeIn[10], eyeIn[11], 0.0f };

	#ifdef ANTI_ALIASING
		eye += uniformlyRandomVector( timeSinceStart ) * 0.005f;
	#endif

	const float pxWidthAndHeight = initRayParts.x;
	const float adjustW = initRayParts.y;
	const float adjustH = initRayParts.z;
	const float pxwhHalf = initRayParts.w;

	const float wgsHalfMulPxWH = WORKGROUPSIZE_HALF * pxWidthAndHeight;

	float4 initialRay = eye + w
			+ wgsHalfMulPxWH * ( v - u )
			+ pxwhHalf * ( u - v )
			+ ( pos.x - adjustW ) * pxWidthAndHeight * u
			- ( WORKGROUPSIZE - pos.y + adjustH ) * pxWidthAndHeight * v;

	dirs[workIndex] = fast_normalize( initialRay - eye ); // WARNING: fast_
	origins[workIndex] = eye;
}


/**
 * KERNEL.
 * Search the kd-tree to find the closest intersection of the ray with a surface.
 * @param {const uint}             offsetW
 * @param {const uint}             offsetH
 * @param {__global float4*}       origins
 * @param {__global float4*}       rays
 * @param {__global float4*}       normals
 * @param {const __global float*}  scVertices
 * @param {const __global uint*}   scFaces
 * @param {const __global float*}  scNormals
 * @param {const __global uint*}   scFacesVN
 * @param {const __global float4*} lights
 * @param {const __global float*}  kdNodeData1
 * @param {const __global int*}    kdNodeData2
 * @param {const __global int*}    kdNodeData3
 * @param {const __global int*}    kdNodeRopes
 * @param {const uint}             kdRoot
 * @param {const float}            timeSinceStart
 */
__kernel __attribute__( ( work_group_size_hint( WORKGROUPSIZE, WORKGROUPSIZE, 1 ) ) )
void pathTracing(
	const uint2 offset, const float timeSinceStart,
	const __global float4* lights,
	const __global float4* origins, const __global float4* dirs,
	const __global float4* scVertices, const __global uint4* scFaces,
	const __global float4* scNormals, const __global uint* scFacesVN,
	const __global float4* kdNodeSplits, const __global float4* kdNodeBbMin, const __global float4* kdNodeBbMax,
	const __global int* kdNodeData2,
	const __global int* kdNodeData3, const __global int* kdNodeRopes,
	const int kdRoot,

	__global float4* hits, __global float4* hitNormals
) {
	const int2 pos = { offset.x + get_global_id( 0 ), offset.y + get_global_id( 1 ) };
	const uint workIndex = pos.x + pos.y * IMG_WIDTH;

	int startNode = kdRoot;

	const float4 bbMinRoot = kdNodeBbMin[kdRoot];
	const float4 bbMaxRoot = kdNodeBbMax[kdRoot];

	float4 origin = origins[workIndex];
	float4 dir = dirs[workIndex];
	float4 normal = (float4)( 0.0f );

	float entryDistance, exitDistance;
	int exitRope;

	if( !intersectBoundingBox( &origin, &dir, (float*) &bbMinRoot, (float*) &bbMaxRoot, &entryDistance, &exitDistance, &exitRope ) ) {
		hits[workIndex * BOUNCES] = (float4)( 0.0f, 0.0f, 0.0f, -2.0f );
		return;
	}

	for( int bounce = 0; bounce < BOUNCES; bounce++ ) {
		startNode = findPath(
			&origin, &dir, &normal, scVertices, scFaces, scNormals, scFacesVN,
			lights, kdNodeSplits, kdNodeBbMin, kdNodeBbMax, kdNodeData2, kdNodeData3, kdNodeRopes,
			startNode, kdRoot, timeSinceStart + bounce, bounce, entryDistance, exitDistance
		);

		hits[workIndex * BOUNCES + bounce] = origin;
		hitNormals[workIndex * BOUNCES + bounce] = normal;

		if( startNode < 0 ) {
			break;
		}

		entryDistance = 0.0f;
		exitDistance = fast_length( origin + bbMaxRoot - bbMinRoot );
	}

	// How to use a barrier:
	// barrier( CLK_LOCAL_MEM_FENCE );
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
__kernel __attribute__( ( work_group_size_hint( WORKGROUPSIZE, WORKGROUPSIZE, 1 ) ) )
void setColors(
	const uint2 offset, const float timeSinceStart, const float textureWeight,
	const __global float4* hits, const __global float4* normals,
	const __global float4* lights,
	const __global int* materialToFace, const __global float4* diffuseColors,
	__read_only image2d_t textureIn, __write_only image2d_t textureOut
) {
	const int2 pos = { offset.x + get_global_id( 0 ), offset.y + get_global_id( 1 ) };
	const uint workIndex = pos.x + pos.y * IMG_WIDTH;

	float4 hit, normal, surfaceColor, toLight;
	float diffuse, luminosity, shadowIntensity, specularHighlight, toLightLength;
	int face, material;

	float4 accumulatedColor = (float4)( 0.0f );
	float4 colorMask = (float4)( 1.0f, 1.0f, 1.0f, 0.0f );

	const float4 light = lights[0];
	const float4 newLight = light + uniformlyRandomVector( timeSinceStart ) * 0.1f;


	for( int bounce = 0; bounce < BOUNCES; bounce++ ) {
		hit = hits[workIndex * BOUNCES + bounce];

		if( hit.w < -1.0f ) {
			break;
		}

		normal = normals[workIndex * BOUNCES + bounce];
		shadowIntensity = hit.w;
		hit.w = 0.0f;

		// TODO: This is ugly, seperate the (unrelated) data
		face = normal.w;
		material = materialToFace[face];
		normal.w = 0.0f;

		surfaceColor = diffuseColors[material];

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

	const float4 texturePixel = read_imagef( textureIn, sampler, pos );
	float4 color = mix( accumulatedColor, texturePixel, textureWeight );
	write_imagef( textureOut, pos, color );
}
