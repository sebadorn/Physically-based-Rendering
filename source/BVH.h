#ifndef BVH_H
#define BVH_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include <CL/cl.hpp>
#include <glm/glm.hpp>
#include <set>
#include <vector>

#include "Cfg.h"
#include "Logger.h"
#include "MathHelp.h"
#include "ModelLoader.h"

using std::set;
using std::vector;


struct BVHNode {
	BVHNode* leftChild;
	BVHNode* rightChild;
	vector<cl_uint4> faces;
	glm::vec3 bbMin;
	glm::vec3 bbMax;
	cl_uint id;
	cl_uint depth;
};


class BVH {

	public:
		BVH( vector<object3D> sceneObjects, vector<cl_float> vertices );
		~BVH();
		vector<BVHNode*> getContainerNodes();
		cl_uint getDepth();
		vector<BVHNode*> getLeafNodes();
		vector<BVHNode*> getNodes();
		BVHNode* getRoot();
		void visualize( vector<cl_float>* vertices, vector<cl_uint>* indices );

	protected:
		void assignFacesToBins(
			vector< vector<cl_uint4> > binFaces, cl_uint splits,
			vector< vector<cl_uint4> >* leftBinFaces, vector< vector<cl_uint4> >* rightBinFaces
		);
		BVHNode* buildTree(
			vector<cl_uint4> faces, vector<cl_float4> vertices, cl_uint depth,
			glm::vec3 bbMin, glm::vec3 bbMax, bool useGivenBB, cl_float rootSA
		);
		vector<BVHNode*> buildTreesFromObjects(
			vector<object3D> sceneObjects, vector<cl_float> vertices
		);
		cl_float calcSAH(
			cl_float nodeSA_recip, cl_float leftSA, cl_float leftNumFaces,
			cl_float rightSA, cl_float rightNumFaces
		);
		void clipLine(
			glm::vec3 p, glm::vec3 q, glm::vec3 s, glm::vec3 nl,
			vector<cl_float4>* vertices
		);
		void clippedFacesAABB(
			vector<cl_uint4> faces, vector<cl_float4> allVertices,
			cl_uint axis, vector<glm::vec3>* binAABB
		);
		void combineNodes( vector<BVHNode*> subTrees );
		cl_float findMean( vector<cl_uint4> faces, vector<cl_float4> allVertices, cl_uint axis );
		cl_float findMeanOfNodes( vector<BVHNode*> nodes, cl_uint axis );
		cl_float findMidpoint( BVHNode* container, cl_uint axis );
		vector< vector<glm::vec3> > getBinAABBs(
			BVHNode* node, vector<cl_float> splitPos, cl_uint axis
		);
		vector< vector<cl_uint4> > getBinFaces(
			vector<cl_uint4> faces, vector<cl_float4> allVertices,
			vector< vector<glm::vec3> > bins, cl_uint axis
		);
		vector<cl_float> getBinSplits( BVHNode* node, cl_uint splits, cl_uint axis );
		void groupTreesToNodes( vector<BVHNode*> nodes, BVHNode* parent, cl_uint depth );
		void growBinAABBs(
			vector< vector<glm::vec3> > binBBs, vector< vector<cl_uint4> > binFaces,
			cl_uint splits,
			vector< vector<glm::vec3> >* leftBB, vector< vector<glm::vec3> >* rightBB
		);
		void logStats( boost::posix_time::ptime timerStart );
		cl_uint longestAxis( BVHNode* node );
		BVHNode* makeNode( vector<cl_uint4> faces, vector<cl_float4> vertices );
		BVHNode* makeContainerNode( vector<BVHNode*> subTrees, bool isRoot );
		cl_uint setMaxFaces( cl_uint value );
		void splitBySAH(
			cl_float nodeSA, cl_float* bestSAH, cl_uint axis, vector<cl_uint4> faces, vector<cl_float4> allVertices,
			vector<cl_uint4>* leftFaces, vector<cl_uint4>* rightFaces, cl_float* lambda
		);
		void splitBySpatialSplit(
			BVHNode* node, cl_uint axis, cl_float* sahBest, vector<cl_uint4> faces, vector<cl_float4> allVertices,
			vector<cl_uint4>* leftFaces, vector<cl_uint4>* rightFaces,
			glm::vec3* bbMinLeft, glm::vec3* bbMaxLeft, glm::vec3* bbMinRight, glm::vec3* bbMaxRight
		);
		void splitFaces(
			vector<cl_uint4> faces, vector<cl_float4> vertices, cl_float midpoint, cl_uint axis,
			vector<cl_uint4>* leftFaces, vector<cl_uint4>* rightFaces
		);
		void splitNodes(
			vector<BVHNode*> nodes, cl_float midpoint, cl_uint axis,
			vector<BVHNode*>* leftGroup, vector<BVHNode*>* rightGroup
		);
		vector<cl_uint4> uniqueFaces( vector<cl_uint4> faces );
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
