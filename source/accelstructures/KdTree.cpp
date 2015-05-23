#include "KdTree.h"

using std::map;
using std::vector;


/**
 * Struct to use as comparator in std::sort() for the faces (Tri).
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
	 * Compare two faces by their bounding box.
	 * @param  {const Tri} a Indizes for vertices that describe a face.
	 * @param  {const Tri} b Indizes for vertices that describe a face.
	 * @return {bool}        a < b
	 */
	bool operator()( const Tri a, const Tri b ) {
		float aMin = a.bbMin[this->axis];
		float bMin = b.bbMin[this->axis];

		if( aMin == bMin ) {
			return a.bbMax[this->axis] < b.bbMin[this->axis];
		}

		return aMin < bMin;
	};

};


/**
 * Constructor.
 * @param {std::vector<cl_uint>}  facesV
 * @param {std::vector<cl_uint>}  facesVN
 * @param {std::vector<cl_float>} vertices
 * @param {std::vector<cl_float>} normals
 */
KdTree::KdTree(
	vector<cl_uint> facesV, vector<cl_uint> facesVN,
	vector<cl_float> vertices, vector<cl_float> normals
) {
	vector<cl_float4> vertices4 = this->packFloatAsFloat4( &vertices );
	vector<cl_float4> normals4 = this->packFloatAsFloat4( &normals );

	vector<Tri> triFaces;

	for( uint i = 0; i < facesV.size(); i += 3 ) {
		cl_uint4 fv = { facesV[i], facesV[i + 1], facesV[i + 2], i / 3 };
		cl_uint4 fn = { facesVN[i], facesVN[i + 1], facesVN[i + 2], i / 3 };

		Tri tri;
		tri.face = fv;
		tri.normals = fn;
		MathHelp::triCalcAABB( &tri, &vertices4, &normals4 );
		triFaces.push_back( tri );
	}


	if( triFaces.size() <= 0 ) {
		Logger::logError( "[KdTree] Didn't receive any faces. No kD-tree could be constructed." );
		return;
	}

	boost::posix_time::ptime timerStart = boost::posix_time::microsec_clock::local_time();


	glm::vec3 bbMin = triFaces[0].bbMin;
	glm::vec3 bbMax = triFaces[0].bbMax;

	for( uint i = 1; i < triFaces.size(); i++ ) {
		bbMin = glm::min( bbMin, triFaces[i].bbMin );
		bbMax = glm::max( bbMax, triFaces[i].bbMax );
	}

	mBBmin = bbMin;
	mBBmax = bbMax;

	mMinFaces = fmax( Cfg::get().value<cl_uint>( Cfg::KDTREE_MINFACES ), 1 );

	mRoot = this->makeTree( triFaces, bbMin, bbMax, 1 );
	if( mRoot->left == NULL && mRoot->right == NULL ) {
		Logger::logWarning( "[KdTree] Root node is a leaf. This isn't supported." );
	}

	this->createRopes( mRoot, vector<kdNode_t*>( 6, NULL ) );
	this->printLeafFacesStat();

	boost::posix_time::ptime timerEnd = boost::posix_time::microsec_clock::local_time();
	float timeDiff = ( timerEnd - timerStart ).total_milliseconds();

	char msg[256];
	snprintf(
		msg, 256, "[KdTree] Generated kd-tree in %g ms. %lu nodes (%lu leaves).",
		timeDiff, mNodes.size(), mLeaves.size()
	);
	Logger::logInfo( msg );
}


/**
 * Destructor.
 */
KdTree::~KdTree() {
	for( cl_uint i = 0; i < mNodes.size(); i++ ) {
		delete mNodes[i];
	}
}


/**
 * Create a leaf node. (Leaf nodes have no children.)
 * @param  {cl_float*}             bbMin    Bounding box minimum.
 * @param  {cl_float*}             bbMax    Bounding box maximum.
 * @param  {std::vector<cl_float>} vertices List of all vertices in the scene.
 * @param  {std::vector<cl_uint4>} faces    List of all faces assigned to this node.
 * @return {kdNode_t*}                      The leaf node with the given bounding box.
 */
kdNode_t* KdTree::createLeafNode( glm::vec3 bbMin, glm::vec3 bbMax, vector<Tri> faces ) {
	kdNode_t* leaf = new kdNode_t;
	leaf->index = mLeaves.size();
	leaf->axis = -1;
	leaf->bbMin = glm::vec3( bbMin );
	leaf->bbMax = glm::vec3( bbMax );
	leaf->left = NULL;
	leaf->right = NULL;
	leaf->faces = faces;

	mNodes.push_back( leaf );
	mLeaves.push_back( leaf );

	return leaf;
}


