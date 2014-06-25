#include "BVH.h"

using std::vector;


/**
 * Struct to use as comparator in std::find_if() for the faces (cl_uint4).
 */
struct findFace : std::unary_function<Tri, bool> {

	Tri* fc;

	/**
	 * Constructor.
	 * @param {cl_uint4*} fc Face to compare other faces with.
	 */
	findFace( Tri* fc ) : fc( fc ) {};

	/**
	 * Compare two faces.
	 * @param  {cl_uint4 const&} f
	 * @return {bool}              True, if they are identical, false otherwise.
	 */
	bool operator()( Tri const& tri ) const {
		return ( fc->face.x == tri.face.x && fc->face.y == tri.face.y && fc->face.z == tri.face.z );
	};

};


/**
 * Struct to use as comparator in std::sort() for the faces (cl_uint4).
 */
struct sortFacesCmp {

	cl_uint axis;
	vector<cl_float4>* v;

	/**
	 * Constructor.
	 * @param {cl_uint}                 axis        Axis to compare the faces on.
	 * @param {std::vector<cl_float4>*} allVertices List of vertices the faces have the indices for.
	 */
	sortFacesCmp( cl_uint axis, vector<cl_float4>* allVertices ) {
		this->axis = axis;
		this->v = allVertices;
	};

	/**
	 * Compare two faces by their centroids.
	 * @param  {Tri} a Indizes for vertices that describe a face.
	 * @param  {Tri} b Indizes for vertices that describe a face.
	 * @return {bool}  a < b
	 */
	bool operator()( Tri a, Tri b ) {
		glm::vec3 cenA = MathHelp::getTriangleCentroid(
			( *this->v )[a.face.x], ( *this->v )[a.face.y], ( *this->v )[a.face.z]
		);
		glm::vec3 cenB = MathHelp::getTriangleCentroid(
			( *this->v )[b.face.x], ( *this->v )[b.face.y], ( *this->v )[b.face.z]
		);

		return cenA[this->axis] < cenB[this->axis];
	};

};


/**
 * Build a sphere tree for each object in the scene and combine them into one big tree.
 * @param  {std::vector<object3D>} sceneObjects
 * @param  {std::vector<cl_float>} vertices
 * @param  {std::vector<cl_float>} normals
 * @return {BVH*}
 */
BVH::BVH( vector<object3D> sceneObjects, vector<cl_float> vertices, vector<cl_float> normals ) {
	boost::posix_time::ptime timerStart = boost::posix_time::microsec_clock::local_time();

	mDepthReached = 0;
	this->setMaxFaces( Cfg::get().value<cl_uint>( Cfg::BVH_MAXFACES ) );

	vector<BVHNode*> subTrees = this->buildTreesFromObjects( sceneObjects, vertices, normals );
	mRoot = this->makeContainerNode( subTrees, true );
	this->groupTreesToNodes( subTrees, mRoot, mDepthReached );
	this->combineNodes( subTrees );
	this->logStats( timerStart );
}


/**
 * Destructor.
 */
BVH::~BVH() {
	for( cl_uint i = 0; i < mNodes.size(); i++ ) {
		delete mNodes[i];
	}
}


/**
 * Assign faces to the created bins.
 * @param {std::vector<std::vector<Tri>>}  binFaces
 * @param {cl_uint}                             splits
 * @param {std::vector<std::vector<Tri>>*} leftBinFaces
 * @param {std::vector<std::vector<Tri>>*} rightBinFaces
 */
void BVH::assignFacesToBins(
	vector< vector<Tri> > binFaces, cl_uint splits,
	vector< vector<Tri> >* leftBinFaces, vector< vector<Tri> >* rightBinFaces
) {
	// Growing left group

	(*leftBinFaces)[0].assign( binFaces[0].begin(), binFaces[0].end() );

	for( int i = 1; i < splits; i++ ) {
		(*leftBinFaces)[i].assign( (*leftBinFaces)[i - 1].begin(), (*leftBinFaces)[i - 1].end() );
		(*leftBinFaces)[i].insert( (*leftBinFaces)[i].end(), binFaces[i].begin(), binFaces[i].end() );
		(*leftBinFaces)[i] = this->uniqueFaces( (*leftBinFaces)[i] );
	}

	// Growing right group

	(*rightBinFaces)[splits - 1].assign( binFaces[splits].begin(), binFaces[splits].end() );

	for( int i = splits - 2; i >= 0; i-- ) {
		(*rightBinFaces)[i].assign( (*rightBinFaces)[i + 1].begin(), (*rightBinFaces)[i + 1].end() );
		(*rightBinFaces)[i].insert( (*rightBinFaces)[i].end(), binFaces[i + 1].begin(), binFaces[i + 1].end() );
		(*rightBinFaces)[i] = this->uniqueFaces( (*rightBinFaces)[i] );
	}
}


/**
 * Build the sphere tree.
 * @param  {std::vector<Tri>}       faces
 * @param  {std::vector<cl_float4>} allVertices
 * @param  {cl_uint}                depth       The current depth of the node in the tree. Starts at 1.
 * @return {BVHNode*}
 */
