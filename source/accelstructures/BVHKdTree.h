#ifndef BVHKDTREE_H
#define BVHKDTREE_H

#include "BVH.h"
#include "KdTree.h"

using std::map;
using std::vector;


class BVHKdTree : public BVH {

	public:
		BVHKdTree(
			const vector<object3D> sceneObjects,
			const vector<cl_float> vertices,
			const vector<cl_float> normals
		);
		~BVHKdTree();
		map<cl_uint, KdTree*> getMapNodeToKdTree();
		virtual void visualize( vector<cl_float>* vertices, vector<cl_uint>* indices );

	protected:
		vector<BVHNode*> createKdTrees(
			const vector<object3D>* objects,
			const vector<cl_float>* vertices,
			const vector<cl_float>* normals
		);

	private:
		map<cl_uint, KdTree*> mNodeToKdTree;
		cl_uint mCounterID;

};


#endif
