#include "KdTree.h"

#define EPSILON 0.00001f

using std::vector;


/**
 * Constructor.
 * @param {std::vector<float>}   vertices Vertices of the model.
 * @param {std::vector<cl_uint>} faces    Indices describing the faces (triangles) of the model.
 * @param {cl_float*}            bbMin    Bounding box minimum.
 * @param {cl_float*}            bbMax    Bounding box maximum.
 */
KdTree::KdTree( vector<cl_float> vertices, vector<cl_uint> faces, cl_float* bbMin, cl_float* bbMax ) {
	if( vertices.size() <= 0 || faces.size() <= 0 ) {
		return;
	}

	mDepthLimit = Cfg::get().value<cl_uint>( Cfg::KDTREE_DEPTH );

	// Start clock
	boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();

	// Create nodes for tree
	mNodes = this->createUnconnectedNodes( &vertices );
	mLeafIndex = mNodes.size() - 1;

	// Generate kd-tree
	cl_int rootIndex = this->makeTree( mNodes, 0, bbMin, bbMax, 1 );
	mNodes.insert( mNodes.end(), mLeaves.begin(), mLeaves.end() );
	mRoot = mNodes[rootIndex];

	// Associate faces with leave nodes
	this->assignFacesToLeaves( &vertices, &faces );

	// Create ropes for leaf nodes
	this->createRopes( mRoot, vector<cl_int>( 6, -1 ) );

	// Stop clock
	boost::posix_time::time_duration msdiff = boost::posix_time::microsec_clock::local_time() - start;
	char msg[80];
	const char* text = "[KdTree] Generated kd-tree in %g ms. %lu nodes.";
	snprintf( msg, 80, text, (float) msdiff.total_milliseconds(), mNodes.size() );
	Logger::logInfo( msg );
}


/**
 * Deconstructor.
 */
KdTree::~KdTree() {
	for( unsigned int i = 0; i < mNodes.size(); i++ ) {
		delete mNodes[i];
	}
}


/**
 * Store in the leaf nodes which faces (triangles) intersect with them.
 * @param {std::vector<cl_float>*} vertices Vertices of the model.
 * @param {std::vector<cl_uint>*}  faces    Faces (triangles) of the model.
 */
void KdTree::assignFacesToLeaves( vector<cl_float>* vertices, vector<cl_uint>* faces ) {
	bool add;

	for( cl_uint i = 0; i < faces->size(); i += 3 ) {
		glm::vec3 a = glm::vec3(
			(*vertices)[(*faces)[i] * 3],
			(*vertices)[(*faces)[i] * 3 + 1],
			(*vertices)[(*faces)[i] * 3 + 2]
		);
		glm::vec3 b = glm::vec3(
			(*vertices)[(*faces)[i + 1] * 3],
			(*vertices)[(*faces)[i + 1] * 3 + 1],
			(*vertices)[(*faces)[i + 1] * 3 + 2]
		);
		glm::vec3 c = glm::vec3(
			(*vertices)[(*faces)[i + 2] * 3],
			(*vertices)[(*faces)[i + 2] * 3 + 1],
			(*vertices)[(*faces)[i + 2] * 3 + 2]
		);

		for( cl_uint j = 0; j < mLeaves.size(); j++ ) {
			kdNode_t* l = mLeaves[j];
			glm::vec3 bbMin = glm::vec3( l->bbMin[0], l->bbMin[1], l->bbMin[2] );
			glm::vec3 bbMax = glm::vec3( l->bbMax[0], l->bbMax[1], l->bbMax[2] );

			if( find( l->faces.begin(), l->faces.end(), i ) == l->faces.end() ) {
				add = false;

				// Fast test: Is at least 1 point inside or on the bounding box?
				// Test can only accept, not decline.
				if(
					(
						a[0] >= bbMin[0] && a[1] >= bbMin[1] && a[2] >= bbMin[2] &&
						a[0] <= bbMax[0] && a[1] <= bbMax[1] && a[2] <= bbMax[2]
					) ||
					(
						b[0] >= bbMin[0] && b[1] >= bbMin[1] && b[2] >= bbMin[2] &&
						b[0] <= bbMax[0] && b[1] <= bbMax[1] && b[2] <= bbMax[2]
					) ||
					(
						c[0] >= bbMin[0] && c[1] >= bbMin[1] && c[2] >= bbMin[2] &&
						c[0] <= bbMax[0] && c[1] <= bbMax[1] && c[2] <= bbMax[2]
					)
				) {
					add = true;
				}

				if( !add && this->hitBoundingBox( bbMin, bbMax, a, b - a ) ) {
					add = true;
				}
				if( !add && this->hitBoundingBox( bbMin, bbMax, b, c - b ) ) {
					add = true;
				}
				if( !add && this->hitBoundingBox( bbMin, bbMax, c, a - c ) ) {
					add = true;
				}
				if( !add && this->hitTriangle( bbMin, bbMax, a, b, c ) ) {
					add = true;
				}

				if( add ) {
					l->faces.push_back( i );
				}
			}
		}
	}

	// this->printNumFacesOfLeaves();
}