BVHNode* BVH::buildTree(
	vector<Tri> faces, vector<cl_float4> allVertices, cl_uint depth,
	glm::vec3 bbMin, glm::vec3 bbMax, bool useGivenBB, cl_float rootSA
) {
	BVHNode* containerNode = this->makeNode( faces, allVertices );
	if( useGivenBB ) {
		containerNode->bbMin = glm::vec3( bbMin );
		containerNode->bbMax = glm::vec3( bbMax );
	}
	containerNode->depth = depth;
	mDepthReached = ( depth > mDepthReached ) ? depth : mDepthReached;

	// leaf node
	if( faces.size() <= mMaxFaces ) {
		if( faces.size() <= 0 ) {
			Logger::logWarning( "[BVH] No faces in node." );
		}
		containerNode->faces = faces;

		return containerNode;
	}

	vector<Tri> leftFaces, rightFaces;
	useGivenBB = false;
	glm::vec3 bbMinLeft, bbMaxLeft, bbMinRight, bbMaxRight;

	// SAH takes some time. Don't do it if there are too many faces.
	if( faces.size() <= Cfg::get().value<cl_uint>( Cfg::BVH_SAHFACESLIMIT ) ) {
		cl_float bestSAH_object = FLT_MAX;
		cl_float nodeSA = 1.0f / MathHelp::getSurfaceArea( containerNode->bbMin, containerNode->bbMax );
		cl_float alpha = 0.00001f;
		cl_float lambda;

		for( cl_uint axis = 0; axis <= 2; axis++ ) {
			this->splitBySAH(
				nodeSA, &bestSAH_object, axis, faces, allVertices,
				&leftFaces, &rightFaces, &lambda
			);
		}

		lambda /= rootSA;

		// printf( "Object SAH: %g\n", bestSAH_object );
		// printf(
		// 	"Node | min %g %g %g | max %g %g %g\n",
		// 	containerNode->bbMin[0], containerNode->bbMin[1], containerNode->bbMin[2],
		// 	containerNode->bbMax[0], containerNode->bbMax[1], containerNode->bbMax[2]
		// );
		// printf( "\n" );

		if( lambda > alpha && Cfg::get().value<cl_uint>( Cfg::BVH_SPATIALSPLITS ) > 0 ) {
			cl_float bestSAH_spatial = bestSAH_object;

			for( cl_uint axis = 0; axis <= 2; axis++ ) {
				this->splitBySpatialSplit(
					containerNode, axis, &bestSAH_spatial, faces, allVertices, &leftFaces, &rightFaces,
					&bbMinLeft, &bbMaxLeft, &bbMinRight, &bbMaxRight
				);
				// printf( "\n" );
			}

			useGivenBB = ( bestSAH_spatial < bestSAH_object );
			if( useGivenBB ) {
				printf( "SPATIAL SPLIT | faces | left: %lu | right: %lu\n", leftFaces.size(), rightFaces.size() );
			}
			// exit( 1 );
		}
	}
	// Faster to build: Splitting at the midpoint of the longest axis.
	else {
		Logger::logDebug( "[BVH] Too many faces in node for SAH. Splitting by midpoint." );

		cl_uint axis = this->longestAxis( containerNode );
		cl_float splitPos = this->findMidpoint( containerNode, axis );

		this->splitFaces( faces, allVertices, splitPos, axis, &leftFaces, &rightFaces );

		if( leftFaces.size() <= 0 || rightFaces.size() <= 0 ) {
			Logger::logDebug( "[BVH] Splitting faces by midpoint didn't work. Trying again with mean." );
			leftFaces.clear();
			rightFaces.clear();
			splitPos = this->findMean( faces, allVertices, axis );

			this->splitFaces( faces, allVertices, splitPos, axis, &leftFaces, &rightFaces );
		}
	}

	containerNode->leftChild = this->buildTree(
		leftFaces, allVertices, depth + 1, bbMinLeft, bbMaxLeft, useGivenBB, rootSA
	);
	containerNode->rightChild = this->buildTree(
		rightFaces, allVertices, depth + 1, bbMinRight, bbMaxRight, useGivenBB, rootSA
	);

	return containerNode;
}


/**
 * Build sphere trees for all given scene objects.
 * @param  {std::vector<object3D>} sceneObjects
 * @param  {std::vector<cl_float>} vertices
 * @return {std::vector<BVHNode*>}
 */
vector<BVHNode*> BVH::buildTreesFromObjects(
	vector<object3D> sceneObjects, vector<cl_float> vertices, vector<cl_float> normals
) {
	vector<BVHNode*> subTrees;
	char msg[256];
	cl_int offset = 0;
	cl_int offsetN = 0;

	vector<cl_float4> vertices4;


	for( cl_uint i = 0; i < vertices.size(); i += 3 ) {
		cl_float4 v = {
			vertices[i + 0],
			vertices[i + 1],
			vertices[i + 2],
			0.0f
		};
		vertices4.push_back( v );
	}

	for( cl_uint i = 0; i < sceneObjects.size(); i++ ) {
		vector<cl_uint4> facesThisObj;
		vector<Tri> triFaces;
		ModelLoader::getFacesOfObject( sceneObjects[i], &facesThisObj, offset );
		offset += facesThisObj.size();

		snprintf(
			msg, 256, "[BVH] Building tree %u/%lu: \"%s\". %lu faces.",
			i + 1, sceneObjects.size(), sceneObjects[i].oName.c_str(), facesThisObj.size()
		);
		Logger::logInfo( msg );


		vector<cl_uint4> faceNormalsThisObj;
		ModelLoader::getFaceNormalsOfObject( sceneObjects[i], &faceNormalsThisObj, offsetN );
		offsetN += faceNormalsThisObj.size();

		for( cl_uint j = 0; j < facesThisObj.size(); j++ ) {
			Tri tri;
			tri.face = facesThisObj[j];
			this->triCalcAABB( &tri, faceNormalsThisObj[j], vertices4, normals );
			triFaces.push_back( tri );
		}


		BVHNode* rootNode = this->makeNode( triFaces, vertices4 );
		cl_float rootSA = MathHelp::getSurfaceArea( rootNode->bbMin, rootNode->bbMax );

		glm::vec3 bbMin, bbMax;
		BVHNode* st = this->buildTree( triFaces, vertices4, 1, bbMin, bbMax, false, rootSA );
		subTrees.push_back( st );
	}

	return subTrees;
}


/**
 * Calculate the SAH value.
 * @param  {cl_float} nodeSA_recip  Reciprocal of the surface area of the container node.
 * @param  {cl_float} leftSA        Surface are of the left node.
 * @param  {cl_float} leftNumFaces  Number of faces in the left node.
 * @param  {cl_float} rightSA       Surface area of the right node.
 * @param  {cl_float} rightNumFaces Number of faces in the right node.
 * @return {cl_float}               Value estimated by the SAH.
 */
cl_float BVH::calcSAH(
	cl_float nodeSA_recip,
	cl_float leftSA, cl_float leftNumFaces,
	cl_float rightSA, cl_float rightNumFaces
) {
	return nodeSA_recip * ( leftSA * leftNumFaces + rightSA * rightNumFaces );
}


/**
 *
 * @param {glm::vec3}               p        Start of line segment.
 * @param {glm::vec3}               q        End of line segment.
 * @param {glm::vec3}               s        Point on the plane.
 * @param {glm::vec3}               nl       Normal of the plane.
 * @param {std::vector<cl_float4>*} vertices List to store the clipped line end in.
 */
