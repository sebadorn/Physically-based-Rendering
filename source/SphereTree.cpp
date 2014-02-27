#include "SphereTree.h"

using std::vector;


/**
 * Build a sphere tree for each object in the scene and combine them into one big tree.
 * @param  {std::vector<object3D>} sceneObjects
 * @param  {std::vector<cl_float>} allVertices
 * @return {SphereTree*}
 */
SphereTree::SphereTree( vector<object3D> sceneObjects, vector<cl_float> allVertices ) {
	boost::posix_time::ptime timerStart = boost::posix_time::microsec_clock::local_time();
	mMaxFaces = 3; // TODO: Cfg
	mDepthReached = 0;

	vector<SphereTreeNode*> subTrees = this->buildTreesFromObjects( sceneObjects, allVertices );
	mRoot = this->makeContainerNode( subTrees, true );
	this->groupTreesToNodes( subTrees, mRoot );

	if( subTrees.size() > 1 ) {
		mNodes.push_back( mRoot );
	}
	mNodes.insert( mNodes.end(), mContainerNodes.begin(), mContainerNodes.end() );
	mNodes.insert( mNodes.end(), mLeafNodes.begin(), mLeafNodes.end() );

	for( cl_uint i = 0; i < mNodes.size(); i++ ) {
		mNodes[i]->id = i;
	}

	this->logStats( timerStart );
}


/**
 * Destructor.
 */
SphereTree::~SphereTree() {
	for( cl_uint i = 0; i < mNodes.size(); i++ ) {
		delete mNodes[i];
	}
}


/**
 * Build the sphere tree.
 * @param  {std::vector<cl_uint>}  faces
 * @param  {std::vector<cl_float>} allVertices
 * @return {SphereTreeNode*}
 */
SphereTreeNode* SphereTree::buildTree( vector<cl_uint4> faces, vector<cl_float4> allVertices ) {
	SphereTreeNode* containerNode = this->makeNode( faces, allVertices );

	// leaf node
	if( faces.size() <= mMaxFaces ) {
		if( faces.size() <= 0 ) {
			Logger::logWarning( "[SphereTree] No faces in node." );
		}
		containerNode->faces = faces;

		return containerNode;
	}

	cl_uint axis = this->longestAxis( containerNode );
	cl_float midpoint = this->findMidpoint( containerNode, axis );

	vector<cl_uint4> leftFaces;
	vector<cl_uint4> rightFaces;
	this->divideFaces( faces, allVertices, midpoint, axis, &leftFaces, &rightFaces );

	if( leftFaces.size() <= 0 || rightFaces.size() <= 0 ) {
		cl_float mean = this->findMean( faces, allVertices, axis );

		leftFaces.clear();
		rightFaces.clear();
		this->divideFaces( faces, allVertices, mean, axis, &leftFaces, &rightFaces );
	}

	containerNode->leftChild = this->buildTree( leftFaces, allVertices );
	containerNode->rightChild = this->buildTree( rightFaces, allVertices );

	return containerNode;
}


/**
 * Build sphere trees for all given scene objects.
 * @param  {std::vector<object3D>}        sceneObjects
 * @param  {std::vector<cl_float>}        allVertices
 * @return {std::vector<SphereTreeNode*>}
 */
vector<SphereTreeNode*> SphereTree::buildTreesFromObjects(
	vector<object3D> sceneObjects, vector<cl_float> allVertices
) {
	vector<SphereTreeNode*> subTrees;
	char msg[256];
	cl_int offset = 0;

	for( cl_uint i = 0; i < sceneObjects.size(); i++ ) {
		vector<cl_uint4> facesThisObj;
		vector<cl_float4> allVertices4;
		ModelLoader::getFacesAndVertices( sceneObjects[i], allVertices, &facesThisObj, &allVertices4, offset );
		offset += facesThisObj.size();

		snprintf(
			msg, 256, "[SphereTree] Building tree %u/%lu: \"%s\". %lu faces.",
			i + 1, sceneObjects.size(), sceneObjects[i].oName.c_str(), facesThisObj.size()
		);
		Logger::logInfo( msg );

		SphereTreeNode* st = this->buildTree( facesThisObj, allVertices4 );
		subTrees.push_back( st );
	}

	return subTrees;
}


