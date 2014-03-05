#include "BVH.h"

using std::vector;


/**
 * Struct to use as comparator in std::sort() for the faces (cl_uint4).
 */
struct sortFacesCmp {

	cl_uint axis;
	vector<cl_float4>* v;

	/**
	 * Constructor.
	 * @param {cl_uint}                 axis        Axis to compare the faces on.
	 * @param {std::vector<cl_float4>*} allVertices List of vertices the faces have the indices for.
	 */
	sortFacesCmp( cl_uint axis, vector<cl_float4>* allVertices ) {
		this->axis = axis;
		this->v = allVertices;
	};

	/**
	 * Compare two faces by their centroids.
	 * @param  {cl_uint4} a Indizes for vertices that describe a face.
	 * @param  {cl_uint4} b Indizes for vertices that describe a face.
	 * @return {bool}       a < b
	 */
	bool operator()( cl_uint4 a, cl_uint4 b ) {
		cl_float4 v0_a = ( *this->v )[a.x];
		cl_float4 v1_a = ( *this->v )[a.y];
		cl_float4 v2_a = ( *this->v )[a.z];
		cl_float4 v0_b = ( *this->v )[b.x];
		cl_float4 v1_b = ( *this->v )[b.y];
		cl_float4 v2_b = ( *this->v )[b.z];

		glm::vec3 cenA = MathHelp::getTriangleCentroid( v0_a, v1_a, v2_a );
		glm::vec3 cenB = MathHelp::getTriangleCentroid( v0_b, v1_b, v2_b );

		return cenA[this->axis] < cenB[this->axis];
	};

};


/**
 * Build a sphere tree for each object in the scene and combine them into one big tree.
 * @param  {std::vector<object3D>} sceneObjects
 * @param  {std::vector<cl_float>} allVertices
 * @return {BVH*}
 */
BVH::BVH( vector<object3D> sceneObjects, vector<cl_float> allVertices ) {
	boost::posix_time::ptime timerStart = boost::posix_time::microsec_clock::local_time();

	mDepthReached = 0;
	this->setMaxFaces( Cfg::get().value<cl_uint>( Cfg::BVH_MAXFACES ) );

	vector<BVHNode*> subTrees = this->buildTreesFromObjects( sceneObjects, allVertices );
	mRoot = this->makeContainerNode( subTrees, true );
	this->groupTreesToNodes( subTrees, mRoot, mDepthReached );

	this->combineNodes( subTrees );

	this->logStats( timerStart );
}


/**
 * Destructor.
 */
BVH::~BVH() {
	for( cl_uint i = 0; i < mNodes.size(); i++ ) {
		delete mNodes[i];
	}
}


/**
 * Build the sphere tree.
 * @param  {std::vector<cl_uint4>}  faces
 * @param  {std::vector<cl_float4>} allVertices
 * @param  {cl_uint}                depth       The current depth of the node in the tree. Starts at 1.
 * @return {BVHNode*}
 */
