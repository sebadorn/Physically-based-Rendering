#include "KdTree.h"

using namespace std;


/**
 * Constructor.
 * @param {std::vector<float>}        vertices Vertices of the model.
 * @param {std::vector<unsigned int>} indices  Indices describing the faces (triangles) of the model.
 */
KdTree::KdTree( vector<float> vertices, vector<unsigned int> indices ) {
	if( vertices.size() <= 0 || indices.size() <= 0 ) {
		return;
	}

	for( unsigned int i = 0; i < indices.size(); i += 3 ) {
		kdNode_t* node = new kdNode_t;
		node->index = mNodes.size();
		node->vertIndex = indices[i];
		node->coord[0] = vertices[indices[i]];
		node->coord[1] = vertices[indices[i + 1]];
		node->coord[2] = vertices[indices[i + 2]];
		node->left = -1;
		node->right = -1;

		unsigned int nSize = mNodes.size();

		if( nSize >= 3 && nSize % 3 == 0 ) {
			mNodes[nSize - 3]->face0 = mNodes[nSize - 2]->vertIndex;
			mNodes[nSize - 3]->face1 = mNodes[nSize - 1]->vertIndex;

			mNodes[nSize - 2]->face0 = mNodes[nSize - 3]->vertIndex;
			mNodes[nSize - 2]->face1 = mNodes[nSize - 1]->vertIndex;

			mNodes[nSize - 1]->face0 = mNodes[nSize - 3]->vertIndex;
			mNodes[nSize - 1]->face1 = mNodes[nSize - 2]->vertIndex;
		}

		mNodes.push_back( node );
	}

	mRoot = mNodes[this->makeTree( mNodes, 0 )];
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
	return ( a->coord[0] < b->coord[0] );
}


/**
 * Comparison function for sorting algorithm. Compares the Y axis.
 * @param  {kdNode_t*} a Object one.
 * @param  {kdNode_t*} b Object two.
 * @return {bool}        True if a < b, false otherwise.
 */
bool KdTree::compFunc1( kdNode_t* a, kdNode_t* b ) {
	return ( a->coord[1] < b->coord[1] );
}


/**
 * Comparison function for sorting algorithm. Compares the Z axis.
 * @param  {kdNode_t*} a Object one.
 * @param  {kdNode_t*} b Object two.
 * @return {bool}        True if a < b, false otherwise.
 */
bool KdTree::compFunc2( kdNode_t* a, kdNode_t* b ) {
	return ( a->coord[2] < b->coord[2] );
}


/**
 * Find the median object of the given nodes.
 * @param  {std::vector<kdNode_t*>*} nodes Pointer to the current list of nodes to find the median of.
 * @param  {int}                     axis  Index of the axis to compare.
 * @return {kdNode_t*}                     The object that is the median.
 */
kdNode_t* KdTree::findMedian( vector<kdNode_t*>* nodes, int axis ) {
	if( nodes->size() == 0 ) {
		return NULL;
	}
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

	int index = round( nodes->size() / 2 ) - 1;
	kdNode_t* median = (*nodes)[index];

	return median;
}


vector<kdNode_t> KdTree::getNodes() {
	vector<kdNode_t> nodes;

	for( unsigned int i = 0; i < mNodes.size(); i++ ) {
		nodes.push_back( *mNodes[i] );
	}

	return nodes;
}


kdNode_t* KdTree::getRootNode() {
	return mRoot;
}


/**
 * Build a kd-tree.
 * @param  {std::vector<kdNode_t*>} nodes List of nodes to build the tree from.
 * @param  {int}                    axis  Axis to use as criterium.
 * @return {int}                          Top element for this part of the tree.
 */