/**
 * Divide the faces into two groups using the given midpoint and axis as criterium.
 * @param {std::vector<cl_uint4>}  faces
 * @param {std::vector<cl_float4>} vertices
 * @param {cl_float}               midpoint
 * @param {cl_uint}                axis
 * @param {std::vector<cl_uint4>*} leftFaces
 * @param {std::vector<cl_uint4>*} rightFaces
 */
void SphereTree::divideFaces(
	vector<cl_uint4> faces, vector<cl_float4> vertices, cl_float midpoint, cl_uint axis,
	vector<cl_uint4>* leftFaces, vector<cl_uint4>* rightFaces
) {
	for( cl_uint i = 0; i < faces.size(); i++ ) {
		cl_uint4 face = faces[i];
		glm::vec3 centroid = this->getTriangleCentroid( face, vertices );

		if( centroid[axis] < midpoint ) {
			leftFaces->push_back( face );
		}
		else {
			rightFaces->push_back( face );
		}
	}

	// One group has no children. We cannot allow that.
	// Try again with the triangle center instead of the centroid.
	if( leftFaces->size() == 0 || rightFaces->size() == 0 ) {
		Logger::logDebugVerbose( "[SphereTree] Dividing faces by centroid left one side empty. Trying again with center." );

		leftFaces->clear();
		rightFaces->clear();

		for( cl_uint i = 0; i < faces.size(); i++ ) {
			cl_uint4 face = faces[i];
			glm::vec3 center = this->getTriangleCenter( face, vertices );

			if( center[axis] < midpoint ) {
				leftFaces->push_back( face );
			}
			else {
				rightFaces->push_back( face );
			}
		}
	}

	// Oh, come on! Just do it 50:50 then.
	if( leftFaces->size() == 0 || rightFaces->size() == 0 ) {
		Logger::logDebugVerbose( "[SphereTree] Dividing faces by center left one side empty. Just doing it 50:50 now." );

		leftFaces->clear();
		rightFaces->clear();

		for( cl_uint i = 0; i < faces.size(); i++ ) {
			if( i < faces.size() / 2 ) {
				leftFaces->push_back( faces[i] );
			}
			else {
				rightFaces->push_back( faces[i] );
			}
		}
	}

	// There has to be somewhere else something wrong.
	if( leftFaces->size() == 0 || rightFaces->size() == 0 ) {
		char msg[256];
		snprintf(
			msg, 256, "[SphereTree] Dividing faces 50:50 left one side empty. Faces: %lu. Vertices: %lu.",
			faces.size(), vertices.size()
		);
		Logger::logError( msg );
	}
}


/**
 * Divide the nodes into two groups using the given midpoint and axis as criterium.
 * @param {std::vector<SphereTreeNode*>}  nodes
 * @param {cl_float}                      midpoint
 * @param {cl_uint}                       axis
 * @param {std::vector<SphereTreeNode*>*} leftGroup
 * @param {std::vector<SphereTreeNode*>*} rightGroup
 */
void SphereTree::divideNodes(
	vector<SphereTreeNode*> nodes, cl_float midpoint, cl_uint axis,
	vector<SphereTreeNode*>* leftGroup, vector<SphereTreeNode*>* rightGroup
) {
	for( cl_uint i = 0; i < nodes.size(); i++ ) {
		SphereTreeNode* node = nodes[i];
		glm::vec3 center = ( node->bbMax - node->bbMin ) / 2.0f;

		if( center[axis] < midpoint ) {
			leftGroup->push_back( node );
		}
		else {
			rightGroup->push_back( node );
		}
	}

	// Just do it 50:50 then.
	if( leftGroup->size() == 0 || rightGroup->size() == 0 ) {
		Logger::logDebugVerbose( "[SphereTree] Dividing nodes by center left one side empty. Just doing it 50:50 now." );

		leftGroup->clear();
		rightGroup->clear();

		for( cl_uint i = 0; i < nodes.size(); i++ ) {
			if( i < nodes.size() / 2 ) {
				leftGroup->push_back( nodes[i] );
			}
			else {
				rightGroup->push_back( nodes[i] );
			}
		}
	}

	// There has to be somewhere else something wrong.
	if( leftGroup->size() == 0 || rightGroup->size() == 0 ) {
		char msg[256];
		snprintf(
			msg, 256, "[SphereTree] Dividing nodes 50:50 left one side empty. Nodes: %lu.", nodes.size()
		);
		Logger::logError( msg );
	}
}


