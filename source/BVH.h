#ifndef BVH_H
#define BVH_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include <CL/cl.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "Cfg.h"
#include "Logger.h"
#include "ModelLoader.h"

using std::vector;


struct BVHNode {
	BVHNode* leftChild;
	BVHNode* rightChild;
	vector<cl_uint4> faces;
	glm::vec3 bbMin;
	glm::vec3 bbMax;
	cl_uint id;
};


class BVH {

	public:
		BVH( vector<object3D> sceneObjects, vector<cl_float> vertices );
		~BVH();
		vector<BVHNode*> getContainerNodes();
		vector<BVHNode*> getLeafNodes();
		vector<BVHNode*> getNodes();
		BVHNode* getRoot();
		void visualize( vector<cl_float>* vertices, vector<cl_uint>* indices );

	protected:
		BVHNode* buildTree( vector<cl_uint4> faces, vector<cl_float4> vertices );
		vector<BVHNode*> buildTreesFromObjects( vector<object3D> sceneObjects, vector<cl_float> vertices );
		void divideFaces(
			vector<cl_uint4> faces, vector<cl_float4> vertices, cl_float midpoint, cl_uint axis,
			vector<cl_uint4>* leftFaces, vector<cl_uint4>* rightFaces
		);
		void divideNodes(
			vector<BVHNode*> nodes, cl_float midpoint, cl_uint axis,
			vector<BVHNode*>* leftGroup, vector<BVHNode*>* rightGroup
		);
		cl_float findMean( vector<cl_uint4> faces, vector<cl_float4> allVertices, cl_uint axis );
		cl_float findMeanOfNodes( vector<BVHNode*> nodes, cl_uint axis );
		cl_float findMidpoint( BVHNode* container, cl_uint axis );
		void getBoundingBox( vector<cl_float4> vertices, glm::vec3* bbMin, glm::vec3* bbMax );
		void getTriangleBB( cl_uint4 face, vector<cl_float4> vertices, glm::vec3* bbMin, glm::vec3* bbMax );
		glm::vec3 getTriangleCenter( cl_uint4 face, vector<cl_float4> vertices );
		glm::vec3 getTriangleCentroid( cl_uint4 face, vector<cl_float4> vertices );
		void groupTreesToNodes( vector<BVHNode*> nodes, BVHNode* parent );
		void logStats( boost::posix_time::ptime timerStart );
		cl_uint longestAxis( BVHNode* node );
		BVHNode* makeNode( vector<cl_uint4> faces, vector<cl_float4> vertices );
		BVHNode* makeContainerNode( vector<BVHNode*> subTrees, bool isRoot );
		void visualizeNextNode( BVHNode* node, vector<cl_float>* vertices, vector<cl_uint>* indices );

	private:
		vector<BVHNode*> mContainerNodes;
		vector<BVHNode*> mLeafNodes;
		vector<BVHNode*> mNodes;
		BVHNode* mRoot;

		cl_uint mMaxFaces;
		cl_uint mDepthReached;

};

#endif