BVHNode* BVH::buildTree( vector<cl_uint4> faces, vector<cl_float4> allVertices, cl_uint depth ) {
	BVHNode* containerNode = this->makeNode( faces, allVertices );
	containerNode->depth = depth;
	mDepthReached = ( depth > mDepthReached ) ? depth : mDepthReached;

	// leaf node
	if( faces.size() <= mMaxFaces ) {
		if( faces.size() <= 0 ) {
			Logger::logWarning( "[BVH] No faces in node." );
		}
		containerNode->faces = faces;

		return containerNode;
	}

	vector<cl_uint4> leftFaces, rightFaces;

	// SAH takes some time. Don't do it if there are too many faces.
	if( faces.size() <= Cfg::get().value<cl_uint>( Cfg::BVH_SAHFACESLIMIT ) ) {
		cl_float bestSAH = FLT_MAX;
		cl_float nodeSA = 1.0f / MathHelp::getSurfaceArea( containerNode->bbMin, containerNode->bbMax );

		for( cl_uint axis = 0; axis <= 2; axis++ ) {
			this->splitBySAH( nodeSA, &bestSAH, axis, faces, allVertices, &leftFaces, &rightFaces );
		}
	}
	// Faster to build: Splitting at the midpoint of the longest axis.
	else {
		Logger::logDebug( "[BVH] Too many faces in node for SAH. Splitting by midpoint." );

		cl_uint axis = this->longestAxis( containerNode );
		cl_float splitPos = this->findMidpoint( containerNode, axis );

		this->splitFaces( faces, allVertices, splitPos, axis, &leftFaces, &rightFaces );

		if( leftFaces.size() <= 0 || rightFaces.size() <= 0 ) {
			Logger::logDebug( "[BVH] Splitting faces by midpoint didn't work. Trying again with mean." );
			leftFaces.clear();
			rightFaces.clear();
			splitPos = this->findMean( faces, allVertices, axis );

			this->splitFaces( faces, allVertices, splitPos, axis, &leftFaces, &rightFaces );
		}
	}

	containerNode->leftChild = this->buildTree( leftFaces, allVertices, depth + 1 );
	containerNode->rightChild = this->buildTree( rightFaces, allVertices, depth + 1 );

	return containerNode;
}


void BVH::splitBySpatialSplit( BVHNode* node, vector<cl_uint4> faces, vector<cl_float4> allVertices ) {
	cl_uint splits = 3;
	cl_uint axis = this->longestAxis( node );


	// Find split positions

	vector<cl_float> splitPos( splits );
	cl_float lenSegment = ( node->bbMax[axis] - node->bbMin[axis] ) / ( (cl_float) splits + 1.0f );
	splitPos[0] = node->bbMin[axis] + lenSegment;
	splitPos[1] = splitPos[0] + lenSegment;
	splitPos[2] = splitPos[1] + lenSegment;


	// Create AABBs for bins

	glm::vec3 bbMinBin0 = glm::vec3( node->bbMin );
	glm::vec3 bbMaxBin0 = glm::vec3( node->bbMax );
	bbMaxBin0[axis] = splitPos[0];

	glm::vec3 bbMinBin1 = glm::vec3( bbMaxBin0 );
	glm::vec3 bbMaxBin1 = glm::vec3( bbMaxBin0 );
	bbMaxBin1[axis] = splitPos[1];

	glm::vec3 bbMinBin2 = glm::vec3( bbMaxBin1 );
	glm::vec3 bbMaxBin2 = glm::vec3( bbMaxBin1 );
	bbMaxBin2[axis] = splitPos[2];

	glm::vec3 bbMinBin3 = glm::vec3( bbMaxBin2 );
	glm::vec3 bbMaxBin3 = glm::vec3( node->bbMax );


	// Assign faces

	vector<cl_uint4> binFaces0, binFaces1, binFaces2, binFaces3;
	cl_float4 v0, v1, v2;

	for( cl_uint i = 0; i < faces.size(); i++ ) {
		v0 = allVertices[faces[i].x];
		v1 = allVertices[faces[i].y];
		v2 = allVertices[faces[i].z];
		glm::vec3 trMinBB, trMaxBB;

		MathHelp::getTriangleAABB( v0, v1, v2, &trMinBB, &trMaxBB );

		if( trMinBB[axis] <= bbMaxBin0[axis] ) {
			binFaces0.push_back( faces[i] );
		}
		if( trMinBB[axis] <= bbMaxBin1[axis] && trMaxBB[axis] >= bbMinBin1[axis] ) {
			binFaces1.push_back( faces[i] );
		}
		if( trMinBB[axis] <= bbMaxBin2[axis] && trMaxBB[axis] >= bbMinBin2[axis] ) {
			binFaces2.push_back( faces[i] );
		}
		if( trMaxBB[axis] >= bbMinBin3[axis] ) {
			binFaces3.push_back( faces[i] );
		}
	}


	// Clip faces and shrink AABB

	vector<cl_float4> vertsForAABB0;
	glm::vec3 vNewA, vNewB;

	for( cl_uint i = 0; i < binFaces0.size(); i++ ) {
		v0 = allVertices[faces[i].x];
		v1 = allVertices[faces[i].y];
		v2 = allVertices[faces[i].z];
		glm::vec3 trMinBB, trMaxBB;

		MathHelp::getTriangleAABB( v0, v1, v2, &trMinBB, &trMaxBB );

		// face extends into next bin
		if( v0[axis] > bbMaxBin0[axis] ) {
			glm::vec3 u = v1 - v0;
			glm::vec3 w = v0 - bbMaxBin0;
			glm::vec3 plNorm = glm::vec3( 0.0f, 0.0f, 0.0f );
			plNorm[axis] = 1.0f;
			cl_float d = glm::dot( plNorm, u );

			if( glm::abs( d ) > 0.0001f ) {
				cl_float fac = -glm::dot( plNorm, w ) / d;
				u *= fac;
				vNewA = bbMaxBin0 + u;
				cl_float4 v0New = { vNewA[0], vNewA[1], vNewA[2], 0.0f };
				vertsForAABB0.push_back( v0New );
			}
			else {
				// parallel, what now?
			}

			vertsForAABB0.push_back( vNewA );
			vertsForAABB0.push_back( vNewB );
		}
		else {
			vertsForAABB0.push_back( v0 );
		}
	}

	MathHelp::getAABB( vertsForAABB0, &bbMinBin0, &bbMaxBin0 );
}


