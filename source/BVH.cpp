#include "BVH.h"

using std::vector;


/**
 * Constructor.
 * @param {std::vector<object3D>} objects
 * @param {std::vector<cl_float>} vertices
 */
BVH::BVH( vector<object3D> objects, vector<cl_float> vertices ) {
	boost::posix_time::ptime timerStart = boost::posix_time::microsec_clock::local_time();

	mBVnodes = this->createKdTrees( objects, vertices );
	vector<cl_float> bb = utils::computeBoundingBox( vertices );

	mRoot = new BVnode;
	mRoot->left = NULL;
	mRoot->right = NULL;
	mRoot->kdtree = NULL;
	mRoot->bbMin = glm::vec3( bb[0], bb[1], bb[2] );
	mRoot->bbMax = glm::vec3( bb[3], bb[4], bb[5] );
	mRoot->bbCenter = ( mRoot->bbMin + mRoot->bbMax ) / 2.0f;

	this->buildHierarchy( mBVnodes, mRoot );

	boost::posix_time::ptime timerEnd = boost::posix_time::microsec_clock::local_time();
	cl_float timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	char msg[128];
	snprintf( msg, 128, "[BVH] Generated in %g ms. Contains %lu kD-trees.", timeDiff, mBVnodes.size() );
	Logger::logInfo( msg );
}


/**
 * Deconstructor.
 */
BVH::~BVH() {
	for( int i = 0; i < mBVnodes.size(); i++ ) {
		if( mBVnodes[i]->kdtree != NULL ) {
			delete mBVnodes[i]->kdtree;
		}
		delete mBVnodes[i];
	}
	mBVnodes.clear();

	delete mRoot;
}


/**
 * Build the Bounding Volume Hierarchy.
 * @param {std::vector<BVnode*>} nodes
 * @param {BVnode*}              parent
 */
void BVH::buildHierarchy( vector<BVnode*> nodes, BVnode* parent ) {
	if( nodes.size() == 1 ) {
		parent = nodes[0];
		return;
	}

	BVnode* leftmost = NULL;
	BVnode* rightmost = NULL;
	this->findCornerNodes( nodes, parent, &leftmost, &rightmost );

	vector<BVnode*> leftGroup;
	vector<BVnode*> rightGroup;
	this->groupByCorner( nodes, leftmost, rightmost, &leftGroup, &rightGroup );

	parent->left = this->makeNodeFromGroup( leftGroup );
	parent->right = this->makeNodeFromGroup( leftGroup );

	this->buildHierarchy( leftGroup, parent->left );
	this->buildHierarchy( rightGroup, parent->right );
}


/**
 * Create a kD-tree for each 3D object in the scene.
 * @param  {std::vector<object3D>} objects
 * @param  {std::vector<cl_float>} vertices
 * @return {std::vector<BVnode*>}
 */
vector<BVnode*> BVH::createKdTrees( vector<object3D> objects, vector<cl_float> vertices ) {
	vector<BVnode*> BVnodes;
	vector<cl_float> bb;
	vector<cl_float> ov;
	vector<cl_uint> ignore;

	for( int i = 0; i < objects.size(); i++ ) {
		object3D o = objects[i];
		ov.clear();
		ignore.clear();

		for( int j = 0; j < o.facesV.size(); j++ ) {
			if( std::find( ignore.begin(), ignore.end(), o.facesV[j] ) != ignore.end() ) {
				continue;
			}
			ov.push_back( vertices[o.facesV[j] * 3] );
			ov.push_back( vertices[o.facesV[j] * 3 + 1] );
			ov.push_back( vertices[o.facesV[j] * 3 + 2] );
			ignore.push_back( o.facesV[j] );
		}

		bb = utils::computeBoundingBox( ov );
		cl_float bbMin[3] = { bb[0], bb[1], bb[2] };
		cl_float bbMax[3] = { bb[3], bb[4], bb[5] };

		BVnode* node = new BVnode;
		node->left = NULL;
		node->right = NULL;
		node->kdtree = new KdTree( ov, o.facesV, bbMin, bbMax );
		node->bbMin = glm::vec3( bb[0], bb[1], bb[2] );
		node->bbMax = glm::vec3( bb[3], bb[4], bb[5] );
		node->bbCenter = ( node->bbMin + node->bbMax ) / 2.0f;
		BVnodes.push_back( node );
	}

	return BVnodes;
}


