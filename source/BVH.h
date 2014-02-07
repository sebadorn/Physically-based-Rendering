#ifndef BVH_H
#define BVH_H

#include <CL/cl.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "Cfg.h"
#include "KdTree.h"
#include "Logger.h"
#include "ModelLoader.h"

using std::vector;


struct BVnode {
	BVnode* left;
	BVnode* right;
	KdTree* kdtree;
	glm::vec3 bbMin;
	glm::vec3 bbMax;
	glm::vec3 bbCenter;
};


class BVH {

	public:
		BVH( vector<object3D> objects, vector<cl_float> vertices );
		~BVH();
		vector<BVnode*> getLeaves();
		BVnode* getRoot();

	protected:
		vector<BVnode*> createKdTrees(
			vector<object3D> objects, vector<cl_float> vertices
		);
		void buildHierarchy( vector<BVnode*> nodes, BVnode* parent );
		void findCornerNodes(
			vector<BVnode*> nodes, BVnode* parent,
			BVnode** leftmost, BVnode** rightmost
		);
		void groupByCorner(
			vector<BVnode*> nodes, BVnode* leftmost, BVnode* rightmost,
			vector<BVnode*>* leftGroup, vector<BVnode*>* rightGroup
		);
		BVnode* makeNodeFromGroup( vector<BVnode*> group );

	private:
		vector<BVnode*> mBVnodes;
		BVnode* mRoot;

};

#endif
