#FILE:pt_header.cl:FILE#

#FILE:pt_utils.cl:FILE#

#FILE:pt_kdtree.cl:FILE#



// KERNELS


/**
 * Accumulate the colors of hit surfaces.
 * @param {__global float4*}       origins        Positions on the hit surfaces.
 * @param {__global float4*}       normals        Normals of the hit surfaces.
 * @param {__global float4*}       accColors      Accumulated color so far.
 * @param {__global float4*}       colorMasks     Color mask so far.
 * @param {const float}            textureWeight  Weight for the mixing of the textures.
 * @param {const float}            timeSinceStart Time since start of the tracer in seconds.
 * @param {__read_only image2d_t}  textureIn      Input. The generated texture so far.
 * @param {__write_only image2d_t} textureOut     Output. The generated texture now.
 */
__kernel void accumulateColors(
	const uint offsetW, const uint offsetH,
	__global float4* origins, __global float4* normals,
	__global float4* accColors, __global float4* colorMasks,
	__global float4* lights,
	__global int* materialToFace, __global float* diffuseColors,
	const float textureWeight, const float timeSinceStart,
	__read_only image2d_t textureIn, __write_only image2d_t textureOut
) {
	const int2 pos = { offsetW + get_global_id( 0 ), offsetH + get_global_id( 1 ) };
	uint workIndex = pos.x + pos.y * IMG_WIDTH;

	float4 hit = origins[workIndex];

	// Lighting
	float4 light = lights[0];

	// New color (or: another color calculated by evaluating different light paths than before)
	// Accumulate the colors of each hit surface
	float4 colorMask = colorMasks[workIndex];
	float4 accumulatedColor = accColors[workIndex];

	// Surface hit
	if( hit.w > -1.0f ) {
		float shadowIntensity = hit.w;
		hit.w = 0.0f;

		// The farther away a shadow is, the more diffuse it becomes
		float4 newLight = light + uniformlyRandomVector( timeSinceStart ) * 0.1f;

		float4 normal = normals[workIndex];

		// TODO: This is ugly, seperate the (unrelated) data
		int face = normal.w;
		int material = materialToFace[face];
		normals[workIndex].w = 0.0f;
		normal.w = 0.0f;

		// Distance of the hit surface point to the light source
		float4 toLight = newLight - hit;

		// Lambert factor (dot product is a cosine)
		// Source: http://learnopengles.com/android-lesson-two-ambient-and-diffuse-lighting/
		float diffuse = fmax( dot( normal, normalize( toLight ) ), 0.0f );
		float toLightLength = length( toLight );
		float luminosity = 1.0f / ( toLightLength * toLightLength );

		float specularHighlight = 0.0f; // Disabled for now
		float4 surfaceColor = (float4)(
			diffuseColors[material * 3],
			diffuseColors[material * 3 + 1],
			diffuseColors[material * 3 + 2],
			0.0f
		);
		// float4 surfaceColor = (float4)( 0.6f, 0.6f, 0.6f, 0.0f );

		colorMask *= surfaceColor;
		accumulatedColor += colorMask * luminosity * diffuse * shadowIntensity;
		accumulatedColor += colorMask * specularHighlight * shadowIntensity;

		accColors[workIndex] = accumulatedColor;
		colorMasks[workIndex] = colorMask;
	}

	// Mix new color with previous one
	float4 texturePixel = read_imagef( textureIn, sampler, pos );
	float4 color = mix( accumulatedColor, texturePixel, textureWeight );
	color.w = 1.0f;

	write_imagef( textureOut, pos, color );
}


/**
 * Search the kd-tree to find the closest intersection of the ray with a surface.
 * @param origins        [description]
 * @param rays           [description]
 * @param normals        [description]
 * @param scVertices     [description]
 * @param scFaces        [description]
 * @param kdNodeData1    [description]
 * @param kdNodeData2    [description]
 * @param kdNodeData3    [description]
 * @param kdNodeRopes    [description]
 * @param kdRoot         [description]
 * @param timeSinceStart [description]
 */
__kernel void findIntersections(
	const uint offsetW, const uint offsetH,
	__global float4* origins, __global float4* rays, __global float4* normals,
	__global float* scVertices, __global uint* scFaces, __global float* scNormals,
	__global uint* scFacesVN,
	__global float4* lights,
	__global float* kdNodeData1, __global int* kdNodeData2, __global int* kdNodeData3,
	__global int* kdNodeRopes, const uint kdRoot, const float timeSinceStart
) {
	const int2 pos = { offsetW + get_global_id( 0 ), offsetH + get_global_id( 1 ) };
	uint workIndex = pos.x + pos.y * IMG_WIDTH;

	float4 origin = origins[workIndex];
	float4 dir = rays[workIndex];

	if( origin.w <= -1.0f ) {
		return;
	}

	origin.w = 0.0f;

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
		uint f = hit.normalIndex;

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
		rays[workIndex] = normalize( newDir );


	#ifdef SHADOWS

		float4 light = lights[0];
		float4 newLight = light + uniformlyRandomVector( timeSinceStart ) * 0.1f;
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
 * Kernel to generate the initial rays into the scene.
 * @param {__global float*}  eyeIn   Camera eye position.
 * @param {__global float*}  wVecIn  Vector from the camera eye to the screen.
 * @param {__global float*}  uVecIn  Vector alongside the X axis of the screen.
 * @param {__global float*}  vVecIn  Vector alongside the Y axis of the screen.
 * @param {const float}      fovRad  Field of view in radians.
 * @param {__global float4*} origins Output. The origin of each ray. (The camera eye.)
 * @param {__global float4*} rays    Output. The generated rays for each pixel.
 */
__kernel void initRays(
	const uint offsetW, const uint offsetH, const float4 initRayParts,
	__global float* eyeIn,
	__global float4* origins, __global float4* rays,
	__global float4* accColors, __global float4* colorMasks
) {
	const int2 pos = { offsetW + get_global_id( 0 ), offsetH + get_global_id( 1 ) };
	uint workIndex = pos.x + pos.y * IMG_WIDTH;

	// Camera eye
	float4 eye = (float4)( eyeIn[0], eyeIn[1], eyeIn[2], 0.0f );

	// Initial ray (first of the to be created path) shooting from the eye and into the scene
	float4 w = (float4)( eyeIn[3], eyeIn[4], eyeIn[5], 0.0f );
	float4 u = (float4)( eyeIn[6], eyeIn[7], eyeIn[8], 0.0f );
	float4 v = (float4)( eyeIn[9], eyeIn[10], eyeIn[11], 0.0f );

	float pxWidthAndHeight = initRayParts.x;
	float adjustW = initRayParts.y;
	float adjustH = initRayParts.z;

	float4 initialRay = eye + w
			- WORKGROUPSIZE / 2.0f * pxWidthAndHeight * u
			+ WORKGROUPSIZE / 2.0f * pxWidthAndHeight * v
			+ pxWidthAndHeight / 2.0f * u
			- pxWidthAndHeight / 2.0f * v;
	initialRay += ( pos.x - adjustW ) * pxWidthAndHeight * u;
	initialRay -= ( WORKGROUPSIZE - pos.y + adjustH ) * pxWidthAndHeight * v;
	initialRay.w = 0.0f;

	rays[workIndex] = normalize( initialRay - eye );
	origins[workIndex] = eye;

	accColors[workIndex] = (float4)( 0.0f );
	colorMasks[workIndex] = (float4)( 1.0f, 1.0f, 1.0f, 0.0f );
}
