#include "BVH.h"

using std::vector;


/**
 * Struct to use as comparator in std::sort() for the faces (cl_uint4).
 */
struct sortFacesCmp {

	cl_uint axis;

	/**
	 * Constructor.
	 * @param {const cl_uint} axis Axis to compare the faces on.
	 */
	sortFacesCmp( const cl_uint axis ) {
		this->axis = axis;
	};

	/**
	 * Compare two faces.
	 * @param  {const Tri} a Indizes for vertices that describe a face.
	 * @param  {const Tri} b Indizes for vertices that describe a face.
	 * @return {bool}        a < b
	 */
	bool operator()( const Tri a, const Tri b ) {
		return a.bbMin[this->axis] < b.bbMin[this->axis];
	};

};


/**
 * Constructor.
 */
BVH::BVH() {}


/**
 * Build a BVH tree for each object in the scene and combine them into one big tree.
 * @param  {std::vector<object3D>} sceneObjects
 * @param  {std::vector<cl_float>} vertices
 * @param  {std::vector<cl_float>} normals
 * @return {BVH*}
 */
BVH::BVH(
	const vector<object3D> sceneObjects,
	const vector<cl_float> vertices,
	const vector<cl_float> normals
) {
	boost::posix_time::ptime timerStart = boost::posix_time::microsec_clock::local_time();
	mDepthReached = 0;
	this->setMaxFaces( Cfg::get().value<cl_uint>( Cfg::BVH_MAXFACES ) );

	vector<BVHNode*> subTrees = this->buildTreesFromObjects( &sceneObjects, &vertices, &normals );
	mRoot = this->makeContainerNode( subTrees, true );
	this->groupTreesToNodes( subTrees, mRoot, mDepthReached );
	this->combineNodes( subTrees.size() );
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
 * Assign faces to the created bins.
 * @param {const cl_uint}                              axis
 * @param {const cl_uint}                              splits
 * @param {const std::vector<Tri>*}                    faces
 * @param {const std::vector<std::vector<glm::vec3>>*} leftBin
 * @param {const std::vector<std::vector<glm::vec3>>*} rightBin
 * @param {std::vector<std::vector<Tri>>*}             leftBinFaces
 * @param {std::vector<std::vector<Tri>>*}             rightBinFaces
 */
void BVH::assignFacesToBins(
	const cl_uint axis, const cl_uint splits, const vector<Tri>* faces,
	const vector< vector<glm::vec3> >* leftBin, const vector< vector<glm::vec3> >* rightBin,
	vector< vector<Tri> >* leftBinFaces, vector< vector<Tri> >* rightBinFaces
) {
	for( cl_uint i = 0; i < splits; i++ ) {
		vector<glm::vec3> leftAABB = (*leftBin)[i];
		vector<glm::vec3> rightAABB = (*rightBin)[i];

		for( cl_uint j = 0; j < faces->size(); j++ ) {
			Tri tri = (*faces)[j];

			if( tri.bbMin[axis] <= leftAABB[1][axis] && tri.bbMax[axis] >= leftAABB[0][axis] ) {
				Tri triCpy;
				triCpy.face = tri.face;
				triCpy.bbMin = glm::vec3( tri.bbMin );
				triCpy.bbMax = glm::vec3( tri.bbMax );
				triCpy.bbMin[axis] = fmax( tri.bbMin[axis], leftAABB[0][axis] );
				triCpy.bbMax[axis] = fmin( tri.bbMax[axis], leftAABB[1][axis] );

				(*leftBinFaces)[i].push_back( triCpy );
			}
			if( tri.bbMin[axis] <= rightAABB[1][axis] && tri.bbMax[axis] >= rightAABB[0][axis] ) {
				Tri triCpy;
				triCpy.face = tri.face;
				triCpy.bbMin = glm::vec3( tri.bbMin );
				triCpy.bbMax = glm::vec3( tri.bbMax );
				triCpy.bbMin[axis] = fmax( tri.bbMin[axis], rightAABB[0][axis] );
				triCpy.bbMax[axis] = fmin( tri.bbMax[axis], rightAABB[1][axis] );

				(*rightBinFaces)[i].push_back( triCpy );
			}
		}
	}
}


/**
 * Build the sphere tree.
 * @param  {std::vector<Tri>} faces
 * @param  {const glm::vec3}  bbMin
 * @param  {const glm::vec3}  bbMax
 * @param  {cl_uint}          depth      The current depth of the node in the tree. Starts at 1.
 * @param  {bool}             useGivenBB
 * @param  {const cl_float}   rootSA
 * @return {BVHNode*}
 */
BVHNode* BVH::buildTree(
	vector<Tri> faces, const glm::vec3 bbMin, const glm::vec3 bbMax,
	cl_uint depth, bool useGivenBB, const cl_float rootSA
) {
	BVHNode* containerNode = this->makeNode( faces, false );

	if( useGivenBB ) {
		containerNode->bbMin = glm::vec3( bbMin );
		containerNode->bbMax = glm::vec3( bbMax );
	}

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


	vector<Tri> leftFaces, rightFaces;
	useGivenBB = false;
	glm::vec3 bbMinLeft, bbMaxLeft, bbMinRight, bbMaxRight;

	// SAH takes some time. Don't do it if there are too many faces.
	if( faces.size() <= Cfg::get().value<cl_uint>( Cfg::BVH_SAHFACESLIMIT ) ) {
		cl_float lambda;
		cl_float objectSAH = this->buildWithSAH(
			containerNode, faces, &leftFaces, &rightFaces, &lambda
		);

		lambda /= rootSA;

		// Split with spatial splits and compare with previous SAH.
		if( lambda > 0.00001f && Cfg::get().value<cl_uint>( Cfg::BVH_SPATIALSPLITS ) > 0 ) {
			cl_float spatialSAH = objectSAH;
			// cl_float spatialSAH = FLT_MAX;

			for( cl_uint axis = 0; axis <= 2; axis++ ) {
				this->splitBySpatialSplit(
					containerNode, axis, &spatialSAH, faces, &leftFaces, &rightFaces,
					&bbMinLeft, &bbMaxLeft, &bbMinRight, &bbMaxRight
				);
			}

			useGivenBB = ( spatialSAH < objectSAH );
		}
	}
	// Faster to build: Splitting at the midpoint of the longest axis.
	else {
		char msg[256];
		snprintf( msg, 256, "[BVH] Too many faces in node for SAH. Splitting by mean position. (%lu faces)", faces.size() );
		Logger::logDebug( msg );

		this->buildWithMeanSplit( containerNode, faces, &leftFaces, &rightFaces );
	}

	if(
		leftFaces.size() == 0 ||
		rightFaces.size() == 0 ||
		leftFaces.size() == faces.size() ||
		rightFaces.size() == faces.size()
	) {
		if( faces.size() <= 0 ) {
			Logger::logWarning( "[BVH] No faces in node." );
		}
		containerNode->faces = faces;

		return containerNode;
	}

	containerNode->leftChild = this->buildTree(
		leftFaces, bbMinLeft, bbMaxLeft, depth + 1, useGivenBB, rootSA
	);
	containerNode->rightChild = this->buildTree(
		rightFaces, bbMinRight, bbMaxRight, depth + 1, useGivenBB, rootSA
	);

	return containerNode;
}


/**
 * Build sphere trees for all given scene objects.
 * @param  {const std::vector<object3D>*} sceneObjects
 * @param  {const std::vector<cl_float>*} vertices
 * @param  {const std::vector<cl_float>*} normals
 * @return {std::vector<BVHNode*>}
 */
vector<BVHNode*> BVH::buildTreesFromObjects(
	const vector<object3D>* sceneObjects,
	const vector<cl_float>* vertices,
	const vector<cl_float>* normals
) {
	vector<BVHNode*> subTrees;
	char msg[256];
	cl_uint offset = 0;
	cl_uint offsetN = 0;

	vector<cl_float4> vertices4 = this->packFloatAsFloat4( vertices );

	for( cl_uint i = 0; i < sceneObjects->size(); i++ ) {
		vector<cl_uint4> facesThisObj;
		ModelLoader::getFacesOfObject( (*sceneObjects)[i], &facesThisObj, offset );
		offset += facesThisObj.size();

		snprintf(
			msg, 256, "[BVH] Building tree %u/%lu: \"%s\". %lu faces.",
			i + 1, sceneObjects->size(), (*sceneObjects)[i].oName.c_str(), facesThisObj.size()
		);
		Logger::logInfo( msg );


		vector<cl_uint4> faceNormalsThisObj;
		ModelLoader::getFaceNormalsOfObject( (*sceneObjects)[i], &faceNormalsThisObj, offsetN );
		offsetN += faceNormalsThisObj.size();

		vector<Tri> triFaces = this->facesToTriStructs(
			&facesThisObj, &faceNormalsThisObj, &vertices4, normals
		);


		BVHNode* rootNode = this->makeNode( triFaces, true );
		cl_float rootSA = MathHelp::getSurfaceArea( rootNode->bbMin, rootNode->bbMax );

		glm::vec3 bbMin, bbMax;
		BVHNode* st = this->buildTree( triFaces, bbMin, bbMax, 1, false, rootSA );
		subTrees.push_back( st );
	}

	return subTrees;
}


/**
 * Build the BVH using mean splits.
 * @param {BVHNode*}                node        Container node for the (sub) tree.
 * @param {const std::vector<Tri>*} faces       Faces to be arranged into a BVH.
 * @param {std::vector<Tri>*}       leftFaces   Output. Faces left of the split.
 * @param {std::vector<Tri>*}       rightFaces  Output. Faces right of the split.
 */
void BVH::buildWithMeanSplit(
	BVHNode* node, const vector<Tri> faces,
	vector<Tri>* leftFaces, vector<Tri>* rightFaces
) {
	cl_float bestSAH = FLT_MAX;

	for( cl_uint axis = 0; axis <= 2; axis++ ) {
		vector<Tri> leftFacesTmp, rightFacesTmp;
		cl_float splitPos = this->getMean( faces, axis );
		cl_float sah = this->splitFaces( faces, splitPos, axis, &leftFacesTmp, &rightFacesTmp );

		if( sah < bestSAH ) {
			bestSAH = sah;
			*leftFaces = leftFacesTmp;
			*rightFaces = rightFacesTmp;
		}
	}
}


/**
 * Build the BVH using SAH.
 * @param  {BVHNode*}          node       Container node for the (sub) tree.
 * @param  {std::vector<Tri>}  faces      Faces to be arranged into a BVH.
 * @param  {std::vector<Tri>*} leftFaces  Output. Faces left of the split.
 * @param  {std::vector<Tri>*} rightFaces Output. Faces right of the split.
 * @param  {cl_float*}         lambda     Output.
 * @return {cl_float}                     Best found SAH value.
 */
cl_float BVH::buildWithSAH(
	BVHNode* node, vector<Tri> faces,
	vector<Tri>* leftFaces, vector<Tri>* rightFaces, cl_float* lambda
) {
	cl_float bestSAH = FLT_MAX;

	for( cl_uint axis = 0; axis <= 2; axis++ ) {
		this->splitBySAH( &bestSAH, axis, faces, leftFaces, rightFaces, lambda );
	}

	return bestSAH;
}


/**
 * Calculate the SAH value.
 * @param  {const cl_float} leftSA        Surface are of the left node.
 * @param  {const cl_float} leftNumFaces  Number of faces in the left node.
 * @param  {const cl_float} rightSA       Surface area of the right node.
 * @param  {const cl_float} rightNumFaces Number of faces in the right node.
 * @return {cl_float}                     Value estimated by the SAH.
 */
cl_float BVH::calcSAH(
	const cl_float leftSA, const cl_float leftNumFaces,
	const cl_float rightSA, const cl_float rightNumFaces
) {
	return ( leftSA * leftNumFaces + rightSA * rightNumFaces );
}


/**
 * Combine the container nodes, leaf nodes and the root node into one list.
 * The root node will be at the very beginning of the list.
 * @param {const cl_uint} numSubTrees The number of generated trees (one for each 3D object).
 */
void BVH::combineNodes( const cl_uint numSubTrees ) {
	if( numSubTrees > 1 ) {
		mNodes.push_back( mRoot );
	}

	mNodes.insert( mNodes.end(), mContainerNodes.begin(), mContainerNodes.end() );

	for( cl_uint i = 0; i < mNodes.size(); i++ ) {
		mNodes[i]->id = i;

		// Leaf node
		if( mNodes[i]->faces.size() > 0 ) {
			mLeafNodes.push_back( mNodes[i] );
		}
		// Not a leaf node
		else {
			mNodes[i]->leftChild->parent = mNodes[i];
			mNodes[i]->rightChild->parent = mNodes[i];

			// Set the node with the bigger surface area as the left one
			cl_float leftSA = MathHelp::getSurfaceArea( mNodes[i]->leftChild->bbMin, mNodes[i]->leftChild->bbMax );
			cl_float rightSA = MathHelp::getSurfaceArea( mNodes[i]->rightChild->bbMin, mNodes[i]->rightChild->bbMax );

			if( rightSA > leftSA ) {
				BVHNode* tmp = mNodes[i]->leftChild;
				mNodes[i]->leftChild = mNodes[i]->rightChild;
				mNodes[i]->rightChild = tmp;
			}
		}
	}

	this->orderNodesByTraversal();
}


/**
 * Create the possible bin combinations for the spatial splits.
 * @param {const BVHNode*}                       node
 * @param {const cl_uint}                        axis
 * @param {const std::vector<cl_float>*}         splitPos
 * @param {std::vector<std::vector<glm::vec3>>*} leftBin
 * @param {std::vector<std::vector<glm::vec3>>*} rightBin
 */
void BVH::createBinCombinations(
	const BVHNode* node, const cl_uint axis, const vector<cl_float>* splitPos,
	vector< vector<glm::vec3> >* leftBin, vector< vector<glm::vec3> >* rightBin
) {
	for( cl_uint i = 0; i < splitPos->size(); i++ ) {
		(*leftBin)[i] = vector<glm::vec3>( 2 );
		(*leftBin)[i][0] = glm::vec3( node->bbMin );
		(*leftBin)[i][1] = glm::vec3( node->bbMax );
		(*leftBin)[i][1][axis] = (*splitPos)[i];

		(*rightBin)[i] = vector<glm::vec3>( 2 );
		(*rightBin)[i][0] = glm::vec3( node->bbMin );
		(*rightBin)[i][1] = glm::vec3( node->bbMax );
		(*rightBin)[i][0][axis] = (*splitPos)[i];
	}
}


/**
 *
 * @param  {const std::vector<cl_uint4>*}  facesThisObj
 * @param  {const std::vector<cl_uint4>*}  faceNormalsThisObj
 * @param  {const std::vector<cl_float4>*} vertices4
 * @param  {const std::vector<float>*}     normals
 * @return {std::vector<Tri>}
 */
vector<Tri> BVH::facesToTriStructs(
	const vector<cl_uint4>* facesThisObj, const vector<cl_uint4>* faceNormalsThisObj,
	const vector<cl_float4>* vertices4, const vector<float>* normals
) {
	vector<cl_float4> normals4 = this->packFloatAsFloat4( normals );
	vector<Tri> triFaces;

	for( uint j = 0; j < facesThisObj->size(); j++ ) {
		Tri tri;
		tri.face = (*facesThisObj)[j];
		tri.normals = (*faceNormalsThisObj)[j];
		MathHelp::triCalcAABB( &tri, vertices4, &normals4 );
		triFaces.push_back( tri );
	}

	return triFaces;
}


/**
 * Get the positions to generate bins at on the given axis for the given node.
 * @param  {const BVHNode*}        node
 * @param  {const cl_uint}         splits
 * @param  {const cl_uint}         axis
 * @return {std::vector<cl_float>}
 */
vector<cl_float> BVH::getBinSplits( const BVHNode* node, const cl_uint splits, const cl_uint axis ) {
	vector<cl_float> pos( splits );
	cl_float lenSegment = ( node->bbMax[axis] - node->bbMin[axis] ) / ( (cl_float) splits + 1.0f );

	pos[0] = node->bbMin[axis] + lenSegment;

	for( cl_uint i = 1; i < splits; i++ ) {
		pos[i] = pos[i - 1] + lenSegment;
	}

	std::sort( pos.begin(), pos.end() );
	vector<cl_float>::iterator it = std::unique( pos.begin(), pos.end() );
	pos.resize( std::distance( pos.begin(), it ) );

	return pos;
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
 * Find the mean of the triangles regarding the given axis.
 * @param  {const std::vector<Tri>} faces
 * @param  {const cl_uint}          axis
 * @return {cl_float}
 */
cl_float BVH::getMean( const vector<Tri> faces, const cl_uint axis ) {
	cl_float sum = 0.0f;

	for( cl_uint i = 0; i < faces.size(); i++ ) {
		Tri tri = faces[i];
		glm::vec3 center = 0.5f * ( tri.bbMin + tri.bbMax );
		sum += center[axis];
	}

	return sum / faces.size();
}


/**
 * Find the mean of the centers of the given nodes.
 * @param  {const std::vector<BVHNode*>} nodes
 * @param  {const cl_uint}               axis
 * @return {cl_float}
 */
cl_float BVH::getMeanOfNodes( const vector<BVHNode*> nodes, const cl_uint axis ) {
	cl_float sum = 0.0f;

	for( cl_uint i = 0; i < nodes.size(); i++ ) {
		glm::vec3 center = ( nodes[i]->bbMax - nodes[i]->bbMin ) * 0.5f;
		sum += center[axis];
	}

	return sum / nodes.size();
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
		return;
	}

	parent->depth = depth;
	mDepthReached = ( depth > mDepthReached ) ? depth : mDepthReached;

	cl_uint axis = this->longestAxis( parent );
	vector<BVHNode*> leftGroup, rightGroup;
	cl_float mean = this->getMeanOfNodes( nodes, axis );
	this->splitNodes( nodes, mean, axis, &leftGroup, &rightGroup );

	BVHNode* leftNode = this->makeContainerNode( leftGroup, false );
	parent->leftChild = leftNode;
	this->groupTreesToNodes( leftGroup, parent->leftChild, depth + 1 );

	BVHNode* rightNode = this->makeContainerNode( rightGroup, false );
	parent->rightChild = rightNode;
	this->groupTreesToNodes( rightGroup, parent->rightChild, depth + 1 );
}


/**
 * Grow AABBs according to the contained faces and calculate their surface areas.
 * @param {const std::vector<Tri>*}              faces
 * @param {std::vector<std::vector<glm::vec3>>*} leftBB
 * @param {std::vector<std::vector<glm::vec3>>*} rightBB
 * @param {std::vector<cl_float>*}               leftSA
 * @param {std::vector<cl_float>*}               rightSA
 */
void BVH::growAABBsForSAH(
	const vector<Tri>* faces,
	vector< vector<glm::vec3> >* leftBB, vector< vector<glm::vec3> >* rightBB,
	vector<cl_float>* leftSA, vector<cl_float>* rightSA
) {
	glm::vec3 bbMin, bbMax;
	const cl_uint numFaces = faces->size();


	// Grow a bounding box face by face starting from the left.
	// Save the growing surface area for each step.

	for( int i = 0; i < numFaces - 1; i++ ) {
		Tri f = (*faces)[i];

		if( i == 0 ) {
			bbMin = glm::vec3( f.bbMin );
			bbMax = glm::vec3( f.bbMax );
		}
		else {
			bbMin = glm::min( bbMin, f.bbMin );
			bbMax = glm::max( bbMax, f.bbMax );
		}

		(*leftBB)[i] = vector<glm::vec3>( 2 );
		(*leftBB)[i][0] = bbMin;
		(*leftBB)[i][1] = bbMax;
		(*leftSA)[i] = MathHelp::getSurfaceArea( bbMin, bbMax );
	}


	// Grow a bounding box face by face starting from the right.
	// Save the growing surface area for each step.

	for( int i = numFaces - 2; i >= 0; i-- ) {
		Tri f = (*faces)[i + 1];

		if( i == numFaces - 2 ) {
			bbMin = glm::vec3( f.bbMin );
			bbMax = glm::vec3( f.bbMax );
		}
		else {
			bbMin = glm::min( bbMin, f.bbMin );
			bbMax = glm::max( bbMax, f.bbMax );
		}

		(*rightBB)[i] = vector<glm::vec3>( 2 );
		(*rightBB)[i][0] = bbMin;
		(*rightBB)[i][1] = bbMax;
		(*rightSA)[i] = MathHelp::getSurfaceArea( bbMin, bbMax );
	}
}


/**
 * Log some stats.
 * @param {boost::posix_time::ptime} timerStart
 */
void BVH::logStats( boost::posix_time::ptime timerStart ) {
	boost::posix_time::ptime timerEnd = boost::posix_time::microsec_clock::local_time();
	cl_float timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	string timeUnits = "ms";

	if( timeDiff > 1000.0f ) {
		timeDiff /= 1000.0f;
		timeUnits = "s";
	}

	char msg[512];
	snprintf(
		msg, 512, "[BVH] Generated in %.2f%s. Contains %lu nodes (%lu leaves). Max faces of %u. Max depth of %u.",
		timeDiff, timeUnits.c_str(), mNodes.size(), mLeafNodes.size(), mMaxFaces, mDepthReached
	);
	Logger::logInfo( msg );
}


/**
 * Get the index of the longest axis.
 * @param  {const BVHNode*} node The node to find the longest side of.
 * @return {cl_uint}             Index of the longest axis (X: 0, Y: 1, Z: 2).
 */
cl_uint BVH::longestAxis( const BVHNode* node ) {
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
 * @param  {const std::vector<BVHNode*>} subTrees
 * @param  {const bool}                  isRoot
 * @return {BVHNode*}
 */
BVHNode* BVH::makeContainerNode( const vector<BVHNode*> subTrees, const bool isRoot ) {
	if( subTrees.size() == 1 ) {
		return subTrees[0];
	}

	BVHNode* node = new BVHNode;

	node->leftChild = NULL;
	node->rightChild = NULL;
	node->parent = NULL;
	node->depth = 0;
	node->bbMin = glm::vec3( subTrees[0]->bbMin );
	node->bbMax = glm::vec3( subTrees[0]->bbMax );

	for( cl_uint i = 1; i < subTrees.size(); i++ ) {
		node->bbMin = glm::min( node->bbMin, subTrees[i]->bbMin );
		node->bbMax = glm::max( node->bbMax, subTrees[i]->bbMax );
	}

	if( !isRoot ) {
		mContainerNodes.push_back( node );
	}

	return node;
}


/**
 * Create a new node.
 * @param  {const std::vector<Tri>} tris
 * @param  {const bool}             ignore
 * @return {BVHNode*}
 */
BVHNode* BVH::makeNode( const vector<Tri> tris, const bool ignore ) {
	BVHNode* node = new BVHNode;
	node->leftChild = NULL;
	node->rightChild = NULL;
	node->parent = NULL;
	node->depth = 0;

	vector<glm::vec3> bbMins, bbMaxs;

	for( cl_uint i = 0; i < tris.size(); i++ ) {
		bbMins.push_back( tris[i].bbMin );
		bbMaxs.push_back( tris[i].bbMax );
	}

	glm::vec3 bbMin;
	glm::vec3 bbMax;
	MathHelp::getAABB( bbMins, bbMaxs, &bbMin, &bbMax );
	node->bbMin = bbMin;
	node->bbMax = bbMax;

	if( !ignore ) {
		mContainerNodes.push_back( node );
	}

	return node;
}


/**
 * Order all BVH nodes for worst-case, left-first, stackless BVH
 * traversal as done in the OpenCL kernel.
 */
void BVH::orderNodesByTraversal() {
	vector<BVHNode*> nodesOrdered;
	BVHNode* node = mNodes[0];

	// Order the nodes.
	while( true ) {
		nodesOrdered.push_back( node );

		if( node->leftChild != NULL ) {
			node = node->leftChild;
		}
		else {
			// is left node, visit right sibling next
			if( node->parent->leftChild == node ) {
				node = node->parent->rightChild;
			}
			// is right node, go up tree
			else if( node->parent->parent != NULL ) {
				BVHNode* dummy = new BVHNode();
				dummy->parent = node->parent;

				// As long as we are on the right side of a (sub)tree,
				// skip parents until we either are at the root or
				// our parent has a true sibling again.
				while( dummy->parent->parent->rightChild == dummy->parent ) {
					dummy->parent = dummy->parent->parent;

					if( dummy->parent->parent == NULL ) {
						break;
					}
				}

				// Reached a parent with a true sibling.
				if( dummy->parent->parent != NULL ) {
					node = dummy->parent->parent->rightChild;
				}
			}
		}

		if( nodesOrdered.size() >= mNodes.size() ) {
			break;
		}
	}

	// Re-assign IDs.
	for( cl_uint i = 0; i < mNodes.size(); i++ ) {
		BVHNode* node = nodesOrdered[i];
		node->id = i;
		mNodes[i] = node;
	}
}


/**
 * Convert an array of floats to an array of float4s.
 * @param  {const std::vector<cl_float>*} vertices Input array of floats.
 * @return {std::vector<cl_float4>}                Array of float4s.
 */
vector<cl_float4> BVH::packFloatAsFloat4( const vector<cl_float>* vertices ) {
	vector<cl_float4> vertices4;

	for( cl_uint i = 0; i < vertices->size(); i += 3 ) {
		cl_float4 v = {
			(*vertices)[i + 0],
			(*vertices)[i + 1],
			(*vertices)[i + 2],
			0.0f
		};
		vertices4.push_back( v );
	}

	return vertices4;
}


/**
 * Resize bins so they fit the AABB of their contained faces.
 * @param {const cl_uint}                        splits
 * @param {const std::vector<std::vector<Tri>>*} leftBinFaces
 * @param {const std::vector<std::vector<Tri>>*} rightBinFaces
 * @param {std::vector<std::vector<glm::vec3>>*} leftBin
 * @param {std::vector<std::vector<glm::vec3>>*} rightBin
 */
void BVH::resizeBinsToFaces(
	const cl_uint splits,
	const vector< vector<Tri> >* leftBinFaces, const vector< vector<Tri> >* rightBinFaces,
	vector< vector<glm::vec3> >* leftBin, vector< vector<glm::vec3> >* rightBin
) {
	for( cl_uint i = 0; i < splits; i++ ) {
		if( (*leftBinFaces)[i].size() == 0 || (*rightBinFaces)[i].size() == 0 ) {
			continue;
		}

		(*leftBin)[i][0] = glm::vec3( (*leftBinFaces)[i][0].bbMin );
		(*leftBin)[i][1] = glm::vec3( (*leftBinFaces)[i][0].bbMax );

		for( cl_uint j = 1; j < (*leftBinFaces)[i].size(); j++ ) {
			(*leftBin)[i][0] = glm::min( (*leftBin)[i][0], (*leftBinFaces)[i][j].bbMin );
			(*leftBin)[i][1] = glm::max( (*leftBin)[i][1], (*leftBinFaces)[i][j].bbMax );
		}

		(*rightBin)[i][0] = glm::vec3( (*rightBinFaces)[i][0].bbMin );
		(*rightBin)[i][1] = glm::vec3( (*rightBinFaces)[i][0].bbMax );

		for( cl_uint j = 1; j < (*rightBinFaces)[i].size(); j++ ) {
			(*rightBin)[i][0] = glm::min( (*rightBin)[i][0], (*rightBinFaces)[i][j].bbMin );
			(*rightBin)[i][1] = glm::max( (*rightBin)[i][1], (*rightBinFaces)[i][j].bbMax );
		}
	}
}


/**
 * Set the number of max faces per (leaf) node.
 * @param  {const int} value     Max faces per (leaf) node.
 * @return {cl_uint}             The now set number of max faces per (leaf) node.
 */
cl_uint BVH::setMaxFaces( const int value ) {
	mMaxFaces = fmax( value, 1 );

	return mMaxFaces;
}


/**
 * Split the faces into two groups.
 * Determine this splitting point by using a Surface Area Heuristic (SAH).
 * @param {cl_float*}         bestSAH    Best SAH value that has been found so far (for the faces in this node).
 * @param {const cl_uint}     axis       The axis to sort the faces by.
 * @param {std::vector<Tri>}  faces      Faces in this node.
 * @param {std::vector<Tri>*} leftFaces  Output. Group for the faces left of the split.
 * @param {std::vector<Tri>*} rightFaces Output. Group for the faces right of the split.
 * @param {cl_float*}         lambda
 */
void BVH::splitBySAH(
	cl_float* bestSAH, const cl_uint axis, vector<Tri> faces,
	vector<Tri>* leftFaces, vector<Tri>* rightFaces, cl_float* lambda
) {
	std::sort( faces.begin(), faces.end(), sortFacesCmp( axis ) );
	const cl_uint numFaces = faces.size();

	vector<cl_float> leftSA( numFaces - 1 );
	vector<cl_float> rightSA( numFaces - 1 );
	vector< vector<glm::vec3> > leftBB( numFaces - 1 ), rightBB( numFaces - 1 );

	this->growAABBsForSAH( &faces, &leftBB, &rightBB, &leftSA, &rightSA );


	// Compute the SAH for each split position and choose the one with the lowest cost.
	// SAH = SA of node * ( SA left of split * faces left of split + SA right of split * faces right of split )

	int indexSplit = -1;
	cl_float newSAH, numFacesLeft, numFacesRight;

	for( cl_uint i = 0; i < numFaces - 1; i++ ) {
		cl_float overlapSA = 0.0f;
		cl_float sideX = leftBB[i][1].x - rightBB[i][0].x;
		cl_float sideY = leftBB[i][1].y - rightBB[i][0].y;
		cl_float sideZ = leftBB[i][1].z - rightBB[i][0].z;

		if( fmin( sideX, fmin( sideY, sideZ ) ) > 0.0f ) {
			overlapSA = 2.0f * ( sideX * sideY + sideX * sideZ + sideY * sideZ );
		}

		newSAH = leftSA[i] * ( i + 1 ) + rightSA[i] * ( numFaces - i - 1 );
		newSAH += overlapSA;

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

		// Overlapping surface area
		int j = indexSplit - 1;
		glm::vec3 diff = rightBB[j][1] - ( rightBB[j][1] - leftBB[j][1] );
		*lambda = diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2];

		leftFaces->insert( leftFaces->begin(), faces.begin(), faces.begin() + indexSplit );
		rightFaces->insert( rightFaces->begin(), faces.begin() + indexSplit, faces.end() );
	}
}


/**
 * Find the best splitting position while allowing spatial splits.
 * @param {BVHNode*}               node
 * @param {cl_uint}                axis
 * @param {cl_float*}              sahBest
 * @param {std::vector<Tri>}       faces
 * @param {std::vector<Tri>*}      leftFaces
 * @param {std::vector<Tri>*}      rightFaces
 * @param {glm::vec3*}             bbMinLeft
 * @param {glm::vec3*}             bbMaxLeft
 * @param {glm::vec3*}             bbMinRight
 * @param {glm::vec3*}             bbMaxRight
 */
void BVH::splitBySpatialSplit(
	BVHNode* node, const cl_uint axis, cl_float* sahBest, const vector<Tri> faces,
	vector<Tri>* leftFaces, vector<Tri>* rightFaces,
	glm::vec3* bbMinLeft, glm::vec3* bbMaxLeft, glm::vec3* bbMinRight, glm::vec3* bbMaxRight
) {
	if( node->bbMax[axis] - node->bbMin[axis] < 0.00001f ) {
		return;
	}


	// Get split positions

	cl_uint splits = Cfg::get().value<cl_uint>( Cfg::BVH_SPATIALSPLITS );
	vector<cl_float> splitPos = this->getBinSplits( node, splits, axis );
	splits = splitPos.size();

	if( splits == 0 ) {
		return;
	}


	// Create possible bin combinations according to split positions
	vector< vector<glm::vec3> > leftBin( splits ), rightBin( splits );
	this->createBinCombinations( node, axis, &splitPos, &leftBin, &rightBin );

	// Assign faces to the bin combinations and clip them on the split positions
	vector< vector<Tri> > leftBinFaces( splits ), rightBinFaces( splits );
	this->assignFacesToBins( axis, splits, &faces, &leftBin, &rightBin, &leftBinFaces, &rightBinFaces );

	// Resize bins according to their faces
	this->resizeBinsToFaces( splits, &leftBinFaces, &rightBinFaces, &leftBin, &rightBin );


	// Surface areas for SAH
	vector<cl_float> leftSA( splits ), rightSA( splits );

	for( int i = 0; i < splits; i++ ) {
		leftSA[i] = MathHelp::getSurfaceArea( leftBin[i][0], leftBin[i][1] );
		rightSA[i] = MathHelp::getSurfaceArea( rightBin[i][0], rightBin[i][1] );
	}


	// Calculate and compare SAH values

	cl_float sah[splits];
	cl_float currentBestSAH = FLT_MAX;
	int index = -1;

	for( int i = 0; i < splits; i++ ) {
		if( leftBinFaces[i].size() == 0 || rightBinFaces[i].size() == 0 ) {
			continue;
		}

		sah[i] = this->calcSAH(
			leftSA[i], leftBinFaces[i].size(), rightSA[i], rightBinFaces[i].size()
		);

		if( sah[i] < currentBestSAH ) {
			currentBestSAH = sah[i];
			index = i;
		}
	}


	// Choose best split

	if( index >= 0 && currentBestSAH == sah[index] && currentBestSAH < *sahBest ) {
		leftFaces->clear();
		rightFaces->clear();
		leftFaces->assign( leftBinFaces[index].begin(), leftBinFaces[index].end() );
		rightFaces->assign( rightBinFaces[index].begin(), rightBinFaces[index].end() );

		*bbMinLeft = leftBin[index][0];
		*bbMaxLeft = leftBin[index][1];
		*bbMinRight = rightBin[index][0];
		*bbMaxRight = rightBin[index][1];

		*sahBest = currentBestSAH;
	}
}


/**
 * Split the faces into two groups using the given pos and axis as criterium.
 * @param {const std::vector<Tri>} faces
 * @param {const cl_float}         pos
 * @param {const cl_uint}          axis
 * @param {std::vector<Tri>*}      leftFaces
 * @param {std::vector<Tri>*}      rightFaces
 */
cl_float BVH::splitFaces(
	const vector<Tri> faces, const cl_float pos, const cl_uint axis,
	vector<Tri>* leftFaces, vector<Tri>* rightFaces
) {
	cl_float sah = FLT_MAX;
	vector<glm::vec3> bbMinsL, bbMinsR, bbMaxsL, bbMaxsR;

	leftFaces->clear();
	rightFaces->clear();

	for( cl_uint i = 0; i < faces.size(); i++ ) {
		Tri tri = faces[i];
		glm::vec3 cen = ( tri.bbMin + tri.bbMax ) * 0.5f;

		if( cen[axis] < pos ) {
			leftFaces->push_back( tri );
			bbMinsL.push_back( tri.bbMin );
			bbMaxsL.push_back( tri.bbMax );
		}
		else {
			rightFaces->push_back( tri );
			bbMinsR.push_back( tri.bbMin );
			bbMaxsR.push_back( tri.bbMax );
		}
	}

	// Just do it 50:50.
	if( leftFaces->size() == 0 || rightFaces->size() == 0 ) {
		Logger::logDebugVerbose( "[BVH] Dividing faces by center left one side empty. Just doing it 50:50 now." );

		bbMinsL.clear();
		bbMaxsL.clear();
		bbMinsR.clear();
		bbMaxsR.clear();
		leftFaces->clear();
		rightFaces->clear();

		for( cl_uint i = 0; i < faces.size(); i++ ) {
			Tri tri = faces[i];

			if( i < faces.size() / 2 ) {
				leftFaces->push_back( tri );
				bbMinsL.push_back( tri.bbMin );
				bbMaxsL.push_back( tri.bbMax );
			}
			else {
				rightFaces->push_back( tri );
				bbMinsR.push_back( tri.bbMin );
				bbMaxsR.push_back( tri.bbMax );
			}
		}
	}

	glm::vec3 bbMinL, bbMinR, bbMaxL, bbMaxR;
	MathHelp::getAABB( bbMinsL, bbMaxsL, &bbMinL, &bbMaxL );
	cl_float leftSA = MathHelp::getSurfaceArea( bbMinL, bbMaxL );
	cl_float rightSA = MathHelp::getSurfaceArea( bbMinR, bbMaxR );

	sah = leftSA * leftFaces->size() + rightSA * rightFaces->size();

	// There has to be somewhere else something wrong.
	if( leftFaces->size() == 0 || rightFaces->size() == 0 ) {
		char msg[256];
		snprintf(
			msg, 256, "[BVH] Dividing faces 50:50 left one side empty. Faces: %lu.",
			faces.size()
		);
		Logger::logError( msg );

		sah = FLT_MAX;
	}

	return sah;
}


/**
 * Split the nodes into two groups using the given pos and axis as criterium.
 * @param {const std::vector<BVHNode*>}  nodes
 * @param {const cl_float}               pos
 * @param {const cl_uint}                axis
 * @param {std::vector<BVHNode*>*}       leftGroup
 * @param {std::vector<BVHNode*>*}       rightGroup
 */
void BVH::splitNodes(
	const vector<BVHNode*> nodes, const cl_float pos, const cl_uint axis,
	vector<BVHNode*>* leftGroup, vector<BVHNode*>* rightGroup
) {
	for( cl_uint i = 0; i < nodes.size(); i++ ) {
		BVHNode* node = nodes[i];
		glm::vec3 center = ( node->bbMax - node->bbMin ) / 2.0f;

		if( center[axis] < pos ) {
			leftGroup->push_back( node );
		}
		else {
			rightGroup->push_back( node );
		}
	}

	// Just do it 50:50 then.
	if( leftGroup->size() == 0 || rightGroup->size() == 0 ) {
		Logger::logDebugVerbose( "[BVH] Dividing nodes by the given position left one side empty. Just doing it 50:50 now." );

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
 * @param {const BVHNode*}         node     Current node.
 * @param {std::vector<cl_float>*} vertices Vector to put the vertices into.
 * @param {std::vector<cl_uint>*}  indices  Vector to put the indices into.
 */
void BVH::visualizeNextNode(
	const BVHNode* node, vector<cl_float>* vertices, vector<cl_uint>* indices
) {
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
