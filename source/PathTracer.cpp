#include "PathTracer.h"

using std::string;
using std::vector;


/**
 * Constructor.
 */
PathTracer::PathTracer() {
	srand( (unsigned) time( 0 ) );

	mWidth = Cfg::get().value<cl_uint>( Cfg::WINDOW_WIDTH );
	mHeight = Cfg::get().value<cl_uint>( Cfg::WINDOW_HEIGHT );

	mCL = new CL();
	mCL->loadProgram( Cfg::get().value<string>( Cfg::OPENCL_PROGRAM ) );

	mKernelRays = mCL->createKernel( "initRays" );
	mKernelPathTracing = mCL->createKernel( "pathTracing" );

	mFOV = Cfg::get().value<cl_float>( Cfg::PERS_FOV );
	mSampleCount = 0;
	mTimeSinceStart = boost::posix_time::microsec_clock::local_time();
}


/**
 * Destructor.
 */
PathTracer::~PathTracer() {
	delete mCL;
}


/**
 * OpenCL: Compute the initial rays into the scene.
 * @param {cl_float} timeSinceStart Time since start of the program in seconds.
 */
void PathTracer::clInitRays( cl_float timeSinceStart ) {
	mCL->setKernelArg( mKernelRays, 1, sizeof( cl_float ), &timeSinceStart );

	mCL->execute( mKernelRays );
	mCL->finish();
}


/**
 * OpenCL: Find the paths in the scene and accumulate the colors of hit surfaces.
 * @param {cl_float} timeSinceStart Time since start of the program in seconds.
 */
void PathTracer::clPathTracing( cl_float timeSinceStart ) {
	cl_float pixelWeight = mSampleCount / (cl_float) ( mSampleCount + 1 );

	mCL->setKernelArg( mKernelPathTracing, 1, sizeof( cl_float ), &timeSinceStart );
	mCL->setKernelArg( mKernelPathTracing, 2, sizeof( cl_float ), &pixelWeight );

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

	this->clInitRays( timeSinceStart );
	this->clPathTracing( timeSinceStart );

	mCL->readImageOutput( mBufTextureOut, mWidth, mHeight, &mTextureOut[0] );
	mSampleCount++;

	return mTextureOut;
}


/**
 * Get the CL object.
 * @return {CL*} CL object.
 */
CL* PathTracer::getCLObject() {
	return mCL;
}


/**
 * Get a jitter vector in order to slightly alter the eye ray.
 * This results in anti-aliasing.
 * @return {glm::vec3} Jitter.
 */
glm::vec3 PathTracer::getJitter() {
	glm::vec3 v = glm::vec3(
		rand() / (float) RAND_MAX * 1.0f - 1.0f,
		rand() / (float) RAND_MAX * 1.0f - 1.0f,
		0.0f
	);

	v[0] *= ( 1.0f / (float) mWidth );
	v[1] *= ( 1.0f / (float) mHeight );

	return v;
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
	cl_uint i = 0;

	++i; // 1: timeSinceStart
	++i; // 2: pixelWeight
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufFaces );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufBVH );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufKdNonLeaves );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufKdLeaves );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufKdFaces );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufRays );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufMaterials );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufSPDs );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufTextureIn );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufTextureOut );
}


/**
 * Init the kernel arguments for the OpenCL kernel to initialize the rays into the scene.
 */
void PathTracer::initArgsKernelRays() {
	cl_float wgs = Cfg::get().value<cl_float>( Cfg::OPENCL_WORKGROUPSIZE );
	cl_float wGsw = mWidth / wgs;
	cl_float hGsh = mHeight / wgs;

	cl_float f = 2.0f * tan( utils::degToRad( mFOV ) / 2.0f );
	f /= ( mWidth > mHeight ) ? std::min( wGsw, hGsh ) : std::max( wGsw, hGsh );

	cl_float pxWidthAndHeight = f / wgs;
	cl_float adjustW = ( mWidth - wgs ) / 2.0f;
	cl_float adjustH = ( mHeight - wgs ) / 2.0f;

	cl_float4 initRayParts;
	initRayParts.x = pxWidthAndHeight;
	initRayParts.y = adjustW;
	initRayParts.z = adjustH;
	initRayParts.w = pxWidthAndHeight / 2.0f;

	cl_uint i = 0;

	++i; // 1: timeSinceStart
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_float4 ), &initRayParts );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufEye );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufRays );
}