/**
 * Build sphere trees for all given scene objects.
 * @param  {std::vector<object3D>} sceneObjects
 * @param  {std::vector<cl_float>} allVertices
 * @return {std::vector<BVHNode*>}
 */
vector<BVHNode*> BVH::buildTreesFromObjects(
	vector<object3D> sceneObjects, vector<cl_float> allVertices
) {
	vector<BVHNode*> subTrees;
	char msg[256];
	cl_int offset = 0;

	for( cl_uint i = 0; i < sceneObjects.size(); i++ ) {
		vector<cl_uint4> facesThisObj;
		vector<cl_float4> allVertices4;
		ModelLoader::getFacesAndVertices( sceneObjects[i], allVertices, &facesThisObj, &allVertices4, offset );
		offset += facesThisObj.size();

		snprintf(
			msg, 256, "[BVH] Building tree %u/%lu: \"%s\". %lu faces.",
			i + 1, sceneObjects.size(), sceneObjects[i].oName.c_str(), facesThisObj.size()
		);
		Logger::logInfo( msg );

		BVHNode* st = this->buildTree( facesThisObj, allVertices4, 1 );
		subTrees.push_back( st );
	}

	return subTrees;
}


/**
 * Combine the container nodes, leaf nodes and the root node into one list.
 * The root node will be at the very beginning of the list.
 * @param {std::vector<BVHNode*>} subTrees The generated trees for each object.
 */
void BVH::combineNodes( vector<BVHNode*> subTrees ) {
	if( subTrees.size() > 1 ) {
		mNodes.push_back( mRoot );
	}
	mNodes.insert( mNodes.end(), mContainerNodes.begin(), mContainerNodes.end() );
	mNodes.insert( mNodes.end(), mLeafNodes.begin(), mLeafNodes.end() );

	for( cl_uint i = 0; i < mNodes.size(); i++ ) {
		mNodes[i]->id = i;
	}
}


/**
 * Find the mean of the triangles regarding the given axis.
 * @param  {std::vector<cl_uint4>}  faces
 * @param  {std::vector<cl_float4>} allVertices
 * @param  {cl_uint}                axis
 * @return {cl_float}
 */
