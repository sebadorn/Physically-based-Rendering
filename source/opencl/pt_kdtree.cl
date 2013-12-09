/**
 * Check all faces of a leaf node for intersections with the given ray.
 * @param {const int}             nodeIndex
 * @param {const int}             faceIndex
 * @param {const float4*}         origin
 * @param {const float4*}         dir
 * @param {const __global float*} scVertices
 * @param {const __global uint*}  scFaces
 * @param {const __global int*}   kdNodeData3
 * @param {const float}           entryDistance
 * @param {float*}                exitDistance
 * @param {hit_t*}                result
 */
void checkFaces(
	const int nodeIndex, const int faceIndex, const float4* origin, const float4* dir,
	const __global float4* scVertices, const __global uint4* scFaces, const __global int* kdNodeData3,
	const float entryDistance, float* exitDistance, hit_t* result
) {
	float4 a, b, c;
	float r;

	uint4 index;
	int i = 1;
	int numFaces = kdNodeData3[faceIndex];
	int f;

	while( i <= numFaces ) {
		f = kdNodeData3[faceIndex + i];
		i++;
		index = scFaces[f];

		a = scVertices[index.x];
		b = scVertices[index.y];
		c = scVertices[index.z];

		r = checkFaceIntersection(
			origin, dir, &a, &b, &c, entryDistance, *exitDistance
		);

		if( r > -1.0f ) {
			*exitDistance = r;

			if( result->t > r || result->t < -1.0f ) {
				result->t = r;
				result->position = (*origin) + r * (*dir);
				result->nodeIndex = nodeIndex;
				result->faceIndex = f;
			}
		}
	}
}


/**
 * Check all faces of a leaf node for intersections with the given ray to test if it can reach a light source.
 * If a hit is found between origin and light source, the function returns immediately.
 * @param  {const int}             nodeIndex
 * @param  {const int}             faceIndex
 * @param  {const float4*}         origin
 * @param  {const float4*}         dir
 * @param  {const __global float*} scVertices
 * @param  {const __global uint*}  scFaces
 * @param  {const __global int*}   kdNodeData3
 * @param  {const float}           entryDistance
 * @param  {float*}                exitDistance
 * @return {bool}                                True, if a hit is detected between origin and light, false otherwise.
 */
bool checkFacesForShadow(
	const int nodeIndex, const int faceIndex, const float4* origin, const float4* dir,
	const __global float4* scVertices, const __global uint4* scFaces, const __global int* kdNodeData3,
	const float entryDistance, float* exitDistance
) {
	float4 a, b, c;
	float r;

	uint4 index;
	int i = 1;
	int numFaces = kdNodeData3[faceIndex];
	int f;

	while( i <= numFaces ) {
		f = kdNodeData3[faceIndex + i];
		i++;
		index = scFaces[f];

		a = scVertices[index.x];
		b = scVertices[index.y];
		c = scVertices[index.z];

		r = checkFaceIntersection(
			origin, dir, &a, &b, &c, entryDistance, *exitDistance
		);

		if( r > -1.0f ) {
			*exitDistance = r;

			if( r <= 1.0f ) {
				return true;
			}
		}
	}

	return false;
}


/**
 * Traverse down the kd-tree to find a leaf node the given ray intersects.
 * @param {int*}                  nodeIndex
 * @param {const __global float*} kdNodeData1
 * @param {const __global int*}   kdNodeData2
 * @param {const float4*}         hitNear
 */
void goToLeafNode(
	int* nodeIndex, const __global float4* kdNodeSplits, const __global int* kdNodeData2,
	const float4* hitNear
) {
	int left = kdNodeData2[(*nodeIndex) * 5];
	int right = kdNodeData2[(*nodeIndex) * 5 + 1];
	int axis = kdNodeData2[(*nodeIndex) * 5 + 2];
	const float hitPos[3] = { hitNear->x, hitNear->y, hitNear->z };
	float4 pos;

	while( left >= 0 && right >= 0 ) {
		pos = kdNodeSplits[(*nodeIndex)];
		*nodeIndex = ( hitPos[axis] < ( (float*) &pos )[axis] ) ? left : right;
		left = kdNodeData2[(*nodeIndex) * 5];
		right = kdNodeData2[(*nodeIndex) * 5 + 1];
		axis = ( 1 << axis ) & 3; // ( axis + 1 ) % 3
	}
}


/**
 * Set the values for the bounding box of the given node.
 * @param {const int}             nodeIndex
 * @param {const __global float*} kdNodeData1
 * @param {float*}                bbMin
 * @param {float*}                bbMax
 */
inline void updateBoundingBox( const int nodeIndex, const __global float4* kdNodeBbMin, const __global float4* kdNodeBbMax, float* bbMin, float* bbMax ) {
	float4 bbMin4 = kdNodeBbMin[nodeIndex];
	float4 bbMax4 = kdNodeBbMax[nodeIndex];

	bbMin[0] = bbMin4.x;
	bbMin[1] = bbMin4.y;
	bbMin[2] = bbMin4.z;
	bbMax[0] = bbMax4.x;
	bbMax[1] = bbMax4.y;
	bbMax[2] = bbMax4.z;
}