/**
 * Create a leaf node. (Leaf nodes have no children.)
 * @param  {cl_float*} bbMin Bounding box minimum.
 * @param  {cl_float*} bbMax Bounding box maximum.
 * @return {kdNode_t*}       The leaf node with the given bounding box.
 */
kdNode_t* KdTree::createLeafNode( cl_float* bbMin, cl_float* bbMax ) {
	mLeafIndex++;

	kdNode_t* leaf = new kdNode_t;
	leaf->index = mLeafIndex;
	leaf->axis = -1;
	leaf->bbMin[0] = bbMin[0];
	leaf->bbMin[1] = bbMin[1];
	leaf->bbMin[2] = bbMin[2];
	leaf->bbMax[0] = bbMax[0];
	leaf->bbMax[1] = bbMax[1];
	leaf->bbMax[2] = bbMax[2];
	leaf->left = -1;
	leaf->right = -1;

	return leaf;
}


/**
 * Create ropes between neighbouring nodes. Only leaf nodes store ropes.
 * @param {kdNode_t*}           node  The current node to process.
 * @param {std::vector<cl_int>} ropes Current list of ropes for this node.
 */
void KdTree::createRopes( kdNode_t* node, vector<cl_int> ropes ) {
	if( node->left < 0 && node->right < 0 ) {
		node->ropes = ropes;
		return;
	}

	for( int i = 0; i < 6; i++ ) {
		if( ropes[i] < 0 ) {
			continue;
		}
		this->optimizeRope( &(ropes[i]), i, node->bbMin, node->bbMax );
	}

	cl_int sideLeft = node->axis * 2;
	cl_int sideRight = node->axis * 2 + 1;

	vector<cl_int> ropesLeft( ropes );
	ropesLeft[sideRight] = node->right;
	this->createRopes( mNodes[node->left], ropesLeft );

	vector<cl_int> ropesRight( ropes );
	ropesRight[sideLeft] = node->left;
	this->createRopes( mNodes[node->right], ropesRight );
}


/**
 * Create nodes for the tree from the given vertices.
 * At this point the nodes are still unconnected.
 * @param  {std::vector<cl_float>*} vertices Vertices to create nodes of.
 * @return {std::vector<kdNode_t*>}          List of unconnected kd-nodes.
 */
vector<kdNode_t*> KdTree::createUnconnectedNodes( vector<cl_float>* vertices ) {
	vector<kdNode_t*> nodes;

	for( cl_uint i = 0; i < vertices->size(); i += 3 ) {
		kdNode_t* node = new kdNode_t;
		node->index = i / 3;
		node->pos[0] = (*vertices)[i];
		node->pos[1] = (*vertices)[i + 1];
		node->pos[2] = (*vertices)[i + 2];
		node->left = -1;
		node->right = -1;

		nodes.push_back( node );
	}

	return nodes;
}


