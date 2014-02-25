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
};


class SphereTree {

	public:
		SphereTree( vector<object3D> sceneObjects, vector<cl_float> vertices );
		~SphereTree();

	protected:
		SphereTreeNode* buildTree( vector<cl_uint4> faces, vector<cl_float4> vertices );
		void divideFaces(
			vector<cl_uint4> faces, vector<cl_float4> vertices, cl_float midpoint, cl_uint axis,
			vector<cl_uint4>* leftFaces, vector<cl_uint4>* rightFaces
		);
		cl_float findMean( vector<cl_uint4> faces, vector<cl_float4> allVertices, cl_uint axis );
		cl_float findMidpoint( SphereTreeNode* container, cl_uint* axis );
		void getBoundingBox( vector<cl_float4> vertices, glm::vec3* bbMin, glm::vec3* bbMax );
		void getTriangleBB( cl_uint4 face, vector<cl_float4> vertices, glm::vec3* bbMin, glm::vec3* bbMax );
		glm::vec3 getTriangleCenter( cl_uint4 face, vector<cl_float4> vertices );
		void logStats( boost::posix_time::ptime timerStart );
		SphereTreeNode* makeNode( vector<cl_uint4> faces, vector<cl_float4> vertices );
		SphereTreeNode* makeRootNode( vector<SphereTreeNode*> subTrees );

	private:
		vector<SphereTreeNode*> mNodes;
		SphereTreeNode* mRoot;

};

#endif
