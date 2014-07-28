#ifndef KD_TREE_H
#define KD_TREE_H

#define KD_EPSILON 0.000001f

#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <CL/cl.hpp>
#include <glm/glm.hpp>
#include <map>
#include <math.h>
#include <stdlib.h>
#include <vector>

#include "AccelStructure.h"
#include "Logger.h"

using std::map;
using std::vector;


/**
 * Node of the kd-tree, used for non-leaves and leaves alike.
 */
struct kdNode_t {
	vector<cl_uint4> faces;
	vector<kdNode_t*> ropes;
	glm::vec3 pos;
	glm::vec3 bbMin;
	glm::vec3 bbMax;
	cl_int axis;
	cl_int index;
	kdNode_t* left;
	kdNode_t* right;
};


/**
 * Comparison object for sorting algorithm.
 */
struct kdSortFloat4 {
	cl_int axis;

	kdSortFloat4( cl_int axis ) : axis( axis ) {};

	bool operator () ( cl_float4 a, cl_float4 b ) const {
		return ( ( (cl_float*) &a )[axis] < ( (cl_float*) &b )[axis] );
	}
};


/**
 * Comparison object for searching algorithm.
 */
struct kdSearchFloat4 {
	cl_float4 v;

	kdSearchFloat4( cl_float4 v ) : v( v ) {};

	bool operator () ( cl_float4 cmp ) const {
		return (
			v.x == cmp.x &&
			v.y == cmp.y &&
			v.z == cmp.z
		);
	}
};


class KdTree : public AccelStructure {

	public:
		KdTree( vector<cl_float> vertices, vector<cl_uint4> faces, glm::vec3 bbMin, glm::vec3 bbMax );
		~KdTree();

		glm::vec3 getBoundingBoxMax();
		glm::vec3 getBoundingBoxMin();
		kdNode_t* getRootNode();
		vector<kdNode_t> getLeaves();
		vector<kdNode_t> getNodes();
		vector<kdNode_t> getNonLeaves();
		static vector<cl_float4> verticesToFloat4( vector<cl_float> vertices );
		void visualize( vector<cl_float>* vertices, vector<cl_uint>* indices );

	protected:
		kdNode_t* createLeafNode(
			glm::vec3 bbMin, glm::vec3 bbMax,
			vector<cl_float> vertices, vector<cl_uint4> faces
		);
		void createRopes( kdNode_t* node, vector<kdNode_t*> ropes );
		kdNode_t* findMedian(
			vector<cl_float4> nodes, cl_int axis, vector<cl_float> splits
		);
		vector<cl_float> getFaceBB( cl_float v0[3], cl_float v1[3], cl_float v2[3] );
		bool hasSameCoordinates( cl_float* a, cl_float* b ) const;
		bool isVertexOnLeft(
			cl_float* v, cl_uint axis, glm::vec3 medianPos,
			vector<cl_float4> vertsForNodes, vector<cl_float4>* leftNodes
		) const;
		bool isVertexOnRight(
			cl_float* v, cl_uint axis, glm::vec3 medianPos,
			vector<cl_float4> vertsForNodes, vector<cl_float4>* rightNodes
		) const;
		kdNode_t* makeTree(
			vector<cl_float4> t, cl_int axis, glm::vec3 bbMin, glm::vec3 bbMax,
			vector<cl_float> vertices, vector<cl_uint4> faces,
			vector< vector<cl_float> > splitsByAxis, cl_uint depth
		);
		void optimizeRope( vector<kdNode_t*>* ropes, glm::vec3 bbMin, glm::vec3 bbMax );
		void printLeafFacesStat();
		void setDepthLimit( vector<cl_float> vertices );
		void splitVerticesAndFacesAtMedian(
			cl_uint axis, glm::vec3 medianPos, vector<cl_float4> vertsForNodes,
			vector<cl_float> vertices, vector<cl_uint4> faces,
			vector<cl_uint4>* leftFaces, vector<cl_uint4>* rightFaces,
			vector<cl_float4>* leftNodes, vector<cl_float4>* rightNodes
		);
		void visualizeNextNode( kdNode_t* node, vector<cl_float>* vertices, vector<cl_uint>* indices );

	private:
		vector<kdNode_t*> mLeaves;
		vector<kdNode_t*> mNodes;
		vector<kdNode_t*> mNonLeaves;
		glm::vec3 mBBmax;
		glm::vec3 mBBmin;
		kdNode_t* mRoot;
		cl_uint mDepthLimit;
		cl_uint mMinFaces;

};

#endif
