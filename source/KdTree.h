#ifndef KD_TREE_H
#define KD_TREE_H

#define KD_EPSILON 0.000001f
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

using std::map;
using std::vector;


struct kdNode_t {
	vector<cl_uint> faces;
	vector<kdNode_t*> ropes;
	cl_float pos[3];
	cl_float bbMax[3];
	cl_float bbMin[3];
	cl_int axis;
	cl_int index;
	kdNode_t* left;
	kdNode_t* right;
};


class KdTree {

	public:
		KdTree( vector<cl_float> vertices, vector<cl_uint> faces, cl_float* bbMin, cl_float* bbMax );
		~KdTree();

		kdNode_t* getRootNode();
		vector<kdNode_t> getNodes();
		bool hitBoundingBox( glm::vec3 bbMin, glm::vec3 bbMax, glm::vec3 origin, glm::vec3 dir );
		bool hitTriangle( glm::vec3 vStart, glm::vec3 vEnd, glm::vec3 a, glm::vec3 b, glm::vec3 c );
		void visualize( vector<cl_float>* vertices, vector<cl_uint>* indices );

	protected:
		kdNode_t* createLeafNode(
			cl_float* bbMin, cl_float* bbMax,
			vector<cl_float> vertices, vector<cl_uint> faces
		);
		void createRopes( kdNode_t* node, vector<kdNode_t*> ropes );
		kdNode_t* findMedian( vector<cl_float4>* nodes, cl_int axis );
		vector<cl_float> getFaceBB( cl_float v0[3], cl_float v1[3], cl_float v2[3] );
		kdNode_t* makeTree(
			vector<cl_float4> t, cl_int axis, cl_float* bbMin, cl_float* bbMax,
			vector<cl_float> vertices, vector<cl_uint> faces,
			vector< vector<cl_float> > splitsByAxis, cl_uint depth
		);
		void optimizeRope(
			vector<kdNode_t*>* ropes, cl_int side, cl_float* bbMin, cl_float* bbMax
		);
		void printLeafFacesStat();
		void printNumFacesOfLeaves();
		void setDepthLimit( vector<cl_float> vertices );
		void splitFaces(
			vector<cl_float> vertices, vector<cl_uint> faces,
			cl_float bbMaxLeft[3], cl_float bbMinRight[3],
			vector<cl_uint>* leftFaces, vector<cl_uint>* rightFaces
		);
		void splitNodesAtMedian(
			vector<cl_float4> nodes, kdNode_t* median,
			vector<cl_float4>* leftNodes, vector<cl_float4>* rightNodes
		);
		void visualizeNextNode( kdNode_t* node, vector<cl_float>* vertices, vector<cl_uint>* indices );

	private:
		static bool compFunc0( kdNode_t* a, kdNode_t* b );
		static bool compFunc1( kdNode_t* a, kdNode_t* b );
		static bool compFunc2( kdNode_t* a, kdNode_t* b );

		kdNode_t* mRoot;
		vector<kdNode_t*> mNodes;
		vector<kdNode_t*> mNonLeaves;
		vector<kdNode_t*> mLeaves;
		cl_uint mDepthLimit;
		cl_uint mIndexCounter;
		cl_int mLeafIndex;

};

#endif