/**
 * Find the mean of the triangles regarding the given axis.
 * @param  {std::vector<cl_uint4>}  faces
 * @param  {std::vector<cl_float4>} allVertices
 * @param  {cl_uint}                axis
 * @return {cl_float}
 */
cl_float SphereTree::findMean( vector<cl_uint4> faces, vector<cl_float4> allVertices, cl_uint axis ) {
	cl_float sum = 0.0f;

	for( cl_uint i = 0; i < faces.size(); i++ ) {
		glm::vec3 center = this->getTriangleCentroid( faces[i], allVertices );
		sum += center[axis];
	}

	return sum / faces.size();
}


/**
 * Find the mean of the centers of the given nodes.
 * @param  {std::vector<SphereTreeNode*>} nodes
 * @param  {cl_uint}                      axis
 * @return {cl_float}
 */
cl_float SphereTree::findMeanOfNodes( vector<SphereTreeNode*> nodes, cl_uint axis ) {
	cl_float sum = 0.0f;

	for( cl_uint i = 0; i < nodes.size(); i++ ) {
		glm::vec3 center = ( nodes[i]->bbMax - nodes[i]->bbMin ) / 2.0f;
		sum += center[axis];
	}

	return sum / nodes.size();
}


/**
 * Find the midpoint on the longest axis of the node.
 * @param  {SphereTreeNode*} container
 * @param  {cl_uint*}        axis
 * @return {cl_float}
 */
cl_float SphereTree::findMidpoint( SphereTreeNode* container, cl_uint axis ) {
	glm::vec3 sides = ( container->bbMax + container->bbMin ) / 2.0f;

	return sides[axis];
}


/**
 * Calculate the bounding box from the given vertices.
 * @param {std::vector<cl_float4>} vertices
 * @param {glm::vec3}              bbMin
 * @param {glm::vec3}              bbMax
 */
void SphereTree::getBoundingBox( vector<cl_float4> vertices, glm::vec3* bbMin, glm::vec3* bbMax ) {
	*bbMin = glm::vec3( vertices[0].x, vertices[0].y, vertices[0].z );
	*bbMax = glm::vec3( vertices[0].x, vertices[0].y, vertices[0].z );

	for( cl_uint i = 1; i < vertices.size(); i++ ) {
		cl_float4 v = vertices[i];

		(*bbMin)[0] = ( (*bbMin)[0] < v.x ) ? (*bbMin)[0] : v.x;
		(*bbMin)[1] = ( (*bbMin)[1] < v.y ) ? (*bbMin)[1] : v.y;
		(*bbMin)[2] = ( (*bbMin)[2] < v.z ) ? (*bbMin)[2] : v.z;

		(*bbMax)[0] = ( (*bbMax)[0] > v.x ) ? (*bbMax)[0] : v.x;
		(*bbMax)[1] = ( (*bbMax)[1] > v.y ) ? (*bbMax)[1] : v.y;
		(*bbMax)[2] = ( (*bbMax)[2] > v.z ) ? (*bbMax)[2] : v.z;
	}
}


vector<SphereTreeNode*> SphereTree::getContainerNodes() {
	return mContainerNodes;
}


vector<SphereTreeNode*> SphereTree::getLeafNodes() {
	return mLeafNodes;
}


vector<SphereTreeNode*> SphereTree::getNodes() {
	return mNodes;
}


SphereTreeNode* SphereTree::getRoot() {
	return mRoot;
}


/**
 * Get the bounding box a face (triangle).
 * @param {cl_uint4}               face
 * @param {std::vector<cl_float4>} vertices
 * @param {glm::vec3}              bbMin
 * @param {glm::vec3}              bbMax
 */
