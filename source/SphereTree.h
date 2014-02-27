#ifndef SPHERETREE_H
#define SPHERETREE_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include <CL/cl.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "Cfg.h"
#include "Logger.h"
#include "ModelLoader.h"

using std::vector;


struct SphereTreeNode {
	SphereTreeNode* leftChild;
	SphereTreeNode* rightChild;
	vector<cl_uint4> faces;
	glm::vec3 bbMin;
	glm::vec3 bbMax;
	// glm::vec3 center;
	// cl_float radius;
	cl_uint id;
};


class SphereTree {

	public:
		SphereTree( vector<object3D> sceneObjects, vector<cl_float> vertices );
		~SphereTree();
		vector<SphereTreeNode*> getContainerNodes();
		vector<SphereTreeNode*> getLeafNodes();
		vector<SphereTreeNode*> getNodes();
		SphereTreeNode* getRoot();
		void visualize( vector<cl_float>* vertices, vector<cl_uint>* indices );

	protected:
		SphereTreeNode* buildTree( vector<cl_uint4> faces, vector<cl_float4> vertices );
		vector<SphereTreeNode*> buildTreesFromObjects( vector<object3D> sceneObjects, vector<cl_float> vertices );
		void divideFaces(
			vector<cl_uint4> faces, vector<cl_float4> vertices, cl_float midpoint, cl_uint axis,
			vector<cl_uint4>* leftFaces, vector<cl_uint4>* rightFaces
		);
		void divideNodes(
			vector<SphereTreeNode*> nodes, cl_float midpoint, cl_uint axis,
			vector<SphereTreeNode*>* leftGroup, vector<SphereTreeNode*>* rightGroup
		);
		cl_float findMean( vector<cl_uint4> faces, vector<cl_float4> allVertices, cl_uint axis );
		cl_float findMeanOfNodes( vector<SphereTreeNode*> nodes, cl_uint axis );
		cl_float findMidpoint( SphereTreeNode* container, cl_uint axis );
		void getBoundingBox( vector<cl_float4> vertices, glm::vec3* bbMin, glm::vec3* bbMax );
		void getTriangleBB( cl_uint4 face, vector<cl_float4> vertices, glm::vec3* bbMin, glm::vec3* bbMax );
		glm::vec3 getTriangleCenter( cl_uint4 face, vector<cl_float4> vertices );
		glm::vec3 getTriangleCentroid( cl_uint4 face, vector<cl_float4> vertices );
		void groupTreesToNodes( vector<SphereTreeNode*> nodes, SphereTreeNode* parent );
		void logStats( boost::posix_time::ptime timerStart );
		cl_uint longestAxis( SphereTreeNode* node );
		SphereTreeNode* makeNode( vector<cl_uint4> faces, vector<cl_float4> vertices );
		SphereTreeNode* makeContainerNode( vector<SphereTreeNode*> subTrees, bool isRoot );
		void visualizeNextNode( SphereTreeNode* node, vector<cl_float>* vertices, vector<cl_uint>* indices );

	private:
		vector<SphereTreeNode*> mContainerNodes;
		vector<SphereTreeNode*> mLeafNodes;
		vector<SphereTreeNode*> mNodes;
		SphereTreeNode* mRoot;

		cl_uint mMaxFaces;
		cl_uint mDepthReached;

};

#endif