void BVH::clipLine(
	glm::vec3 p, glm::vec3 q, glm::vec3 s, glm::vec3 nl,
	vector<cl_float4>* vertices
) {
	bool bad;
	glm::vec3 clipped = MathHelp::intersectLinePlane( p, q, s, nl, &bad );

	if( !bad ) {
		cl_float4 clipped_f4 = { clipped[0], clipped[1], clipped[2], 0.0f };
		vertices->push_back( clipped_f4 );
	}
}


/**
 * Clip faces of a box on the given axis and re-calculate the bounding box.
 * @param {std::vector<Tri>}   faces
 * @param {std::vector<cl_float4>}  allVertices
 * @param {cl_uint}                 axis
 * @param {std::vector<glm::vec3>*} binAABB
 */
void BVH::clippedFacesAABB(
	vector<Tri> faces, vector<cl_float4> allVertices, cl_uint axis, vector<glm::vec3>* binAABB
) {
	vector<cl_float4> vAABB;
	cl_float4 v0, v1, v2;
	glm::vec3 v0glm, v1glm, v2glm;

	glm::vec3 bbMin = (*binAABB)[0];
	glm::vec3 bbMax = (*binAABB)[1];

	glm::vec3 nl = glm::vec3( 0.0f, 0.0f, 0.0f );
	nl[axis] = 1.0f;

	for( cl_uint i = 0; i < faces.size(); i++ ) {
		v0 = allVertices[faces[i].face.x];
		v1 = allVertices[faces[i].face.y];
		v2 = allVertices[faces[i].face.z];
		v0glm = FLOAT4_TO_VEC3( v0 );
		v1glm = FLOAT4_TO_VEC3( v1 );
		v2glm = FLOAT4_TO_VEC3( v2 );


		// Face extends into previous or next bin

		bool isInPrev0 = ( v0glm[axis] < bbMin[axis] );
		bool isInNext0 = ( v0glm[axis] > bbMax[axis] );
		bool isInPrev1 = ( v1glm[axis] < bbMin[axis] );
		bool isInNext1 = ( v1glm[axis] > bbMax[axis] );
		bool isInPrev2 = ( v2glm[axis] < bbMin[axis] );
		bool isInNext2 = ( v2glm[axis] > bbMax[axis] );


		if( !isInPrev0 && !isInNext0 ) {
			vAABB.push_back( v0 );
		}
		if( ( isInPrev0 && !isInPrev1 ) || ( isInNext0 && !isInNext1 ) ) {
			this->clipLine( v0glm, v1glm, ( isInPrev0 ? bbMin : bbMax ), nl, &vAABB );
		}
		if( ( isInPrev0 && !isInPrev2 ) || ( isInNext0 && !isInNext2 ) ) {
			this->clipLine( v0glm, v2glm, ( isInPrev0 ? bbMin : bbMax ), nl, &vAABB );
		}


		if( !isInPrev1 && !isInNext1 ) {
			vAABB.push_back( v1 );
		}
		if( ( isInPrev1 && !isInPrev0 ) || ( isInNext1 && !isInNext0 ) ) {
			this->clipLine( v1glm, v0glm, ( isInPrev1 ? bbMin : bbMax ), nl, &vAABB );
		}
		if( ( isInPrev1 && !isInPrev2 ) || ( isInNext1 && !isInNext2 ) ) {
			this->clipLine( v1glm, v2glm, ( isInPrev1 ? bbMin : bbMax ), nl, &vAABB );
		}


		if( !isInPrev2 && !isInNext2 ) {
			vAABB.push_back( v2 );
		}
		if( ( isInPrev2 && !isInPrev0 ) || ( isInNext2 && !isInNext0 ) ) {
			this->clipLine( v2glm, v0glm, ( isInPrev2 ? bbMin : bbMax ), nl, &vAABB );
		}
		if( ( isInPrev2 && !isInPrev1 ) || ( isInNext2 && !isInNext1 ) ) {
			this->clipLine( v2glm, v1glm, ( isInPrev2 ? bbMin : bbMax ), nl, &vAABB );
		}
	}

	if( vAABB.size() > 0 ) {
		MathHelp::getAABB( vAABB, &bbMin, &bbMax );
	}

	(*binAABB)[0] = bbMin;
	(*binAABB)[1] = bbMax;
}


/**
 * Combine the container nodes, leaf nodes and the root node into one list.
 * The root node will be at the very beginning of the list.
 * @param {std::vector<BVHNode*>} subTrees The generated trees for each object.
 */
void BVH::combineNodes( vector<BVHNode*> subTrees ) {
	if( subTrees.size() > 1 ) {
		mNodes.push_back( mRoot );
	}
	mNodes.insert( mNodes.end(), mContainerNodes.begin(), mContainerNodes.end() );
	mNodes.insert( mNodes.end(), mLeafNodes.begin(), mLeafNodes.end() );

	for( cl_uint i = 0; i < mNodes.size(); i++ ) {
		mNodes[i]->id = i;
	}
}


/**
 * Find the mean of the triangles regarding the given axis.
 * @param  {std::vector<Tri>}       faces
 * @param  {std::vector<cl_float4>} allVertices
 * @param  {cl_uint}                axis
 * @return {cl_float}
 */
cl_float BVH::findMean( vector<Tri> faces, vector<cl_float4> allVertices, cl_uint axis ) {
	cl_float sum = 0.0f;
	cl_float4 v0, v1, v2;
	glm::vec3 center;

	for( cl_uint i = 0; i < faces.size(); i++ ) {
		v0 = allVertices[faces[i].face.x];
		v1 = allVertices[faces[i].face.y];
		v2 = allVertices[faces[i].face.z];
		glm::vec3 center = MathHelp::getTriangleCentroid( v0, v1, v2 );

		sum += center[axis];
	}

	return sum / faces.size();
}


/**
 * Find the mean of the centers of the given nodes.
 * @param  {std::vector<BVHNode*>} nodes
 * @param  {cl_uint}               axis
 * @return {cl_float}
 */
