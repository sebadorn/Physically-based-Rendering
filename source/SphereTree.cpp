#include "SphereTree.h"

using std::vector;


/**
 * Build a sphere tree for each object in the scene and combine them into one big tree.
 * @param  {std::vector<object3D>} sceneObjects
 * @param  {std::vector<cl_float>} vertices
 * @return {SphereTree*}
 */
SphereTree::SphereTree( vector<object3D> sceneObjects, vector<cl_float> vertices ) {
	boost::posix_time::ptime timerStart = boost::posix_time::microsec_clock::local_time();
	vector<SphereTreeNode*> subTrees;
	char msg[256];

	for( cl_uint i = 0; i < sceneObjects.size(); i++ ) {
		vector<cl_uint4> facesThisObj;
		vector<cl_float4> verticesThisObj;
		ModelLoader::getFacesAndVertices( sceneObjects[i], vertices, &facesThisObj, &verticesThisObj );

		snprintf(
			msg, 256, "[SphereTree] Object %u/%lu: \"%s\". %lu faces. %lu vertices.",
			i + 1, sceneObjects.size(), sceneObjects[i].oName.c_str(), facesThisObj.size(), verticesThisObj.size()
		);
		Logger::logInfo( msg );

		SphereTreeNode* st = this->buildTree( facesThisObj, verticesThisObj );
		subTrees.push_back( st );
	}

	mRoot = this->makeRootNode( subTrees );

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
 * @param  {std::vector<cl_float>} vertices
 * @return {SphereTreeNode*}
 */
SphereTreeNode* SphereTree::buildTree( vector<cl_uint4> faces, vector<cl_float4> vertices ) {
	SphereTreeNode* containerNode = this->makeNode( faces, vertices );

	if( faces.size() <= 3 ) { // TODO: faces maximum in cfg
		if( faces.size() <= 0 ) {
			Logger::logWarning( "[SphereTree] No faces in node." );
		}
		containerNode->faces = faces;

		return containerNode;
	}

	cl_uint axis;
	cl_float midpoint = this->findMidpoint( containerNode, &axis );

	vector<cl_uint4> leftFaces;
	vector<cl_uint4> rightFaces;
	this->divideFaces( faces, vertices, midpoint, axis, &leftFaces, &rightFaces );

	if( leftFaces.size() <= 0 || rightFaces.size() <= 0 ) {
		midpoint = this->findMean( faces, vertices, axis );
		leftFaces.clear();
		rightFaces.clear();
		this->divideFaces( faces, vertices, midpoint, axis, &leftFaces, &rightFaces );
	}

	containerNode->leftChild = this->buildTree( leftFaces, vertices );
	containerNode->rightChild = this->buildTree( rightFaces, vertices );

	return containerNode;
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
		glm::vec3 center = this->getTriangleCenter( face, vertices );

		if( center[axis] < midpoint ) {
			leftFaces->push_back( face );
		}
		else {
			rightFaces->push_back( face );
		}
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
		glm::vec3 center = this->getTriangleCenter( faces[i], allVertices );
		sum += center[axis];
	}

	return sum / faces.size();
}


/**
 * Find the midpoint on the longest axis of the node.
 * @param  {SphereTreeNode*} container
 * @param  {cl_uint*}        axis
 * @return {cl_float}
 */
cl_float SphereTree::findMidpoint( SphereTreeNode* container, cl_uint* axis ) {
	glm::vec3 bbLen = ( container->bbMax - container->bbMin );
	cl_float mp;

	if( bbLen[0] > bbLen[1] ) {
		mp = bbLen[0];
		*axis = 0;

		if( bbLen[2] > mp ) {
			mp = bbLen[2];
			*axis = 2;
		}
	}
	else {
		mp = bbLen[1];
		*axis = 1;

		if( bbLen[2] > mp ) {
			mp = bbLen[2];
			*axis = 2;
		}
	}

	return mp;
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
 * Calculate the bounding box from the given vertices.
 * @param {std::vector<cl_float4>} vertices
 * @param {glm::vec3}              bbMin
 * @param {glm::vec3}              bbMax
 */
void SphereTree::getBoundingBox( vector<cl_float4> vertices, glm::vec3* bbMin, glm::vec3* bbMax ) {
	*bbMin = glm::vec3( vertices[0].x, vertices[0].y, vertices[0].z );
	*bbMax = glm::vec3( vertices[0].x, vertices[0].y, vertices[0].z );

	for( int i = 1; i < vertices.size(); i++ ) {
		cl_float4 v = vertices[i];

		(*bbMin)[0] = ( (*bbMin)[0] < v.x ) ? (*bbMin)[0] : v.x;
		(*bbMin)[1] = ( (*bbMin)[1] < v.y ) ? (*bbMin)[1] : v.y;
		(*bbMin)[2] = ( (*bbMin)[2] < v.z ) ? (*bbMin)[2] : v.z;

		(*bbMax)[0] = ( (*bbMax)[0] > v.x ) ? (*bbMax)[0] : v.x;
		(*bbMax)[1] = ( (*bbMax)[1] > v.y ) ? (*bbMax)[1] : v.y;
		(*bbMax)[2] = ( (*bbMax)[2] > v.z ) ? (*bbMax)[2] : v.z;
	}
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
		msg, 128, "[SphereTree] Generated in %g ms. Contains %lu nodes.", timeDiff, mNodes.size()
	);
	Logger::logInfo( msg );
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

	mNodes.push_back( node );

	return node;
}


/**
 * Create a root node that can contain the created sub-trees.
 * @param  {std::vector<SphereTreeNode*>} subTrees
 * @return {SphereTreeNode*}
 */
SphereTreeNode* SphereTree::makeRootNode( vector<SphereTreeNode*> subTrees ) {
	SphereTreeNode* node = new SphereTreeNode;

	node->leftChild = NULL;
	node->rightChild = NULL;

	node->bbMin = glm::vec3( subTrees[0]->bbMin );
	node->bbMax = glm::vec3( subTrees[0]->bbMax );

	for( int i = 1; i < subTrees.size(); i++ ) {
		node->bbMin[0] = ( node->bbMin[0] < subTrees[i]->bbMin[0] ) ? node->bbMin[0] : subTrees[i]->bbMin[0];
		node->bbMin[1] = ( node->bbMin[1] < subTrees[i]->bbMin[1] ) ? node->bbMin[1] : subTrees[i]->bbMin[1];
		node->bbMin[2] = ( node->bbMin[2] < subTrees[i]->bbMin[2] ) ? node->bbMin[2] : subTrees[i]->bbMin[2];

		node->bbMax[0] = ( node->bbMax[0] > subTrees[i]->bbMax[0] ) ? node->bbMax[0] : subTrees[i]->bbMax[0];
		node->bbMax[1] = ( node->bbMax[1] > subTrees[i]->bbMax[1] ) ? node->bbMax[1] : subTrees[i]->bbMax[1];
		node->bbMax[2] = ( node->bbMax[2] > subTrees[i]->bbMax[2] ) ? node->bbMax[2] : subTrees[i]->bbMax[2];
	}

	mNodes.push_back( node );

	return node;
}
