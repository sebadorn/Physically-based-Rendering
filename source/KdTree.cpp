#include "KdTree.h"

using namespace std;


/**
 * Constructor.
 * @param {std::vector<float>}        vertices Vertices of the model.
 * @param {std::vector<unsigned int>} indices  Indices describing the faces (triangles) of the model.
 */
KdTree::KdTree( vector<cl_float> vertices, vector<cl_uint> indices, cl_float* bbMin, cl_float* bbMax ) {
	if( vertices.size() <= 0 || indices.size() <= 0 ) {
		return;
	}

	unsigned int i, j;

	// Create unconnected nodes
	for( i = 0; i < vertices.size(); i += 3 ) {
		kdNode_t* node = new kdNode_t;
		node->index = i / 3;
		node->pos[0] = vertices[i];
		node->pos[1] = vertices[i + 1];
		node->pos[2] = vertices[i + 2];
		node->left = -1;
		node->right = -1;

		mNodes.push_back( node );
	}

	mLeafIndex = mNodes.size() - 1;


	// Generate kd-tree
	cl_int rootIndex = this->makeTree( mNodes, 0, bbMin, bbMax );
	mNodes.insert( mNodes.end(), mLeaves.begin(), mLeaves.end() );
	mRoot = mNodes[rootIndex];


	// Associate faces with leave nodes
	float tNear, tFar;
	bool add;

	for( i = 0; i < indices.size(); i += 3 ) {
		cl_float a[3] = {
			vertices[indices[i] * 3],
			vertices[indices[i] * 3 + 1],
			vertices[indices[i] * 3 + 2]
		};
		cl_float b[3] = {
			vertices[indices[i + 1] * 3],
			vertices[indices[i + 1] * 3 + 1],
			vertices[indices[i + 1] * 3 + 2]
		};
		cl_float c[3] = {
			vertices[indices[i + 2] * 3],
			vertices[indices[i + 2] * 3 + 1],
			vertices[indices[i + 2] * 3 + 2]
		};
		cl_float abDir[3] = {
			b[0] - a[0],
			b[1] - a[1],
			b[2] - a[2]
		};
		cl_float bcDir[3] = {
			c[0] - b[0],
			c[1] - b[1],
			c[2] - b[2]
		};
		cl_float caDir[3] = {
			a[0] - c[0],
			a[1] - c[1],
			a[2] - c[2]
		};

		for( j = 0; j < mLeaves.size(); j++ ) {
			kdNode_t* l = mLeaves[j];

			if( find( l->faces.begin(), l->faces.end(), i ) == l->faces.end() ) {
				add = false;

				if( utils::hitBoundingBox( l->bbMin, l->bbMax, a, abDir, &tNear, &tFar ) ) {
					if( tFar >= 0.0f && tNear <= 1.0f ) {
						add = true;
					}
				}
				if( !add && utils::hitBoundingBox( l->bbMin, l->bbMax, b, bcDir, &tNear, &tFar ) ) {
					if( tFar >= 0.0f && tNear <= 1.0f ) {
						add = true;
					}
				}
				if( !add && utils::hitBoundingBox( l->bbMin, l->bbMax, c, caDir, &tNear, &tFar ) ) {
					if( tFar >= 0.0f && tNear <= 1.0f ) {
						add = true;
					}
				}

				if( add ) {
					l->faces.push_back( i );
				}
			}
		}
	}


	cl_int ropesInit[6] = { -1, -1, -1, -1, -1, -1 };
	this->processNode( mRoot, vector<cl_int>( ropesInit, ropesInit + 6 ) );
}


/**
 * Deconstructor.
 */
KdTree::~KdTree() {
	for( unsigned int i = 0; i < mNodes.size(); i++ ) {
		delete mNodes[i];
	}
}


void KdTree::processNode( kdNode_t* node, vector<cl_int> ropes ) {
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
	this->processNode( mNodes[node->left], ropesLeft );

	vector<cl_int> ropesRight( ropes );
	ropesRight[sideLeft] = node->left;
	this->processNode( mNodes[node->right], ropesRight );
}


// TODO: Doesn't work
void KdTree::optimizeRope( cl_int* rope, cl_int side, cl_float* bbMin, cl_float* bbMax ) {
	return; // REMOVE
	while( mNodes[*rope]->left >= 0 && mNodes[*rope]->right >= 0 ) {
		int axis = mNodes[*rope]->axis;

		// split axis of rope parallel to side
		if( axis == side || axis == side - 1 ) {
			*rope = ( axis == side - 1 ) ? mNodes[*rope]->left : mNodes[*rope]->right;
		}
		// split plane of rope above bbMin
		else if( mNodes[*rope]->pos[axis] > bbMin[axis] ) {
			*rope = mNodes[*rope]->right;
		}
		// split plane of rope below bbMax
		else if( mNodes[*rope]->pos[axis] < bbMax[axis] ) {
			*rope = mNodes[*rope]->left;
		}
		else {
			break;
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
 * @param  {int}                     axis  Index of the axis to compare.
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

	for( unsigned int i = 0; i < mNodes.size(); i++ ) {
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
 * @param  {int}                    axis  Axis to use as criterium.
 * @return {int}                          Top element for this part of the tree.
 */
cl_int KdTree::makeTree( vector<kdNode_t*> nodes, cl_int axis, cl_float* bbMin, cl_float* bbMax ) {
	// No more nodes to split planes.
	// We have reached a leaf node.
	if( nodes.size() == 0 ) {
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

		mLeaves.push_back( leaf );

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

	for( unsigned int i = 0; i < nodes.size(); i++ ) {
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
	median->left = this->makeTree( left, axis, bbMinLeft, bbMaxLeft );
	median->right = this->makeTree( right, axis, bbMinRight, bbMaxRight );

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
 * @param {kdNode_t*}                  node     Current node.
 * @param {float*}                     bbMin    Bounding box minimum coordinates.
 * @param {float*}                     bbMax    Bounding box maximum coordinates.
 * @param {std::vector<GLfloat>*}        vertices Vector to put the vertices into.
 * @param {std::vector<GLuint>*} indices  Vector to put the indices into.
 * @param {unsigned int}               axis     Current axis.
 */
void KdTree::visualizeNextNode( kdNode_t* node, vector<GLfloat>* vertices, vector<GLuint>* indices ) {
	if( node->left == -1 && node->right == -1 ) {
		return;
	}

	GLfloat a[3], b[3], c[3], d[3];

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

	// if(
	// 	!( a[0] == b[0] && a[1] == b[1] && a[2] == b[2] ) &&
	// 	!( b[0] == c[0] && b[1] == c[1] && b[2] == c[2] ) &&
	// 	!( c[0] == d[0] && c[1] == d[1] && c[2] == d[2] ) &&
	// 	!( d[0] == a[0] && d[1] == a[1] && d[2] == a[2] )
	// ) {
		GLuint i = vertices->size() / 3;
		vertices->insert( vertices->end(), a, a + 3 );
		vertices->insert( vertices->end(), b, b + 3 );
		vertices->insert( vertices->end(), c, c + 3 );
		vertices->insert( vertices->end(), d, d + 3 );

		GLuint newIndices[8] = {
			i, i+1,
			i+1, i+2,
			i+2, i+3,
			i+3, i
		};
		indices->insert( indices->end(), newIndices, newIndices + 8 );
	// }

	// Proceed with left side
	if( node->left > -1 ) {
		this->visualizeNextNode( mNodes[node->left], vertices, indices );
	}

	// Proceed width right side
	if( node->right > -1 ) {
		this->visualizeNextNode( mNodes[node->right], vertices, indices );
	}
}
