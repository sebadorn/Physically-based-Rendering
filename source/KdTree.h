#ifndef KD_TREE_H
#define KD_TREE_H

#define KD_DIM 3

#include <algorithm>
#include <map>
#include <math.h>
#include <stdlib.h>
#include <vector>

#include "Logger.h"


typedef struct kdNode_t {
	float coord[3];
	kdNode_t* left;
	kdNode_t* right;
} kdNode_t;


class KdTree {

	public:
		KdTree( std::vector<float> vertices, std::vector<unsigned int> indices );
		~KdTree();
		void print();

	protected:
		// float distance( kdNode_t* a, kdNode_t* b );
		kdNode_t* findMedian( std::vector<kdNode_t*>* nodes, int axis );
		kdNode_t* makeTree( std::vector<kdNode_t*> t, int axis );
		// void nearest( kdNode_t* root, kdNode_t* nd, int i, kdNode_t** best, float* bestDist );
		void printNode( kdNode_t* node );

	private:
		static bool compFunc0( kdNode_t* a, kdNode_t* b );
		static bool compFunc1( kdNode_t* a, kdNode_t* b );
		static bool compFunc2( kdNode_t* a, kdNode_t* b );

		kdNode_t* mRoot;
		std::vector<kdNode_t*> mNodes;
		int mVisited;

};

#endif