cl_float BVH::findMean( vector<cl_uint4> faces, vector<cl_float4> allVertices, cl_uint axis ) {
	cl_float sum = 0.0f;
	cl_float4 v0, v1, v2;
	glm::vec3 center;

	for( cl_uint i = 0; i < faces.size(); i++ ) {
		v0 = allVertices[faces[i].x];
		v1 = allVertices[faces[i].y];
		v2 = allVertices[faces[i].z];
		glm::vec3 center = MathHelp::getTriangleCentroid( v0, v1, v2 );

		sum += center[axis];
	}

	return sum / faces.size();
}


/**
 * Find the mean of the centers of the given nodes.
 * @param  {std::vector<BVHNode*>} nodes
 * @param  {cl_uint}               axis
 * @return {cl_float}
 */
cl_float BVH::findMeanOfNodes( vector<BVHNode*> nodes, cl_uint axis ) {
	cl_float sum = 0.0f;

	for( cl_uint i = 0; i < nodes.size(); i++ ) {
		glm::vec3 center = ( nodes[i]->bbMax - nodes[i]->bbMin ) / 2.0f;
		sum += center[axis];
	}

	return sum / nodes.size();
}


/**
 * Find the midpoint on the longest axis of the node.
 * @param  {BVHNode*} container
 * @param  {cl_uint*} axis
 * @return {cl_float}
 */
cl_float BVH::findMidpoint( BVHNode* container, cl_uint axis ) {
	glm::vec3 sides = ( container->bbMax + container->bbMin ) / 2.0f;

	return sides[axis];
}


/**
 * Get all container nodes (all nodes that aren't leaves).
 * @return {std::vector<BVHNode*>} List of all container nodes.
 */
vector<BVHNode*> BVH::getContainerNodes() {
	return mContainerNodes;
}


/**
 * Get the max reached depth.
 * @return {cl_uint} The max reached depth.
 */
cl_uint BVH::getDepth() {
	return mDepthReached;
}


/**
 * Get all leaf nodes.
 * @return {std::vector<BVHNode*>} List of all leaf nodes.
 */
vector<BVHNode*> BVH::getLeafNodes() {
	return mLeafNodes;
}


/**
 * Get all nodes (container and leaf nodes).
 * The first node in the list is the root node.
 * @return {std::vector<BVHNode*>} List of all nodes.
 */
vector<BVHNode*> BVH::getNodes() {
	return mNodes;
}


/**
 * Get the root node.
 * @return {BVHNode*} The root node.
 */
BVHNode* BVH::getRoot() {
	return mRoot;
}


/**
 * Group the sphere nodes into two groups and assign them to the given parent node.
 * @param {std::vector<BVHNode*>} nodes
 * @param {BVHNode*}              parent
 * @param {cl_uint}               depth
 */
void BVH::groupTreesToNodes( vector<BVHNode*> nodes, BVHNode* parent, cl_uint depth ) {
	if( nodes.size() == 1 ) {
		// Implies: parent == nodes[0], nothing to do
		return;
	}

	parent->depth = depth;
	mDepthReached = ( depth > mDepthReached ) ? depth : mDepthReached;

	cl_uint axis = this->longestAxis( parent );
	cl_float midpoint = this->findMidpoint( parent, axis );

	vector<BVHNode*> leftGroup, rightGroup;
	this->splitNodes( nodes, midpoint, axis, &leftGroup, &rightGroup );

	if( leftGroup.size() <= 0 || rightGroup.size() <= 0 ) {
		leftGroup.clear();
		rightGroup.clear();
		cl_float mean = this->findMeanOfNodes( nodes, axis );

		this->splitNodes( nodes, mean, axis, &leftGroup, &rightGroup );
	}

	BVHNode* leftNode = this->makeContainerNode( leftGroup, false );
	parent->leftChild = leftNode;
	this->groupTreesToNodes( leftGroup, parent->leftChild, depth + 1 );

	BVHNode* rightNode = this->makeContainerNode( rightGroup, false );
	parent->rightChild = rightNode;
	this->groupTreesToNodes( rightGroup, parent->rightChild, depth + 1 );
}


/**
 * Log some stats.
 * @param {boost::posix_time::ptime} timerStart
 */
