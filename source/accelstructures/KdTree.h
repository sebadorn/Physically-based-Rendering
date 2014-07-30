#ifndef KD_TREE_H
#define KD_TREE_H

#define KD_EPSILON 0.000001f

#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <map>
#include <math.h>
#include <stdlib.h>

#include "AccelStructure.h"
#include "../Logger.h"

using std::map;
using std::vector;


/**
 * Node of the kd-tree, used for non-leaves and leaves alike.
 */
struct kdNode_t {
	vector<Tri> faces;
	vector<kdNode_t*> ropes;
	glm::vec3 pos;
	glm::vec3 bbMin;
	glm::vec3 bbMax;
	cl_int axis;
	cl_int index;
	kdNode_t* left;
	kdNode_t* right;
};


class KdTree : public AccelStructure {

	public:
		KdTree( vector<Tri> faces );
		~KdTree();

		glm::vec3 getBoundingBoxMax();
		glm::vec3 getBoundingBoxMin();
		kdNode_t* getRootNode();
		vector<kdNode_t> getLeaves();
		vector<kdNode_t> getNodes();
		vector<kdNode_t> getNonLeaves();
		void visualize( vector<cl_float>* vertices, vector<cl_uint>* indices );

	protected:
		kdNode_t* createLeafNode(
			glm::vec3 bbMin, glm::vec3 bbMax,
			vector<cl_float> vertices, vector<cl_uint4> faces
		);
		void createRopes( kdNode_t* node, vector<kdNode_t*> ropes );
		kdNode_t* getSplit(
			vector<Tri>* faces, short axis,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces
		);
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
			vector<Tri> faces, glm::vec3 bbMin, glm::vec3 bbMax, short axis, uint depth
		);
		void optimizeRope( vector<kdNode_t*>* ropes, glm::vec3 bbMin, glm::vec3 bbMax );
		void printLeafFacesStat();
		void setDepthLimit( uint numFaces );
		void splitVerticesAndFacesAtMedian(
			cl_uint axis, glm::vec3 medianPos, vector<cl_float4> vertsForNodes,
			vector<cl_float> vertices, vector<Tri> faces,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces,
			vector<cl_float4>* leftNodes, vector<cl_float4>* rightNodes
		);
		void visualizeNextNode(
			kdNode_t* node, vector<cl_float>* vertices, vector<cl_uint>* indices
		);

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
