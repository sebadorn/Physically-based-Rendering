#ifndef KD_TREE_H
#define KD_TREE_H

#define KD_DIM 3

#include <algorithm>
#include <CL/cl.hpp>
#include <GL/gl.h>
#include <map>
#include <math.h>
#include <stdlib.h>
#include <vector>

#include "Logger.h"
#include "utils.h"

using std::vector;


typedef struct kdNode_t {
	vector<cl_int> faces;
	vector<cl_int> ropes;
	cl_float pos[3];
	cl_float bbMax[3];
	cl_float bbMin[3];
	cl_int axis;
	cl_int index;
	cl_int left;
	cl_int right;
} kdNode_t;


class KdTree {

	public:
		KdTree( vector<cl_float> vertices, vector<cl_uint> indices, cl_float* bbMin, cl_float* bbMax );
		~KdTree();

		kdNode_t* getRootNode();
		vector<kdNode_t> getNodes();
		void print();
		void visualize( vector<cl_float>* vertices, vector<cl_uint>* indices );

	protected:
		kdNode_t* findMedian( vector<kdNode_t*>* nodes, cl_int axis );
		cl_int makeTree( vector<kdNode_t*> t, cl_int axis, cl_float* bbMin, cl_float* bbMax );
		void optimizeRope( cl_int* rope, cl_int side, cl_float* bbMin, cl_float* bbMax );
		void printNode( kdNode_t* node );
		void processNode( kdNode_t* node, vector<cl_int> ropes );
		void visualizeNextNode( kdNode_t* node, vector<GLfloat>* vertices, vector<GLuint>* indices );

	private:
		static bool compFunc0( kdNode_t* a, kdNode_t* b );
		static bool compFunc1( kdNode_t* a, kdNode_t* b );
		static bool compFunc2( kdNode_t* a, kdNode_t* b );

		kdNode_t* mRoot;
		vector<kdNode_t*> mNodes;
		vector<kdNode_t*> mLeaves;
		cl_int mLeafIndex;

};

#endif