/**
 * Set the OpenCL kernel arguments that either don't change or point to a CL buffer.
 */
void PathTracer::initKernelArgs() {
	this->initArgsKernelRays();
	this->initArgsKernelPathTracing();
}


/**
 * Init the needed OpenCL buffers: Faces, vertices, camera eye and rays.
 * @param {std::vector<cl_float>} vertices Vertices of the model.
 * @param {std::vector<cl_uint>}  faces    Faces (triangles) of the model.
 * @param {std::vector<cl_float>} normals  Normals of the model.
 * @param {ModelLoader*}          ml       Model loader already holding the needed model data.
 * @param {std::vector<kdNode_t>} kdNodes  Nodes of the kd-tree.
 * @param {kdNode_t*}             rootNode Root node of the kd-tree.
 */
void PathTracer::initOpenCLBuffers(
	vector<cl_float> vertices, vector<cl_uint> faces, vector<cl_float> normals,
	ModelLoader* ml, BVH* bvh
) {
	Logger::logInfo( "[PathTracer] Initializing OpenCL buffers ..." );
	Logger::indent( LOG_INDENT );
	mCL->freeBuffers();
	this->initOpenCLBuffers_BVH( bvh );
	Logger::logDebug( "[PathTracer] Finished buffer for BVH." );
	this->initOpenCLBuffers_KdTree( ml, bvh, vertices, faces, normals );
	Logger::logDebug( "[PathTracer] Finished buffer for kD-trees." );
	this->initOpenCLBuffers_Materials( ml );
	Logger::logDebug( "[PathTracer] Finished buffer for materials." );
	this->initOpenCLBuffers_Rays();
	Logger::logDebug( "[PathTracer] Finished buffer for rays." );
	this->initOpenCLBuffers_Textures();
	Logger::logDebug( "[PathTracer] Finished buffer for target textures." );
	Logger::indent( 0 );
	Logger::logInfo( "[PathTracer] ... Done." );

	this->initKernelArgs();
}


/**
 * Init OpenCL buffer for the BVH.
 * @param {BVH*} bvh
 */
void PathTracer::initOpenCLBuffers_BVH( BVH* bvh ) {
	vector<bvhNode_cl> BVHnodesCL;
	vector<BVHnode*> BVHnodes = bvh->getNodes();
	cl_int offsetNonLeaves = 0;

	for( cl_uint i = 0; i < BVHnodes.size(); i++ ) {
		BVHnode* node = BVHnodes[i];

		cl_float4 bbMin = { node->bbMin[0], node->bbMin[1], node->bbMin[2], 0.0f };
		cl_float4 bbMax = { node->bbMax[0], node->bbMax[1], node->bbMax[2], 0.0f };

		bvhNode_cl nodeCL;
		nodeCL.bbMin = bbMin;
		nodeCL.bbMax = bbMax;
		nodeCL.bbMin.w = ( node->left != NULL ) ? (cl_float) ( node->left->id + 1 ) : 0.0f;
		nodeCL.bbMax.w = ( node->right != NULL ) ? (cl_float) ( node->right->id + 1 ) : 0.0f;

		if( node->kdtree != NULL ) {
			nodeCL.bbMin.w = (cl_float) -( offsetNonLeaves + 1 ); // Offset for the index of the kD-tree root node
			offsetNonLeaves += node->kdtree->getNonLeaves().size();
		}

		BVHnodesCL.push_back( nodeCL );
	}

	mBufBVH = mCL->createBuffer( BVHnodesCL, sizeof( bvhNode_cl ) * BVHnodesCL.size() );
}


