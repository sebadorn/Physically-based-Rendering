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
#include "../MathHelp.h"

using std::map;
using std::vector;


/**
 * Node of the kd-tree, used for non-leaves and leaves alike.
 */
struct kdNode_t {
	vector<Tri> faces;
	vector<kdNode_t*> ropes;
	glm::vec3 bbMin;
	glm::vec3 bbMax;
	float split;
	int index;
	short axis;
	kdNode_t* left;
	kdNode_t* right;
};


class KdTree : public AccelStructure {

	public:
		KdTree(
			vector<cl_uint> facesV, vector<cl_uint> facesVN,
			vector<cl_float> vertices, vector<cl_float> normals
		);
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
			glm::vec3 bbMin, glm::vec3 bbMax, vector<Tri> faces
		);
		void createRopes( kdNode_t* node, vector<kdNode_t*> ropes );
		kdNode_t* getSplit(
			vector<Tri>* faces, glm::vec3 bbMinNode, glm::vec3 bbMaxNode,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces
		);
		kdNode_t* makeTree(
			vector<Tri> faces, glm::vec3 bbMin, glm::vec3 bbMax, uint depth
		);
		void optimizeRope( vector<kdNode_t*>* ropes, glm::vec3 bbMin, glm::vec3 bbMax );
		void printLeafFacesStat();
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
		cl_uint mMinFaces;

};

#endif
