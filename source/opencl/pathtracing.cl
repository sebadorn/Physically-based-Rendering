#FILE:pt_header.cl:FILE#

#FILE:pt_utils.cl:FILE#

#FILE:pt_kdtree.cl:FILE#



// KERNELS


/**
 * KERNEL.
 * Accumulate the colors of hit surfaces.
 * @param {const uint}             offsetW
 * @param {const uint}             offsetH
 * @param {const __global float4*} origins        Positions on the hit surfaces.
 * @param {const __global float4*} normals        Normals of the hit surfaces.
 * @param {__global float4*}       accColors      Accumulated color so far.
 * @param {__global float4*}       colorMasks     Color mask so far.
 * @param {const __global float*}  lights
 * @param {const __global int*}    materialToFace
 * @param {const __global float*}  diffuseColors
 * @param {const float}            textureWeight  Weight for the mixing of the textures.
 * @param {const float}            timeSinceStart Time since start of the tracer in seconds.
 * @param {__read_only image2d_t}  textureIn      Input. The generated texture so far.
 * @param {__write_only image2d_t} textureOut     Output. The generated texture now.
 */
__kernel __attribute__( ( work_group_size_hint( WORKGROUPSIZE, WORKGROUPSIZE, 1 ) ) )
void accumulateColors(
	const uint offsetW, const uint offsetH,
	const __global float4* origins, const __global float4* normals,
	__global float4* accColors, __global float4* colorMasks,
	const __global float4* lights,
	const __global int* materialToFace, const __global float* diffuseColors,
	const float textureWeight, const float timeSinceStart,
	__read_only image2d_t textureIn, __write_only image2d_t textureOut
) {
	const int2 pos = { offsetW + get_global_id( 0 ), offsetH + get_global_id( 1 ) };
	const uint workIndex = pos.x + pos.y * IMG_WIDTH;

	float4 hit = origins[workIndex];
	const float4 light = lights[0];

	// New color (or: another color calculated by evaluating different light paths than before)
	// Accumulate the colors of each hit surface
	float4 colorMask = colorMasks[workIndex];
	float4 accumulatedColor = accColors[workIndex];

	// Surface hit
	if( hit.w > -1.0f ) {
		const float shadowIntensity = hit.w;
		hit.w = 0.0f;

		float4 normal = normals[workIndex];

		// TODO: This is ugly, seperate the (unrelated) data
		int face = normal.w;
		int material = materialToFace[face];
		normals[workIndex].w = 0.0f;
		normal.w = 0.0f;

		// The farther away a shadow is, the more diffuse it becomes (penumbrae)
		const float4 newLight = light + uniformlyRandomVector( timeSinceStart ) * 0.1f;
		float4 toLight = newLight - hit;

		// Lambert factor (dot product is a cosine)
		// Source: http://learnopengles.com/android-lesson-two-ambient-and-diffuse-lighting/
		const float diffuse = fmax( dot( normal, fast_normalize( toLight ) ), 0.0f );
		const float toLightLength = fast_length( toLight );
		const float luminosity = native_recip( toLightLength * toLightLength );

		const float specularHighlight = 0.0f; // Disabled for now

		const float4 surfaceColor = {
			diffuseColors[material * 3],
			diffuseColors[material * 3 + 1],
			diffuseColors[material * 3 + 2],
			0.0f
		};
		// float4 surfaceColor = (float4)( 0.6f, 0.6f, 0.6f, 0.0f );

		colorMask *= surfaceColor;
		accumulatedColor += colorMask * luminosity * diffuse * shadowIntensity;
		accumulatedColor += colorMask * specularHighlight * shadowIntensity;

		accColors[workIndex] = accumulatedColor;
		colorMasks[workIndex] = colorMask;
	}

	// Mix new color with previous one
	const float4 texturePixel = read_imagef( textureIn, sampler, pos );
	float4 color = mix( accumulatedColor, texturePixel, textureWeight );
	write_imagef( textureOut, pos, color );
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
void findIntersections(
	const uint offsetW, const uint offsetH,
	__global float4* origins, __global float4* rays, __global float4* normals,
	const __global float* scVertices, const __global uint* scFaces, const __global float* scNormals,
	const __global uint* scFacesVN,
	const __global float4* lights,
	const __global float* kdNodeData1, const __global int* kdNodeData2, const __global int* kdNodeData3,
	const __global int* kdNodeRopes, const uint kdRoot, const float timeSinceStart
) {
	const int2 pos = { offsetW + get_global_id( 0 ), offsetH + get_global_id( 1 ) };
	const uint workIndex = pos.x + pos.y * IMG_WIDTH;

	float4 origin = origins[workIndex];
	const float4 dir = rays[workIndex];

	if( origin.w <= -1.0f ) {
		return;
	}

	hit_t hit;
	hit.distance = -2.0f;
	hit.position = (float4)( 0.0f, 0.0f, 0.0f, -2.0f );
	hit.nodeIndex = -1;

	traverseKdTree(
		&origin, &dir, kdRoot, scVertices, scFaces,
		kdNodeData1, kdNodeData2, kdNodeData3, kdNodeRopes, &hit
	);

	// How to use a barrier:
	// barrier( CLK_LOCAL_MEM_FENCE );

	if( hit.distance > -1.0f ) {
		// Normal of hit position
		const uint f = hit.normalIndex;

		float4 faceNormal = (float4)(
			scNormals[scFacesVN[f] * 3],
			scNormals[scFacesVN[f] * 3 + 1],
			scNormals[scFacesVN[f] * 3 + 2],
			0.0f
		);
		normals[workIndex] = faceNormal;

		// New ray
		float4 newDir = cosineWeightedDirection( timeSinceStart + hit.distance, faceNormal );
		newDir.w = 0.0f;
		rays[workIndex] = fast_normalize( newDir ); // WARNING: fast_


	#ifdef SHADOWS

		const float4 light = lights[0];
		const float4 newLight = light + uniformlyRandomVector( timeSinceStart ) * 0.1f;
		float4 newOrigin = hit.position + normals[workIndex] * EPSILON;
		float4 toLight = newLight - hit.position;

		// TODO: hit.nodeIndex instead of kdRoot
		bool isInShadow = shadowTest(
			&newOrigin, &toLight, kdRoot, scVertices, scFaces,
			kdNodeData1, kdNodeData2, kdNodeData3, kdNodeRopes
		);

		hit.position.w = !isInShadow;

	#else

		hit.position.w = 1.0f;

	#endif

		normals[workIndex].w = hit.faceIndex;
	}

	origins[workIndex] = hit.position;
}


/**
 * KERNEL.
 * Generate the initial rays into the scene.
 * @param {const uint}             offsetW
 * @param {const uint}             offsetH
 * @param {const float4}           initRayParts
 * @param {const __global float*}  eyeIn        Camera eye position.
 * @param {__global float4*}       origins      Output. The origin of each ray. (The camera eye.)
 * @param {__global float4*}       rays         Output. The generated rays for each pixel.
 * @param {__global float4*}       accColors    Output.
 * @param {__global float4*}       colorsMasks  Output.
 */
__kernel __attribute__( ( work_group_size_hint( WORKGROUPSIZE, WORKGROUPSIZE, 1 ) ) )
void initRays(
	const uint offsetW, const uint offsetH, const float4 initRayParts,
	const __global float* eyeIn,
	__global float4* origins, __global float4* rays,
	__global float4* accColors, __global float4* colorMasks
) {
	const int2 pos = { offsetW + get_global_id( 0 ), offsetH + get_global_id( 1 ) };
	const uint workIndex = pos.x + pos.y * IMG_WIDTH;

	const float4 eye = { eyeIn[0], eyeIn[1], eyeIn[2], 0.0f };
	const float4 w = { eyeIn[3], eyeIn[4], eyeIn[5], 0.0f };
	const float4 u = { eyeIn[6], eyeIn[7], eyeIn[8], 0.0f };
	const float4 v = { eyeIn[9], eyeIn[10], eyeIn[11], 0.0f };

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

	rays[workIndex] = fast_normalize( initialRay - eye ); // WARNING: fast_
	origins[workIndex] = eye;

	accColors[workIndex] = (float4)( 0.0f );
	colorMasks[workIndex] = (float4)( 1.0f, 1.0f, 1.0f, 0.0f );
}