void BVH::logStats( boost::posix_time::ptime timerStart ) {
	boost::posix_time::ptime timerEnd = boost::posix_time::microsec_clock::local_time();
	cl_float timeDiff = ( timerEnd - timerStart ).total_milliseconds();

	char msg[256];
	snprintf(
		msg, 256, "[BVH] Generated in %g ms. Contains %lu nodes (%lu leaves). Max faces of %u. Max depth of %u.",
		timeDiff, mNodes.size(), mLeafNodes.size(), mMaxFaces, mDepthReached
	);
	Logger::logInfo( msg );
}


/**
 * Get the index of the longest axis.
 * @param  {BVHNode*} node The node to find the longest side of.
 * @return {cl_uint}       Index of the longest axis (X: 0, Y: 1, Z: 2).
 */
cl_uint BVH::longestAxis( BVHNode* node ) {
	glm::vec3 sides = node->bbMax - node->bbMin;

	if( sides[0] > sides[1] ) {
		return ( sides[0] > sides[2] ) ? 0 : 2;
	}
	else { // sides[1] > sides[0]
		return ( sides[1] > sides[2] ) ? 1 : 2;
	}
}


/**
 * Create a container node that can contain the created sub-trees.
 * @param  {std::vector<BVHNode*>} subTrees
 * @return {BVHNode*}
 */
BVHNode* BVH::makeContainerNode( vector<BVHNode*> subTrees, bool isRoot ) {
	if( subTrees.size() == 1 ) {
		return subTrees[0];
	}

	BVHNode* node = new BVHNode;

	node->leftChild = NULL;
	node->rightChild = NULL;
	node->depth = 0;

	node->bbMin = glm::vec3( subTrees[0]->bbMin );
	node->bbMax = glm::vec3( subTrees[0]->bbMax );

	for( cl_uint i = 1; i < subTrees.size(); i++ ) {
		node->bbMin[0] = ( node->bbMin[0] < subTrees[i]->bbMin[0] ) ? node->bbMin[0] : subTrees[i]->bbMin[0];
		node->bbMin[1] = ( node->bbMin[1] < subTrees[i]->bbMin[1] ) ? node->bbMin[1] : subTrees[i]->bbMin[1];
		node->bbMin[2] = ( node->bbMin[2] < subTrees[i]->bbMin[2] ) ? node->bbMin[2] : subTrees[i]->bbMin[2];

		node->bbMax[0] = ( node->bbMax[0] > subTrees[i]->bbMax[0] ) ? node->bbMax[0] : subTrees[i]->bbMax[0];
		node->bbMax[1] = ( node->bbMax[1] > subTrees[i]->bbMax[1] ) ? node->bbMax[1] : subTrees[i]->bbMax[1];
		node->bbMax[2] = ( node->bbMax[2] > subTrees[i]->bbMax[2] ) ? node->bbMax[2] : subTrees[i]->bbMax[2];
	}

	if( !isRoot ) {
		mContainerNodes.push_back( node );
	}

	return node;
}


/**
 * Create a new node.
 * @param  {std::vector<cl_uint4>} faces
 * @param  {std::vector<cl_float>} allVertices Vertices to compute the bounding box from.
 * @return {BVHNode*}
 */
BVHNode* BVH::makeNode( vector<cl_uint4> faces, vector<cl_float4> allVertices ) {
	BVHNode* node = new BVHNode;
	node->leftChild = NULL;
	node->rightChild = NULL;
	node->depth = 0;

	vector<cl_float4> vertices;
	for( cl_uint i = 0; i < faces.size(); i++ ) {
		vertices.push_back( allVertices[faces[i].x] );
		vertices.push_back( allVertices[faces[i].y] );
		vertices.push_back( allVertices[faces[i].z] );
	}

	glm::vec3 bbMin;
	glm::vec3 bbMax;
	MathHelp::getAABB( vertices, &bbMin, &bbMax );
	node->bbMin = bbMin;
	node->bbMax = bbMax;

	if( faces.size() <= mMaxFaces ) {
		mLeafNodes.push_back( node );
	}
	else {
		mContainerNodes.push_back( node );
	}

	return node;
}