/**
 * Find the closest hit of the ray with a surface.
 * Uses stackless kd-tree traversal.
 * @param {const float4*}         origin
 * @param {const float4*}         dir
 * @param {const uint}            startNode
 * @param {const __global float*} scVertices
 * @param {const __global uint*}  scFaces
 * @param {const __global float*} kdNodeData1
 * @param {const __global int*}   kdNodeData2
 * @param {const __global int*}   kdNodeData3
 * @param {const __global int*}   kdNodeRopes
 * @param {hit_t*}                result
 */
inline void traverseKdTree(
	const float4* origin, const float4* dir, const int startNode,
	const __global float4* scVertices, const __global uint4* scFaces,
	const __global float4* kdNodeSplits, const __global float4* kdNodeBbMin, const __global float4* kdNodeBbMax,
	const __global int* kdNodeData2, const __global int* kdNodeData3,
	const __global int* kdNodeRopes, hit_t* result, const int bounce, const int kdRoot,
	const float initEntryDistance, const float initExitDistance
) {
	int nodeIndex = startNode;
	float entryDistance = initEntryDistance;
	float exitDistance = initExitDistance;

	float4 hitNear;
	float bbMax[3], bbMin[3];
	int exitRope, faceIndex, ropeIndex;
	int i = 0; // TODO: remove

	while( nodeIndex >= 0 && entryDistance < exitDistance ) {
		if( i++ > 1000 ) return; // TODO: remove

		// Find a leaf node for this ray
		hitNear = (*origin) + entryDistance * (*dir);
		goToLeafNode( &nodeIndex, kdNodeSplits, kdNodeData2, &hitNear );

		// At a leaf node now, check triangle faces
		faceIndex = kdNodeData2[nodeIndex * 5 + 3];

		checkFaces(
			nodeIndex, faceIndex, origin, dir, scVertices, scFaces, kdNodeData3,
			entryDistance, &exitDistance, result
		);

		// Exit leaf node
		updateBoundingBox( nodeIndex, kdNodeBbMin, kdNodeBbMax, bbMin, bbMax );
		updateEntryDistanceAndExitRope( origin, dir, bbMin, bbMax, &entryDistance, &exitRope );

		if( entryDistance >= exitDistance ) {
			break;
		}

		// Follow the rope
		ropeIndex = kdNodeData2[nodeIndex * 5 + 4];
		nodeIndex = kdNodeRopes[ropeIndex + exitRope];
	}

	if( result->t > -1.0f ) {
		nodeIndex = kdRoot;
		hitNear = result->position;
		goToLeafNode( &nodeIndex, kdNodeSplits, kdNodeData2, &hitNear );
		result->nodeIndex = nodeIndex;
	}
}


/**
 * Test if from the current hit location there is an unobstracted direct path to the light source.
 * @param  {const float4*}         origin
 * @param  {const float4*}         toLight
 * @param  {const uint}            startNode
 * @param  {const __global float*} scVertices
 * @param  {const __global uint*}  scFaces
 * @param  {const __global float*} kdNodeData1
 * @param  {const __global int*}   kdNodeData2
 * @param  {const __global int*}   kdNodeData3
 * @param  {const __global int*}   kdNodeRopes
 * @return {bool}
 */
inline bool shadowTest(
	const float4* origin, const float4* dir, const uint startNode,
	const __global float4* scVertices, const __global uint4* scFaces,
	const __global float4* kdNodeSplits, const __global float4* kdNodeBbMin, const __global float4* kdNodeBbMax,
	const __global int* kdNodeData2, const __global int* kdNodeData3,
	const __global int* kdNodeRopes, const int kdRoot,
	const float initEntryDistance, const float initExitDistance
) {
	int nodeIndex = startNode;
	float entryDistance = initEntryDistance;
	float exitDistance = initExitDistance;

	float4 hitNear;
	float bbMax[3], bbMin[3];
	int exitRope, faceIndex, ropeIndex;
	int i = 0; // TODO: remove

	while( nodeIndex >= 0 && entryDistance < exitDistance ) {
		if( i++ > 1000 ) return false; // TODO: remove

		// Find a leaf node for this ray
		hitNear = (*origin) + entryDistance * (*dir);
		goToLeafNode( &nodeIndex, kdNodeSplits, kdNodeData2, &hitNear );

		// At a leaf node now, check triangle faces
		faceIndex = kdNodeData2[nodeIndex * 5 + 3];

		if( checkFacesForShadow(
			nodeIndex, faceIndex, origin, dir, scVertices, scFaces, kdNodeData3,
			entryDistance, &exitDistance
		) ) {
			return true;
		}

		// Exit leaf node
		updateBoundingBox( nodeIndex, kdNodeBbMin, kdNodeBbMax, bbMin, bbMax );
		updateEntryDistanceAndExitRope( origin, dir, bbMin, bbMax, &entryDistance, &exitRope );

		if( entryDistance > 1.0f ) {
			return false;
		}

		// Follow the rope
		ropeIndex = kdNodeData2[nodeIndex * 5 + 4];
		nodeIndex = kdNodeRopes[ropeIndex + exitRope];
	}

	return false;
}
