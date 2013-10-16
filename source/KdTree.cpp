#include "KdTree.h"


KdTree::KdTree( std::vector<float> vertices, std::vector<unsigned int> indices ) {
	if( vertices.size() <= 0 || indices.size() <= 0 ) {
		return;
	}

	struct kdNode_t input[indices.size()];

	for( unsigned int i = 0; i < indices.size(); i += 3 ) {
		struct kdNode_t n;
		n.coord[0] = vertices[indices[i * 3]];
		n.coord[1] = vertices[indices[i * 3 + 1]];
		n.coord[2] = vertices[indices[i * 3 + 2]];
		input[i] = n;
	}

	mVisited = 0;
	mRoot = this->makeTree( input, indices.size(), 0 );
}


KdTree::~KdTree() {
	//
}


struct kdNode_t* KdTree::makeTree( struct kdNode_t* t, int len, int i ) {
	struct kdNode_t* n;

	if( len == 0 ) {
		return 0;
	}

	n = this->findMedian( t, t + len, i );

	if( n ) {
		i = ( i + 1 ) % KD_DIM;
		n->left = this->makeTree( t, n - t, i );
		n->right = this->makeTree( n + 1, t + len - ( n + 1 ), i );
	}

	return n;
}

void KdTree::swap( struct kdNode_t* x, struct kdNode_t* y ) {
	float tmp[KD_DIM];
	memcpy( tmp, x->coord, sizeof( tmp ) );
	memcpy( x->coord, y->coord, sizeof( tmp ) );
	memcpy( y->coord, tmp, sizeof( tmp ) );
}


struct kdNode_t* KdTree::findMedian( struct kdNode_t* start, struct kdNode_t* end, int index ) {
	if( end <= start ) {
		return NULL;
	}
	if( end == start + 1 ) {
		return start;
	}

	struct kdNode_t* p;
	struct kdNode_t* store;
	struct kdNode_t* md = start + ( end - start ) / 2;
	float pivot;

	while( 1 ) {
		pivot = md->coord[index];
		this->swap( md, end - 1 );

		for( store = p = start; p < end; p++ ) {
			if( p->coord[index] < pivot ) {
				if( p != store ) {
					this->swap( p, store );
				}
				store++;
			}
		}

		this->swap( store, end - 1 );

		if( store->coord[index] == md->coord[index] ) {
			return md;
		}
		if( store > md ) {
			end = store;
		}
		else {
			start = store;
		}
	}
}


float KdTree::distance( struct kdNode_t* a, struct kdNode_t* b ) {
	float t;
	float d = 0.0f;
	int dim = KD_DIM;

	while( dim-- ) {
		t = a->coord[dim] - b->coord[dim];
		d += t * t;
	}

	return d;
}


void KdTree::nearest( struct kdNode_t* root, struct kdNode_t* nd, int i, struct kdNode_t** best, float* bestDist ) {
	float d, dx, dx2;

	if( !root ) {
		return;
	}

	d = this->distance( root, nd );
	dx = root->coord[i] - nd->coord[i];
	dx2 = dx * dx;

	mVisited++;

	if( !*best || d < *bestDist ) {
		*bestDist = d;
		*best = root;
	}
	if( !*bestDist ) {
		return;
	}
	if( ++i >= KD_DIM ) {
		i = 0;
	}

	struct kdNode_t* next;

	next = ( dx > 0 ) ? root->left : root->right;
	this->nearest( next, nd, i, best, bestDist );

	if( dx2 >= *bestDist ) {
		return;
	}

	next = ( dx > 0 ) ? root->right : root->left;
	this->nearest( next, nd, i, best, bestDist );
}
