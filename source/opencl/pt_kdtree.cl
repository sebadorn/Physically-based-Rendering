/**
 * Check all faces of a leaf node for intersections with the given ray.
 * @param faceIndex     [description]
 * @param origin        [description]
 * @param dir           [description]
 * @param scVertices    [description]
 * @param scFaces       [description]
 * @param kdNodeData3   [description]
 * @param entryDistance [description]
 * @param exitDistance  [description]
 * @param result        [description]
 */
void checkFaces(
	int nodeIndex, int faceIndex, float4* origin, float4* dir,
	__global float* scVertices, __global uint* scFaces, __global int* kdNodeData3,
	float entryDistance, float* exitDistance, hit_t* result
) {
	float4 a, b, c;
	float r;
	hit_t newResult;

	int i = 0;
	int numFaces = kdNodeData3[faceIndex];
	int aIndex, bIndex, cIndex, f;

	while( ++i <= numFaces ) {
		f = kdNodeData3[faceIndex + i];
		aIndex = scFaces[f] * 3;
		bIndex = scFaces[f + 1] * 3;
		cIndex = scFaces[f + 2] * 3;

		a = (float4)(
			scVertices[aIndex],
			scVertices[aIndex + 1],
			scVertices[aIndex + 2],
			0.0f
		);
		b = (float4)(
			scVertices[bIndex],
			scVertices[bIndex + 1],
			scVertices[bIndex + 2],
			0.0f
		);
		c = (float4)(
			scVertices[cIndex],
			scVertices[cIndex + 1],
			scVertices[cIndex + 2],
			0.0f
		);

		r = checkFaceIntersection( origin, dir, a, b, c, entryDistance, *exitDistance, &newResult );

		if( r > -1.0f ) {
			*exitDistance = r;

			if( result->distance < 0.0f || result->distance > newResult.distance ) {
				result->distance = newResult.distance;
				result->position = newResult.position;
				result->normalIndex = f;
				result->nodeIndex = nodeIndex;
				result->faceIndex = f / 3;
			}
		}
	}
}


bool checkFacesForShadow(
	int nodeIndex, int faceIndex, float4* origin, float4* dir,
	__global float* scVertices, __global uint* scFaces, __global int* kdNodeData3,
	float entryDistance, float* exitDistance, float distLimit
) {
	float4 a, b, c;
	float r;
	hit_t newResult;

	int i = 0;
	int numFaces = kdNodeData3[faceIndex];
	int aIndex, bIndex, cIndex, f;

	while( ++i <= numFaces ) {
		f = kdNodeData3[faceIndex + i];
		aIndex = scFaces[f] * 3;
		bIndex = scFaces[f + 1] * 3;
		cIndex = scFaces[f + 2] * 3;

		a = (float4)(
			scVertices[aIndex],
			scVertices[aIndex + 1],
			scVertices[aIndex + 2],
			0.0f
		);
		b = (float4)(
			scVertices[bIndex],
			scVertices[bIndex + 1],
			scVertices[bIndex + 2],
			0.0f
		);
		c = (float4)(
			scVertices[cIndex],
			scVertices[cIndex + 1],
			scVertices[cIndex + 2],
			0.0f
		);

		r = checkFaceIntersection( origin, dir, a, b, c, entryDistance, *exitDistance, &newResult );

		if( r > -1.0f ) {
			*exitDistance = r;

			if( newResult.distance <= distLimit ) {
				return true;
			}
		}
	}

	return false;
}


void goToLeafNode(
	int* nodeIndex, __global float* const kdNodeData1, __global int* const kdNodeData2,
	float4* const hitNear, float* bbMin, float* bbMax
) {
	int left = kdNodeData2[*nodeIndex * 5];
	int right = kdNodeData2[*nodeIndex * 5 + 1];
	int axis = kdNodeData2[*nodeIndex * 5 + 2];

	while( left >= 0 && right >= 0 ) {
		*nodeIndex = ( ( (float*) hitNear )[axis] < kdNodeData1[(*nodeIndex) * 9 + axis] ) ? left : right;
		left = kdNodeData2[*nodeIndex * 5];
		right = kdNodeData2[*nodeIndex * 5 + 1];
		axis = kdNodeData2[*nodeIndex * 5 + 2];
	}

	bbMin[0] = kdNodeData1[*nodeIndex * 9 + 3];
	bbMin[1] = kdNodeData1[*nodeIndex * 9 + 4];
	bbMin[2] = kdNodeData1[*nodeIndex * 9 + 5];
	bbMax[0] = kdNodeData1[*nodeIndex * 9 + 6];
	bbMax[1] = kdNodeData1[*nodeIndex * 9 + 7];
	bbMax[2] = kdNodeData1[*nodeIndex * 9 + 8];
}


/**
 * Find the closest hit of the ray with a surface.
 * Uses stackless kd-tree traversal.
 * @param origin      [description]
 * @param dir         [description]
 * @param kdRoot      [description]
 * @param scVertices  [description]
 * @param scFaces     [description]
 * @param kdNodeData1 [description]
 * @param kdNodeData2 [description]
 * @param kdNodeData3 [description]
 * @param kdNodeRopes [description]
 * @param result      [description]
 */