/**
 * Create ropes between neighbouring nodes. Only leaf nodes store ropes.
 * @param {kdNode_t*}              node  The current node to process.
 * @param {std::vector<kdNode_t*>} ropes Current list of ropes for this node.
 */
void KdTree::createRopes( kdNode_t* node, vector<kdNode_t*> ropes ) {
	if( node->axis < 0 ) {
		node->ropes = ropes;
		return;
	}

	this->optimizeRope( &ropes, node->bbMin, node->bbMax );

	cl_int sideLeft = node->axis * 2;
	cl_int sideRight = node->axis * 2 + 1;

	vector<kdNode_t*> ropesLeft( ropes );
	ropesLeft[sideRight] = node->right;
	this->createRopes( node->left, ropesLeft );

	vector<kdNode_t*> ropesRight( ropes );
	ropesRight[sideLeft] = node->left;
	this->createRopes( node->right, ropesRight );
}


/**
 * Find a suitable split position using SAH.
 * @param  {std::vector<cl_float4>}  vertsForNodes Current list of vertices to pick the median from.
 * @param  {cl_int}                  axis          Index of the axis to compare.
 * @return {kdNode_t*}                             The object that is the median.
 */
kdNode_t* KdTree::getSplit(
	vector<Tri>* faces, uint axis, glm::vec3 bbMinNode, glm::vec3 bbMaxNode,
	vector<Tri>* leftFaces, vector<Tri>* rightFaces
) {
	const cl_uint numFaces = faces->size();

	std::sort( faces->begin(), faces->end(), sortFacesCmp( axis ) );

	vector<cl_float> leftSA( numFaces - 1 );
	vector<cl_float> rightSA( numFaces - 1 );
	vector< vector<glm::vec3> > leftBB( numFaces - 1 ), rightBB( numFaces - 1 );
	vector<glm::vec3> bbMins, bbMaxs;
	vector<uint> numFacesLeft, numFacesRight;


	int medianIndex = floor( ( numFaces - 1 ) / 2 );
	cl_float pos = (*faces)[medianIndex].bbMin[axis];

	for( uint i = 0; i < numFaces; i++ ) {
		if( (*faces)[i].bbMin[axis] <= pos ) {
			leftFaces->push_back( (*faces)[i] );
		}
		if( (*faces)[i].bbMax[axis] > pos ) {
			rightFaces->push_back( (*faces)[i] );
		}
	}


	if(
		leftFaces->size() == numFaces || rightFaces->size() == numFaces ||
		leftFaces->size() == 0 || rightFaces->size() == 0
	) {
		leftFaces->clear();
		rightFaces->clear();

		for( uint i = 0; i < numFaces; i++ ) {
			if( i <= medianIndex ) {
				leftFaces->push_back( (*faces)[i] );
			}
			else {
				rightFaces->push_back( (*faces)[i] );
			}
		}
	}


	// // Grow a bounding box face by face starting from the left.
	// // Save the growing surface area for each step.

	// for( int i = 0; i < numFaces - 1; i++ ) {
	// 	glm::vec3 bbMin, bbMax;

	// 	bbMins.push_back( (*faces)[i].bbMin );
	// 	bbMaxs.push_back( (*faces)[i].bbMax );
	// 	MathHelp::getAABB( bbMins, bbMaxs, &bbMin, &bbMax );

	// 	leftBB[i] = vector<glm::vec3>( 2 );
	// 	leftBB[i][0] = bbMin;
	// 	leftBB[i][1] = bbMax;
	// 	leftSA[i] = MathHelp::getSurfaceArea( bbMin, bbMax );
	// }


	// // Grow a bounding box face by face starting from the right.
	// // Save the growing surface area for each step.

	// bbMins.clear();
	// bbMaxs.clear();

	// for( int i = numFaces - 2; i >= 0; i-- ) {
	// 	glm::vec3 bbMin, bbMax;

	// 	bbMins.push_back( (*faces)[i + 1].bbMin );
	// 	bbMaxs.push_back( (*faces)[i + 1].bbMax );
	// 	MathHelp::getAABB( bbMins, bbMaxs, &bbMin, &bbMax );

	// 	rightBB[i] = vector<glm::vec3>( 2 );
	// 	rightBB[i][0] = bbMin;
	// 	rightBB[i][1] = bbMax;
	// 	rightSA[i] = MathHelp::getSurfaceArea( bbMin, bbMax );
	// }


	// for( uint i = 0; i < numFaces - 1; i++ ) {
	// 	float split = leftBB[i][1][axis];
	// 	numFacesLeft.push_back( 0 );
	// 	numFacesRight.push_back( 0 );

	// 	for( uint j = 0; j < numFaces; j++ ) {
	// 		if( (*faces)[j].bbMin[axis] <= split ) {
	// 			numFacesLeft[i] += 1;
	// 		}
	// 		if( (*faces)[j].bbMin[axis] > split ) {
	// 			numFacesRight[i] += 1;
	// 		}
	// 	}
	// }


	// float bestSAH = FLT_MAX;
	// float pos;

	// // Compute the SAH for each split position and choose the one with the lowest cost.
	// // SAH = SA of node * ( SA left of split * faces left of split + SA right of split * faces right of split )
	// for( uint i = 0; i < numFaces - 1; i++ ) {
	// 	// Ignore splits where one side contains all faces or the
	// 	// left bounding box takes all the space or none at all.
	// 	if(
	// 		// leftBB[i][1][axis] == bbMinNode[axis] ||
	// 		// leftBB[i][1][axis] == bbMaxNode[axis] ||
	// 		numFacesLeft[i] == numFaces ||
	// 		numFacesRight[i] == numFaces
	// 	) {
	// 		continue;
	// 	}

	// 	float newSAH = leftSA[i] * (float) numFacesLeft[i] + rightSA[i] * (float) numFacesRight[i];

	// 	// Better split position found
	// 	if( newSAH < bestSAH ) {
	// 		bestSAH = newSAH;
	// 		pos = leftBB[i][1][axis];
	// 	}
	// }


	// // Using the SAH didn't find a split position, so just split 50:50.
	// if( bestSAH == FLT_MAX ) {
	// 	char msg[128] = "[KdTree] Could not split node with SAH. Doing it 50:50 instead.";
	// 	Logger::logDebug( msg );

	// 	glm::vec3 nodeLen = bbMaxNode - bbMinNode;

	// 	leftFaces->clear();
	// 	rightFaces->clear();
	// 	pos = 0.5f * ( bbMinNode[axis] + bbMaxNode[axis] );

	// 	for( uint i = 0; i < numFaces; i++ ) {
	// 		if( (*faces)[i].bbMin[axis] <= pos ) {
	// 			leftFaces->push_back( (*faces)[i] );
	// 		}
	// 		if( (*faces)[i].bbMax[axis] > pos ) {
	// 			rightFaces->push_back( (*faces)[i] );
	// 		}
	// 	}
	// }
	// // Split at the position found by the SAH.
	// else {
	// 	for( uint i = 0; i < numFaces; i++ ) {
	// 		if( (*faces)[i].bbMin[axis] <= pos ) {
	// 			leftFaces->push_back( (*faces)[i] );
	// 		}
	// 		if( (*faces)[i].bbMax[axis] > pos ) {
	// 			rightFaces->push_back( (*faces)[i] );
	// 		}
	// 	}
	// }

	if( leftFaces->size() == faces->size() || rightFaces->size() == faces->size() ) {
		Logger::logError( "[KdTree] All faces are on one side of the split!" );
		leftFaces->clear();
		rightFaces->clear();
	}
	if( leftFaces->size() == 0 || rightFaces->size() == 0 ) {
		Logger::logError( "[KdTree] One side of the split is empty!" );
		leftFaces->clear();
		rightFaces->clear();
	}

	kdNode_t* node = new kdNode_t;
	node->index = mNonLeaves.size();
	node->axis = axis;
	node->split = pos;
	node->bbMin = glm::vec3( bbMinNode );
	node->bbMax = glm::vec3( bbMaxNode );

	return node;
}