/**
 * Test if a given line segment intersects a given bounding box.
 * @param  {glm::vec3} bbMin  [description]
 * @param  {glm::vec3} bbMax  [description]
 * @param  {glm::vec3} origin [description]
 * @param  {glm::vec3} dir    [description]
 * @return {bool}             [description]
 */
bool KdTree::hitBoundingBox(
	glm::vec3 bbMin, glm::vec3 bbMax, glm::vec3 origin, glm::vec3 dir
) {
	glm::vec3 invDir = 1.0f / dir;
	float bounds[2][3] = {
		bbMin[0], bbMin[1], bbMin[2],
		bbMax[0], bbMax[1], bbMax[2]
	};
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	bool signX = invDir[0] < 0.0f;
	bool signY = invDir[1] < 0.0f;
	bool signZ = invDir[2] < 0.0f;

	// X
	tmin = ( bounds[signX][0] - origin[0] ) * invDir[0];
	tmax = ( bounds[1 - signX][0] - origin[0] ) * invDir[0];
	// Y
	tymin = ( bounds[signY][1] - origin[1] ) * invDir[1];
	tymax = ( bounds[1 - signY][1] - origin[1] ) * invDir[1];

	if( tmin > tymax || tymin > tmax ) {
		return false;
	}

	// X vs. Y
	tmin = ( tymin > tmin ) ? tymin : tmin;
	tmax = ( tymax < tmax ) ? tymax : tmax;
	// Z
	tzmin = ( bounds[signZ][2] - origin[2] ) * invDir[2];
	tzmax = ( bounds[1 - signZ][2] - origin[2] ) * invDir[2];

	if( tmin > tzmax || tzmin > tmax ) {
		return false;
	}

	// Z vs. previous winner
	tmin = ( tzmin > tmin ) ? tzmin : tmin;
	tmax = ( tzmax < tmax ) ? tzmax : tmax;

	// TODO: Improve test. Preferably without using "isnan()".
	return (
		( tmin >= -EPSILON && tmax <= 1.0f + EPSILON ) ||
		( isnan( tmin ) && tmax <= 1.0f + EPSILON ) ||
		( isnan( tmax ) && tmin >= -EPSILON )
	);
}


/**
 * Test if a given vector hits a given triangle.
 * @param  {glm::vec3} vStart Start point of the vector.
 * @param  {glm::vec3} vEnd   End point of the vector.
 * @param  {glm::vec3} a      Point of the triangle.
 * @param  {glm::vec3} b      Point of the triangle.
 * @param  {glm::vec3} c      Point of the triangle.
 * @return {bool}             True if vector intersects triangle, false otherwise.
 */
bool KdTree::hitTriangle( glm::vec3 vStart, glm::vec3 vEnd, glm::vec3 a, glm::vec3 b, glm::vec3 c ) {
	glm::vec3 dir = vEnd - vStart;
	glm::vec3 edge1 = b - a;
	glm::vec3 edge2 = c - a;
	glm::vec3 pVec = glm::cross( dir, edge2 );
	cl_float det = glm::dot( edge1, pVec );

	if( abs( det ) < EPSILON ) {
		return false;
	}

	glm::vec3 tVec = vStart - a;
	cl_float u = glm::dot( tVec, pVec ) / det;

	if( u < 0.0f || u > 1.0f ) {
		return false;
	}

	glm::vec3 qVec = glm::cross( tVec, edge1 );
	cl_float v = glm::dot( dir, qVec ) / det;

	if( v < 0.0f || u + v > 1.0f ) {
		return false;
	}

	cl_float t = glm::dot( edge2, qVec ) / det;

	if( t < 0.0f || t > 1.0f ) {
		return false;
	}

	return true;
}