int KdTree::makeTree( vector<kdNode_t*> nodes, int axis ) {
	if( nodes.size() == 0 ) {
		return -1;
	}

	kdNode_t* median = this->findMedian( &nodes, axis );

	if( median == NULL ) {
		return -1;
	}

	vector<kdNode_t*> left;
	vector<kdNode_t*> right;
	bool leftSide = true;

	for( int i = 0; i < nodes.size(); i++ ) {
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

	if( nodes.size() == 2 ) {
		median->left = ( left.size() == 0 ) ? -1 : left[0]->index;
		median->right = ( right.size() == 0 ) ? -1 : right[0]->index;
	}
	else {
		axis = ( axis + 1 ) % KD_DIM;
		median->left = this->makeTree( left, axis );
		median->right = this->makeTree( right, axis );
	}

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
	if( node == NULL ) {
		printf( "NULL\n" );
		return;
	}

	printf( "(%g %g %g) ", node->coord[0], node->coord[1], node->coord[2] );
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
void KdTree::visualize( float* bbMin, float* bbMax, vector<float>* vertices, vector<unsigned int>* indices ) {
	this->visualizeNextNode( mRoot, bbMin, bbMax, vertices, indices, 0 );
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
void KdTree::visualizeNextNode( kdNode_t* node, float* bbMin, float* bbMax, vector<float>* vertices, vector<unsigned int>* indices, unsigned int axis ) {
	if( node == NULL ) {
		return;
	}

	float a[3], b[3], c[3], d[3];

	float bbMinLeft[3] = { bbMin[0], bbMin[1], bbMin[2] };
	float bbMaxLeft[3] = { bbMax[0], bbMax[1], bbMax[2] };

	float bbMinRight[3] = { bbMin[0], bbMin[1], bbMin[2] };
	float bbMaxRight[3] = { bbMax[0], bbMax[1], bbMax[2] };


	switch( axis ) {

		case 0: // x
			a[0] = b[0] = c[0] = d[0] = node->coord[0];
			a[1] = d[1] = bbMin[1];
			a[2] = b[2] = bbMin[2];
			b[1] = c[1] = bbMax[1];
			c[2] = d[2] = bbMax[2];

			bbMaxLeft[0] = node->coord[0];
			bbMinRight[0] = node->coord[0];
			break;

		case 1: // y
			a[1] = b[1] = c[1] = d[1] = node->coord[1];
			a[0] = d[0] = bbMin[0];
			a[2] = b[2] = bbMin[2];
			b[0] = c[0] = bbMax[0];
			c[2] = d[2] = bbMax[2];

			bbMaxLeft[1] = node->coord[1];
			bbMinRight[1] = node->coord[1];
			break;

		case 2: // z
			a[2] = b[2] = c[2] = d[2] = node->coord[2];
			a[1] = d[1] = bbMin[1];
			a[0] = b[0] = bbMin[0];
			b[1] = c[1] = bbMax[1];
			c[0] = d[0] = bbMax[0];

			bbMaxLeft[2] = node->coord[2];
			bbMinRight[2] = node->coord[2];
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
		this->visualizeNextNode( mNodes[node->left], bbMinLeft, bbMaxLeft, vertices, indices, axis );
	}

	// Proceed width right side
	if( node->right > -1 ) {
		this->visualizeNextNode( mNodes[node->right], bbMinRight, bbMaxRight, vertices, indices, axis );
	}
}


/**
 * Calculate the distance between two nodes.
 * @param  {kdNode_t*} a A node with coordinates.
 * @param  {kdNode_t*} b A node with coordinates.
 * @return {float}       Distance between node a and b.
 */
float KdTree::distance( kdNode_t* a, kdNode_t* b ) {
	float t;
	float d = 0.0f;
	int axis = KD_DIM;

	while( axis-- ) {
		t = a->coord[axis] - b->coord[axis];
		d += t * t;
	}

	return d;
}


void KdTree::nearest( kdNode_t* input, kdNode_t* currentNode, int axis, kdNode_t** bestNode, float* bestDist ) {
	float d, dx, dx2;

	if( !currentNode ) {
		return;
	}

	d = this->distance( currentNode, input );
	dx = currentNode->coord[axis] - input->coord[axis];

	if( !*bestNode || d < *bestDist ) {
		*bestDist = d;
		*bestNode = currentNode;
	}
	if( !*bestDist ) {
		return;
	}

	axis = ( axis + 1 ) % KD_DIM;
	unsigned int next;

	next = ( dx > 0 ) ? currentNode->left : currentNode->right;
	this->nearest( input, mNodes[next], axis, bestNode, bestDist );

	if( dx * dx >= *bestDist ) {
		return;
	}

	next = ( dx > 0 ) ? currentNode->right : currentNode->left;
	this->nearest( input, mNodes[next], axis, bestNode, bestDist );
}
