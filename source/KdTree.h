#ifndef KD_TREE_H
#define KD_TREE_H

#define KD_DIM 3

#include <algorithm>
#include <map>
#include <math.h>
#include <stdlib.h>
#include <vector>

#include "KdNode.h"
#include "Logger.h"


class KdTree {

	public:
		KdTree( std::vector<float> vertices, std::vector<unsigned int> indices );
		~KdTree();
		void print();

	protected:
		// float distance( KdNode* a, KdNode* b );
		KdNode* findMedian( std::vector<KdNode*>* nodes, int axis );
		KdNode* makeTree( std::vector<KdNode*> t, int axis );
		// void nearest( KdNode* root, KdNode* nd, int i, KdNode** best, float* bestDist );
		void printNode( KdNode* node );

	private:
		static bool compFunc0( KdNode* a, KdNode* b );
		static bool compFunc1( KdNode* a, KdNode* b );
		static bool compFunc2( KdNode* a, KdNode* b );
		KdNode* mRoot;
		int mVisited;

};

#endif