/**
 * Optimize the rope connection for a node by "pushing" it further down in the tree.
 * The neighbouring leaf node will be reached faster later by following optimized ropes.
 * @param {cl_int*}   rope  The rope (index to neighbouring node).
 * @param {cl_int}    side  The side (left, right, bottom, top, back, front) the rope leads to.
 * @param {cl_float*} bbMin Bounding box minimum.
 * @param {cl_float*} bbMax Bounding box maximum.
 */
void KdTree::optimizeRope( cl_int* rope, cl_int side, cl_float* bbMin, cl_float* bbMax ) {
	int axis;

	while( mNodes[*rope]->left >= 0 && mNodes[*rope]->right >= 0 ) {
		axis = mNodes[*rope]->axis;

		if( side % 2 == 0 ) { // left, bottom, back
			if( axis == ( side >> 1 ) || mNodes[*rope]->pos[axis] <= bbMin[axis] ) {
				*rope = mNodes[*rope]->right;
			}
			else {
				break;
			}
		}
		else { // right, top, front
			if( axis == ( side >> 1 ) || mNodes[*rope]->pos[axis] >= bbMax[axis] ) {
				*rope = mNodes[*rope]->left;
			}
			else {
				break;
			}
		}
	}

}


/**
 * Comparison function for sorting algorithm. Compares the X axis.
 * @param  {kdNode_t*} a Object one.
 * @param  {kdNode_t*} b Object two.
 * @return {bool}        True if a < b, false otherwise.
 */
bool KdTree::compFunc0( kdNode_t* a, kdNode_t* b ) {
	return ( a->pos[0] < b->pos[0] );
}


/**
 * Comparison function for sorting algorithm. Compares the Y axis.
 * @param  {kdNode_t*} a Object one.
 * @param  {kdNode_t*} b Object two.
 * @return {bool}        True if a < b, false otherwise.
 */
bool KdTree::compFunc1( kdNode_t* a, kdNode_t* b ) {
	return ( a->pos[1] < b->pos[1] );
}


/**
 * Comparison function for sorting algorithm. Compares the Z axis.
 * @param  {kdNode_t*} a Object one.
 * @param  {kdNode_t*} b Object two.
 * @return {bool}        True if a < b, false otherwise.
 */
bool KdTree::compFunc2( kdNode_t* a, kdNode_t* b ) {
	return ( a->pos[2] < b->pos[2] );
}


/**
 * Find the median object of the given nodes.
 * @param  {std::vector<kdNode_t*>*} nodes Pointer to the current list of nodes to find the median of.
 * @param  {cl_int}                  axis  Index of the axis to compare.
 * @return {kdNode_t*}                     The object that is the median.
 */
