#include "PathTracer.h"

using std::string;
using std::vector;


/**
 * Constructor.
 */
PathTracer::PathTracer( GLWidget* parent ) {
	srand( (unsigned) time( 0 ) );

	mWidth = Cfg::get().value<cl_uint>( Cfg::WINDOW_WIDTH );
	mHeight = Cfg::get().value<cl_uint>( Cfg::WINDOW_HEIGHT );

	mGLWidget = parent;
	mCL = NULL;

	mFOV = Cfg::get().value<cl_float>( Cfg::PERS_FOV );
	mSampleCount = 0;
	mTimeSinceStart = boost::posix_time::microsec_clock::local_time();

	mSunPos.x = 6.0f;
	mSunPos.y = 20.0f;
	mSunPos.z = 0.0f;
	mSunPos.w = 0.0f;
}


/**
 * Destructor.
 */
PathTracer::~PathTracer() {
	delete mCL;
}


/**
 * OpenCL: Find the paths in the scene and accumulate the colors of hit surfaces.
 * @param {cl_float} timeSinceStart Time since start of the program in seconds.
 */
void PathTracer::clPathTracing( cl_float timeSinceStart ) {
	cl_float pixelWeight = mSampleCount / (cl_float) ( mSampleCount + 1 );

	mCL->setKernelArg( mKernelPathTracing, 0, sizeof( cl_float ), &timeSinceStart );
	mCL->setKernelArg( mKernelPathTracing, 1, sizeof( cl_float ), &pixelWeight );
	mCL->setKernelArg( mKernelPathTracing, 2, sizeof( cl_float4 ), &mSunPos );

	mCL->execute( mKernelPathTracing );
	mCL->finish();
}


/**
 * Generate the path traced image, which is basically just a 2D texture.
 * @return {std::vector<cl_float>} Float vector representing a 2D image.
 */
vector<cl_float> PathTracer::generateImage() {
	this->updateEyeBuffer();
	mCL->updateImageReadOnly( mBufTextureIn, mWidth, mHeight, &mTextureOut[0] );

	cl_float timeSinceStart = this->getTimeSinceStart();
	this->clPathTracing( timeSinceStart );

	mCL->readImageOutput( mBufTextureOut, mWidth, mHeight, &mTextureOut[0] );
	mSampleCount++;

	return mTextureOut;
}


/**
 * Get the time in seconds since start of rendering.
 * @return {cl_float} Time since start of rendering.
 */
cl_float PathTracer::getTimeSinceStart() {
	boost::posix_time::time_duration msdiff = boost::posix_time::microsec_clock::local_time() - mTimeSinceStart;

	return msdiff.total_milliseconds() * 0.001f;
}


/**
 * Init the kernel arguments for the OpenCL kernel to do the path tracing
 * (find intersections and accumulate the color).
 */
void PathTracer::initArgsKernelPathTracing() {
	cl_float aspect = (cl_float) mWidth / (cl_float) mHeight;
	cl_float f = aspect * 2.0f * tan( MathHelp::degToRad( mFOV ) / 2.0f );
	cl_float pxDim = f / (cl_float) mWidth;

	char msg[128];
	snprintf( msg, 128, "[PathTracer] Aspect ratio: %g. Pixel size: %g", aspect, pxDim );
	Logger::logDebugVerbose( msg );

	cl_uint i = 0;
	i++; // 0: timeSinceStart
	i++; // 1: pixelWeight
	i++; // 2: sun position
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_float ), &pxDim );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufEye );

	switch( Cfg::get().value<int>( Cfg::ACCEL_STRUCT ) ) {

		case ACCELSTRUCT_BVH:
			mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufBVH );
			break;

		case ACCELSTRUCT_KDTREE:
			mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufBVH );
			mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufKdNonLeaves );
			mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufKdLeaves );
			mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufKdFaces );
			break;

		default:
			Logger::logError( "[PathTracer] Unknown acceleration structure." );
			exit( EXIT_FAILURE );

	}

	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufFaces );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufMaterials );

	if( Cfg::get().value<bool>( Cfg::USE_SPECTRAL ) ) {
		mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufSPDs );
	}

	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufTextureIn );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufTextureOut );
}


