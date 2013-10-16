#ifndef KD_TREE_H
#define KD_TREE_H

#define KD_DIM 3

#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>


struct kdNode_t {
	float coord[KD_DIM];
	struct kdNode_t* left;
	struct kdNode_t* right;
};


class KdTree {

	public:
		KdTree( std::vector<float> vertices, std::vector<unsigned int> indices );
		~KdTree();

	protected:
		float distance( struct kdNode_t* a, struct kdNode_t* b );
		struct kdNode_t* findMedian( struct kdNode_t* start, struct kdNode_t* end, int index );
		struct kdNode_t* makeTree( struct kdNode_t* t, int len, int i );
		void nearest( struct kdNode_t* root, struct kdNode_t* nd, int i, struct kdNode_t** best, float* bestDist );
		void swap( struct kdNode_t* x, struct kdNode_t* y );

	private:
		struct kdNode_t* mRoot;
		int mVisited;

};

#endif
