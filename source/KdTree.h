#ifndef KD_TREE_H
#define KD_TREE_H

#define KD_DIM 3

#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <CL/cl.hpp>
#include <glm/glm.hpp>
#include <map>
#include <math.h>
#include <stdlib.h>
#include <vector>

#include "Logger.h"

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
		KdTree( vector<cl_float> vertices, vector<cl_uint> faces, cl_float* bbMin, cl_float* bbMax );
		~KdTree();

		kdNode_t* getRootNode();
		vector<kdNode_t> getNodes();
		bool hitBoundingBox( glm::vec3 bbMin, glm::vec3 bbMax, glm::vec3 origin, glm::vec3 dir );
		bool hitTriangle( glm::vec3 vStart, glm::vec3 vEnd, glm::vec3 a, glm::vec3 b, glm::vec3 c );
		void print();
		void visualize( vector<cl_float>* vertices, vector<cl_uint>* indices );

	protected:
		void assignFacesToLeaves( vector<cl_float>* vertices, vector<cl_uint>* faces );
		kdNode_t* createLeafNode( cl_float* bbMin, cl_float* bbMax );
		void createRopes( kdNode_t* node, vector<cl_int> ropes );
		vector<kdNode_t*> createUnconnectedNodes( vector<cl_float>* vertices );
		kdNode_t* findMedian( vector<kdNode_t*>* nodes, cl_int axis );
		cl_int makeTree( vector<kdNode_t*> t, cl_int axis, cl_float* bbMin, cl_float* bbMax, cl_uint depth );
		void optimizeRope( cl_int* rope, cl_int side, cl_float* bbMin, cl_float* bbMax );
		void printNode( kdNode_t* node );
		void printNumFacesOfLeaves();
		void visualizeNextNode( kdNode_t* node, vector<cl_float>* vertices, vector<cl_uint>* indices );

	private:
		static bool compFunc0( kdNode_t* a, kdNode_t* b );
		static bool compFunc1( kdNode_t* a, kdNode_t* b );
		static bool compFunc2( kdNode_t* a, kdNode_t* b );

		kdNode_t* mRoot;
		vector<kdNode_t*> mNodes;
		vector<kdNode_t*> mLeaves;
		cl_uint mDepthLimit;
		cl_int mLeafIndex;

};

#endif