/**
 * Init OpenCL buffers for the kD-tree.
 * @param {std::vector<cl_float>} vertices Vertices of the model.
 * @param {std::vector<cl_uint>}  faces    Faces (triangles) of the model.
 * @param {std::vector<kdNode_t>} kdNodes  Nodes of the kd-tree.
 * @param {kdNode_t*}             rootNode Root node of the kd-tree.
 */
void PathTracer::initOpenCLBuffers_KdTree(
	ModelLoader* ml, BVH* bvh,
	vector<cl_float> vertices, vector<cl_uint> faces, vector<cl_float> normals
) {
	vector<cl_uint> facesVN = ml->getFacesVN();
	vector<cl_int> facesMtl = ml->getFacesMtl();
	vector<face_cl> faceStructs;

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

	mBufFaces = mCL->createBuffer( faceStructs, sizeof( face_cl ) * faceStructs.size() );


	vector<kdNode_t> kdNodes;
	vector<kdNonLeaf_cl> kdNonLeaves;
	vector<kdLeaf_cl> kdLeaves;
	vector<cl_uint> kdFaces;
	vector<BVHnode*> bvLeaves = bvh->getLeaves();
	vector<object3D> objects = ml->getObjects();
	cl_uint2 offset = { 0, 0 };

	for( int i = 0; i < bvLeaves.size(); i++ ) {
		kdNodes = bvLeaves[i]->kdtree->getNodes();
		this->kdNodesToVectors( faces, objects[i].facesV, kdNodes, &kdFaces, &kdNonLeaves, &kdLeaves, offset );
		offset.x = kdNonLeaves.size();
		offset.y = kdLeaves.size();
	}

	mBufKdNonLeaves = mCL->createBuffer( kdNonLeaves, sizeof( kdNonLeaf_cl ) * kdNonLeaves.size() );
	mBufKdLeaves = mCL->createBuffer( kdLeaves, sizeof( kdLeaf_cl ) * kdLeaves.size() );
	mBufKdFaces = mCL->createBuffer( kdFaces, sizeof( cl_uint ) * kdFaces.size() );
}


/**
 * Init OpenCL buffers for the materials, including spectral power distributions.
 * @param {ModelLoader*} ml Model loader already holding the needed model data.
 */
void PathTracer::initOpenCLBuffers_Materials( ModelLoader* ml ) {
	vector<material_t> materials = ml->getMaterials();
	map<string, string> mtl2spd = ml->getMaterialToSPD();
	map<string, vector<cl_float> > spectra = ml->getSpectralPowerDistributions();

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
	}


	vector<material_cl_t> materialsCL;

	for( int i = 0; i < materials.size(); i++ ) {
		material_cl_t mtl;
		mtl.d = materials[i].d;
		mtl.Ni = materials[i].Ni;
		mtl.gloss = materials[i].gloss;
		mtl.illum = materials[i].illum;
		mtl.light = materials[i].light;
		mtl.scratch = materials[i].scratch;
		mtl.scratch.w = materials[i].p;

		string spdName = mtl2spd[materials[i].mtlName];
		mtl.spd = specID[spdName];

		materialsCL.push_back( mtl );
	}


	mBufMaterials = mCL->createBuffer( materialsCL, sizeof( material_cl_t ) * materialsCL.size() );
	mBufSPDs = mCL->createBuffer( spectraCL, sizeof( cl_float ) * spectraCL.size() );
}


/**
 * Init OpenCL buffers of the rays.
 */
void PathTracer::initOpenCLBuffers_Rays() {
	cl_uint pixels = mWidth * mHeight;
	mBufEye = mCL->createEmptyBuffer( sizeof( cl_float ) * 12, CL_MEM_READ_ONLY );
	mBufRays = mCL->createEmptyBuffer( sizeof( ray4 ) * pixels, CL_MEM_READ_WRITE );
}


/**
 * Init OpenCL buffers of the textures.
 */
void PathTracer::initOpenCLBuffers_Textures() {
	mTextureOut = vector<cl_float>( mWidth * mHeight * 4, 0.0f );
	mBufTextureIn = mCL->createImage2DReadOnly( mWidth, mHeight, &mTextureOut[0] );
	mBufTextureOut = mCL->createImage2DWriteOnly( mWidth, mHeight );
}