/**
 * Get the max component of the kd-tree bounding box.
 * @return {glm::vec3}
 */
glm::vec3 KdTree::getBoundingBoxMax() {
	return mBBmax;
}


/**
 * Get the min component of the kd-tree bounding box.
 * @return {glm::vec3}
 */
glm::vec3 KdTree::getBoundingBoxMin() {
	return mBBmin;
}


/**
 * Get the leaf nodes.
 * @param {std::vector<kdNode_t>} The leaf nodes of the kd-tree.
 */
vector<kdNode_t> KdTree::getLeaves() {
	vector<kdNode_t> leaves;

	for( cl_uint i = 0; i < mLeaves.size(); i++ ) {
		leaves.push_back( *mLeaves[i] );
	}

	return leaves;
}


/**
 * Get the list of generated nodes.
 * @return {std::vector<kdNode_t>} The nodes of the kd-tree.
 */
vector<kdNode_t> KdTree::getNodes() {
	vector<kdNode_t> nodes;

	for( cl_uint i = 0; i < mNodes.size(); i++ ) {
		nodes.push_back( *mNodes[i] );
	}

	return nodes;
}


/**
 * Get all non-leaf nodes.
 * @return {std::vector<kdNode_t>} The non-leaf nodes of the kd-tree.
 */
