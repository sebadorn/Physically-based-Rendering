#ifndef BVH_H
#define BVH_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include <set>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

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
	uint numSkipsToHere;
	bool skipNextLeft;
};


class BVH : public AccelStructure {

	public:
		BVH();
		BVH(
			const vector<object3D> sceneObjects,
			const vector<float> vertices,
			const vector<float> normals
		);
		~BVH();
		vector<BVHNode*> getContainerNodes();
		unsigned long int getDepth();
		vector<BVHNode*> getLeafNodes();
		vector<BVHNode*> getNodes();
		BVHNode* getRoot();
		virtual void visualize( vector<float>* vertices, vector<unsigned long int>* indices );

	protected:
		void assignFacesToBins(
			const int axis, const unsigned long int splits, const vector<Tri>* faces,
			const vector< vector<glm::vec3> >* leftBin,
			const vector< vector<glm::vec3> >* rightBin,
			vector< vector<Tri> >* leftBinFaces, vector< vector<Tri> >* rightBinFaces
		);
		BVHNode* buildTree(
			const vector<Tri> faces, const glm::vec3 bbMin, const glm::vec3 bbMax,
			unsigned long int depth, const float rootSA
		);
		vector<BVHNode*> buildTreesFromObjects(
			const vector<object3D>* sceneObjects,
			const vector<float>* vertices,
			const vector<float>* normals
		);
		void buildWithMeanSplit(
			BVHNode* node, const vector<Tri> faces,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces
		);
		float buildWithSAH(
			BVHNode* node, vector<Tri> faces,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces
		);
		float calcSAH(
			const float leftSA, const float leftNumFaces,
			const float rightSA, const float rightNumFaces
		);
		void combineNodes( const unsigned long int numSubTrees );
		vector<Tri> facesToTriStructs(
			const vector<glm::uvec4>* facesThisObj, const vector<glm::uvec4>* faceNormalsThisObj,
			const vector<glm::vec4>* vertices4, const vector<float>* normals
		);
		float getMean( const vector<Tri> faces, const int axis );
		float getMeanOfNodes( const vector<BVHNode*> nodes, const int axis );
		void groupTreesToNodes( vector<BVHNode*> nodes, BVHNode* parent, unsigned long int depth );
		void growAABBsForSAH(
			const vector<Tri>* faces,
			vector< vector<glm::vec3> >* leftBB, vector< vector<glm::vec3> >* rightBB,
			vector<float>* leftSA, vector<float>* rightSA
		);
		void logStats( boost::posix_time::ptime timerStart );
		int longestAxis( const BVHNode* node );
		BVHNode* makeNode( const vector<Tri> faces, bool ignore );
		BVHNode* makeContainerNode( const vector<BVHNode*> subTrees, const bool isRoot );
		void orderNodesByTraversal();
		vector<glm::vec4> packFloatAsFloat4( const vector<float>* vertices );
		unsigned long int setMaxFaces( const int value );
		void skipAheadOfNodes();
		void splitBySAH(
			float* bestSAH, const int axis, vector<Tri> faces,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces
		);
		float splitFaces(
			const vector<Tri> faces, const float midpoint, const int axis,
			vector<Tri>* leftFaces, vector<Tri>* rightFaces
		);
		void splitNodes(
			const vector<BVHNode*> nodes, const float midpoint, const int axis,
			vector<BVHNode*>* leftGroup, vector<BVHNode*>* rightGroup
		);
		void visualizeNextNode(
			const BVHNode* node, vector<float>* vertices, vector<unsigned long int>* indices
		);

		vector<BVHNode*> mContainerNodes;
		vector<BVHNode*> mLeafNodes;
		vector<BVHNode*> mNodes;
		BVHNode* mRoot;

		unsigned long int mMaxFaces;
		unsigned long int mDepthReached;

};

#endif