/**
 * Store the kd-nodes data in seperate lists for each data type
 * in order to pass it to OpenCL.
 * @param {std::vector<cl_uint>*}      faces       Faces.
 * @param {std::vector<cl_uint>}       objectFaces Faces of the individuell objects in the scene.
 * @param {std::vector<KdNode_t>}      kdNodes     All nodes of the kd-tree.
 * @param {std::vector<cl_float>*}     kdFaces     Data: [a, b, c ..., faceIndex]
 * @param {std::vector<kdNonLeaf_cl>*} kdNonLeaves Output: Nodes of the kd-tree, that aren't leaves.
 * @param {std::vector<kdLeaf_cl>*}    kdLeaves    Output: Leaf nodes of the kd-tree.
 * @param {cl_uint2}                   offset
 */
void PathTracer::kdNodesToVectors(
	vector<cl_uint> faces, vector<cl_uint> objectFaces, vector<kdNode_t> kdNodes,
	vector<cl_uint>* kdFaces, vector<kdNonLeaf_cl>* kdNonLeaves,
	vector<kdLeaf_cl>* kdLeaves, cl_uint2 offset
) {
	cl_uint a, b, c;
	cl_int pos;

	for( cl_uint i = 0; i < kdNodes.size(); i++ ) {
		// Non-leaf node
		if( kdNodes[i].axis >= 0 ) {
			kdNonLeaf_cl node;

			cl_float4 split = {
				kdNodes[i].pos[0],
				kdNodes[i].pos[1],
				kdNodes[i].pos[2],
				(cl_float) kdNodes[i].axis
			};
			node.split = split;

			cl_int4 children = {
				kdNodes[i].left->index,
				kdNodes[i].right->index,
				( kdNodes[i].left->axis < 0 ), // Is node a leaf node
				( kdNodes[i].right->axis < 0 )
			};

			children.x += ( kdNodes[i].left->axis < 0 ) ? offset.y : offset.x;
			children.y += ( kdNodes[i].right->axis < 0 ) ? offset.y : offset.x;

			node.children = children;
			kdNonLeaves->push_back( node );

			if( kdNodes[i].faces.size() > 0 ) {
				Logger::logWarning( "[PathTracer] Converting kd-node data: Non-leaf node with faces." );
			}
		}
		// Leaf node
		else {
			kdLeaf_cl node;
			vector<kdNode_t*> nodeRopes = kdNodes[i].ropes;

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
			ropes.s7 = kdNodes[i].faces.size() / 3;

			node.ropes = ropes;

			// Bounding box
			cl_float4 bbMin = {
				kdNodes[i].bbMin[0],
				kdNodes[i].bbMin[1],
				kdNodes[i].bbMin[2],
				0.0f
			};
			cl_float4 bbMax = {
				kdNodes[i].bbMax[0],
				kdNodes[i].bbMax[1],
				kdNodes[i].bbMax[2],
				0.0f
			};
			node.bbMin = bbMin;
			node.bbMax = bbMax;

			kdLeaves->push_back( node );

			if( kdNodes[i].faces.size() == 0 ) {
				Logger::logWarning( "[PathTracer] Converting kd-node data: Leaf node without any faces." );
			}


			// Faces
			for( cl_uint j = 0; j < kdNodes[i].faces.size(); j += 3 ) {
				pos = -1;

				for( cl_uint k = 0; k < faces.size(); k += 3 ) {
					if(
						faces[k] == objectFaces[kdNodes[i].faces[j]] &&
						faces[k + 1] == objectFaces[kdNodes[i].faces[j + 1]] &&
						faces[k + 2] == objectFaces[kdNodes[i].faces[j + 2]]
					) {
						pos = k / 3;
						break;
					}
				}

				if( pos < 0 ) {
					Logger::logError( "[PathTracer] Converting kd-node data: No index for face found." );
				}

				kdFaces->push_back( pos );
			}
		}
	}
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

	glm::vec3 w = glm::normalize( glm::vec3( c[0] - eye[0], c[1] - eye[1], c[2] - eye[2] ) );
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