cl_float BVH::findMeanOfNodes( vector<BVHNode*> nodes, cl_uint axis ) {
	cl_float sum = 0.0f;

	for( cl_uint i = 0; i < nodes.size(); i++ ) {
		glm::vec3 center = ( nodes[i]->bbMax - nodes[i]->bbMin ) / 2.0f;
		sum += center[axis];
	}

	return sum / nodes.size();
}


/**
 * Find the midpoint on the longest axis of the node.
 * @param  {BVHNode*} container
 * @param  {cl_uint*} axis
 * @return {cl_float}
 */
cl_float BVH::findMidpoint( BVHNode* container, cl_uint axis ) {
	glm::vec3 sides = ( container->bbMax + container->bbMin ) / 2.0f;

	return sides[axis];
}


/**
 * Get the bounding boxes for the bins.
 * @param  {BVHNode*}                            node
 * @param  {std::vector<cl_float>}               splitPos
 * @param  {cl_uint}                             axis
 * @return {std::vector<std::vector<glm::vec3>>}
 */
vector< vector<glm::vec3> > BVH::getBinAABBs(
	BVHNode* node, vector<cl_float> splitPos, cl_uint axis
) {
	vector< vector<glm::vec3> > bins( splitPos.size() + 1 );

	// First bin
	bins[0] = vector<glm::vec3>( 2 );
	bins[0][0] = glm::vec3( node->bbMin );
	bins[0][1] = glm::vec3( node->bbMax );
	bins[0][1][axis] = splitPos[0];

	// All the bins between the first one and last one.
	for( cl_uint i = 1; i < bins.size() - 1; i++ ) {
		bins[i] = vector<glm::vec3>( 2 );
		// bbMin
		bins[i][0] = glm::vec3( bins[i - 1][0] );
		bins[i][0][axis] = splitPos[i - 1];
		// bbMax
		bins[i][1] = glm::vec3( node->bbMax );
		bins[i][1][axis] = splitPos[i];
	}

	// Last bin
	bins[bins.size() - 1] = vector<glm::vec3>( 2 );
	bins[bins.size() - 1][0] = glm::vec3( bins[bins.size() - 2][0] );
	bins[bins.size() - 1][0][axis] = splitPos[splitPos.size() - 1];
	bins[bins.size() - 1][1] = glm::vec3( node->bbMax );

	return bins;
}


/**
 * Get the faces for each bin.
 * @param  {std::vector<Tri>}               faces
 * @param  {std::vector<cl_float4>}              allVertices
 * @param  {std::vector<std::vector<glm::vec3>>} bins
 * @param  {cl_uint}                             axis
 * @return {std::vector<std::vector<Tri>>}
 */
vector< vector<Tri> > BVH::getBinFaces(
	vector<Tri> faces, vector<cl_float4> allVertices, vector< vector<glm::vec3> > bins, cl_uint axis
) {
	vector< vector<Tri> > binFaces( bins.size() );
	cl_float4 v0, v1, v2;
	glm::vec3 bbMin, bbMax;

	for( cl_uint i = 0; i < faces.size(); i++ ) {
		v0 = allVertices[faces[i].face.x];
		v1 = allVertices[faces[i].face.y];
		v2 = allVertices[faces[i].face.z];

		MathHelp::getTriangleAABB( v0, v1, v2, &bbMin, &bbMax );

		// First bin
		if( bbMin[axis] <= bins[0][1][axis] ) {
			binFaces[0].push_back( faces[i] );
		}

		// All the bins in between
		for( cl_uint j = 1; j < bins.size() - 1; j++ ) {
			if( bbMin[axis] <= bins[j][1][axis] && bbMax[axis] >= bins[j][0][axis] ) {
				binFaces[j].push_back( faces[i] );
			}
		}

		// Last bin
		if( bbMax[axis] >= bins[bins.size() - 1][0][axis] ) {
			binFaces[bins.size() - 1].push_back( faces[i] );
		}
	}

	return binFaces;
}


/**
 * Get the positions to generate bins at on the given axis for the given node.
 * @param  {BVHNode*}              node
 * @param  {cl_uint}               splits
 * @param  {cl_uint}               axis
 * @return {std::vector<cl_float>}
 */
vector<cl_float> BVH::getBinSplits( BVHNode* node, cl_uint splits, cl_uint axis ) {
	vector<cl_float> pos( splits );
	cl_float lenSegment = ( node->bbMax[axis] - node->bbMin[axis] ) / ( (cl_float) splits + 1.0f );

	pos[0] = node->bbMin[axis] + lenSegment;

	for( cl_uint i = 1; i < splits; i++ ) {
		pos[i] = pos[i - 1] + lenSegment;
	}

	return pos;
}


/**
 * Get all container nodes (all nodes that aren't leaves).
 * @return {std::vector<BVHNode*>} List of all container nodes.
 */
vector<BVHNode*> BVH::getContainerNodes() {
	return mContainerNodes;
}


/**
 * Get the max reached depth.
 * @return {cl_uint} The max reached depth.
 */
cl_uint BVH::getDepth() {
	return mDepthReached;
}


/**
 * Get all leaf nodes.
 * @return {std::vector<BVHNode*>} List of all leaf nodes.
 */
vector<BVHNode*> BVH::getLeafNodes() {
	return mLeafNodes;
}


/**
 * Get all nodes (container and leaf nodes).
 * The first node in the list is the root node.
 * @return {std::vector<BVHNode*>} List of all nodes.
 */
vector<BVHNode*> BVH::getNodes() {
	return mNodes;
}


/**
 * Get the root node.
 * @return {BVHNode*} The root node.
 */
BVHNode* BVH::getRoot() {
	return mRoot;
}


/**
 * Group the sphere nodes into two groups and assign them to the given parent node.
 * @param {std::vector<BVHNode*>} nodes
 * @param {BVHNode*}              parent
 * @param {cl_uint}               depth
 */
