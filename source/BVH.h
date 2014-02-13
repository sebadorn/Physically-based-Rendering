#ifndef BVH_H
#define BVH_H

#include <algorithm>
#include <CL/cl.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "Cfg.h"
#include "KdTree.h"
#include "Logger.h"
#include "ModelLoader.h"

using std::vector;


struct BVHnode {
	cl_uint id;
	BVHnode* left;
	BVHnode* right;
	KdTree* kdtree;
	glm::vec3 bbMin;
	glm::vec3 bbMax;
	glm::vec3 bbCenter;
};


class BVH {

	public:
		BVH( vector<object3D> objects, vector<cl_float> vertices );
		~BVH();
		vector<BVHnode*> getLeaves();
		vector<BVHnode*> getNodes();
		BVHnode* getRoot();
		void visualize( vector<cl_float>* vertices, vector<cl_uint>* indices );

	protected:
		vector<BVHnode*> createKdTrees(
			vector<object3D> objects, vector<cl_float> vertices
		);
		void buildHierarchy( vector<BVHnode*> nodes, BVHnode* parent );
		void findCornerNodes(
			vector<BVHnode*> nodes, BVHnode* parent,
			BVHnode** leftmost, BVHnode** rightmost
		);
		void groupByCorner(
			vector<BVHnode*> nodes, BVHnode* leftmost, BVHnode* rightmost,
			vector<BVHnode*>* leftGroup, vector<BVHnode*>* rightGroup
		);
		BVHnode* makeNodeFromGroup( vector<BVHnode*> group );
		void visualizeNextNode( BVHnode* node, vector<cl_float>* vertices, vector<cl_uint>* indices );

	private:
		vector<BVHnode*> mBVHnodes;
		vector<BVHnode*> mBVleaves;
		BVHnode* mRoot;
		cl_uint mCounterID;

};

#endif
