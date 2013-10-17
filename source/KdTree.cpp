#include "KdTree.h"


KdTree::KdTree( std::vector<float> vertices, std::vector<unsigned int> indices ) {
	if( vertices.size() <= 0 || indices.size() <= 0 ) {
		return;
	}

	std::vector<KdNode*> input;

	for( unsigned int i = 0; i < indices.size(); i += 3 ) {
		KdNode* node = new KdNode(
			vertices[indices[i]],
			vertices[indices[i + 1]],
			vertices[indices[i + 2]]
		);
		input.push_back( node );
	}

	mVisited = 0;
	mRoot = this->makeTree( input, 0 );
}


KdTree::~KdTree() {
	//
}


bool KdTree::compFunc0( KdNode* a, KdNode* b ) {
	return ( a->mCoord[0] < b->mCoord[0] );
}


bool KdTree::compFunc1( KdNode* a, KdNode* b ) {
	return ( a->mCoord[1] < b->mCoord[1] );
}


bool KdTree::compFunc2( KdNode* a, KdNode* b ) {
	return ( a->mCoord[2] < b->mCoord[2] );
}


KdNode* KdTree::findMedian( std::vector<KdNode*>* nodes, int axis ) {
	if( nodes->size() == 0 ) {
		return NULL;
	}
	if( nodes->size() == 1 ) {
		return (*nodes)[0];
	}

	if( axis == 0 ) {
		std::sort( nodes->begin(), nodes->end(), this->compFunc0 );
	}
	else if( axis == 1 ) {
		std::sort( nodes->begin(), nodes->end(), this->compFunc1 );
	}
	else if( axis == 2 ) {
		std::sort( nodes->begin(), nodes->end(), this->compFunc2 );
	}
	else {
		Logger::logError( "[KdTree] Unknown index for axis." );
	}

	int index = round( nodes->size() / 2 ) - 1;
	printf( "Median index: %d | Nodes: %lu\n", index, nodes->size() );
	KdNode* median = (*nodes)[index];

	return median;
}


KdNode* KdTree::makeTree( std::vector<KdNode*> nodes, int axis ) {
	if( nodes.size() == 0 ) {
		return NULL;
	}

	KdNode* median = this->findMedian( &nodes, axis );

	if( median == NULL ) {
		return median;
	}

	std::vector<KdNode*> left;
	std::vector<KdNode*> right;
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
		median->mLeft = ( left.size() == 0 ) ? NULL : left[0];
		median->mRight = ( right.size() == 0 ) ? NULL : right[0];
	}
	else {
		axis = ( axis + 1 ) % KD_DIM;
		median->mLeft = this->makeTree( left, axis );
		median->mRight = this->makeTree( right, axis );
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


void KdTree::printNode( KdNode* node ) {
	if( node == NULL ) {
		printf( "NULL\n" );
		return;
	}

	printf( "(%g %g %g) ", node->mCoord[0], node->mCoord[1], node->mCoord[2] );
	this->printNode( node->mLeft );
	this->printNode( node->mRight );
}


// float KdTree::distance( KdNode* a, KdNode* b ) {
// 	float t;
// 	float d = 0.0f;
// 	int dim = KD_DIM;

// 	while( dim-- ) {
// 		t = a->mCoord[dim] - b->mCoord[dim];
// 		d += t * t;
// 	}

// 	return d;
// }


// void KdTree::nearest( KdNode* root, KdNode* nd, int i, KdNode** best, float* bestDist ) {
// 	float d, dx, dx2;

// 	if( !root ) {
// 		return;
// 	}

// 	d = this->distance( root, nd );
// 	dx = root->mCoord[i] - nd->mCoord[i];
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

// 	KdNode* next;

// 	next = ( dx > 0 ) ? root->mLeft : root->mRight;
// 	this->nearest( next, nd, i, best, bestDist );

// 	if( dx2 >= *bestDist ) {
// 		return;
// 	}

// 	next = ( dx > 0 ) ? root->mRight : root->mLeft;
// 	this->nearest( next, nd, i, best, bestDist );
// }
