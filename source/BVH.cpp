#include "BVH.h"

using std::vector;


/**
 * Constructor.
 * @param {std::vector<object3D>} objects
 * @param {std::vector<cl_float>} vertices
 */
BVH::BVH( vector<object3D> objects, vector<cl_float> vertices ) {
	boost::posix_time::ptime timerStart = boost::posix_time::microsec_clock::local_time();

	vector<cl_float> bb = utils::computeBoundingBox( vertices );
	mCounterID = 0;

	mRoot = new BVHnode;
	mRoot->id = mCounterID++;
	mRoot->left = NULL;
	mRoot->right = NULL;
	mRoot->kdtree = NULL;
	mRoot->bbMin = glm::vec3( bb[0], bb[1], bb[2] );
	mRoot->bbMax = glm::vec3( bb[3], bb[4], bb[5] );
	mRoot->bbCenter = ( mRoot->bbMin + mRoot->bbMax ) / 2.0f;

	mBVleaves = this->createKdTrees( objects, vertices );
	mBVHnodes.insert( mBVHnodes.end(), mBVleaves.begin(), mBVleaves.end() );

	this->buildHierarchy( mBVHnodes, mRoot );
	mBVHnodes.insert( mBVHnodes.begin(), mRoot );

	boost::posix_time::ptime timerEnd = boost::posix_time::microsec_clock::local_time();
	cl_float timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	char msg[128];
	snprintf(
		msg, 128, "[BVH] Generated in %g ms. Contains %lu nodes and %lu kD-tree(s).",
		timeDiff, mBVHnodes.size(), mBVleaves.size()
	);
	Logger::logInfo( msg );
}


/**
 * Destructor.
 */
BVH::~BVH() {
	for( int i = 0; i < mBVHnodes.size(); i++ ) {
		if( mBVHnodes[i]->kdtree != NULL ) {
			delete mBVHnodes[i]->kdtree;
		}
		delete mBVHnodes[i];
	}
}


/**
 * Build the Bounding Volume Hierarchy.
 * @param {std::vector<BVHnode*>} nodes
 * @param {BVHnode*}              parent
 */
void BVH::buildHierarchy( vector<BVHnode*> nodes, BVHnode* parent ) {
	if( nodes.size() == 1 ) {
		if( parent == mRoot ) {
			parent->left = nodes[0];
		}
		return;
	}

	BVHnode* leftmost = NULL;
	BVHnode* rightmost = NULL;
	this->findCornerNodes( nodes, parent, &leftmost, &rightmost );

	// TODO: Fix bug (happens only with some models)
	if( leftmost == rightmost ) {
		Logger::logError( "[BVH] Leftmost node is also rightmost node." );
		return;
	}

	vector<BVHnode*> leftGroup;
	vector<BVHnode*> rightGroup;
	this->groupByCorner( nodes, leftmost, rightmost, &leftGroup, &rightGroup );

	if( leftGroup.size() > 1 ) {
		parent->left = this->makeNodeFromGroup( leftGroup );
		mBVHnodes.push_back( parent->left );
	}
	else {
		parent->left = leftGroup[0];
	}

	if( rightGroup.size() > 1 ) {
		parent->right = this->makeNodeFromGroup( rightGroup );
		mBVHnodes.push_back( parent->right );
	}
	else {
		parent->right = rightGroup[0];
	}

	this->buildHierarchy( leftGroup, parent->left );
	this->buildHierarchy( rightGroup, parent->right );
}


/**
 * Create a kD-tree for each 3D object in the scene.
 * @param  {std::vector<object3D>} objects
 * @param  {std::vector<cl_float>} vertices
 * @return {std::vector<BVHnode*>}
 */
vector<BVHnode*> BVH::createKdTrees( vector<object3D> objects, vector<cl_float> vertices ) {
	vector<BVHnode*> BVHnodes;
	vector<cl_float> bb, ov;
	vector<cl_uint> of;
	char msg[128];

	for( cl_uint i = 0; i < objects.size(); i++ ) {
		object3D o = objects[i];
		ov.clear();
		of.clear();

		for( cl_uint j = 0; j < o.facesV.size(); j++ ) {
			ov.push_back( vertices[o.facesV[j] * 3] );
			ov.push_back( vertices[o.facesV[j] * 3 + 1] );
			ov.push_back( vertices[o.facesV[j] * 3 + 2] );

			of.push_back( j );
		}

		bb = utils::computeBoundingBox( ov );
		cl_float bbMin[3] = { bb[0], bb[1], bb[2] };
		cl_float bbMax[3] = { bb[3], bb[4], bb[5] };

		BVHnode* node = new BVHnode;
		node->id = mCounterID++;
		node->left = NULL;
		node->right = NULL;
		node->kdtree = new KdTree( ov, of, bbMin, bbMax );
		node->bbMin = glm::vec3( bb[0], bb[1], bb[2] );
		node->bbMax = glm::vec3( bb[3], bb[4], bb[5] );
		node->bbCenter = ( node->bbMin + node->bbMax ) / 2.0f;
		BVHnodes.push_back( node );

		snprintf( msg, 128, "[BVH] Build kD-tree %u of %lu: \"%s\"", i + 1, objects.size(), o.oName.c_str() );
		Logger::logInfo( msg );
	}

	return BVHnodes;
}


/**
 * Find the two nodes that are closest to the min and max bounding box of the parent node.
 * @param {std::vector<BVHnode*>} nodes
 * @param {BVHnode*}              parent
 * @param {BVHnode**}             leftmost
 * @param {BVHnode**}             rightmost
 */