vector<kdNode_t> KdTree::getNonLeaves() {
	vector<kdNode_t> nonLeaves;

	for( cl_uint i = 0; i < mNonLeaves.size(); i++ ) {
		nonLeaves.push_back( *mNonLeaves[i] );
	}

	return nonLeaves;
}


/**
 * Get the root node of the kd-tree.
 * @return {kdNode_t*} Pointer to the root node.
 */
kdNode_t* KdTree::getRootNode() {
	return mRoot;
}


/**
 * Build a kd-tree.
 * @param  {std::vector<cl_float4>}               vertsForNodes Vertices to make kd-nodes from.
 * @param  {cl_int}                               axis          Axis to use as criterium.
 * @param  {cl_float*}                            bbMin         Bounding box minimum.
 * @param  {cl_float*}                            bbMax         Bounding box maximum.
 * @param  {std::vector<cl_float>}                vertices      All vertices of the model.
 * @param  {std::vector<cl_uint4>}                faces         Faces reaching into the bounding box of this node.
 * @param  {std::vector< std::vector<cl_float> >} splitsByAxis  List of all the coordinates used as split on each axis.
 * @param  {cl_uint}                              depth         Current kd-tree depth.
 * @return {kdNode_t*}                                          Top element for this part of the tree.
 */
kdNode_t* KdTree::makeTree(
	vector<Tri> faces, glm::vec3 bbMin, glm::vec3 bbMax, uint depth
) {
	// Faces limit reached
	if( faces.size() <= mMinFaces ) {
		return this->createLeafNode( bbMin, bbMax, faces );
	}

	// Build node from found split position.
	vector<Tri> leftFaces, rightFaces;

	uint axis = depth % 3;
	kdNode_t* node = this->getSplit( &faces, axis, bbMin, bbMax, &leftFaces, &rightFaces );

	// Keep limit for minimum faces per node
	if( mMinFaces >= glm::min( leftFaces.size(), rightFaces.size() ) ) {
		return this->createLeafNode( bbMin, bbMax, faces );
	}

	mNodes.push_back( node );
	mNonLeaves.push_back( node );

	// Bounding box of the "left" part
	glm::vec3 bbMaxLeft = glm::vec3( bbMax );
	bbMaxLeft[node->axis] = node->split;

	// Bounding box of the "right" part
	glm::vec3 bbMinRight = glm::vec3( bbMin );
	bbMinRight[node->axis] = node->split;

	// Proceed with child nodes
	node->left = this->makeTree( leftFaces, bbMin, bbMaxLeft, depth + 1 );
	node->right = this->makeTree( rightFaces, bbMinRight, bbMax, depth + 1 );

	return node;
}


/**
 * Optimize the rope connection for a node by "pushing" it further down in the tree.
 * The neighbouring leaf node will be reached faster later by following optimized ropes.
 * @param {std::vector<kdNode_t*>*} ropes The ropes (indices to neighbouring nodes).
 * @param {cl_float*}               bbMin Bounding box minimum.
 * @param {cl_float*}               bbMax Bounding box maximum.
 */
void KdTree::optimizeRope( vector<kdNode_t*>* ropes, glm::vec3 bbMin, glm::vec3 bbMax ) {
	if( !Cfg::get().value<bool>( Cfg::KDTREE_OPTIMIZEROPES ) ) {
		return;
	}

	kdNode_t* sideNode;
	uint axis;

	for( short side = 0; side < 6; side++ ) {
		if( (*ropes)[side] == NULL ) {
			continue;
		}

		while( (*ropes)[side]->axis >= 0 ) {
			sideNode = (*ropes)[side];
			axis = sideNode->axis;

			if( side % 2 == 0 ) { // left, bottom, back
				if( axis == side / 2 || sideNode->split <= bbMin[axis] ) {
					(*ropes)[side] = sideNode->right;
				}
				else {
					break;
				}
			}
			else { // right, top, front
				if( axis == ( side - 1 ) / 2 || sideNode->split >= bbMax[axis] ) {
					(*ropes)[side] = sideNode->left;
				}
				else {
					break;
				}
			}
		}
	}
}


