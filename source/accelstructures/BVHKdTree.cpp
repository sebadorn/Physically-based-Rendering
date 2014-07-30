#include "BVHKdTree.h"


/**
 * Build a sphere tree for each object in the scene and combine them into one big tree.
 * @param  {std::vector<object3D*>} sceneObjects
 * @param  {std::vector<cl_float*>} vertices
 * @param  {std::vector<cl_float*>} normals
 * @return {BVH*}
 */
BVHKdTree::BVHKdTree(
	const vector<object3D> sceneObjects,
	const vector<cl_float> vertices,
	const vector<cl_float> normals
) {
	boost::posix_time::ptime timerStart = boost::posix_time::microsec_clock::local_time();

	vector<cl_float4> vertices4 = this->packFloatAsFloat4( &vertices );
	glm::vec3 bbMin, bbMax;
	MathHelp::getAABB( vertices4, &bbMin, &bbMax );
	mCounterID = 0;
	mDepthReached = 1;

	mRoot = new BVHNode;
	mRoot->id = mCounterID++;
	mRoot->leftChild = NULL;
	mRoot->rightChild = NULL;
	mRoot->bbMin = glm::vec3( bbMin );
	mRoot->bbMax = glm::vec3( bbMax );

	mLeafNodes = this->createKdTrees( &sceneObjects, &vertices, &normals );
	mNodes.assign( mLeafNodes.begin(), mLeafNodes.end() );

	this->groupTreesToNodes( mNodes, mRoot, mDepthReached );

	for( cl_uint i = 0; i < mContainerNodes.size(); i++ ) {
		mContainerNodes[i]->id = mCounterID++;
	}

	mNodes.insert( mNodes.begin(), mContainerNodes.begin(), mContainerNodes.end() );
	mNodes.insert( mNodes.begin(), mRoot );

	boost::posix_time::ptime timerEnd = boost::posix_time::microsec_clock::local_time();
	cl_float timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	char msg[128];
	snprintf(
		msg, 128, "[BVH] Generated in %g ms. Contains %lu nodes and %lu kD-tree(s). Max depth at %u.",
		timeDiff, mNodes.size(), mLeafNodes.size(), mDepthReached
	);
	Logger::logInfo( msg );
}


/**
 * Destructor.
 */
BVHKdTree::~BVHKdTree() {
	for( int i = 0; i < mNodes.size(); i++ ) {
		delete mNodes[i];
	}
	for( map<cl_uint, KdTree*>::iterator it = mNodeToKdTree.begin(); it != mNodeToKdTree.end(); it++ ) {
		delete it->second;
	}
}


/**
* Create a kD-tree for each 3D object in the scene.
* @param {std::vector<object3D*>}        objects
* @param {std::vector<cl_float*>}        vertices
* @return {std::vector<BVHKdTreeNode*>}
*/
vector<BVHNode*> BVHKdTree::createKdTrees(
	const vector<object3D>* sceneObjects,
	const vector<cl_float>* vertices,
	const vector<cl_float>* normals
) {
	vector<BVHNode*> BVHnodes;
	glm::vec3 bbMin, bbMax;

	char msg[128];
	uint offset = 0;
	uint offsetN = 0;

	vector<cl_float4> vertices4 = this->packFloatAsFloat4( vertices );

	for( cl_uint i = 0; i < sceneObjects->size(); i++ ) {
		object3D o = (*sceneObjects)[i];
		vector<cl_float> ov;

		vector<cl_uint4> facesThisObj;
		vector<cl_uint4> faceNormalsThisObj;
		ModelLoader::getFacesOfObject( o, &facesThisObj, offset );
		ModelLoader::getFaceNormalsOfObject( o, &faceNormalsThisObj, offsetN );
		offset += facesThisObj.size();
		offsetN += faceNormalsThisObj.size();

		vector<Tri> triFaces = this->facesToTriStructs(
			&facesThisObj, &faceNormalsThisObj, &vertices4, normals
		);


		snprintf( msg, 128, "[BVH] Building kD-tree %u of %lu: \"%s\"", i + 1, sceneObjects->size(), o.oName.c_str() );
		Logger::indent( 0 );
		Logger::logInfo( msg );
		Logger::indent( LOG_INDENT );

		BVHNode* node = new BVHNode;
		node->id = mCounterID++;
		node->leftChild = NULL;
		node->rightChild = NULL;
		node->bbMin = glm::vec3( bbMin );
		node->bbMax = glm::vec3( bbMax );
		BVHnodes.push_back( node );

		mNodeToKdTree[node->id] = new KdTree( triFaces );
	}

	Logger::indent( 0 );

	return BVHnodes;
}


map<cl_uint, KdTree*> BVHKdTree::getMapNodeToKdTree() {
	return mNodeToKdTree;
}


void BVHKdTree::visualize( vector<cl_float>* vertices, vector<cl_uint>* indices ) {
	map<cl_uint, KdTree*>::iterator it = mNodeToKdTree.begin();

	while( it != mNodeToKdTree.end() ) {
		it->second->visualize( vertices, indices );
		it++;
	}
}