void BVH::groupTreesToNodes( vector<BVHNode*> nodes, BVHNode* parent, cl_uint depth ) {
	if( nodes.size() == 1 ) {
		// Implies: parent == nodes[0], nothing to do
		return;
	}

	parent->depth = depth;
	mDepthReached = ( depth > mDepthReached ) ? depth : mDepthReached;

	cl_uint axis = this->longestAxis( parent );
	cl_float midpoint = this->findMidpoint( parent, axis );

	vector<BVHNode*> leftGroup, rightGroup;
	this->splitNodes( nodes, midpoint, axis, &leftGroup, &rightGroup );

	if( leftGroup.size() <= 0 || rightGroup.size() <= 0 ) {
		leftGroup.clear();
		rightGroup.clear();
		cl_float mean = this->findMeanOfNodes( nodes, axis );

		this->splitNodes( nodes, mean, axis, &leftGroup, &rightGroup );
	}

	BVHNode* leftNode = this->makeContainerNode( leftGroup, false );
	parent->leftChild = leftNode;
	this->groupTreesToNodes( leftGroup, parent->leftChild, depth + 1 );

	BVHNode* rightNode = this->makeContainerNode( rightGroup, false );
	parent->rightChild = rightNode;
	this->groupTreesToNodes( rightGroup, parent->rightChild, depth + 1 );
}


/**
 * Grow bounding boxes from the bins.
 * @param {std::vector<std::vector<glm::vec3>>}  binBBs
 * @param {std::vector<std::vector<Tri>>}   binFaces
 * @param {cl_uint}                              splits
 * @param {std::vector<std::vector<glm::vec3>>*} leftBB
 * @param {std::vector<std::vector<glm::vec3>>*} rightBB
 */
void BVH::growBinAABBs(
	vector< vector<glm::vec3> > binBBs, vector< vector<Tri> > binFaces,
	cl_uint splits,
	vector< vector<glm::vec3> >* leftBB, vector< vector<glm::vec3> >* rightBB
) {
	// Growing bounding boxes from the left

	(*leftBB)[0] = vector<glm::vec3>( 2 );
	(*leftBB)[0][0] = glm::vec3( binBBs[0][0] );
	(*leftBB)[0][1] = glm::vec3( binBBs[0][1] );

	for( int i = 1; i < splits; i++ ) {
		(*leftBB)[i] = vector<glm::vec3>( 2 );

		(*leftBB)[i][0] = (*leftBB)[i - 1][0];
		(*leftBB)[i][1] = (*leftBB)[i - 1][1];

		if( binFaces[i].size() > 0 ) {
			(*leftBB)[i][0][0] = glm::min( binBBs[i][0][0], (*leftBB)[i][0][0] );
			(*leftBB)[i][0][1] = glm::min( binBBs[i][0][1], (*leftBB)[i][0][1] );
			(*leftBB)[i][0][2] = glm::min( binBBs[i][0][2], (*leftBB)[i][0][2] );

			(*leftBB)[i][1][0] = glm::max( binBBs[i][1][0], (*leftBB)[i][1][0] );
			(*leftBB)[i][1][1] = glm::max( binBBs[i][1][1], (*leftBB)[i][1][1] );
			(*leftBB)[i][1][2] = glm::max( binBBs[i][1][2], (*leftBB)[i][1][2] );
		}
	}

	// Growing bounding boxes from the right

	(*rightBB)[splits - 1] = vector<glm::vec3>( 2 );
	(*rightBB)[splits - 1][0] = glm::vec3( binBBs[splits][0] );
	(*rightBB)[splits - 1][1] = glm::vec3( binBBs[splits][1] );

	for( int i = splits - 2; i >= 0; i-- ) {
		(*rightBB)[i] = vector<glm::vec3>( 2 );

		(*rightBB)[i][0] = glm::vec3( (*rightBB)[i + 1][0] );
		(*rightBB)[i][1] = glm::vec3( (*rightBB)[i + 1][1] );

		if( binFaces[i + 1].size() > 0 ) {
			(*rightBB)[i][0][0] = glm::min( binBBs[i + 1][0][0], (*rightBB)[i][0][0] );
			(*rightBB)[i][0][1] = glm::min( binBBs[i + 1][0][1], (*rightBB)[i][0][1] );
			(*rightBB)[i][0][2] = glm::min( binBBs[i + 1][0][2], (*rightBB)[i][0][2] );

			(*rightBB)[i][1][0] = glm::max( binBBs[i + 1][1][0], (*rightBB)[i][1][0] );
			(*rightBB)[i][1][1] = glm::max( binBBs[i + 1][1][1], (*rightBB)[i][1][1] );
			(*rightBB)[i][1][2] = glm::max( binBBs[i + 1][1][2], (*rightBB)[i][1][2] );
		}
	}
}


/**
 * Log some stats.
 * @param {boost::posix_time::ptime} timerStart
 */
void BVH::logStats( boost::posix_time::ptime timerStart ) {
	boost::posix_time::ptime timerEnd = boost::posix_time::microsec_clock::local_time();
	cl_float timeDiff = ( timerEnd - timerStart ).total_milliseconds();

	char msg[256];
	snprintf(
		msg, 256, "[BVH] Generated in %g ms. Contains %lu nodes (%lu leaves). Max faces of %u. Max depth of %u.",
		timeDiff, mNodes.size(), mLeafNodes.size(), mMaxFaces, mDepthReached
	);
	Logger::logInfo( msg );
}


/**
 * Get the index of the longest axis.
 * @param  {BVHNode*} node The node to find the longest side of.
 * @return {cl_uint}       Index of the longest axis (X: 0, Y: 1, Z: 2).
 */