/**
 * Set the number of max faces per (leaf) node.
 * @param  {cl_uint} value Max faces per (leaf) node.
 * @return {cl_uint}       The now set number of max faces per (leaf) node.
 */
cl_uint BVH::setMaxFaces( cl_uint value ) {
	mMaxFaces = ( value < 1 || value > 4 ) ? 4 : value;

	return mMaxFaces;
}


/**
 * Split the faces into two groups.
 * Determine this splitting point by using a Surface Area Heuristic (SAH).
 * @param {cl_float}               nodeSA      Reciprocal of the surface area of the current node.
 * @param {cl_float*}              bestSAH     Best SAH value that has been found so far (for the faces in this node).
 * @param {cl_uint}                axis        The axis to sort the faces by.
 * @param {std::vector<cl_uint4>}  faces       Faces in this node.
 * @param {std::vector<cl_float4>} allVertices All vertices in the model.
 * @param {std::vector<cl_uint4>*} leftFaces   Output. Group for the faces left of the split.
 * @param {std::vector<cl_uint4>*} rightFaces  Output. Group for the faces right of the split.
 */
void BVH::splitBySAH(
	cl_float nodeSA, cl_float* bestSAH, cl_uint axis, vector<cl_uint4> faces, vector<cl_float4> allVertices,
	vector<cl_uint4>* leftFaces, vector<cl_uint4>* rightFaces
) {
	std::sort( faces.begin(), faces.end(), sortFacesCmp( axis, &allVertices ) );
	int numFaces = faces.size();

	vector<cl_float> leftSA( numFaces - 1 );
	vector<cl_float> rightSA( numFaces - 1 );
	vector<cl_float4> vertsForSA;


	// Grow a bounding box face by face starting from the left.
	// Save the growing surface area for each step.

	for( int i = 0; i < numFaces - 1; i++ ) {
		glm::vec3 bbMin, bbMax;
		vertsForSA.push_back( allVertices[faces[i].x] );
		vertsForSA.push_back( allVertices[faces[i].y] );
		vertsForSA.push_back( allVertices[faces[i].z] );

		MathHelp::getAABB( vertsForSA, &bbMin, &bbMax );
		leftSA[i] = MathHelp::getSurfaceArea( bbMin, bbMax );
	}


	// Grow a bounding box face by face starting from the right.
	// Save the growing surface area for each step.

	vertsForSA.clear();

	for( int i = numFaces - 2; i >= 0; i-- ) {
		glm::vec3 bbMin, bbMax;
		vertsForSA.push_back( allVertices[faces[i + 1].x] );
		vertsForSA.push_back( allVertices[faces[i + 1].y] );
		vertsForSA.push_back( allVertices[faces[i + 1].z] );

		MathHelp::getAABB( vertsForSA, &bbMin, &bbMax );
		rightSA[i] = MathHelp::getSurfaceArea( bbMin, bbMax );
	}


	// Compute the SAH for each split position and choose the one with the lowest cost.
	// SAH = SA of node * ( SA left of split * faces left of split + SA right of split * faces right of split )

	int indexSplit = -1;
	cl_float newSAH, numFacesLeft, numFacesRight;

	for( cl_uint i = 0; i < numFaces - 1; i++ ) {
		numFacesLeft = (cl_float) ( i + 1 );
		numFacesRight = (cl_float) ( numFaces - i - 1 );
		newSAH = nodeSA * ( leftSA[i] * numFacesLeft + rightSA[i] * numFacesRight );

		// Better split position found
		if( newSAH < *bestSAH ) {
			*bestSAH = newSAH;
			// Exclusive this index, up to this face it is preferable to split
			indexSplit = i + 1;
		}
	}


	// If a splitting index has been found, split the faces into two groups.

	if( indexSplit >= 0 ) {
		leftFaces->clear();
		rightFaces->clear();

		for( cl_uint i = 0; i < indexSplit; i++ ) {
			leftFaces->push_back( faces[i] );
		}

		for( cl_uint i = indexSplit; i < numFaces; i++ ) {
			rightFaces->push_back( faces[i] );
		}
	}
}