/**
 * Find the two nodes that are closest to the min and max bounding box of the parent node.
 * @param {std::vector<BVnode*>} nodes
 * @param {BVnode*}              parent
 * @param {BVnode*}              leftmost
 * @param {BVnode*}              rightmost
 */
void BVH::findCornerNodes( vector<BVnode*> nodes, BVnode* parent, BVnode** leftmost, BVnode** rightmost ) {
	cl_float distNodeLeft, distNodeRight;
	cl_float distLeftmost, distRightmost;

	*leftmost = nodes[0];
	*rightmost = nodes[0];

	for( int i = 1; i < nodes.size(); i++ ) {
		BVnode* node = nodes[i];

		distNodeLeft = glm::length( node->bbCenter - parent->bbMin );
		distNodeRight = glm::length( node->bbCenter - parent->bbMax );
		distLeftmost = glm::length( (*leftmost)->bbCenter - parent->bbMin );
		distRightmost = glm::length( (*rightmost)->bbCenter - parent->bbMax );

		*leftmost = ( distNodeLeft < distLeftmost ) ? node : *leftmost;
		*rightmost = ( distNodeRight < distRightmost ) ? node : *rightmost;
	}
}


/**
 * Get the leaves of the BVH.
 * That are all nodes with a reference to a kD-tree.
 * @return {std::vector<BVnode*>}
 */
vector<BVnode*> BVH::getLeaves() {
	return mBVnodes;
}


/**
 * Get the root node of the BVH.
 * @return {BVnode*}
 */
BVnode* BVH::getRoot() {
	return mRoot;
}


/**
 * Group the nodes in two groups.
 * @param {std::vector<BVnode*>}  nodes
 * @param {BVnode*}               leftmost
 * @param {BVnode*}               rightmost
 * @param {std::vector<BVnode*>*} leftGroup
 * @param {std::vector<BVnode*>*} rightGroup
 */
void BVH::groupByCorner(
	vector<BVnode*> nodes, BVnode* leftmost, BVnode* rightmost,
	vector<BVnode*>* leftGroup, vector<BVnode*>* rightGroup
) {
	cl_float distToLeftmost, distToRightmost;

	for( int i = 0; i < nodes.size(); i++ ) {
		BVnode* node = nodes[i];

		distToLeftmost = glm::length( node->bbCenter - leftmost->bbCenter );
		distToRightmost = glm::length( node->bbCenter - rightmost->bbCenter );

		if( distToLeftmost < distToRightmost ) {
			leftGroup->push_back( node );
		}
		else {
			rightGroup->push_back( node );
		}
	}
}


/**
 * Make a BV node out of a given group of BV nodes.
 * @param  {std::vector<BVnode*>} group
 * @return {BVnode*}
 */
BVnode* BVH::makeNodeFromGroup( vector<BVnode*> group ) {
	BVnode* vol = new BVnode;
	vol->left = NULL;
	vol->right = NULL;
	vol->kdtree = NULL;
	vol->bbMin = glm::vec3( group[0]->bbMin );
	vol->bbMax = glm::vec3( group[0]->bbMax );

	for( int i = 1; i < group.size(); i++ ) {
		BVnode* node = group[i];

		vol->bbMin[0] = ( node->bbMin[0] < vol->bbMin[0] ) ? node->bbMin[0] : vol->bbMin[0];
		vol->bbMin[1] = ( node->bbMin[1] < vol->bbMin[1] ) ? node->bbMin[1] : vol->bbMin[1];
		vol->bbMin[2] = ( node->bbMin[2] < vol->bbMin[2] ) ? node->bbMin[2] : vol->bbMin[2];
		vol->bbMax[0] = ( node->bbMax[0] > vol->bbMax[0] ) ? node->bbMax[0] : vol->bbMax[0];
		vol->bbMax[1] = ( node->bbMax[1] > vol->bbMax[1] ) ? node->bbMax[1] : vol->bbMax[1];
		vol->bbMax[2] = ( node->bbMax[2] > vol->bbMax[2] ) ? node->bbMax[2] : vol->bbMax[2];
	}

	vol->bbCenter = ( vol->bbMin + vol->bbMax ) / 2.0f;

	return vol;
}