cl_uint BVH::longestAxis( BVHNode* node ) {
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
 * @param  {std::vector<BVHNode*>} subTrees
 * @return {BVHNode*}
 */
BVHNode* BVH::makeContainerNode( vector<BVHNode*> subTrees, bool isRoot ) {
	if( subTrees.size() == 1 ) {
		return subTrees[0];
	}

	BVHNode* node = new BVHNode;

	node->leftChild = NULL;
	node->rightChild = NULL;
	node->depth = 0;

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
 * @param  {std::vector<Tri>} tris
 * @param  {std::vector<cl_float>} allVertices Vertices to compute the bounding box from.
 * @return {BVHNode*}
 */
BVHNode* BVH::makeNode( vector<Tri> tris, vector<cl_float4> allVertices ) {
	BVHNode* node = new BVHNode;
	node->leftChild = NULL;
	node->rightChild = NULL;
	node->depth = 0;

	vector<cl_float4> vertices;
	for( cl_uint i = 0; i < tris.size(); i++ ) {
		vertices.push_back( allVertices[tris[i].face.x] );
		vertices.push_back( allVertices[tris[i].face.y] );
		vertices.push_back( allVertices[tris[i].face.z] );
	}

	glm::vec3 bbMin;
	glm::vec3 bbMax;
	MathHelp::getAABB( vertices, &bbMin, &bbMax );
	node->bbMin = bbMin;
	node->bbMax = bbMax;

	if( tris.size() <= mMaxFaces ) {
		mLeafNodes.push_back( node );
	}
	else {
		mContainerNodes.push_back( node );
	}

	return node;
}


/**
 * Set the number of max faces per (leaf) node.
 * @param  {cl_uint} value Max faces per (leaf) node.
 * @return {cl_uint}       The now set number of max faces per (leaf) node.
 */
cl_uint BVH::setMaxFaces( cl_uint value ) {
	mMaxFaces = ( value < 1 || value > 4 ) ? 4 : value;

	return mMaxFaces;
}


/**
 * Split the faces into two groups.
 * Determine this splitting point by using a Surface Area Heuristic (SAH).
 * @param {cl_float}               nodeSA      Reciprocal of the surface area of the current node.
 * @param {cl_float*}              bestSAH     Best SAH value that has been found so far (for the faces in this node).
 * @param {cl_uint}                axis        The axis to sort the faces by.
 * @param {std::vector<Tri>}  faces       Faces in this node.
 * @param {std::vector<cl_float4>} allVertices All vertices in the model.
 * @param {std::vector<Tri>*} leftFaces   Output. Group for the faces left of the split.
 * @param {std::vector<Tri>*} rightFaces  Output. Group for the faces right of the split.
 */
void BVH::splitBySAH(
	cl_float nodeSA, cl_float* bestSAH, cl_uint axis, vector<Tri> faces, vector<cl_float4> allVertices,
	vector<Tri>* leftFaces, vector<Tri>* rightFaces, cl_float* lambda
) {
	std::sort( faces.begin(), faces.end(), sortFacesCmp( axis, &allVertices ) );
	int numFaces = faces.size();

	vector<cl_float> leftSA( numFaces - 1 );
	vector<cl_float> rightSA( numFaces - 1 );
	vector<cl_float4> vertsForSA;

	vector< vector<glm::vec3> > leftBB( numFaces - 1 ), rightBB( numFaces - 1 );


	// Grow a bounding box face by face starting from the left.
	// Save the growing surface area for each step.

	for( int i = 0; i < numFaces - 1; i++ ) {
		glm::vec3 bbMin, bbMax;
		vertsForSA.push_back( allVertices[faces[i].face.x] );
		vertsForSA.push_back( allVertices[faces[i].face.y] );
		vertsForSA.push_back( allVertices[faces[i].face.z] );

		MathHelp::getAABB( vertsForSA, &bbMin, &bbMax );
		leftBB[i] = vector<glm::vec3>( 2 );
		leftBB[i][0] = bbMin;
		leftBB[i][1] = bbMax;
		leftSA[i] = MathHelp::getSurfaceArea( bbMin, bbMax );
	}


	// Grow a bounding box face by face starting from the right.
	// Save the growing surface area for each step.

	vertsForSA.clear();

	for( int i = numFaces - 2; i >= 0; i-- ) {
		glm::vec3 bbMin, bbMax;
		vertsForSA.push_back( allVertices[faces[i + 1].face.x] );
		vertsForSA.push_back( allVertices[faces[i + 1].face.y] );
		vertsForSA.push_back( allVertices[faces[i + 1].face.z] );

		MathHelp::getAABB( vertsForSA, &bbMin, &bbMax );
		rightBB[i] = vector<glm::vec3>( 2 );
		rightBB[i][0] = bbMin;
		rightBB[i][1] = bbMax;
		rightSA[i] = MathHelp::getSurfaceArea( bbMin, bbMax );
	}


	// Compute the SAH for each split position and choose the one with the lowest cost.
	// SAH = SA of node * ( SA left of split * faces left of split + SA right of split * faces right of split )

	int indexSplit = -1;
	cl_float newSAH, numFacesLeft, numFacesRight;

	for( cl_uint i = 0; i < numFaces - 1; i++ ) {
		newSAH = this->calcSAH(
			nodeSA, leftSA[i], (cl_float) ( i + 1 ),
			rightSA[i], (cl_float) ( numFaces - i - 1 )
		);

		// Better split position found
		if( newSAH < *bestSAH ) {
			*bestSAH = newSAH;
			// Exclusive this index, up to this face it is preferable to split
			indexSplit = i + 1;
		}
	}


	// If a splitting index has been found, split the faces into two groups.

	if( indexSplit >= 0 ) {
		leftFaces->clear();
		rightFaces->clear();

		// Overlapping surface area
		int j = indexSplit - 1;
		glm::vec3 diff = rightBB[j][1] - ( rightBB[j][1] - leftBB[j][1] );
		*lambda = diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2];

		for( cl_uint i = 0; i < indexSplit; i++ ) {
			leftFaces->push_back( faces[i] );
		}

		for( cl_uint i = indexSplit; i < numFaces; i++ ) {
			rightFaces->push_back( faces[i] );
		}
	}
}


/**
 * Find the best splitting position while allowing spatial splits.
 * @param {BVHNode*}               node
 * @param {cl_uint}                axis
 * @param {cl_float*}              sahBest
 * @param {std::vector<Tri>}  faces
 * @param {std::vector<cl_float4>} allVertices
 * @param {std::vector<Tri>*} leftFaces
 * @param {std::vector<Tri>*} rightFaces
 * @param {glm::vec3*}             bbMinLeft
 * @param {glm::vec3*}             bbMaxLeft
 * @param {glm::vec3*}             bbMinRight
 * @param {glm::vec3*}             bbMaxRight
 */
void BVH::splitBySpatialSplit(
	BVHNode* node, cl_uint axis, cl_float* sahBest, vector<Tri> faces, vector<cl_float4> allVertices,
	vector<Tri>* leftFaces, vector<Tri>* rightFaces,
	glm::vec3* bbMinLeft, glm::vec3* bbMaxLeft, glm::vec3* bbMinRight, glm::vec3* bbMaxRight
) {
	if( node->bbMax[axis] - node->bbMin[axis] < 0.0001f ) {
		return;
	}


	cl_uint splits = Cfg::get().value<cl_uint>( Cfg::BVH_SPATIALSPLITS );
	vector<cl_float> splitPos = this->getBinSplits( node, splits, axis );

	// Create AABBs for bins
	vector< vector<glm::vec3> > binBBs = this->getBinAABBs( node, splitPos, axis );

	// Assign face
	vector< vector<Tri> > binFaces = this->getBinFaces( faces, allVertices, binBBs, axis );

	// Clip faces and shrink AABB
	for( int i = 0; i < binBBs.size(); i++ ) {
		this->clippedFacesAABB( binFaces[i], allVertices, axis, &(binBBs[i]) );
	}

	// Faces for different combinations of bins
	vector< vector<Tri> > leftBinFaces( splits ), rightBinFaces( splits );
	this->assignFacesToBins( binFaces, splits, &leftBinFaces, &rightBinFaces );

	// Bounding boxes for the bin combinations
	vector< vector<glm::vec3> > leftBB( splits ), rightBB( splits );
	this->growBinAABBs( binBBs, binFaces, splits, &leftBB, &rightBB );


	// Surface areas for SAH

	cl_float saNode_recip = 1.0f / MathHelp::getSurfaceArea( node->bbMin, node->bbMax );
	vector<cl_float> leftSA( splits ), rightSA( splits );

	for( int i = 0; i < splits; i++ ) {
		leftSA[i] = MathHelp::getSurfaceArea( leftBB[i][0], leftBB[i][1] );
		rightSA[i] = MathHelp::getSurfaceArea( rightBB[i][0], rightBB[i][1] );
	}


	// Calculate and compare SAH values

	vector<cl_float> sah( splits );
	cl_float currentBestSAH = FLT_MAX;
	int index = -1;

	for( int i = 0; i < splits; i++ ) {
		if( leftBinFaces[i].size() == 0 || rightBinFaces[i].size() == 0 ) {
			continue;
		}

		sah[i] = this->calcSAH(
			saNode_recip, leftSA[i], leftBinFaces[i].size(), rightSA[i], rightBinFaces[i].size()
		);

		if( sah[i] < currentBestSAH ) {
			currentBestSAH = sah[i];
			index = i;
		}
	}

	printf( "-> Axis: %u | Spatial SAH: %g\n", axis, currentBestSAH );


	// Choose best split
	if( currentBestSAH == sah[index] && currentBestSAH < *sahBest ) {
		if( leftBinFaces[index].size() == faces.size() || rightBinFaces[index].size() == faces.size() ) {
			return;
		}

		leftFaces->assign( leftBinFaces[index].begin(), leftBinFaces[index].end() );
		rightFaces->assign( rightBinFaces[index].begin(), rightBinFaces[index].end() );

		*bbMinLeft = leftBB[index][0];
		*bbMaxLeft = leftBB[index][1];
		*bbMinRight = rightBB[index][0];
		*bbMaxRight = rightBB[index][1];

		*sahBest = currentBestSAH;
	}
}


/**
 * Split the faces into two groups using the given midpoint and axis as criterium.
 * @param {std::vector<Tri>}  faces
 * @param {std::vector<cl_float4>} vertices
 * @param {cl_float}               midpoint
 * @param {cl_uint}                axis
 * @param {std::vector<Tri>*} leftFaces
 * @param {std::vector<Tri>*} rightFaces
 */
void BVH::splitFaces(
	vector<Tri> faces, vector<cl_float4> vertices, cl_float midpoint, cl_uint axis,
	vector<Tri>* leftFaces, vector<Tri>* rightFaces
) {
	Tri tri;
	cl_float4 v0, v1, v2;
	glm::vec3 cen;

	for( cl_uint i = 0; i < faces.size(); i++ ) {
		tri = faces[i];
		v0 = vertices[tri.face.x];
		v1 = vertices[tri.face.y];
		v2 = vertices[tri.face.z];
		cen = MathHelp::getTriangleCentroid( v0, v1, v2 );

		if( cen[axis] < midpoint ) {
			leftFaces->push_back( tri );
		}
		else {
			rightFaces->push_back( tri );
		}
	}

	// One group has no children. We cannot allow that.
	// Try again with the triangle center instead of the centroid.
	if( leftFaces->size() == 0 || rightFaces->size() == 0 ) {
		Logger::logDebugVerbose( "[BVH] Dividing faces by centroid left one side empty. Trying again with center." );

		leftFaces->clear();
		rightFaces->clear();

		for( cl_uint i = 0; i < faces.size(); i++ ) {
			tri = faces[i];
			v0 = vertices[tri.face.x];
			v1 = vertices[tri.face.y];
			v2 = vertices[tri.face.z];
			cen = MathHelp::getTriangleCenter( v0, v1, v2 );

			if( cen[axis] < midpoint ) {
				leftFaces->push_back( tri );
			}
			else {
				rightFaces->push_back( tri );
			}
		}
	}

	// Oh, come on! Just do it 50:50 then.
	if( leftFaces->size() == 0 || rightFaces->size() == 0 ) {
		Logger::logDebugVerbose( "[BVH] Dividing faces by center left one side empty. Just doing it 50:50 now." );

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
			msg, 256, "[BVH] Dividing faces 50:50 left one side empty. Faces: %lu. Vertices: %lu.",
			faces.size(), vertices.size()
		);
		Logger::logError( msg );
	}
}


