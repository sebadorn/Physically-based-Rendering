#include "KdTree.h"


KdTree::KdTree( std::vector<float> vertices, std::vector<unsigned int> indices ) {
	if( vertices.size() <= 0 || indices.size() <= 0 ) {
		return;
	}

	std::vector<kdNode_t*> mNodes;

	for( unsigned int i = 0; i < indices.size(); i += 3 ) {
		kdNode_t* node = new kdNode_t;
		node->coord[0] = vertices[indices[i]];
		node->coord[1] = vertices[indices[i + 1]];
		node->coord[2] = vertices[indices[i + 2]];
		node->left = NULL;
		node->right = NULL;

		mNodes.push_back( node );
	}

	mVisited = 0;
	mRoot = this->makeTree( mNodes, 0 );
}


KdTree::~KdTree() {
	for( unsigned int i = 0; i < mNodes.size(); i++ ) {
		delete mNodes[i];
	}
}


bool KdTree::compFunc0( kdNode_t* a, kdNode_t* b ) {
	return ( a->coord[0] < b->coord[0] );
}


bool KdTree::compFunc1( kdNode_t* a, kdNode_t* b ) {
	return ( a->coord[1] < b->coord[1] );
}


bool KdTree::compFunc2( kdNode_t* a, kdNode_t* b ) {
	return ( a->coord[2] < b->coord[2] );
}


kdNode_t* KdTree::findMedian( std::vector<kdNode_t*>* nodes, int axis ) {
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


kdNode_t* KdTree::makeTree( std::vector<kdNode_t*> nodes, int axis ) {
	if( nodes.size() == 0 ) {
		return NULL;
	}

	kdNode_t* median = this->findMedian( &nodes, axis );

	if( median == NULL ) {
		return median;
	}

	std::vector<kdNode_t*> left;
	std::vector<kdNode_t*> right;
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
		median->left = ( left.size() == 0 ) ? NULL : left[0];
		median->right = ( right.size() == 0 ) ? NULL : right[0];
	}
	else {
		axis = ( axis + 1 ) % KD_DIM;
		median->left = this->makeTree( left, axis );
		median->right = this->makeTree( right, axis );
	}

	return median;
}


void KdTree::print() {
	if( mRoot == NULL ) {
		printf( "Tree is empty.\n" );
		return;
	}

	this->printNode( mRoot );
}


void KdTree::printNode( kdNode_t* node ) {
	if( node == NULL ) {
		printf( "NULL\n" );
		return;
	}

	printf( "(%g %g %g) ", node->coord[0], node->coord[1], node->coord[2] );
	printf( "l" ); this->printNode( node->left );
	printf( "r" ); this->printNode( node->right );
}


// float KdTree::distance( kdNode_t* a, kdNode_t* b ) {
// 	float t;
// 	float d = 0.0f;
// 	int dim = KD_DIM;

// 	while( dim-- ) {
// 		t = a->coord[dim] - b->coord[dim];
// 		d += t * t;
// 	}

// 	return d;
// }


// void KdTree::nearest( kdNode_t* root, kdNode_t* nd, int i, kdNode_t** best, float* bestDist ) {
// 	float d, dx, dx2;

// 	if( !root ) {
// 		return;
// 	}

// 	d = this->distance( root, nd );
// 	dx = root->coord[i] - nd->coord[i];
// 	dx2 = dx * dx;

// 	mVisited++;

// 	if( !*best || d < *bestDist ) {
// 		*bestDist = d;
// 		*best = root;
// 	}
// 	if( !*bestDist ) {
// 		return;
// 	}
// 	if( ++i >= KD_DIM ) {
// 		i = 0;
// 	}

// 	kdNode_t* next;

// 	next = ( dx > 0 ) ? root->left : root->right;
// 	this->nearest( next, nd, i, best, bestDist );

// 	if( dx2 >= *bestDist ) {
// 		return;
// 	}

// 	next = ( dx > 0 ) ? root->right : root->left;
// 	this->nearest( next, nd, i, best, bestDist );
// }