void SphereTree::getTriangleBB( cl_uint4 face, vector<cl_float4> vertices, glm::vec3* bbMin, glm::vec3* bbMax ) {
	vector<cl_float4> triangleVertices;

	triangleVertices.push_back( vertices[face.x] );
	triangleVertices.push_back( vertices[face.y] );
	triangleVertices.push_back( vertices[face.z] );

	this->getBoundingBox( triangleVertices, bbMin, bbMax );
}


/**
 * Get the center of a face (triangle).
 * @param  {cl_uint4}               face
 * @param  {std::vector<cl_float4>} vertices
 * @return {glm::vec3}
 */
glm::vec3 SphereTree::getTriangleCenter( cl_uint4 face, vector<cl_float4> vertices ) {
	glm::vec3 bbMin;
	glm::vec3 bbMax;
	this->getTriangleBB( face, vertices, &bbMin, &bbMax );

	return ( bbMax - bbMin ) / 2.0f;
}


/**
 * Get the centroid of a face (triangle).
 * @param  {cl_uint4}               face
 * @param  {std::vector<cl_float4>} vertices
 * @return {glm::vec3}
 */
glm::vec3 SphereTree::getTriangleCentroid( cl_uint4 face, vector<cl_float4> vertices ) {
	glm::vec3 v0( vertices[face.x].x, vertices[face.x].y, vertices[face.x].z );
	glm::vec3 v1( vertices[face.y].x, vertices[face.y].y, vertices[face.y].z );
	glm::vec3 v2( vertices[face.z].x, vertices[face.z].y, vertices[face.z].z );

	return ( v0 + v1 + v2 ) / 3.0f;
}


/**
 * Group the sphere nodes into two groups and assign them to the given parent node.
 * @param {std::vector<SphereTreeNode*>} nodes
 * @param {SphereTreeNode*}              parent
 */
void SphereTree::groupTreesToNodes( vector<SphereTreeNode*> nodes, SphereTreeNode* parent ) {
	if( nodes.size() == 1 ) {
		// Implies: parent == nodes[0], nothing to do
		return;
	}

	cl_uint axis = this->longestAxis( parent );
	cl_float midpoint = this->findMidpoint( parent, axis );

	vector<SphereTreeNode*> leftGroup;
	vector<SphereTreeNode*> rightGroup;
	this->divideNodes( nodes, midpoint, axis, &leftGroup, &rightGroup );

	if( leftGroup.size() <= 0 || rightGroup.size() <= 0 ) {
		cl_float mean = this->findMeanOfNodes( nodes, axis );

		leftGroup.clear();
		rightGroup.clear();
		this->divideNodes( nodes, mean, axis, &leftGroup, &rightGroup );
	}

	SphereTreeNode* leftNode = this->makeContainerNode( leftGroup, false );
	parent->leftChild = leftNode;
	this->groupTreesToNodes( leftGroup, parent->leftChild );

	SphereTreeNode* rightNode = this->makeContainerNode( rightGroup, false );
	parent->rightChild = rightNode;
	this->groupTreesToNodes( rightGroup, parent->rightChild );
}


/**
 * Log some stats.
 * @param {boost::posix_time::ptime} timerStart
 */
void SphereTree::logStats( boost::posix_time::ptime timerStart ) {
	boost::posix_time::ptime timerEnd = boost::posix_time::microsec_clock::local_time();
	cl_float timeDiff = ( timerEnd - timerStart ).total_milliseconds();

	char msg[128];
	snprintf(
		msg, 128, "[SphereTree] Generated in %g ms. Contains %lu nodes (%lu leaves).",
		timeDiff, mNodes.size(), mLeafNodes.size()
	);
	Logger::logInfo( msg );
}


cl_uint SphereTree::longestAxis( SphereTreeNode* node ) {
	glm::vec3 sides = node->bbMax - node->bbMin;

	if( sides[0] > sides[1] ) {
		return ( sides[0] > sides[2] ) ? 0 : 2;
	}
	else { // sides[1] > sides[0]
		return ( sides[1] > sides[2] ) ? 1 : 2;
	}
}