kdNode_t* KdTree::findMedian( vector<kdNode_t*>* nodes, cl_int axis ) {
	if( nodes->size() == 1 ) {
		return (*nodes)[0];
	}

	switch( axis ) {

		case 0:
			std::sort( nodes->begin(), nodes->end(), this->compFunc0 );
			break;

		case 1:
			std::sort( nodes->begin(), nodes->end(), this->compFunc1 );
			break;

		case 2:
			std::sort( nodes->begin(), nodes->end(), this->compFunc2 );
			break;

		default:
			Logger::logError( "[KdTree] Unknown index for axis. No sorting done." );

	}

	cl_int index = floor( nodes->size() / 2.0f );
	kdNode_t* median = (*nodes)[index];

	return median;
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


/**
 * Build a kd-tree.
 * @param  {std::vector<kdNode_t*>} nodes List of nodes to build the tree from.
 * @param  {cl_int}                 axis  Axis to use as criterium.
 * @param  {cl_float*}              bbMin Bounding box minimum.
 * @param  {cl_float*}              bbMax Bounding box maximum.
 * @return {cl_int}                       Top element for this part of the tree.
 */
cl_int KdTree::makeTree( vector<kdNode_t*> nodes, cl_int axis, cl_float* bbMin, cl_float* bbMax, cl_uint depth ) {
	// No more nodes to split planes. We have reached a leaf node.
	if( ( mDepthLimit > 0 && depth > mDepthLimit ) || nodes.size() == 0 ) {
		mLeaves.push_back( this->createLeafNode( bbMin, bbMax ) );
		return mLeafIndex;
	}

	kdNode_t* median = this->findMedian( &nodes, axis );
	median->axis = axis;
	median->bbMin[0] = bbMin[0];
	median->bbMin[1] = bbMin[1];
	median->bbMin[2] = bbMin[2];
	median->bbMax[0] = bbMax[0];
	median->bbMax[1] = bbMax[1];
	median->bbMax[2] = bbMax[2];

	vector<kdNode_t*> left;
	vector<kdNode_t*> right;
	bool leftSide = true;

	for( cl_uint i = 0; i < nodes.size(); i++ ) {
		if( leftSide ) {
			if( nodes[i] == median ) {
				leftSide = false;
			}
			else {
				left.push_back( nodes[i] );
			}
		}
		else {
			right.push_back( nodes[i] );
		}
	}

	// Bounding box of the "left" part
	cl_float bbMinLeft[3] = { bbMin[0], bbMin[1], bbMin[2] };
	cl_float bbMaxLeft[3] = { bbMax[0], bbMax[1], bbMax[2] };
	bbMaxLeft[axis] = median->pos[median->axis];

	// Bounding box of the "right" part
	cl_float bbMinRight[3] = { bbMin[0], bbMin[1], bbMin[2] };
	cl_float bbMaxRight[3] = { bbMax[0], bbMax[1], bbMax[2] };
	bbMinRight[axis] = median->pos[median->axis];

	axis = ( axis + 1 ) % KD_DIM;
	median->left = this->makeTree( left, axis, bbMinLeft, bbMaxLeft, ++depth );
	median->right = this->makeTree( right, axis, bbMinRight, bbMaxRight, ++depth );

	return median->index;
}


/**
 * Print the tree to console.
 */
void KdTree::print() {
	if( mRoot == NULL ) {
		printf( "Tree is empty.\n" );
		return;
	}

	this->printNode( mRoot );
}


/**
 * Print part of the tree to console.
 * @param {kdNode_t*} node Starting node.
 */
void KdTree::printNode( kdNode_t* node ) {
	if( node->left == -1 && node->right == -1 ) {
		printf( "END\n" );
		return;
	}

	printf( "(%g %g %g) ", node->pos[0], node->pos[1], node->pos[2] );
	if( node->left >= 0 ) {
		printf( "l" ); this->printNode( mNodes[node->left] );
	}
	if( node->right >= 0 ) {
		printf( "r" ); this->printNode( mNodes[node->right] );
	}
}


/**
 * Print the number of faces of each leaf node.
 */
void KdTree::printNumFacesOfLeaves() {
	for( int i = 0; i < mLeaves.size(); i++ ) {
		printf( "%3d: %3lu faces\n", mLeaves[i]->index, mLeaves[i]->faces.size() );
	}
}


/**
 * Get vertices and indices to draw a 3D visualization of the kd-tree.
 * @param {float*}                     bbMin    Bounding box minimum coordinates.
 * @param {float*}                     bbMax    Bounding box maximum coordinates.
 * @param {std::vector<float>*}        vertices Vector to put the vertices into.
 * @param {std::vector<unsigned int>*} indices  Vector to put the indices into.
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
	if( node->left == -1 && node->right == -1 ) {
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
		i, i+1,
		i+1, i+2,
		i+2, i+3,
		i+3, i
	};
	indices->insert( indices->end(), newIndices, newIndices + 8 );

	// Proceed with left side
	if( node->left > -1 ) {
		this->visualizeNextNode( mNodes[node->left], vertices, indices );
	}

	// Proceed width right side
	if( node->right > -1 ) {
		this->visualizeNextNode( mNodes[node->right], vertices, indices );
	}
}
