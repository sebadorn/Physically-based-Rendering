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
	const uint2 offset, const float timeSinceStart, float pixelWeight,
	const global float4* scVertices, const global uint4* scFaces,
	const global float4* scNormals, const global uint* scFacesVN,
	const global kdNonLeaf* kdNonLeaves, const global kdLeaf* kdLeaves,
	const global uint* kdFaces, const float8 kdRootBB,
	global ray4* rays, const global int* faceToMaterial, const global material* materials,
	const global float* specPowerDists,
	read_only image2d_t imageIn, write_only image2d_t imageOut
) {
	const int2 pos = {
		offset.x + get_global_id( 0 ),
		offset.y + get_global_id( 1 )
	};
	const uint workIndex = pos.x + pos.y * IMG_WIDTH;

	const float4 bbMinRoot = { kdRootBB.s0, kdRootBB.s1, kdRootBB.s2, 0.0f };
	const float4 bbMaxRoot = { kdRootBB.s4, kdRootBB.s5, kdRootBB.s6, 0.0f };

	ray4 explicitRay, newRay;
	ray4 ray = rays[workIndex];
	ray.t = -2.0f;
	ray.nodeIndex = -1;
	ray.faceIndex = -1;

	float entryDistance = 0.0f;
	float exitDistance = FLT_MAX;

	uint bounce, f;
	material mtl, mtlLight;
	float tFar = exitDistance;

	pathPoint path[100];
	int pathIndex = 0;

	float4 H, T;
	float t, v, vIn, w;
	float isotropy = 1.0f, roughness;
	float rndNum1, rndNum2;

	material mtlFirst, mtlSecond;

	float spdSource[40], spdLight[40];

	for( int i = 0; i < 40; i++ ) {
		spdLight[i] = 0.0f;
	}

	float d, u, cosLaw;

	if( intersectBoundingBox( ray.origin, ray.dir, bbMinRoot, bbMaxRoot, &entryDistance, &tFar ) ) {
		int startNode = goToLeafNode( 0, kdNonLeaves, fma( entryDistance, ray.dir, ray.origin ) );


		traverseKdTree(
			&ray, startNode, kdNonLeaves, kdLeaves, kdFaces,
			scVertices, scFaces, entryDistance, exitDistance
		);

		if( ray.nodeIndex >= 0 ) {
			f = ray.faceIndex;
			ray.normal = scNormals[scFacesVN[f * 3]];
			mtlFirst = materials[faceToMaterial[f]];

			if( mtlFirst.light == 1 ) {
				for( int i = 0; i < 40; i++ ) {
					spdLight[i] = specPowerDists[mtlFirst.spd * 40 + i];
				}
			}

			newRay.t = -2.0f;
			newRay.nodeIndex = -1;
			newRay.faceIndex = -1;
			newRay.origin = fma( ray.t, ray.dir, ray.origin );
			newRay.dir = fast_normalize( cosineWeightedDirection( timeSinceStart + ray.t, ray.normal ) );

			traverseKdTree(
				&newRay, ray.nodeIndex, kdNonLeaves, kdLeaves, kdFaces,
				scVertices, scFaces, 0.0f, FLT_MAX
			);

			if( mtlFirst.light != 1 && newRay.nodeIndex >= 0 ) {
				f = newRay.faceIndex;
				mtlSecond = materials[faceToMaterial[f]];
				newRay.normal = scNormals[scFacesVN[f * 3]];
				roughness = 1.0f - mtlFirst.gloss;
				T = getT( newRay.normal );

				getValuesBRDF( newRay.dir, -ray.dir, ray.normal, T, &H, &t, &v, &vIn, &w );
				d = D( t, v, vIn, w, roughness, isotropy );
				u = cosineLaw( H, -ray.dir );
				cosLaw = cosineLaw( ray.normal, newRay.dir );

				if( mtlSecond.light == 1 ) {
					for( int i = 0; i < 40; i++ ) {
						spdLight[i] = specPowerDists[mtlFirst.spd * 40 + i] * specPowerDists[mtlSecond.spd * 40 + i] * cosLaw;
					}
				}
				else {
					ray = newRay;

					for( bounce = 0; bounce < 40; bounce++ ) {
						float seed = timeSinceStart + bounce + ray.t;
						newRay = getNewRay( ray, mtlSecond, seed );

						traverseKdTree(
							&newRay, ray.nodeIndex, kdNonLeaves, kdLeaves, kdFaces,
							scVertices, scFaces, 0.0f, FLT_MAX
						);

						if( newRay.nodeIndex < 0 ) {
							continue;
						}

						path[pathIndex] = getDefaultPathPoint();
						f = newRay.faceIndex;
						mtl = materials[faceToMaterial[f]];

						if( mtl.light == 1 ) {
							getValuesBRDF( newRay.dir, -ray.dir, ray.normal, T, &H, &t, &v, &vIn, &w );
							path[pathIndex].D = D( t, v, vIn, w, roughness, isotropy );
							path[pathIndex].u = cosineLaw( H, -ray.dir );
							path[pathIndex].cosLaw = cosineLaw( ray.normal, newRay.dir );
							path[pathIndex].spdLight = mtl.spd;
							pathIndex++;
						}
					}
				}
			}
		}
	}

	for( int i = 0; i < 40; i++ ) {
		spdSource[i] = specPowerDists[mtlSecond.spd * 40 + i];
	}

	for( int i = pathIndex - 1; i >= 0; i-- ) {
		pathPoint p = path[i];

		if( p.spdMaterial == -1 ) { // light hit
			for( int j = 0; j < 40; j++ ) {
				spdLight[j] += spdSource[j] * specPowerDists[p.spdLight * 40 + j] * p.cosLaw;
			}
		}
	}

	if( mtlFirst.light != 1 && mtlSecond.light != 1 ) {
		for( int i = 0; i < 40; i++ ) {
			spdLight[i] /= pathIndex;
			spdLight[i] = specPowerDists[mtlFirst.spd * 40 + i] * spdLight[i] * cosLaw;
		}
	}


	setColors( pos, imageIn, imageOut, pixelWeight, spdLight );
}