inline void traverseKdTree(
	float4* origin, float4* dir, const uint kdRoot,
	__global float* scVertices, __global uint* scFaces,
	__global float* kdNodeData1, __global int* kdNodeData2, __global int* kdNodeData3,
	__global int* kdNodeRopes, hit_t* result
) {
	int nodeIndex = kdRoot;
	uint ni9 = nodeIndex * 9;

	float bbMin[3] = {
		kdNodeData1[ni9 + 3],
		kdNodeData1[ni9 + 4],
		kdNodeData1[ni9 + 5]
	};
	float bbMax[3] = {
		kdNodeData1[ni9 + 6],
		kdNodeData1[ni9 + 7],
		kdNodeData1[ni9 + 8]
	};

	float entryDistance, exitDistance, tNear;
	int exitRope;

	if( !intersectBoundingBox( origin, dir, bbMin, bbMax, &entryDistance, &exitDistance, &exitRope ) ) {
		return;
	}
	if( exitDistance < 0.0f ) {
		return;
	}

	entryDistance = fmax( entryDistance, 0.0f );


	float4 hitNear;
	int faceIndex, ropeIndex;

	while( entryDistance < exitDistance ) {
		// Find a leaf node for this ray
		hitNear = (*origin) + entryDistance * (*dir);
		goToLeafNode( &nodeIndex, kdNodeData1, kdNodeData2, &hitNear, bbMin, bbMax );

		// At a leaf node now, check triangle faces
		faceIndex = kdNodeData2[nodeIndex * 5 + 3];

		checkFaces(
			nodeIndex, faceIndex, origin, dir, scVertices, scFaces, kdNodeData3,
			entryDistance, &exitDistance, result
		);

		// Exit leaf node
		intersectBoundingBox( origin, dir, bbMin, bbMax, &tNear, &entryDistance, &exitRope );

		if( entryDistance >= exitDistance ) {
			return;
		}

		// Follow the rope
		ropeIndex = kdNodeData2[nodeIndex * 5 + 4];
		nodeIndex = kdNodeRopes[ropeIndex + exitRope];

		if( nodeIndex < 0 ) {
			return;
		}
	}
}


/**
 * Test if from the current hit location there is an unobstracted direct path to the light source.
 * @param  origin      [description]
 * @param  toLight     [description]
 * @param  startNode   [description]
 * @param  scVertices  [description]
 * @param  scFaces     [description]
 * @param  kdNodeData1 [description]
 * @param  kdNodeData2 [description]
 * @param  kdNodeData3 [description]
 * @param  kdNodeRopes [description]
 * @return             [description]
 */
inline bool shadowTest(
	float4* origin, float4* toLight, const uint startNode,
	__global float* scVertices, __global uint* scFaces,
	__global float* kdNodeData1, __global int* kdNodeData2, __global int* kdNodeData3,
	__global int* kdNodeRopes
) {
	float4 dir = normalize( *toLight );
	float distLimit = length( *toLight );

	int nodeIndex = startNode;
	uint ni9 = nodeIndex * 9;

	float bbMin[3] = {
		kdNodeData1[ni9 + 3],
		kdNodeData1[ni9 + 4],
		kdNodeData1[ni9 + 5]
	};
	float bbMax[3] = {
		kdNodeData1[ni9 + 6],
		kdNodeData1[ni9 + 7],
		kdNodeData1[ni9 + 8]
	};

	float entryDistance, exitDistance, tNear;
	int exitRope;

	if( !intersectBoundingBox( origin, &dir, bbMin, bbMax, &entryDistance, &exitDistance, &exitRope ) ) {
		return false;
	}
	if( exitDistance < 0.0f ) {
		return false;
	}

	entryDistance = fmax( entryDistance, 0.0f );

	float4 hitNear;
	int faceIndex, ropeIndex;

	while( entryDistance < exitDistance ) {
		// Find a leaf node for this ray
		hitNear = (*origin) + entryDistance * dir;
		goToLeafNode( &nodeIndex, kdNodeData1, kdNodeData2, &hitNear, bbMin, bbMax );

		// At a leaf node now, check triangle faces
		faceIndex = kdNodeData2[nodeIndex * 5 + 3];

		if(
			checkFacesForShadow(
				nodeIndex, faceIndex, origin, &dir, scVertices, scFaces, kdNodeData3,
				entryDistance, &exitDistance, distLimit
			)
		) {
			return true;
		}


		// Exit leaf node
		intersectBoundingBox( origin, &dir, bbMin, bbMax, &tNear, &entryDistance, &exitRope );

		if( entryDistance >= exitDistance ) {
			return false;
		}

		// Follow the rope
		ropeIndex = kdNodeData2[nodeIndex * 5 + 4];
		nodeIndex = kdNodeRopes[ropeIndex + exitRope];

		if( nodeIndex < 0 ) {
			return false;
		}
	}
}
