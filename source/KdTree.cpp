#include "KdTree.h"

using namespace std;


/**
 * Constructor.
 * @param {std::vector<float>}        vertices Vertices of the model.
 * @param {std::vector<unsigned int>} indices  Indices describing the faces (triangles) of the model.
 */
KdTree::KdTree( vector<float> vertices, vector<unsigned int> indices, cl_float* bbMin, cl_float* bbMax ) {
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


	// Generate kd-tree
	int rootIndex = this->makeTree( mNodes, 0, bbMin, bbMax );
	mRoot = mNodes[rootIndex];


	// Get all the leave nodes
	vector<kdNode_t*> leaves;

	for( i = 0; i < mNodes.size(); i++ ) {
		if( mNodes[i]->left == -1 && mNodes[i]->right == -1 ) {
			leaves.push_back( mNodes[i] );
		}
	}


	// Associate faces with leave nodes
	for( i = 0; i < indices.size(); i += 3 ) {
		float a[3] = {
			vertices[indices[i] * 3],
			vertices[indices[i] * 3 + 1],
			vertices[indices[i] * 3 + 2]
		};
		float b[3] = {
			vertices[indices[i + 1] * 3],
			vertices[indices[i + 1] * 3 + 1],
			vertices[indices[i + 1] * 3 + 2]
		};
		float c[3] = {
			vertices[indices[i + 2] * 3],
			vertices[indices[i + 2] * 3 + 1],
			vertices[indices[i + 2] * 3 + 2]
		};
		float abDir[3] = {
			b[0] - a[0],
			b[1] - a[1],
			b[2] - a[2]
		};
		float bcDir[3] = {
			c[0] - b[0],
			c[1] - b[1],
			c[2] - b[2]
		};
		float caDir[3] = {
			a[0] - c[0],
			a[1] - c[1],
			a[2] - c[2]
		};
		float hit[3];

		for( j = 0; j < leaves.size(); j++ ) {
			kdNode_t* l = leaves[j];

			if( find( l->faces.begin(), l->faces.end(), i ) == l->faces.end() ) {
				bool add = false;
				float distanceLimit, distanceHit;

				if( utils::hitBoundingBox( l->bbMin, l->bbMax, a, abDir, hit ) ) {
					float d[3] = { hit[0] - a[0], hit[1] - a[1], hit[2] - a[2] };
					distanceLimit = sqrt( abDir[0] * abDir[0] + abDir[1] * abDir[1] + abDir[2] * abDir[2] );
					distanceHit = sqrt( d[0] * d[0] + d[1] * d[1] + d[2] * d[2] );

					if( distanceHit <= distanceLimit ) {
						add = true;
					}
				}
				if( !add && utils::hitBoundingBox( l->bbMin, l->bbMax, b, bcDir, hit ) ) {
					float d[3] = { hit[0] - b[0], hit[1] - b[1], hit[2] - b[2] };
					distanceLimit = sqrt( bcDir[0] * bcDir[0] + bcDir[1] * bcDir[1] + bcDir[2] * bcDir[2] );
					distanceHit = sqrt( d[0] * d[0] + d[1] * d[1] + d[2] * d[2] );

					if( distanceHit <= distanceLimit ) {
						add = true;
					}
				}
				if( !add && utils::hitBoundingBox( l->bbMin, l->bbMax, c, caDir, hit ) ) {
					float d[3] = { hit[0] - c[0], hit[1] - c[1], hit[2] - c[2] };
					distanceLimit = sqrt( caDir[0] * caDir[0] + caDir[1] * caDir[1] + caDir[2] * caDir[2] );
					distanceHit = sqrt( d[0] * d[0] + d[1] * d[1] + d[2] * d[2] );

					if( distanceHit <= distanceLimit ) {
						add = true;
					}
				}

				if( add ) {
					l->faces.push_back( i );
				}
			}
		}
	}

	// for( i = 0; i < leaves.size(); i++ ) {
	// 	cout << leaves[i]->index << ": " << leaves[i]->faces.size() << endl;
	// }
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
kdNode_t* KdTree::findMedian( vector<kdNode_t*>* nodes, int axis ) {
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

	int index = floor( nodes->size() / 2.0f );
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
int KdTree::makeTree( vector<kdNode_t*> nodes, int axis, cl_float* bbMin, cl_float* bbMax ) {
	if( nodes.size() == 0 ) {
		return -1;
	}

	kdNode_t* median = this->findMedian( &nodes, axis );
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
	bbMaxLeft[axis] = median->pos[axis];

	// Bounding box of the "right" part
	cl_float bbMinRight[3] = { bbMin[0], bbMin[1], bbMin[2] };
	cl_float bbMaxRight[3] = { bbMax[0], bbMax[1], bbMax[2] };
	bbMinRight[axis] = median->pos[axis];

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
	if( node->index == -1 ) {
		printf( "END\n" );
		return;
	}

	printf( "(%g %g %g) ", node->pos[0], node->pos[1], node->pos[2] );
	printf( "l" ); this->printNode( mNodes[node->left] );
	printf( "r" ); this->printNode( mNodes[node->right] );
}


/**
 * Get vertices and indices to draw a 3D visualization of the kd-tree.
 * @param {float*}                     bbMin    Bounding box minimum coordinates.
 * @param {float*}                     bbMax    Bounding box maximum coordinates.
 * @param {std::vector<float>*}        vertices Vector to put the vertices into.
 * @param {std::vector<unsigned int>*} indices  Vector to put the indices into.
 */
void KdTree::visualize( vector<float>* vertices, vector<unsigned int>* indices ) {
	this->visualizeNextNode( mRoot, 0, vertices, indices );
}


/**
 * Visualize the next node in the kd-tree.
 * @param {kdNode_t*}                  node     Current node.
 * @param {float*}                     bbMin    Bounding box minimum coordinates.
 * @param {float*}                     bbMax    Bounding box maximum coordinates.
 * @param {std::vector<float>*}        vertices Vector to put the vertices into.
 * @param {std::vector<unsigned int>*} indices  Vector to put the indices into.
 * @param {unsigned int}               axis     Current axis.
 */
void KdTree::visualizeNextNode(
	kdNode_t* node, unsigned int axis,
	vector<float>* vertices, vector<unsigned int>* indices
) {
	if( node->index == -1 ) {
		return;
	}

	float a[3], b[3], c[3], d[3];

	a[axis] = b[axis] = c[axis] = d[axis] = node->pos[axis];

	switch( axis ) {

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

	unsigned int i = vertices->size() / 3;
	vertices->insert( vertices->end(), a, a + 3 );
	vertices->insert( vertices->end(), b, b + 3 );
	vertices->insert( vertices->end(), c, c + 3 );
	vertices->insert( vertices->end(), d, d + 3 );

	unsigned int newIndices[8] = {
		i, i+1,
		i+1, i+2,
		i+2, i+3,
		i+3, i
	};
	indices->insert( indices->end(), newIndices, newIndices + 8 );

	axis = ( axis + 1 ) % KD_DIM;

	// Proceed with left side
	if( node->left > -1 ) {
		this->visualizeNextNode(
			mNodes[node->left], axis,
			vertices, indices
		);
	}

	// Proceed width right side
	if( node->right > -1 ) {
		this->visualizeNextNode(
			mNodes[node->right], axis,
			vertices, indices
		);
	}
}