/**
 * Split the faces into two groups using the given midpoint and axis as criterium.
 * @param {std::vector<cl_uint4>}  faces
 * @param {std::vector<cl_float4>} vertices
 * @param {cl_float}               midpoint
 * @param {cl_uint}                axis
 * @param {std::vector<cl_uint4>*} leftFaces
 * @param {std::vector<cl_uint4>*} rightFaces
 */
void BVH::splitFaces(
	vector<cl_uint4> faces, vector<cl_float4> vertices, cl_float midpoint, cl_uint axis,
	vector<cl_uint4>* leftFaces, vector<cl_uint4>* rightFaces
) {
	cl_uint4 face;
	cl_float4 v0, v1, v2;
	glm::vec3 cen;

	for( cl_uint i = 0; i < faces.size(); i++ ) {
		face = faces[i];
		v0 = vertices[face.x];
		v1 = vertices[face.y];
		v2 = vertices[face.z];
		cen = MathHelp::getTriangleCentroid( v0, v1, v2 );

		if( cen[axis] < midpoint ) {
			leftFaces->push_back( face );
		}
		else {
			rightFaces->push_back( face );
		}
	}

	// One group has no children. We cannot allow that.
	// Try again with the triangle center instead of the centroid.
	if( leftFaces->size() == 0 || rightFaces->size() == 0 ) {
		Logger::logDebugVerbose( "[BVH] Dividing faces by centroid left one side empty. Trying again with center." );

		leftFaces->clear();
		rightFaces->clear();

		for( cl_uint i = 0; i < faces.size(); i++ ) {
			face = faces[i];
			v0 = vertices[face.x];
			v1 = vertices[face.y];
			v2 = vertices[face.z];
			cen = MathHelp::getTriangleCenter( v0, v1, v2 );

			if( cen[axis] < midpoint ) {
				leftFaces->push_back( face );
			}
			else {
				rightFaces->push_back( face );
			}
		}
	}

	// Oh, come on! Just do it 50:50 then.
	if( leftFaces->size() == 0 || rightFaces->size() == 0 ) {
		Logger::logDebugVerbose( "[BVH] Dividing faces by center left one side empty. Just doing it 50:50 now." );

		leftFaces->clear();
		rightFaces->clear();

		for( cl_uint i = 0; i < faces.size(); i++ ) {
			if( i < faces.size() / 2 ) {
				leftFaces->push_back( faces[i] );
			}
			else {
				rightFaces->push_back( faces[i] );
			}
		}
	}

	// There has to be somewhere else something wrong.
	if( leftFaces->size() == 0 || rightFaces->size() == 0 ) {
		char msg[256];
		snprintf(
			msg, 256, "[BVH] Dividing faces 50:50 left one side empty. Faces: %lu. Vertices: %lu.",
			faces.size(), vertices.size()
		);
		Logger::logError( msg );
	}
}


/**
 * Split the nodes into two groups using the given midpoint and axis as criterium.
 * @param {std::vector<BVHNode*>}  nodes
 * @param {cl_float}               midpoint
 * @param {cl_uint}                axis
 * @param {std::vector<BVHNode*>*} leftGroup
 * @param {std::vector<BVHNode*>*} rightGroup
 */
