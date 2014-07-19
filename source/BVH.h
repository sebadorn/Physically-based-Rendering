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


struct Tri {
	cl_uint4 face;
	glm::vec3 bbMin;
	glm::vec3 bbMax;
};

struct BVHNode {
	BVHNode* leftChild;
	BVHNode* rightChild;
	vector<Tri> faces;
	glm::vec3 bbMin;
	glm::vec3 bbMax;
	cl_uint id;
	cl_uint depth;
};


class BVH {

	public:
		BVH(
			const vector<object3D> sceneObjects,
			const vector<cl_float> vertices,
			const vector<cl_float> normals
		);
		~BVH();
		vector<BVHNode*> getContainerNodes();
		cl_uint getDepth();
		vector<BVHNode*> getLeafNodes();
		vector<BVHNode*> getNodes();
		BVHNode* getRoot();
		void visualize( vector<cl_float>* vertices, vector<cl_uint>* indices );

	protected:
		void assignFacesToBins(
			const cl_uint axis, const cl_uint splits, const vector<Tri>* faces,
			const vector< vector<glm::vec3> >* leftBin,
			const vector< vector<glm::vec3> >* rightBin,
			vector< vector<Tri> >* leftBinFaces, vector< vector<Tri> >* rightBinFaces
		);
		BVHNode* buildTree(
			const vector<Tri> faces, const glm::vec3 bbMin, const glm::vec3 bbMax,
			cl_uint depth, bool useGivenBB, const cl_float rootSA
		);
		vector<BVHNode*> buildTreesFromObjects(
			const vector<object3D>* sceneObjects,
			const vector<cl_float>* vertices,
			const vector<cl_float>* normals
		);
		void buildWithMidpointSplit(
			BVHNode* node, const vector<Tri> faces,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces
		);
		cl_float buildWithSAH(
			BVHNode* node, vector<Tri> faces,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces, cl_float* lambda
		);
		cl_float calcSAH(
			const cl_float nodeSA_recip, const cl_float leftSA, const cl_float leftNumFaces,
			const cl_float rightSA, const cl_float rightNumFaces
		);
		void combineNodes( const cl_uint numSubTrees );
		void createBinCombinations(
			const BVHNode* node, const cl_uint axis, const vector<cl_float>* splitPos,
			vector< vector<glm::vec3> >* leftBin, vector< vector<glm::vec3> >* rightBin
		);
		cl_float getMean( const vector<Tri> faces, const cl_uint axis );
		cl_float getMeanOfNodes( const vector<BVHNode*> nodes, const cl_uint axis );
		vector<cl_float> getBinSplits(
			const BVHNode* node, const cl_uint splits, const cl_uint axis
		);
		void groupTreesToNodes( vector<BVHNode*> nodes, BVHNode* parent, cl_uint depth );
		void logStats( boost::posix_time::ptime timerStart );
		cl_uint longestAxis( const BVHNode* node );
		BVHNode* makeNode( const vector<Tri> faces, bool ignore );
		BVHNode* makeContainerNode( const vector<BVHNode*> subTrees, const bool isRoot );
		vector<cl_float4> packFloatAsFloat4( const vector<cl_float>* vertices );
		glm::vec3 phongTessellate(
			const glm::vec3 p1, const glm::vec3 p2, const glm::vec3 p3,
			const glm::vec3 n1, const glm::vec3 n2, const glm::vec3 n3,
			const float alpha, const float u, const float v
		);
		void resizeBinsToFaces(
			const cl_uint splits,
			const vector< vector<Tri> >* leftBinFaces, const vector< vector<Tri> >* rightBinFaces,
			vector< vector<glm::vec3> >* leftBin, vector< vector<glm::vec3> >* rightBin
		);
		cl_uint setMaxFaces( const int value );
		void splitBySAH(
			const cl_float nodeSA, cl_float* bestSAH, const cl_uint axis, vector<Tri> faces,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces, cl_float* lambda
		);
		void splitBySpatialSplit(
			BVHNode* node, const cl_uint axis, cl_float* sahBest, const vector<Tri> faces,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces,
			glm::vec3* bbMinLeft, glm::vec3* bbMaxLeft, glm::vec3* bbMinRight, glm::vec3* bbMaxRight
		);
		void splitFaces(
			const vector<Tri> faces, const cl_float midpoint, const cl_uint axis,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces
		);
		void splitNodes(
			const vector<BVHNode*> nodes, const cl_float midpoint, const cl_uint axis,
			vector<BVHNode*>* leftGroup, vector<BVHNode*>* rightGroup
		);
		void triCalcAABB(
			Tri* tri, const cl_uint4 fn,
			const vector<cl_float4>* vertices, const vector<cl_float>* normals
		);
		void triThicknessAndSidedrop(
			const glm::vec3 p1, const glm::vec3 p2, const glm::vec3 p3,
			const glm::vec3 n1, const glm::vec3 n2, const glm::vec3 n3,
			float* thickness, glm::vec3* sidedropMin, glm::vec3* sidedropMax
		);
		void visualizeNextNode(
			const BVHNode* node, vector<cl_float>* vertices, vector<cl_uint>* indices
		);

	private:
		vector<BVHNode*> mContainerNodes;
		vector<BVHNode*> mLeafNodes;
		vector<BVHNode*> mNodes;
		BVHNode* mRoot;

		cl_uint mMaxFaces;
		cl_uint mDepthReached;

};

#endif