/**
 * Create a container node that can contain the created sub-trees.
 * @param  {std::vector<SphereTreeNode*>} subTrees
 * @return {SphereTreeNode*}
 */
SphereTreeNode* SphereTree::makeContainerNode( vector<SphereTreeNode*> subTrees, bool isRoot ) {
	if( subTrees.size() == 1 ) {
		return subTrees[0];
	}

	SphereTreeNode* node = new SphereTreeNode;

	node->leftChild = NULL;
	node->rightChild = NULL;

	node->bbMin = glm::vec3( subTrees[0]->bbMin );
	node->bbMax = glm::vec3( subTrees[0]->bbMax );

	for( cl_uint i = 1; i < subTrees.size(); i++ ) {
		node->bbMin[0] = ( node->bbMin[0] < subTrees[i]->bbMin[0] ) ? node->bbMin[0] : subTrees[i]->bbMin[0];
		node->bbMin[1] = ( node->bbMin[1] < subTrees[i]->bbMin[1] ) ? node->bbMin[1] : subTrees[i]->bbMin[1];
		node->bbMin[2] = ( node->bbMin[2] < subTrees[i]->bbMin[2] ) ? node->bbMin[2] : subTrees[i]->bbMin[2];

		node->bbMax[0] = ( node->bbMax[0] > subTrees[i]->bbMax[0] ) ? node->bbMax[0] : subTrees[i]->bbMax[0];
		node->bbMax[1] = ( node->bbMax[1] > subTrees[i]->bbMax[1] ) ? node->bbMax[1] : subTrees[i]->bbMax[1];
		node->bbMax[2] = ( node->bbMax[2] > subTrees[i]->bbMax[2] ) ? node->bbMax[2] : subTrees[i]->bbMax[2];
	}

	if( !isRoot ) {
		mContainerNodes.push_back( node );
	}

	return node;
}


/**
 * Create a new node.
 * @param  {std::vector<cl_uint4>} faces
 * @param  {std::vector<cl_float>} allVertices Vertices to compute the bounding box from.
 * @return {SphereTreeNode*}
 */
SphereTreeNode* SphereTree::makeNode( vector<cl_uint4> faces, vector<cl_float4> allVertices ) {
	SphereTreeNode* node = new SphereTreeNode;
	node->leftChild = NULL;
	node->rightChild = NULL;

	vector<cl_float4> vertices;
	for( cl_uint i = 0; i < faces.size(); i++ ) {
		vertices.push_back( allVertices[faces[i].x] );
		vertices.push_back( allVertices[faces[i].y] );
		vertices.push_back( allVertices[faces[i].z] );
	}

	glm::vec3 bbMin;
	glm::vec3 bbMax;
	this->getBoundingBox( vertices, &bbMin, &bbMax );
	node->bbMin = bbMin;
	node->bbMax = bbMax;

	if( faces.size() <= mMaxFaces ) {
		mLeafNodes.push_back( node );
	}
	else {
		mContainerNodes.push_back( node );
	}

	return node;
}


/**
 * Get vertices and indices to draw a 3D visualization of the bounding box.
 * @param {std::vector<cl_float>*} vertices Vector to put the vertices into.
 * @param {std::vector<cl_uint>*}  indices  Vector to put the indices into.
 */
void SphereTree::visualize( vector<cl_float>* vertices, vector<cl_uint>* indices ) {
	this->visualizeNextNode( mRoot, vertices, indices );
}


/**
 * Visualize the next node in the SphereTree.
 * @param {SphereTreeNode*}        node     Current node.
 * @param {std::vector<cl_float>*} vertices Vector to put the vertices into.
 * @param {std::vector<cl_uint>*}  indices  Vector to put the indices into.
 */
void SphereTree::visualizeNextNode( SphereTreeNode* node, vector<cl_float>* vertices, vector<cl_uint>* indices ) {
	if( node == NULL ) {
		return;
	}

	// Only visualize leaf nodes
	if( node->faces.size() > 0 ) {
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
	}

	// Proceed with left side
	this->visualizeNextNode( node->leftChild, vertices, indices );

	// Proceed width right side
	this->visualizeNextNode( node->rightChild, vertices, indices );
}
