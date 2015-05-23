#ifndef BVH_H
#define BVH_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include <set>

#include "AccelStructure.h"
#include "../Cfg.h"
#include "../Logger.h"
#include "../MathHelp.h"
#include "../ModelLoader.h"

using std::vector;


struct BVHNode {
	BVHNode* leftChild;
	BVHNode* rightChild;
	BVHNode* parent;
	vector<Tri> faces;
	glm::vec3 bbMin;
	glm::vec3 bbMax;
	uint id;
	uint depth;
};


class BVH : public AccelStructure {

	public:
		BVH();
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
		virtual void visualize( vector<cl_float>* vertices, vector<cl_uint>* indices );

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
		void buildWithMeanSplit(
			BVHNode* node, const vector<Tri> faces,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces
		);
		cl_float buildWithSAH(
			BVHNode* node, vector<Tri> faces,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces, cl_float* lambda
		);
		cl_float calcSAH(
			const cl_float leftSA, const cl_float leftNumFaces,
			const cl_float rightSA, const cl_float rightNumFaces
		);
		void combineNodes( const cl_uint numSubTrees );
		void createBinCombinations(
			const BVHNode* node, const cl_uint axis, const vector<cl_float>* splitPos,
			vector< vector<glm::vec3> >* leftBin, vector< vector<glm::vec3> >* rightBin
		);
		vector<Tri> facesToTriStructs(
			const vector<cl_uint4>* facesThisObj, const vector<cl_uint4>* faceNormalsThisObj,
			const vector<cl_float4>* vertices4, const vector<float>* normals
		);
		cl_float getMean( const vector<Tri> faces, const cl_uint axis );
		cl_float getMeanOfNodes( const vector<BVHNode*> nodes, const cl_uint axis );
		vector<cl_float> getBinSplits(
			const BVHNode* node, const cl_uint splits, const cl_uint axis
		);
		void groupTreesToNodes( vector<BVHNode*> nodes, BVHNode* parent, cl_uint depth );
		void growAABBsForSAH(
			const vector<Tri>* faces,
			vector< vector<glm::vec3> >* leftBB, vector< vector<glm::vec3> >* rightBB,
			vector<cl_float>* leftSA, vector<cl_float>* rightSA
		);
		void logStats( boost::posix_time::ptime timerStart );
		cl_uint longestAxis( const BVHNode* node );
		BVHNode* makeNode( const vector<Tri> faces, bool ignore );
		BVHNode* makeContainerNode( const vector<BVHNode*> subTrees, const bool isRoot );
		void orderNodesByTraversal();
		vector<cl_float4> packFloatAsFloat4( const vector<cl_float>* vertices );
		void resizeBinsToFaces(
			const cl_uint splits,
			const vector< vector<Tri> >* leftBinFaces, const vector< vector<Tri> >* rightBinFaces,
			vector< vector<glm::vec3> >* leftBin, vector< vector<glm::vec3> >* rightBin
		);
		cl_uint setMaxFaces( const int value );
		void splitBySAH(
			cl_float* bestSAH, const cl_uint axis, vector<Tri> faces,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces, cl_float* lambda
		);
		void splitBySpatialSplit(
			BVHNode* node, const cl_uint axis, cl_float* sahBest, const vector<Tri> faces,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces,
			glm::vec3* bbMinLeft, glm::vec3* bbMaxLeft,
			glm::vec3* bbMinRight, glm::vec3* bbMaxRight
		);
		cl_float splitFaces(
			const vector<Tri> faces, const cl_float midpoint, const cl_uint axis,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces
		);
		void splitNodes(
			const vector<BVHNode*> nodes, const cl_float midpoint, const cl_uint axis,
			vector<BVHNode*>* leftGroup, vector<BVHNode*>* rightGroup
		);
		void visualizeNextNode(
			const BVHNode* node, vector<cl_float>* vertices, vector<cl_uint>* indices
		);

		vector<BVHNode*> mContainerNodes;
		vector<BVHNode*> mLeafNodes;
		vector<BVHNode*> mNodes;
		BVHNode* mRoot;

		cl_uint mMaxFaces;
		cl_uint mDepthReached;

};

#endif