/**
 * Split the nodes into two groups using the given midpoint and axis as criterium.
 * @param {std::vector<BVHNode*>}  nodes
 * @param {cl_float}               midpoint
 * @param {cl_uint}                axis
 * @param {std::vector<BVHNode*>*} leftGroup
 * @param {std::vector<BVHNode*>*} rightGroup
 */
void BVH::splitNodes(
	vector<BVHNode*> nodes, cl_float midpoint, cl_uint axis,
	vector<BVHNode*>* leftGroup, vector<BVHNode*>* rightGroup
) {
	for( cl_uint i = 0; i < nodes.size(); i++ ) {
		BVHNode* node = nodes[i];
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
		Logger::logDebugVerbose( "[BVH] Dividing nodes by center left one side empty. Just doing it 50:50 now." );

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
			msg, 256, "[BVH] Dividing nodes 50:50 left one side empty. Nodes: %lu.", nodes.size()
		);
		Logger::logError( msg );
	}
}


/**
 * Calculate and set the AABB for a Tri (face).
 * @param {Tri}                    tri
 * @param {std::vector<cl_float4>} vertices
 */
void BVH::triCalcAABB(
	Tri* tri, cl_uint4 fn, vector<cl_float4> vertices, vector<cl_float> normals
) {
	vector<cl_float4> v;
	v.push_back( vertices[tri->face.x] );
	v.push_back( vertices[tri->face.y] );
	v.push_back( vertices[tri->face.z] );

	glm::vec3 bbMin, bbMax;
	MathHelp::getAABB( v, &bbMin, &bbMax );
	tri->bbMin = bbMin;
	tri->bbMax = bbMax;

	// ALPHA <= 0.0, no Phong Tessellation
	if( Cfg::get().value<float>( Cfg::RENDER_PHONGTESS ) <= 0.0f ) {
		return;
	}

	glm::vec3 p1 = glm::vec3( v[0].x, v[0].y, v[0].z );
	glm::vec3 p2 = glm::vec3( v[1].x, v[1].y, v[1].z );
	glm::vec3 p3 = glm::vec3( v[2].x, v[2].y, v[2].z );

	glm::vec3 n1 = glm::vec3(
		normals[fn.x * 3],
		normals[fn.x * 3 + 1],
		normals[fn.x * 3 + 2]
	);
	glm::vec3 n2 = glm::vec3(
		normals[fn.y * 3],
		normals[fn.y * 3 + 1],
		normals[fn.y * 3 + 2]
	);
	glm::vec3 n3 = glm::vec3(
		normals[fn.z * 3],
		normals[fn.z * 3 + 1],
		normals[fn.z * 3 + 2]
	);

	// Normals are the same, which means no Phong Tessellation possible
	glm::vec3 test = ( n1 - n2 ) + ( n2 - n3 );
	if(
		fabs( test.x ) <= 0.00001f &&
		fabs( test.y ) <= 0.00001f &&
		fabs( test.z ) <= 0.00001f
	) {
		return;
	}

	float thickness;
	glm::vec3 sidedrop;
	this->triThicknessAndSidedrop( p1, p2, p3, n1, n2, n3, &thickness, &sidedrop );
}