void BVH::splitNodes(
	vector<BVHNode*> nodes, cl_float midpoint, cl_uint axis,
	vector<BVHNode*>* leftGroup, vector<BVHNode*>* rightGroup
) {
	for( cl_uint i = 0; i < nodes.size(); i++ ) {
		BVHNode* node = nodes[i];
		glm::vec3 center = ( node->bbMax - node->bbMin ) / 2.0f;

		if( center[axis] < midpoint ) {
			leftGroup->push_back( node );
		}
		else {
			rightGroup->push_back( node );
		}
	}

	// Just do it 50:50 then.
	if( leftGroup->size() == 0 || rightGroup->size() == 0 ) {
		Logger::logDebugVerbose( "[BVH] Dividing nodes by center left one side empty. Just doing it 50:50 now." );

		leftGroup->clear();
		rightGroup->clear();

		for( cl_uint i = 0; i < nodes.size(); i++ ) {
			if( i < nodes.size() / 2 ) {
				leftGroup->push_back( nodes[i] );
			}
			else {
				rightGroup->push_back( nodes[i] );
			}
		}
	}

	// There has to be somewhere else something wrong.
	if( leftGroup->size() == 0 || rightGroup->size() == 0 ) {
		char msg[256];
		snprintf(
			msg, 256, "[BVH] Dividing nodes 50:50 left one side empty. Nodes: %lu.", nodes.size()
		);
		Logger::logError( msg );
	}
}


/**
 * Get vertices and indices to draw a 3D visualization of the bounding box.
 * @param {std::vector<cl_float>*} vertices Vector to put the vertices into.
 * @param {std::vector<cl_uint>*}  indices  Vector to put the indices into.
 */
void BVH::visualize( vector<cl_float>* vertices, vector<cl_uint>* indices ) {
	this->visualizeNextNode( mRoot, vertices, indices );
}


/**
 * Visualize the next node in the BVH.
 * @param {BVHNode*}               node     Current node.
 * @param {std::vector<cl_float>*} vertices Vector to put the vertices into.
 * @param {std::vector<cl_uint>*}  indices  Vector to put the indices into.
 */
void BVH::visualizeNextNode( BVHNode* node, vector<cl_float>* vertices, vector<cl_uint>* indices ) {
	if( node == NULL ) {
		return;
	}

	// Only visualize leaf nodes
	if( node->faces.size() > 0 ) {
		cl_uint i = vertices->size() / 3;

		// bottom
		vertices->push_back( node->bbMin[0] ); vertices->push_back( node->bbMin[1] ); vertices->push_back( node->bbMin[2] );
		vertices->push_back( node->bbMin[0] ); vertices->push_back( node->bbMin[1] ); vertices->push_back( node->bbMax[2] );
		vertices->push_back( node->bbMax[0] ); vertices->push_back( node->bbMin[1] ); vertices->push_back( node->bbMax[2] );
		vertices->push_back( node->bbMax[0] ); vertices->push_back( node->bbMin[1] ); vertices->push_back( node->bbMin[2] );

		// top
		vertices->push_back( node->bbMin[0] ); vertices->push_back( node->bbMax[1] ); vertices->push_back( node->bbMin[2] );
		vertices->push_back( node->bbMin[0] ); vertices->push_back( node->bbMax[1] ); vertices->push_back( node->bbMax[2] );
		vertices->push_back( node->bbMax[0] ); vertices->push_back( node->bbMax[1] ); vertices->push_back( node->bbMax[2] );
		vertices->push_back( node->bbMax[0] ); vertices->push_back( node->bbMax[1] ); vertices->push_back( node->bbMin[2] );

		cl_uint newIndices[24] = {
			// bottom
			i + 0, i + 1,
			i + 1, i + 2,
			i + 2, i + 3,
			i + 3, i + 0,
			// top
			i + 4, i + 5,
			i + 5, i + 6,
			i + 6, i + 7,
			i + 7, i + 4,
			// back
			i + 0, i + 4,
			i + 3, i + 7,
			// front
			i + 1, i + 5,
			i + 2, i + 6
		};
		indices->insert( indices->end(), newIndices, newIndices + 24 );
	}

	// Proceed with left side
	this->visualizeNextNode( node->leftChild, vertices, indices );

	// Proceed width right side
	this->visualizeNextNode( node->rightChild, vertices, indices );
}