void BVH::findCornerNodes(
	vector<BVHnode*> nodes, BVHnode* parent,
	BVHnode** leftmost, BVHnode** rightmost
) {
	cl_float distNodeLeft, distNodeRight;
	cl_float distLeftmost, distRightmost;

	*leftmost = nodes[0];
	*rightmost = nodes[0];

	for( cl_uint i = 1; i < nodes.size(); i++ ) {
		BVHnode* node = nodes[i];

		distNodeLeft = glm::length( node->bbCenter - parent->bbMin );
		distNodeRight = glm::length( node->bbCenter - parent->bbMax );
		distLeftmost = glm::length( (*leftmost)->bbCenter - parent->bbMin );
		distRightmost = glm::length( (*rightmost)->bbCenter - parent->bbMax );

		if( distNodeLeft < distLeftmost ) {
			*leftmost = node;
		}
		if( distNodeRight < distRightmost ) {
			*rightmost = node;
		}
	}
}


/**
 * Get the leaves of the BVH.
 * That are all nodes with a reference to a kD-tree.
 * @return {std::vector<BVHnode*>}
 */
vector<BVHnode*> BVH::getLeaves() {
	return mBVleaves;
}


/**
 * Get the nodes of the BVH.
 * @return {std::vector<BVHnode*>}
 */
vector<BVHnode*> BVH::getNodes() {
	return mBVHnodes;
}


/**
 * Get the root node of the BVH.
 * @return {BVHnode*}
 */
BVHnode* BVH::getRoot() {
	return mRoot;
}


/**
 * Group the nodes in two groups.
 * @param {std::vector<BVHnode*>}  nodes
 * @param {BVHnode*}               leftmost
 * @param {BVHnode*}               rightmost
 * @param {std::vector<BVHnode*>*} leftGroup
 * @param {std::vector<BVHnode*>*} rightGroup
 */
void BVH::groupByCorner(
	vector<BVHnode*> nodes, BVHnode* leftmost, BVHnode* rightmost,
	vector<BVHnode*>* leftGroup, vector<BVHnode*>* rightGroup
) {
	cl_float distToLeftmost, distToRightmost;

	leftGroup->push_back( leftmost );
	rightGroup->push_back( rightmost );

	for( int i = 0; i < nodes.size(); i++ ) {
		BVHnode* node = nodes[i];

		if( node == leftmost || node == rightmost ) {
			continue;
		}

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
 * @param  {std::vector<BVHnode*>} group
 * @return {BVHnode*}
 */
BVHnode* BVH::makeNodeFromGroup( vector<BVHnode*> group ) {
	BVHnode* vol = new BVHnode;
	vol->id = mCounterID++;
	vol->left = NULL;
	vol->right = NULL;
	vol->kdtree = NULL;
	vol->bbMin = glm::vec3( group[0]->bbMin );
	vol->bbMax = glm::vec3( group[0]->bbMax );

	for( int i = 1; i < group.size(); i++ ) {
		BVHnode* node = group[i];

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


/**
 * Get vertices and indices to draw a 3D visualization of the bounding box.
 * @param {std::vector<cl_float>*} vertices Vector to put the vertices into.
 * @param {std::vector<cl_uint>*}  indices  Vector to put the indices into.
 */
void BVH::visualize( vector<cl_float>* vertices, vector<cl_uint>* indices ) {
	this->visualizeNextNode( mRoot, vertices, indices );
}


/**
 * Visualize the next node in the BVH.
 * @param {kdNode_t*}              node     Current node.
 * @param {std::vector<cl_float>*} vertices Vector to put the vertices into.
 * @param {std::vector<cl_uint>*}  indices  Vector to put the indices into.
 */
void BVH::visualizeNextNode( BVHnode* node, vector<cl_float>* vertices, vector<cl_uint>* indices ) {
	if( node == NULL ) {
		return;
	}

	cl_uint i = vertices->size() / 3;

	// bottom
	vertices->push_back( node->bbMin[0] ); vertices->push_back( node->bbMin[1] ); vertices->push_back( node->bbMin[2] );
	vertices->push_back( node->bbMin[0] ); vertices->push_back( node->bbMin[1] ); vertices->push_back( node->bbMax[2] );
	vertices->push_back( node->bbMax[0] ); vertices->push_back( node->bbMin[1] ); vertices->push_back( node->bbMax[2] );
	vertices->push_back( node->bbMax[0] ); vertices->push_back( node->bbMin[1] ); vertices->push_back( node->bbMin[2] );

	// top
	vertices->push_back( node->bbMin[0] ); vertices->push_back( node->bbMax[1] ); vertices->push_back( node->bbMin[2] );
	vertices->push_back( node->bbMin[0] ); vertices->push_back( node->bbMax[1] ); vertices->push_back( node->bbMax[2] );
	vertices->push_back( node->bbMax[0] ); vertices->push_back( node->bbMax[1] ); vertices->push_back( node->bbMax[2] );
	vertices->push_back( node->bbMax[0] ); vertices->push_back( node->bbMax[1] ); vertices->push_back( node->bbMin[2] );

	cl_uint newIndices[24] = {
		// bottom
		i + 0, i + 1,
		i + 1, i + 2,
		i + 2, i + 3,
		i + 3, i + 0,
		// top
		i + 4, i + 5,
		i + 5, i + 6,
		i + 6, i + 7,
		i + 7, i + 4,
		// back
		i + 0, i + 4,
		i + 3, i + 7,
		// front
		i + 1, i + 5,
		i + 2, i + 6
	};
	indices->insert( indices->end(), newIndices, newIndices + 24 );

	// Proceed with left side
	this->visualizeNextNode( node->left, vertices, indices );

	// Proceed width right side
	this->visualizeNextNode( node->right, vertices, indices );
}
