#include "KdTree.h"

using std::map;
using std::vector;


/**
 * Constructor.
 * @param {std::vector<cl_float>} vertices Vertices of the model.
 * @param {std::vector<cl_uint>}  faces    Indices describing the faces (triangles) of the model.
 * @param {cl_float*}             bbMin    Bounding box minimum.
 * @param {cl_float*}             bbMax    Bounding box maximum.
 */
KdTree::KdTree( vector<cl_float> vertices, vector<cl_uint> faces, cl_float* bbMin, cl_float* bbMax ) {
	if( vertices.size() <= 0 || faces.size() <= 0 ) {
		Logger::logError( "[KdTree] Didn't receive any vertices or faces. No kd-tree could be constructed." );
		return;
	}

	boost::posix_time::ptime timerStart = boost::posix_time::microsec_clock::local_time();

	vector<cl_float4> vertsForNodes = this->verticesToFloat4( vertices );

	vector< vector<cl_float> > splitsByAxis;
	splitsByAxis.push_back( vector<cl_float>() );
	splitsByAxis.push_back( vector<cl_float>() );
	splitsByAxis.push_back( vector<cl_float>() );

	mMinFaces = Cfg::get().value<cl_uint>( Cfg::KDTREE_MINFACES );

	this->setDepthLimit( vertices );
	cl_uint depth = 1;
	cl_int startAxis = 0;

	mRoot = this->makeTree(
		vertsForNodes, startAxis, bbMin, bbMax,
		vertices, faces, splitsByAxis, depth
	);
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
 * Deconstructor.
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
 * @param  {std::vector<cl_uint>}  faces    List of all faces assigned to this node.
 * @return {kdNode_t*}                      The leaf node with the given bounding box.
 */
kdNode_t* KdTree::createLeafNode(
	cl_float* bbMin, cl_float* bbMax,
	vector<cl_float> vertices, vector<cl_uint> faces
) {
	kdNode_t* leaf = new kdNode_t;
	leaf->index = mLeaves.size();
	leaf->axis = -1;
	leaf->bbMin[0] = bbMin[0];
	leaf->bbMin[1] = bbMin[1];
	leaf->bbMin[2] = bbMin[2];
	leaf->bbMax[0] = bbMax[0];
	leaf->bbMax[1] = bbMax[1];
	leaf->bbMax[2] = bbMax[2];
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
 * Find the median object of the given nodes.
 * @param  {std::vector<cl_float4>}  vertsForNodes Current list of vertices to pick the median from.
 * @param  {cl_int}                  axis          Index of the axis to compare.
 * @return {kdNode_t*}                             The object that is the median.
 */
kdNode_t* KdTree::findMedian( vector<cl_float4> vertsForNodes, cl_int axis, vector<cl_float> splits ) {
	kdNode_t* median = new kdNode_t;
	cl_int index = 0;

	vector<cl_float4> candidates;

	for( int i = 0; i < vertsForNodes.size(); i++ ) {
		cl_float4 node = vertsForNodes[i];
		cl_float coord = ( (cl_float*) &node )[axis];

		if( std::find( splits.begin(), splits.end(), coord ) == splits.end() ) {
			candidates.push_back( node );
		}
	}

	if( candidates.size() > 0 ) {
		std::sort( candidates.begin(), candidates.end(), kdSortFloat4( axis ) );
		index = floor( candidates.size() / 2.0f );
	}
	else {
		return NULL;
	}

	cl_float4 medianVert = candidates[index];

	median->index = mNonLeaves.size();
	median->axis = axis;
	median->pos[0] = medianVert.x;
	median->pos[1] = medianVert.y;
	median->pos[2] = medianVert.z;

	return median;
}


/**
 * Get the bounding box of a triangle face.
 * @param  {cl_float[3]}           v0 Vertex of the triangle.
 * @param  {cl_float[3]}           v1 Vertex of the triangle.
 * @param  {cl_float[3]}           v2 Vertex of the triangle.
 * @return {std::vector<cl_float>}    Bounding box of the triangle face. First the min, then the max.
 */
vector<cl_float> KdTree::getFaceBB( cl_float v0[3], cl_float v1[3], cl_float v2[3] ) {
	vector<cl_float> bb = vector<cl_float>( 6 );

	bb[0] = glm::min( glm::min( v0[0], v1[0] ), v2[0] );
	bb[1] = glm::min( glm::min( v0[1], v1[1] ), v2[1] );
	bb[2] = glm::min( glm::min( v0[2], v1[2] ), v2[2] );

	bb[3] = glm::max( glm::max( v0[0], v1[0] ), v2[0] );
	bb[4] = glm::max( glm::max( v0[1], v1[1] ), v2[1] );
	bb[5] = glm::max( glm::max( v0[2], v1[2] ), v2[2] );

	return bb;
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
 * Get the root node of the kd-tree.
 * @return {kdNode_t*} Pointer to the root node.
 */
kdNode_t* KdTree::getRootNode() {
	return mRoot;
}


bool KdTree::hasSameCoordinates( cl_float* a, cl_float* b ) const {
	return (
		a[0] == b[0] &&
		a[1] == b[1] &&
		a[2] == b[2]
	);
}


bool KdTree::isVertexOnLeft(
	cl_float* v, cl_uint axis, cl_float* medianPos,
	vector<cl_float4> vertsForNodes, vector<cl_float4>* leftNodes
) const {
	cl_float4 clV = { v[0], v[1], v[2], 0.0f };

	return (
		v[axis] <= medianPos[axis] &&
		std::find_if( vertsForNodes.begin(), vertsForNodes.end(), kdSearchFloat4( clV ) ) != vertsForNodes.end() &&
		std::find_if( leftNodes->begin(), leftNodes->end(), kdSearchFloat4( clV ) ) == leftNodes->end()
	);
}


bool KdTree::isVertexOnRight(
	cl_float* v, cl_uint axis, cl_float* medianPos,
	vector<cl_float4> vertsForNodes, vector<cl_float4>* rightNodes
) const {
	cl_float4 clV = { v[0], v[1], v[2], 0.0f };

	return (
		v[axis] >= medianPos[axis] &&
		std::find_if( vertsForNodes.begin(), vertsForNodes.end(), kdSearchFloat4( clV ) ) != vertsForNodes.end() &&
		std::find_if( rightNodes->begin(), rightNodes->end(), kdSearchFloat4( clV ) ) == rightNodes->end()
	);
}


/**
 * Build a kd-tree.
 * @param  {std::vector<cl_float4>}               vertsForNodes Vertices to make kd-nodes from.
 * @param  {cl_int}                               axis          Axis to use as criterium.
 * @param  {cl_float*}                            bbMin         Bounding box minimum.
 * @param  {cl_float*}                            bbMax         Bounding box maximum.
 * @param  {std::vector<cl_float>}                vertices      All vertices of the model.
 * @param  {std::vector<cl_uint>}                 faces         Faces reaching into the bounding box of this node.
 * @param  {std::vector< std::vector<cl_float> >} splitsByAxis  List of all the coordinates used as split on each axis.
 * @param  {cl_uint}                              depth         Current kd-tree depth.
 * @return {kdNode_t*}                                          Top element for this part of the tree.
 */
kdNode_t* KdTree::makeTree(
	vector<cl_float4> vertsForNodes, cl_int axis, cl_float* bbMin, cl_float* bbMax,
	vector<cl_float> vertices, vector<cl_uint> faces,
	vector< vector<cl_float> > splitsByAxis, cl_uint depth
) {
	// Depth limit reached or no more vertices to split by
	if( ( mDepthLimit > 0 && depth > mDepthLimit ) || vertsForNodes.size() == 0 ) {
		return this->createLeafNode( bbMin, bbMax, vertices, faces );
	}


	// Build node from found median
	// Not decided yet if a leaf or not
	kdNode_t* median = this->findMedian( vertsForNodes, axis, splitsByAxis[axis] );

	if( median == NULL ) {
		Logger::logDebugVerbose( "[KdTree] No more unused coordinates for this axis. Making this node a leaf." );
		return this->createLeafNode( bbMin, bbMax, vertices, faces );
	}

	median->bbMin[0] = bbMin[0];
	median->bbMin[1] = bbMin[1];
	median->bbMin[2] = bbMin[2];
	median->bbMax[0] = bbMax[0];
	median->bbMax[1] = bbMax[1];
	median->bbMax[2] = bbMax[2];


	// Bounding box of the "left" part
	cl_float bbMinLeft[3] = { bbMin[0], bbMin[1], bbMin[2] };
	cl_float bbMaxLeft[3] = { bbMax[0], bbMax[1], bbMax[2] };
	bbMaxLeft[axis] = median->pos[axis];

	// Bounding box of the "right" part
	cl_float bbMinRight[3] = { bbMin[0], bbMin[1], bbMin[2] };
	cl_float bbMaxRight[3] = { bbMax[0], bbMax[1], bbMax[2] };
	bbMinRight[axis] = median->pos[axis];


	if(
		depth > 1 &&
		(
			bbMaxLeft[0] - bbMinLeft[0] < 0.00001f ||
			bbMaxLeft[1] - bbMinLeft[1] < 0.00001f ||
			bbMaxLeft[2] - bbMinLeft[2] < 0.00001f ||
			bbMaxRight[0] - bbMinRight[0] < 0.00001f ||
			bbMaxRight[1] - bbMinRight[1] < 0.00001f ||
			bbMaxRight[2] - bbMinRight[2] < 0.00001f
		)
	) {
		Logger::logDebugVerbose( "[KdTree] Bounding box of child node would be too small. Making this node a leaf." );
		return this->createLeafNode( bbMin, bbMax, vertices, faces );
	}


	// Assign vertices and faces to each side
	vector<cl_uint> leftFaces, rightFaces;
	vector<cl_float4> leftNodes, rightNodes;

	this->splitVerticesAndFacesAtMedian(
		axis, median->pos, vertsForNodes, vertices, faces,
		&leftFaces, &rightFaces, &leftNodes, &rightNodes
	);


	// Keep limit for minimum faces per node
	if( depth > 1 && mMinFaces > glm::min( leftFaces.size(), rightFaces.size() ) / 3 ) {
		Logger::logDebugVerbose( "[KdTree] Left and/or right child node would have less faces than set as minimum. Making this node a leaf." );
		return this->createLeafNode( bbMin, bbMax, vertices, faces );
	}


	// Decided: Not a leaf node
	splitsByAxis[axis].push_back( median->pos[axis] );
	mNodes.push_back( median );
	mNonLeaves.push_back( median );

	// Proceed with child nodes
	axis = ( axis + 1 ) % 3;
	median->left = this->makeTree( leftNodes, axis, bbMinLeft, bbMaxLeft, vertices, leftFaces, splitsByAxis, depth + 1 );
	median->right = this->makeTree( rightNodes, axis, bbMinRight, bbMaxRight, vertices, rightFaces, splitsByAxis, depth + 1 );

	return median;
}


/**
 * Optimize the rope connection for a node by "pushing" it further down in the tree.
 * The neighbouring leaf node will be reached faster later by following optimized ropes.
 * @param {std::vector<kdNode_t*>*} ropes The ropes (indices to neighbouring nodes).
 * @param {cl_float*}               bbMin Bounding box minimum.
 * @param {cl_float*}               bbMax Bounding box maximum.
 */
void KdTree::optimizeRope( vector<kdNode_t*>* ropes, cl_float* bbMin, cl_float* bbMax ) {
	if( !Cfg::get().value<bool>( Cfg::KDTREE_OPTIMIZEROPES ) ) {
		return;
	}

	kdNode_t* sideNode;
	cl_uint axis;

	for( int side = 0; side < 6; side++ ) {
		if( (*ropes)[side] == NULL ) {
			continue;
		}

		while( (*ropes)[side]->axis >= 0 ) {
			sideNode = (*ropes)[side];
			axis = sideNode->axis;

			if( side % 2 == 0 ) { // left, bottom, back
				if( axis == side / 2 || sideNode->pos[axis] <= bbMin[axis] ) {
					(*ropes)[side] = sideNode->right;
				}
				else {
					break;
				}
			}
			else { // right, top, front
				if( axis == ( side - 1 ) / 2 || sideNode->pos[axis] >= bbMax[axis] ) {
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
	cl_uint facesTotal = 0;

	for( cl_uint i = 0; i < mLeaves.size(); i++ ) {
		facesTotal += mLeaves[i]->faces.size() / 3;
	}

	char msg[128];
	const char* text = "[KdTree] On average there are %.2f faces in the %lu leaf nodes.";
	snprintf( msg, 128, text, facesTotal / (float) mLeaves.size(), mLeaves.size() );
	Logger::logInfo( msg );
}


/**
 * Set the depth limit for the tree.
 * Read the limit from the config.
 * @param {std::vector<cl_float>} vertices
 */
void KdTree::setDepthLimit( vector<cl_float> vertices ) {
	mDepthLimit = Cfg::get().value<cl_uint>( Cfg::KDTREE_DEPTH );

	if( mDepthLimit == -1 ) {
		mDepthLimit = round( log2( vertices.size() / 3 ) );
	}

	char msg[64];
	snprintf( msg, 64, "[KdTree] Maximum depth set to %d.", mDepthLimit );
	Logger::logInfo( msg );
}


/**
 * Assign vertices and faces to each side, depending on how the face bounding box
 * reaches into the area. Vertices and faces can be assigned to both sides, if the
 * face lies directly on the split axis.
 * @param {cl_uint}                 axis       Axis where the split occurs.
 * @param {cl_float}                axisSplit  The coordinate for the split on the axis.
 * @param {std::vector<cl_float>}   vertices   All vertices of the model.
 * @param {std::vector<cl_uint>}    faces      Faces of the current node.
 * @param {std::vector<cl_uint>*}   leftFaces  Output. Faces for the left child node.
 * @param {std::vector<cl_uint>*}   rightFaces Output. Faces for the right child node.
 * @param {std::vector<cl_float4>*} leftNodes  Output. Vertices for the left child node.
 * @param {std::vector<cl_float4>*} rightNodes Output. Vertices for the right child node.
 */
void KdTree::splitVerticesAndFacesAtMedian(
	cl_uint axis, cl_float* medianPos, vector<cl_float4> vertsForNodes,
	vector<cl_float> vertices, vector<cl_uint> faces,
	vector<cl_uint>* leftFaces, vector<cl_uint>* rightFaces,
	vector<cl_float4>* leftNodes, vector<cl_float4>* rightNodes
) {
	cl_float axisSplit = medianPos[axis];
	cl_uint a, b, c;
	bool isFaceOnPlane;
	vector<cl_float> vBB;

	for( int i = 0; i < faces.size(); i += 3 ) {
		a = faces[i] * 3;
		b = faces[i + 1] * 3;
		c = faces[i + 2] * 3;

		cl_float v0[3] = { vertices[a], vertices[a + 1], vertices[a + 2] };
		cl_float v1[3] = { vertices[b], vertices[b + 1], vertices[b + 2] };
		cl_float v2[3] = { vertices[c], vertices[c + 1], vertices[c + 2] };

		cl_float4 clV0 = { v0[0], v0[1], v0[2], 0.0f };
		cl_float4 clV1 = { v1[0], v1[1], v1[2], 0.0f };
		cl_float4 clV2 = { v2[0], v2[1], v2[2], 0.0f };

		vBB = this->getFaceBB( v0, v1, v2 );

		isFaceOnPlane = (
			v0[axis] == axisSplit &&
			v1[axis] == axisSplit &&
			v2[axis] == axisSplit
		);

		// Bounding box of face reaches into area "left" of split
		if( isFaceOnPlane || vBB[axis] < axisSplit ) {
			leftFaces->push_back( a / 3 );
			leftFaces->push_back( b / 3 );
			leftFaces->push_back( c / 3 );

			if( this->isVertexOnLeft( v0, axis, medianPos, vertsForNodes, leftNodes ) ) {
				leftNodes->push_back( clV0 );
			}
			if( this->isVertexOnLeft( v1, axis, medianPos, vertsForNodes, leftNodes ) ) {
				leftNodes->push_back( clV1 );
			}
			if( this->isVertexOnLeft( v2, axis, medianPos, vertsForNodes, leftNodes ) ) {
				leftNodes->push_back( clV2 );
			}
		}

		// Bounding box of face reaches into area "right" of split
		if( isFaceOnPlane || vBB[3 + axis] > axisSplit ) {
			rightFaces->push_back( a / 3 );
			rightFaces->push_back( b / 3 );
			rightFaces->push_back( c / 3 );

			if( this->isVertexOnRight( v0, axis, medianPos, vertsForNodes, rightNodes ) ) {
				rightNodes->push_back( clV0 );
			}
			if( this->isVertexOnRight( v1, axis, medianPos, vertsForNodes, rightNodes ) ) {
				rightNodes->push_back( clV1 );
			}
			if( this->isVertexOnRight( v2, axis, medianPos, vertsForNodes, rightNodes ) ) {
				rightNodes->push_back( clV2 );
			}
		}
	}
}


/**
 * Pack the list of single float values for the vertices as a list of cl_float4 values.
 * @param  {std::vector<cl_float>}  vertices The vertices to pack.
 * @return {std::vector<cl_float4>}          List of packed vertices.
 */
vector<cl_float4> KdTree::verticesToFloat4( vector<cl_float> vertices ) {
	vector<cl_float4> vertsForNodes;

	for( cl_uint i = 0; i < vertices.size(); i += 3 ) {
		cl_float4 node = {
			vertices[i],
			vertices[i + 1],
			vertices[i + 2],
			0.0f
		};
		vertsForNodes.push_back( node );
	}

	return vertsForNodes;
}


/**
 * Get vertices and indices to draw a 3D visualization of the kd-tree.
 * @param {std::vector<cl_float>*} vertices Vector to put the vertices into.
 * @param {std::vector<cl_uint>*}  indices  Vector to put the indices into.
 */
void KdTree::visualize( vector<cl_float>* vertices, vector<cl_uint>* indices ) {
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
	a[node->axis] = b[node->axis] = c[node->axis] = d[node->axis] = node->pos[node->axis];

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