void BVH::triThicknessAndSidedrop(
	glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
	glm::vec3 n1, glm::vec3 n2, glm::vec3 n3,
	float* thickness, glm::vec3* sidedrop
) {
	float alpha = Cfg::get().value<float>( Cfg::RENDER_PHONGTESS );

	glm::vec3 e12 = p2 - p1;
	glm::vec3 e13 = p3 - p1;
	glm::vec3 e23 = p3 - p2;
	glm::vec3 e31 = p1 - p3;
	glm::vec3 c12 = alpha * ( glm::dot( n2, e12 ) * n2 - glm::dot( n1, e12 ) * n1 );
	glm::vec3 c23 = alpha * ( glm::dot( n3, e23 ) * n3 - glm::dot( n2, e23 ) * n2 );
	glm::vec3 c31 = alpha * ( glm::dot( n1, e31 ) * n1 - glm::dot( n3, e31 ) * n3 );
	glm::vec3 ng = glm::normalize( glm::cross( e12, e13 ) );

	float k_tmp = glm::dot( ng, c12 - c23 - c31 );
	float k = 1.0f / ( 4.0f * glm::dot( ng, c23 ) * glm::dot( ng, c31 ) - k_tmp * k_tmp );

	float u = k * (
		2.0f * glm::dot( ng, c23 ) * glm::dot( ng, c31 + e31 ) +
		glm::dot( ng, c23 - e23 ) * glm::dot( ng, c12 - c23 - c31 )
	);
	float v = k * (
		2.0f * glm::dot( ng, c31 ) * glm::dot( ng, c23 - e23 ) +
		glm::dot( ng, c31 + e31 ) * glm::dot( ng, c12 - c23 - c31 )
	);

	u = ( u < 0.0f || u > 1.0f ) ? 0.0f : u;
	v = ( v < 0.0f || v > 1.0f ) ? 0.0f : v;

	glm::vec3 pt = this->phongTessellate( p1, p2, p3, n1, n2, n3, alpha, u, v );

	*thickness = glm::dot( ng, pt - p1 );
	sidedrop->x = glm::length( glm::cross( p2 - pt, p1 - pt ) ) / glm::length( e12 );
	sidedrop->y = glm::length( glm::cross( p3 - pt, p2 - pt ) ) / glm::length( e23 );
	sidedrop->z = glm::length( glm::cross( p1 - pt, p3 - pt ) ) / glm::length( e31 );
}


glm::vec3 BVH::phongTessellate(
	glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
	glm::vec3 n1, glm::vec3 n2, glm::vec3 n3,
	float alpha, float u, float v
) {
	float w = 1.0f - u - v;
	glm::vec3 pBary = p1 * u + p2 * v + p3 * w;
	glm::vec3 pTessellated =
		u * MathHelp::projectOnPlane( pBary, p1, n1 ) +
		v * MathHelp::projectOnPlane( pBary, p2, n2 ) +
		w * MathHelp::projectOnPlane( pBary, p3, n3 );

	return ( 1.0f - alpha ) * pBary + alpha * pTessellated;
}


/**
 * Remove duplicate faces from a list.
 * @param  {std::vector<Tri>} faces List of faces.
 * @return {std::vector<Tri>}       Duplicate free list.
 */
vector<Tri> BVH::uniqueFaces( vector<Tri> faces ) {
	vector<Tri> uniqueFaces;

	for( cl_uint i = 0; i < faces.size(); i++ ) {
		if( find_if( uniqueFaces.begin(), uniqueFaces.end(), findFace( &(faces[i]) ) ) == uniqueFaces.end() ) {
			uniqueFaces.push_back( faces[i] );
		}
	}

	return uniqueFaces;
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
 * @param {BVHNode*}               node     Current node.
 * @param {std::vector<cl_float>*} vertices Vector to put the vertices into.
 * @param {std::vector<cl_uint>*}  indices  Vector to put the indices into.
 */
void BVH::visualizeNextNode( BVHNode* node, vector<cl_float>* vertices, vector<cl_uint>* indices ) {
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