/**
 * Set the OpenCL kernel arguments that either don't change or point to a CL buffer.
 */
void PathTracer::initKernelArgs() {
	this->initArgsKernelPathTracing();
}


/**
 * Init the needed OpenCL buffers: Faces, vertices, camera eye and rays.
 * @param {std::vector<cl_float>} vertices   Vertices of the model.
 * @param {std::vector<cl_uint>}  faces      Faces (triangles) of the model.
 * @param {std::vector<cl_float>} normals    Normals of the model.
 * @param {ModelLoader*}          ml         Model loader already holding the needed model data.
 * @param {AccelStructure*}       accelStruc The generated acceleration structure.
 */
void PathTracer::initOpenCLBuffers(
	vector<cl_float> vertices, vector<cl_uint> faces, vector<cl_float> normals,
	ModelLoader* ml, AccelStructure* accelStruc
) {
	boost::posix_time::ptime timerStart;
	boost::posix_time::ptime timerEnd;
	cl_float timeDiff;
	size_t bytes;
	float bytesFloat;
	string unit;
	char msg[128];

	if( mCL != NULL ) {
		delete mCL;
	}
	mCL = new CL();

	Logger::logInfo( "[PathTracer] Initializing OpenCL buffers ..." );
	Logger::indent( LOG_INDENT );


	// Buffer: Faces
	timerStart = boost::posix_time::microsec_clock::local_time();
	bytes = this->initOpenCLBuffers_Faces( ml, vertices, faces, normals );
	timerEnd = boost::posix_time::microsec_clock::local_time();
	timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	utils::formatBytes( bytes, &bytesFloat, &unit );
	snprintf( msg, 128, "[PathTracer] Created faces buffer in %g ms -- %.2f %s.", timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	// Buffer: Acceleration Structure
	timerStart = boost::posix_time::microsec_clock::local_time();
	const short usedAccelStruct = Cfg::get().value<short>( Cfg::ACCEL_STRUCT );
	uint bvhDepth = 0;
	string accelName;

	if( usedAccelStruct == ACCELSTRUCT_BVH ) {
		bytes = this->initOpenCLBuffers_BVH( (BVH*) accelStruc );
		bvhDepth = ( (BVH*) accelStruc )->getDepth();
		accelName = "BVH";
	}
	else if( usedAccelStruct == ACCELSTRUCT_KDTREE ) {
		bytes = this->initOpenCLBuffers_KdTree( (BVHKdTree*) accelStruc );
		bvhDepth = ( (BVHKdTree*) accelStruc )->getDepth();
		accelName = "BVH/kD-tree";
	}

	timerEnd = boost::posix_time::microsec_clock::local_time();
	timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	utils::formatBytes( bytes, &bytesFloat, &unit );
	snprintf( msg, 128, "[PathTracer] Created %s buffer in %g ms -- %.2f %s", accelName.c_str(), timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	// Buffer: Material(s)
	timerStart = boost::posix_time::microsec_clock::local_time();
	bytes = this->initOpenCLBuffers_Materials( ml );
	timerEnd = boost::posix_time::microsec_clock::local_time();
	timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	utils::formatBytes( bytes, &bytesFloat, &unit );
	snprintf( msg, 128, "[PathTracer] Created material buffer in %g ms -- %.2f %s", timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	// Buffer: Rays
	timerStart = boost::posix_time::microsec_clock::local_time();
	bytes = this->initOpenCLBuffers_Rays();
	timerEnd = boost::posix_time::microsec_clock::local_time();
	timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	utils::formatBytes( bytes, &bytesFloat, &unit );
	snprintf( msg, 128, "[PathTracer] Created ray buffer in %g ms -- %.2f %s", timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	// Buffer: Textures
	timerStart = boost::posix_time::microsec_clock::local_time();
	bytes = this->initOpenCLBuffers_Textures();
	timerEnd = boost::posix_time::microsec_clock::local_time();
	timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	utils::formatBytes( bytes, &bytesFloat, &unit );
	snprintf( msg, 128, "[PathTracer] Created texture buffer in %g ms -- %.2f %s", timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	Logger::indent( 0 );
	Logger::logInfo( "[PathTracer] ... Done." );


	snprintf( msg, 128, "%u", bvhDepth );
	mCL->setReplacement( string( "#BVH_STACKSIZE#" ), string( msg ) );
	mCL->loadProgram( Cfg::get().value<string>( Cfg::OPENCL_PROGRAM ) );

	mKernelPathTracing = mCL->createKernel( "pathTracing" );
	mGLWidget->createKernelWindow( mCL );

	this->initKernelArgs();
}


/**
 * Init OpenCL buffers for the BVH.
 * @param {BVH*} bvh The generated Bounding Volume Hierarchy.
 */
size_t PathTracer::initOpenCLBuffers_BVH( BVH* bvh ) {
	vector<BVHNode*> bvhNodes = bvh->getNodes();
	vector<bvhNode_cl> bvhNodesCL;

	for( cl_uint i = 0; i < bvhNodes.size(); i++ ) {
		BVHNode* node = bvhNodes[i];

		cl_float4 bbMin = { node->bbMin[0], node->bbMin[1], node->bbMin[2], 0.0f };
		cl_float4 bbMax = { node->bbMax[0], node->bbMax[1], node->bbMax[2], 0.0f };

		bvhNode_cl sn;
		sn.bbMin = bbMin;
		sn.bbMax = bbMax;
		sn.bbMin.w = ( node->leftChild == NULL ) ? -1.0f : (cl_float) node->leftChild->id;
		sn.bbMax.w = ( node->rightChild == NULL ) ? -1.0f : (cl_float) node->rightChild->id;

		vector<Tri> facesVec = node->faces;
		cl_uint fvecLen = facesVec.size();
		sn.faces.x = ( fvecLen > 0 ) ? facesVec[0].face.w : -1;
		sn.faces.y = ( fvecLen > 1 ) ? facesVec[1].face.w : -1;
		sn.faces.z = ( fvecLen > 2 ) ? facesVec[2].face.w : -1;

		// No parent means it's the root node.
		if( node->parent == NULL ) {
			sn.faces.w = 0;
		}
		// Otherwise it is some other node, including leaves.
		else {
			bool isLeftNode = ( node->parent->leftChild == node );

			if( !isLeftNode ) {
				if( node->parent->parent == NULL ) {
					sn.faces.w = 0;
				}
				else {
					// As long as we are on the right side of a (sub)tree,
					// skip parents until we either are at the root or
					// our parent has a true sibling again.
					while( node->parent->parent->rightChild == node->parent ) {
						node->parent = node->parent->parent;

						if( node->parent->parent == NULL ) {
							break;
						}
					}

					// Reached the root node.
					if( node->parent->parent == NULL ) {
						sn.faces.w = 0;
					}
					// Reached a parent with a true sibling.
					else {
						sn.faces.w = node->parent->parent->rightChild->id * -1;
					}
				}
			}
			// Node on the left, go to the right sibling.
			else {
				sn.faces.w = node->parent->rightChild->id;
			}
		}

		bvhNodesCL.push_back( sn );
	}

	size_t bytesBVH = sizeof( bvhNode_cl ) * bvhNodesCL.size();
	mBufBVH = mCL->createBuffer( bvhNodesCL, bytesBVH );

	return bytesBVH;
}


/**
* Init OpenCL buffers for the kD-tree.
* @param {std::vector<cl_float>} vertices Vertices of the model.
* @param {std::vector<cl_uint>}  faces    Faces (triangles) of the model.
* @param {std::vector<kdNode_t>} kdNodes  Nodes of the kd-tree.
* @param {kdNode_t*}             rootNode Root node of the kd-tree.
*/
size_t PathTracer::initOpenCLBuffers_KdTree( BVHKdTree* bvhKdTree ) {
	map<cl_uint, KdTree*> nodeToKdTree = bvhKdTree->getMapNodeToKdTree();
	vector<BVHNode*> bvhNodes = bvhKdTree->getNodes();
	vector<bvhKdTreeNode_cl> bvhNodesCL;
	cl_int offsetNonLeaves = 0;

	for( cl_uint i = 0; i < bvhNodes.size(); i++ ) {
		BVHNode* node = bvhNodes[i];

		cl_float4 bbMin = { node->bbMin[0], node->bbMin[1], node->bbMin[2], 0.0f };
		cl_float4 bbMax = { node->bbMax[0], node->bbMax[1], node->bbMax[2], 0.0f };

		bvhKdTreeNode_cl sn;
		sn.bbMin = bbMin;
		sn.bbMax = bbMax;
		sn.bbMin.w = ( node->leftChild == NULL ) ? 0.0f : (cl_float) ( node->leftChild->id + 1 );
		sn.bbMax.w = ( node->rightChild == NULL ) ? 0.0f : (cl_float) ( node->rightChild->id + 1 );

		if( nodeToKdTree.find( node->id ) != nodeToKdTree.end() ) {
			sn.bbMin.w = (cl_float) -( offsetNonLeaves + 1 ); // Offset for the index of the kD-tree root node
			KdTree* kdTree = nodeToKdTree[node->id];
			offsetNonLeaves += kdTree->getNonLeaves().size();
		}

		bvhNodesCL.push_back( sn );
	}


	vector<kdNode_t> kdNodes;
	vector<kdNonLeaf_cl> kdNonLeaves;
	vector<kdLeaf_cl> kdLeaves;
	vector<cl_uint> kdFaces;
	vector<BVHNode*> bvhLeaves = bvhKdTree->getLeafNodes();
	cl_uint2 offset = { 0, 0 };

	for( uint i = 0; i < bvhLeaves.size(); i++ ) {
		kdNodes = nodeToKdTree[bvhLeaves[i]->id]->getNodes();
		this->kdNodesToVectors( kdNodes, &kdFaces, &kdNonLeaves, &kdLeaves, offset );
		offset.x = kdNonLeaves.size();
		offset.y = kdLeaves.size();
	}


	size_t bytesBVH = sizeof( bvhKdTreeNode_cl ) * bvhNodesCL.size();
	size_t bytesNonLeaves = sizeof( kdNonLeaf_cl ) * kdNonLeaves.size();
	size_t bytesLeaves = sizeof( kdLeaf_cl ) * kdLeaves.size();
	size_t bytesFaces = sizeof( cl_uint ) * kdFaces.size();
	mBufBVH = mCL->createBuffer( bvhNodesCL, bytesBVH );
	mBufKdNonLeaves = mCL->createBuffer( kdNonLeaves, bytesNonLeaves );
	mBufKdLeaves = mCL->createBuffer( kdLeaves, bytesLeaves );
	mBufKdFaces = mCL->createBuffer( kdFaces, bytesFaces );

	return bytesBVH + bytesNonLeaves + bytesLeaves + bytesFaces;
}


/**
* Store the kd-nodes data in seperate lists for each data type
* in order to pass it to OpenCL.
* @param {std::vector<KdNode_t>}      kdNodes     All nodes of the kd-tree.
* @param {std::vector<cl_float>*}     kdFaces     Data: [a, b, c ..., faceIndex]
* @param {std::vector<kdNonLeaf_cl>*} kdNonLeaves Output: Nodes of the kd-tree, that aren't leaves.
* @param {std::vector<kdLeaf_cl>*}    kdLeaves    Output: Leaf nodes of the kd-tree.
* @param {cl_uint2}                   offset
*/
void PathTracer::kdNodesToVectors(
	vector<kdNode_t> kdNodes, vector<cl_uint>* kdFaces, vector<kdNonLeaf_cl>* kdNonLeaves,
	vector<kdLeaf_cl>* kdLeaves, cl_uint2 offset
) {
	for( uint i = 0; i < kdNodes.size(); i++ ) {
		kdNode_t kdNode = kdNodes[i];

		// Non-leaf node
		if( kdNode.axis >= 0 ) {
			cl_int4 children = {
				kdNode.left->index,
				kdNode.right->index,
				( kdNode.left->axis < 0 ), // Is node a leaf node
				( kdNode.right->axis < 0 )
			};

			children.x += ( kdNode.left->axis < 0 ) ? offset.y : offset.x;
			children.y += ( kdNode.right->axis < 0 ) ? offset.y : offset.x;

			kdNonLeaf_cl node;
			node.axis = kdNode.axis;
			node.split = kdNode.split;
			node.children = children;
			kdNonLeaves->push_back( node );

			if( kdNode.faces.size() > 0 ) {
				Logger::logWarning( "[PathTracer] Converting kd-node data: Non-leaf node with faces." );
			}
		}
		// Leaf node
		else {
			kdLeaf_cl leaf;
			vector<kdNode_t*> nodeRopes = kdNode.ropes;

			cl_int8 ropes = {
				( nodeRopes[0] == NULL ) ? 0 : nodeRopes[0]->index + 1,
				( nodeRopes[1] == NULL ) ? 0 : nodeRopes[1]->index + 1,
				( nodeRopes[2] == NULL ) ? 0 : nodeRopes[2]->index + 1,
				( nodeRopes[3] == NULL ) ? 0 : nodeRopes[3]->index + 1,
				( nodeRopes[4] == NULL ) ? 0 : nodeRopes[4]->index + 1,
				( nodeRopes[5] == NULL ) ? 0 : nodeRopes[5]->index + 1,
				0, 0
			};

			// Set highest bit as flag for being a leaf node. (Use a negative value.)
			// The -1 isn't going to be a problem, because the entryDistance < exitDistance condition
			// in the kernel will stop the loop.
			ropes.s0 = ( ropes.s0 != 0 && nodeRopes[0]->axis < 0 ) ? -( ropes.s0 + offset.y ) : ropes.s0 + offset.x;
			ropes.s1 = ( ropes.s1 != 0 && nodeRopes[1]->axis < 0 ) ? -( ropes.s1 + offset.y ) : ropes.s1 + offset.x;
			ropes.s2 = ( ropes.s2 != 0 && nodeRopes[2]->axis < 0 ) ? -( ropes.s2 + offset.y ) : ropes.s2 + offset.x;
			ropes.s3 = ( ropes.s3 != 0 && nodeRopes[3]->axis < 0 ) ? -( ropes.s3 + offset.y ) : ropes.s3 + offset.x;
			ropes.s4 = ( ropes.s4 != 0 && nodeRopes[4]->axis < 0 ) ? -( ropes.s4 + offset.y ) : ropes.s4 + offset.x;
			ropes.s5 = ( ropes.s5 != 0 && nodeRopes[5]->axis < 0 ) ? -( ropes.s5 + offset.y ) : ropes.s5 + offset.x;

			// Index of faces of this node in kdFaces
			ropes.s6 = kdFaces->size();

			// Number of faces in this node
			ropes.s7 = kdNode.faces.size();

			leaf.ropes = ropes;

			// Bounding box
			cl_float4 bbMin = {
				kdNode.bbMin[0],
				kdNode.bbMin[1],
				kdNode.bbMin[2],
				0.0f
			};
			cl_float4 bbMax = {
				kdNode.bbMax[0],
				kdNode.bbMax[1],
				kdNode.bbMax[2],
				0.0f
			};
			leaf.bbMin = bbMin;
			leaf.bbMax = bbMax;

			kdLeaves->push_back( leaf );

			if( kdNode.faces.size() == 0 ) {
				Logger::logWarning( "[PathTracer] Converting kd-node data: Leaf node without any faces." );
			}

			// Faces
			for( uint j = 0; j < kdNode.faces.size(); j++ ) {
				kdFaces->push_back( kdNode.faces[j].face.w );
			}
		}
	}
}


/**
 * Init OpenCL buffers for the faces.
 * @param {ModelLoader*}          ml       Model loader holding the model data.
 * @param {std::vector<cl_float>} vertices Vertices of the model.
 * @param {std::vector<cl_uint>}  faces    Faces of the model.
 * @param {std::vector<cl_float>} normals  Normals of the model.
 */
size_t PathTracer::initOpenCLBuffers_Faces(
	ModelLoader* ml, vector<cl_float> vertices, vector<cl_uint> faces, vector<cl_float> normals
) {
	vector<cl_uint> facesVN = ml->getObjParser()->getFacesVN();
	vector<cl_int> facesMtl = ml->getObjParser()->getFacesMtl();
	vector<face_cl> faceStructs;

	// Convert array of face indices to own face struct.
	for( int i = 0; i < faces.size(); i += 3 ) {
		face_cl face;

		cl_float4 a = { vertices[faces[i + 0] * 3], vertices[faces[i + 0] * 3 + 1], vertices[faces[i + 0] * 3 + 2], 0.0f };
		cl_float4 b = { vertices[faces[i + 1] * 3], vertices[faces[i + 1] * 3 + 1], vertices[faces[i + 1] * 3 + 2], 0.0f };
		cl_float4 c = { vertices[faces[i + 2] * 3], vertices[faces[i + 2] * 3 + 1], vertices[faces[i + 2] * 3 + 2], 0.0f };
		face.a = a;
		face.b = b;
		face.c = c;

		cl_float4 an = { normals[facesVN[i + 0] * 3], normals[facesVN[i + 0] * 3 + 1], normals[facesVN[i + 0] * 3 + 2], 0.0f };
		cl_float4 bn = { normals[facesVN[i + 1] * 3], normals[facesVN[i + 1] * 3 + 1], normals[facesVN[i + 1] * 3 + 2], 0.0f };
		cl_float4 cn = { normals[facesVN[i + 2] * 3], normals[facesVN[i + 2] * 3 + 1], normals[facesVN[i + 2] * 3 + 2], 0.0f };
		face.an = an;
		face.bn = bn;
		face.cn = cn;

		face.a.w = (cl_float) facesMtl[i / 3];
		faceStructs.push_back( face );
	}

	size_t bytes = sizeof( face_cl ) * faceStructs.size();
	mBufFaces = mCL->createBuffer( faceStructs, bytes );

	return bytes;
}


/**
 * Init OpenCL buffers for the materials, including spectral power distributions.
 * @param {ModelLoader*} ml Model loader already holding the needed model data.
 */
size_t PathTracer::initOpenCLBuffers_Materials( ModelLoader* ml ) {
	vector<material_t> materials = ml->getObjParser()->getMaterials();
	bool useSPD = Cfg::get().value<bool>( Cfg::USE_SPECTRAL );
	size_t bytesMTL;

	if( useSPD ) {
		bytesMTL = this->initOpenCLBuffers_MaterialsSPD( materials, ml->getSpecParser() );
	}
	else {
		bytesMTL = this->initOpenCLBuffers_MaterialsRGB( materials );
	}

	return bytesMTL;
}


/**
 * Init OpenCL buffer for the materials (RGB mode).
 * @param  {std::vector<material_t>} materials Loaded materials.
 * @return {size_t}                            Size of the created buffer in bytes.
 */
size_t PathTracer::initOpenCLBuffers_MaterialsRGB( vector<material_t> materials ) {
	int brdf = Cfg::get().value<int>( Cfg::RENDER_BRDF );
	size_t bytesMTL;
	bool foundSkyLight = false;

	// BRDF: Schlick
	if( brdf == 0 ) {
		vector<material_schlick_rgb> materialsCL;

		for( int i = 0; i < materials.size(); i++ ) {
			material_schlick_rgb mtl;
			mtl.d = materials[i].d;
			mtl.Ni = materials[i].Ni;
			mtl.p = materials[i].p;
			mtl.rough = materials[i].rough;
			mtl.light = materials[i].light;
			mtl.rgbDiff = materials[i].Kd;
			mtl.rgbSpec = materials[i].Ks;

			materialsCL.push_back( mtl );

			if( materials[i].mtlName == "sky_light" && materials[i].light == 1 ) {
				cl_float4 Kd = materials[i].Kd;
				char msg[128];
				snprintf( msg, 128, "(float4)( %f, %f, %f, 0.0f )", Kd.x, Kd.y, Kd.z );
				mCL->setReplacement( string( "#SKY_LIGHT#" ), string( msg ) );
				foundSkyLight = true;
			}
		}

		bytesMTL = sizeof( material_schlick_rgb ) * materialsCL.size();
		mBufMaterials = mCL->createBuffer( materialsCL, bytesMTL );
	}
	// BRDF: Shirley-Ashikhmin
	else if( brdf == 1 ) {
		vector<material_shirley_ashikhmin_rgb> materialsCL;

		for( int i = 0; i < materials.size(); i++ ) {
			material_shirley_ashikhmin_rgb mtl;
			mtl.d = materials[i].d;
			mtl.Ni = materials[i].Ni;
			mtl.light = materials[i].light;
			mtl.nu = materials[i].nu;
			mtl.nv = materials[i].nv;
			mtl.Rs = materials[i].Rs;
			mtl.Rd = materials[i].Rd;
			mtl.rgbDiff = materials[i].Kd;
			mtl.rgbSpec = materials[i].Ks;

			materialsCL.push_back( mtl );

			if( materials[i].mtlName == "sky_light" && materials[i].light == 1 ) {
				cl_float4 Kd = materials[i].Kd;
				char msg[128];
				snprintf( msg, 128, "(float4)( %f, %f, %f, 0.0f )", Kd.x, Kd.y, Kd.z );
				mCL->setReplacement( string( "#SKY_LIGHT#" ), string( msg ) );
				foundSkyLight = true;
			}
		}

		bytesMTL = sizeof( material_shirley_ashikhmin_rgb ) * materialsCL.size();
		mBufMaterials = mCL->createBuffer( materialsCL, bytesMTL );
	}
	else {
		Logger::logError( "[PathTracer] Unknown BRDF selected." );
		exit( EXIT_FAILURE );
	}

	if( !foundSkyLight ) {
		mCL->setReplacement( string( "#SKY_LIGHT#" ), string( "(float4)( 1.0f, 1.0f, 1.0f, 0.0f )" ) );
	}

	return bytesMTL;
}


/**
 * Init OpenCL buffer for the materials (SPD mode).
 * @param  {std::vector<material_t>} materials Loaded materials.
 * @param  {SpecParser*}             sp        SpecParser that holds data about loaded SPDs.
 * @return {size_t}                            Size of the created buffer in bytes.
 */
size_t PathTracer::initOpenCLBuffers_MaterialsSPD( vector<material_t> materials, SpecParser* sp ) {
	map< string, map<string, string> > mtl2spd = sp->getMaterialToSPD();
	map< string, vector<cl_float> > spectra = sp->getSpectralPowerDistributions();
	string sky = sp->getSkySPDName();


	// Spectral Power Distributions

	vector<cl_float> spd, spectraCL;
	map<string, cl_ushort> specID;
	map<string, vector<cl_float> >::iterator it;
	ushort specCounter = 0;

	for( it = spectra.begin(); it != spectra.end(); it++, specCounter++ ) {
		specID[it->first] = specCounter;
		spd = it->second;

		// Spectral power distribution has wavelength steps of 5nm, but we use only 10nm steps
		for( int i = 0; i < spd.size(); i += 2 ) {
			spectraCL.push_back( spd[i] );
		}

		if( sky == it->first ) {
			char msg[16];
			snprintf( msg, 16, "%u", specCounter );
			mCL->setReplacement( string( "#SKY_LIGHT#" ), string( msg ) );
		}
	}

	size_t bytesSPD = sizeof( cl_float ) * spectraCL.size();
	mBufSPDs = mCL->createBuffer( spectraCL, bytesSPD );


	// Materials

	int brdf = Cfg::get().value<int>( Cfg::RENDER_BRDF );
	string spdName;
	size_t bytesMTL;

	// BRDF: Schlick
	if( brdf == 0 ) {
		vector<material_schlick_spd> materialsCL;

		for( int i = 0; i < materials.size(); i++ ) {
			material_schlick_spd mtl;
			mtl.d = materials[i].d;
			mtl.Ni = materials[i].Ni;
			mtl.p = materials[i].p;
			mtl.rough = materials[i].rough;
			mtl.light = materials[i].light;

			spdName = mtl2spd[materials[i].mtlName]["diff"];
			mtl.spd.x = specID[spdName];
			spdName = mtl2spd[materials[i].mtlName]["spec"];
			mtl.spd.y = specID[spdName];

			materialsCL.push_back( mtl );
		}

		bytesMTL = sizeof( material_schlick_spd ) * materialsCL.size();
		mBufMaterials = mCL->createBuffer( materialsCL, bytesMTL );
	}
	// BRDF: Shirley-Ashikhmin
	else if( brdf == 1 ) {
		vector<material_shirley_ashikhmin_spd> materialsCL;

		for( int i = 0; i < materials.size(); i++ ) {
			material_shirley_ashikhmin_spd mtl;
			mtl.d = materials[i].d;
			mtl.Ni = materials[i].Ni;
			mtl.light = materials[i].light;
			mtl.nu = materials[i].nu;
			mtl.nv = materials[i].nv;
			mtl.Rs = materials[i].Rs;
			mtl.Rd = materials[i].Rd;

			spdName = mtl2spd[materials[i].mtlName]["diff"];
			mtl.spd.x = specID[spdName];
			spdName = mtl2spd[materials[i].mtlName]["spec"];
			mtl.spd.y = specID[spdName];

			materialsCL.push_back( mtl );
		}

		bytesMTL = sizeof( material_shirley_ashikhmin_spd ) * materialsCL.size();
		mBufMaterials = mCL->createBuffer( materialsCL, bytesMTL );
	}
	else {
		Logger::logError( "[PathTracer] Unknown BRDF selected." );
		exit( EXIT_FAILURE );
	}

	return bytesSPD + bytesMTL;
}


/**
 * Init OpenCL buffers of the rays.
 */
size_t PathTracer::initOpenCLBuffers_Rays() {
	cl_uint pixels = mWidth * mHeight;
	mBufEye = mCL->createEmptyBuffer( sizeof( cl_float ) * 12, CL_MEM_READ_ONLY );

	return sizeof( cl_float ) * 12;
}


/**
 * Init OpenCL buffers of the textures.
 */
size_t PathTracer::initOpenCLBuffers_Textures() {
	mTextureOut = vector<cl_float>( mWidth * mHeight * 4, 0.0f );
	mBufTextureIn = mCL->createImage2DReadOnly( mWidth, mHeight, &mTextureOut[0] );
	mBufTextureOut = mCL->createImage2DWriteOnly( mWidth, mHeight );

	return sizeof( cl_float ) * mTextureOut.size();
}


/**
 * Move the position of the sun. This will also reset the sample count.
 * @param {const int} key Pressed key.
 */
void PathTracer::moveSun( const int key ) {
	switch( key ) {

		case Qt::Key_W:
			mSunPos.x += 0.25f;
			break;

		case Qt::Key_S:
			mSunPos.x -= 0.25f;
			break;

		case Qt::Key_A:
			mSunPos.z -= 0.25f;
			break;

		case Qt::Key_D:
			mSunPos.z += 0.25f;
			break;

		case Qt::Key_Q:
			mSunPos.y += 0.25f;
			break;

		case Qt::Key_E:
			mSunPos.y -= 0.25f;
			break;

	}

	this->resetSampleCount();
}


/**
 * Reset the sample counter. Should be done whenever the camera is changed.
 */
void PathTracer::resetSampleCount() {
	mSampleCount = 0;
}


/**
 * Set the camera of the scene.
 * @param {Camera*} camera The camera.
 */
void PathTracer::setCamera( Camera* camera ) {
	mCamera = camera;
}


/**
 * Set the field-of-view in degree.
 * @param {cl_float} fov New value for the field-of-view in degree.
 */
void PathTracer::setFOV( cl_float fov ) {
	mFOV = fov;
}


/**
 * Set the width and height for the image.
 * @param {cl_uint} width  Width in pixel.
 * @param {cl_uint} height Height in pixel.
 */
void PathTracer::setWidthAndHeight( cl_uint width, cl_uint height ) {
	mWidth = width;
	mHeight = height;
}


/**
 * Update the OpenCL buffer of the camera eye and related vectors.
 */
void PathTracer::updateEyeBuffer() {
	glm::vec3 c = mCamera->getAdjustedCenter_glmVec3();
	glm::vec3 eye = mCamera->getEye_glmVec3();
	glm::vec3 up = mCamera->getUp_glmVec3();

	glm::vec3 w = glm::normalize( c - eye );
	glm::vec3 u = glm::normalize( glm::cross( w, up ) );
	glm::vec3 v = glm::normalize( glm::cross( u, w ) );

	cl_float eyeBuffer[12] = {
		eye[0], eye[1], eye[2],
		w[0], w[1], w[2],
		u[0], u[1], u[2],
		v[0], v[1], v[2]
	};

	mCL->updateBuffer( mBufEye, sizeof( cl_float ) * 12, &eyeBuffer );
}