/**
 * Print statistic of average number of faces per leaf node.
 */
void KdTree::printLeafFacesStat() {
	uint facesTotal = 0;

	for( uint i = 0; i < mLeaves.size(); i++ ) {
		facesTotal += mLeaves[i]->faces.size();
	}

	char msg[128];
	const char* text = "[KdTree] On average there are %.2f faces in the %lu leaf nodes.";
	snprintf( msg, 128, text, facesTotal / (float) mLeaves.size(), mLeaves.size() );
	Logger::logInfo( msg );
}


/**
 * Get vertices and indices to draw a 3D visualization of the kd-tree.
 * @param {std::vector<cl_float>*} vertices Vector to put the vertices into.
 * @param {std::vector<cl_uint>*}  indices  Vector to put the indices into.
 */
void KdTree::visualize( vector<cl_float>* vertices, vector<cl_uint>* indices ) {
	uint i = vertices->size() / 3;

	// bottom
	vertices->push_back( mBBmin[0] ); vertices->push_back( mBBmin[1] ); vertices->push_back( mBBmin[2] );
	vertices->push_back( mBBmin[0] ); vertices->push_back( mBBmin[1] ); vertices->push_back( mBBmax[2] );
	vertices->push_back( mBBmax[0] ); vertices->push_back( mBBmin[1] ); vertices->push_back( mBBmax[2] );
	vertices->push_back( mBBmax[0] ); vertices->push_back( mBBmin[1] ); vertices->push_back( mBBmin[2] );

	// top
	vertices->push_back( mBBmin[0] ); vertices->push_back( mBBmax[1] ); vertices->push_back( mBBmin[2] );
	vertices->push_back( mBBmin[0] ); vertices->push_back( mBBmax[1] ); vertices->push_back( mBBmax[2] );
	vertices->push_back( mBBmax[0] ); vertices->push_back( mBBmax[1] ); vertices->push_back( mBBmax[2] );
	vertices->push_back( mBBmax[0] ); vertices->push_back( mBBmax[1] ); vertices->push_back( mBBmin[2] );

	uint newIndices[24] = {
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

	this->visualizeNextNode( mRoot, vertices, indices );
}


/**
 * Visualize the next node in the kd-tree.
 * @param {kdNode_t*}              node     Current node.
 * @param {std::vector<cl_float>*} vertices Vector to put the vertices into.
 * @param {std::vector<cl_uint>*}  indices  Vector to put the indices into.
 */
void KdTree::visualizeNextNode( kdNode_t* node, vector<cl_float>* vertices, vector<cl_uint>* indices ) {
	if( node->axis < 0 ) {
		return;
	}

	cl_float a[3], b[3], c[3], d[3];
	a[node->axis] = b[node->axis] = c[node->axis] = d[node->axis] = node->split;

	switch( node->axis ) {

		case 0: // x
			a[1] = d[1] = node->bbMin[1];
			a[2] = b[2] = node->bbMin[2];
			b[1] = c[1] = node->bbMax[1];
			c[2] = d[2] = node->bbMax[2];
			break;

		case 1: // y
			a[0] = d[0] = node->bbMin[0];
			a[2] = b[2] = node->bbMin[2];
			b[0] = c[0] = node->bbMax[0];
			c[2] = d[2] = node->bbMax[2];
			break;

		case 2: // z
			a[1] = d[1] = node->bbMin[1];
			a[0] = b[0] = node->bbMin[0];
			b[1] = c[1] = node->bbMax[1];
			c[0] = d[0] = node->bbMax[0];
			break;

		default:
			Logger::logError( "[KdTree] Function visualize() encountered unknown axis index." );

	}

	cl_uint i = vertices->size() / 3;
	vertices->insert( vertices->end(), a, a + 3 );
	vertices->insert( vertices->end(), b, b + 3 );
	vertices->insert( vertices->end(), c, c + 3 );
	vertices->insert( vertices->end(), d, d + 3 );

	cl_uint newIndices[8] = {
		i,     i + 1,
		i + 1, i + 2,
		i + 2, i + 3,
		i + 3, i
	};
	indices->insert( indices->end(), newIndices, newIndices + 8 );

	// Proceed with left side
	this->visualizeNextNode( node->left, vertices, indices );

	// Proceed width right side
	this->visualizeNextNode( node->right, vertices, indices );
}
