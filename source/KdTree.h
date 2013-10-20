#ifndef KD_TREE_H
#define KD_TREE_H

#define KD_DIM 3

#include <algorithm>
#include <map>
#include <math.h>
#include <stdlib.h>
#include <vector>

#include "Logger.h"

using namespace std;


typedef struct kdNode_t {
	float coord[3];
	kdNode_t* left;
	kdNode_t* right;
} kdNode_t;


class KdTree {

	public:
		KdTree( vector<float> vertices, vector<unsigned int> indices );
		~KdTree();
		void print();
		void visualize( float* bbMin, float* bbMax, vector<float>* vertices, vector<unsigned int>* indices );

	protected:
		float distance( kdNode_t* a, kdNode_t* b );
		kdNode_t* findMedian( vector<kdNode_t*>* nodes, int axis );
		kdNode_t* makeTree( vector<kdNode_t*> t, int axis );
		void nearest( kdNode_t* input, kdNode_t* currentNode, int axis, kdNode_t** bestNode, float* bestDist );
		void printNode( kdNode_t* node );
		void visualizeNextNode( kdNode_t* node, float* bbMin, float* bbMax, vector<float>* vertices, vector<unsigned int>* indices, unsigned int axis );

	private:
		static bool compFunc0( kdNode_t* a, kdNode_t* b );
		static bool compFunc1( kdNode_t* a, kdNode_t* b );
		static bool compFunc2( kdNode_t* a, kdNode_t* b );

		kdNode_t* mRoot;
		vector<kdNode_t*> mNodes;

};

#endif
