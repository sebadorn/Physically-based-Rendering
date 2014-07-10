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
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_float ), &pxDim );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufEye );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufFaces );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufBVH );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufBVHFaces );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufMaterials );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufSPDs );
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
 * @param {std::vector<cl_float>} vertices Vertices of the model.
 * @param {std::vector<cl_uint>}  faces    Faces (triangles) of the model.
 * @param {std::vector<cl_float>} normals  Normals of the model.
 * @param {ModelLoader*}          ml       Model loader already holding the needed model data.
 * @param {BVH*}                  bvh      The generated Bounding Volume Hierarchy.
 */
void PathTracer::initOpenCLBuffers(
	vector<cl_float> vertices, vector<cl_uint> faces, vector<cl_float> normals,
	ModelLoader* ml, BVH* bvh
) {
	boost::posix_time::ptime timerStart;
	boost::posix_time::ptime timerEnd;
	cl_float timeDiff;
	size_t bytes;
	float bytesFloat;
	string unit;
	char msg[64];

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
	snprintf( msg, 64, "[PathTracer] Created faces buffer in %g ms -- %.2f %s.", timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	// Buffer: Bounding Volume Hierarchy
	timerStart = boost::posix_time::microsec_clock::local_time();
	bytes = this->initOpenCLBuffers_BVH( bvh );
	timerEnd = boost::posix_time::microsec_clock::local_time();
	timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	utils::formatBytes( bytes, &bytesFloat, &unit );
	snprintf( msg, 64, "[PathTracer] Created BVH buffer in %g ms -- %.2f %s", timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	// Buffer: Material(s)
	timerStart = boost::posix_time::microsec_clock::local_time();
	bytes = this->initOpenCLBuffers_Materials( ml );
	timerEnd = boost::posix_time::microsec_clock::local_time();
	timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	utils::formatBytes( bytes, &bytesFloat, &unit );
	snprintf( msg, 64, "[PathTracer] Created material buffer in %g ms -- %.2f %s", timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	// Buffer: Rays
	timerStart = boost::posix_time::microsec_clock::local_time();
	bytes = this->initOpenCLBuffers_Rays();
	timerEnd = boost::posix_time::microsec_clock::local_time();
	timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	utils::formatBytes( bytes, &bytesFloat, &unit );
	snprintf( msg, 64, "[PathTracer] Created ray buffer in %g ms -- %.2f %s", timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	// Buffer: Textures
	timerStart = boost::posix_time::microsec_clock::local_time();
	bytes = this->initOpenCLBuffers_Textures();
	timerEnd = boost::posix_time::microsec_clock::local_time();
	timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	utils::formatBytes( bytes, &bytesFloat, &unit );
	snprintf( msg, 64, "[PathTracer] Created texture buffer in %g ms -- %.2f %s", timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	Logger::indent( 0 );
	Logger::logInfo( "[PathTracer] ... Done." );


	snprintf( msg, 64, "%u", bvh->getDepth() );
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
	vector<cl_uint> bvhNodeFaces;

	for( cl_uint i = 0; i < bvhNodes.size(); i++ ) {
		cl_float4 bbMin = { bvhNodes[i]->bbMin[0], bvhNodes[i]->bbMin[1], bvhNodes[i]->bbMin[2], 0.0f };
		cl_float4 bbMax = { bvhNodes[i]->bbMax[0], bvhNodes[i]->bbMax[1], bvhNodes[i]->bbMax[2], 0.0f };

		bvhNode_cl sn;
		sn.bbMin = bbMin;
		sn.bbMax = bbMax;
		sn.bbMin.w = ( bvhNodes[i]->leftChild == NULL ) ? -1.0f : (cl_float) bvhNodes[i]->leftChild->id;
		sn.bbMax.w = ( bvhNodes[i]->rightChild == NULL ) ? -1.0f : (cl_float) bvhNodes[i]->rightChild->id;

		vector<Tri> facesVec = bvhNodes[i]->faces;
		sn.facesInterval.x = bvhNodeFaces.size();
		sn.facesInterval.y = facesVec.size();

		for( cl_uint i = 0; i < facesVec.size(); i++ ) {
			bvhNodeFaces.push_back( facesVec[i].face.w );
		}

		bvhNodesCL.push_back( sn );
	}

	size_t bytesBVH = sizeof( bvhNode_cl ) * bvhNodesCL.size();
	size_t bytesFaces = sizeof( cl_uint ) * bvhNodeFaces.size();
	mBufBVH = mCL->createBuffer( bvhNodesCL, bytesBVH );
	mBufBVHFaces = mCL->createBuffer( bvhNodeFaces, bytesFaces );

	return bytesBVH + bytesFaces;
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
	map< string, map<string, string> > mtl2spd = ml->getSpecParser()->getMaterialToSPD();
	map< string, vector<cl_float> > spectra = ml->getSpecParser()->getSpectralPowerDistributions();
	string sky = ml->getSpecParser()->getSkySPDName();


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
	size_t bytesMTL;
	string spdName;

	// BRDF: Schlick
	if( brdf == 0 ) {
		vector<material_schlick> materialsCL;

		for( int i = 0; i < materials.size(); i++ ) {
			material_schlick mtl;
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

		bytesMTL = sizeof( material_schlick ) * materialsCL.size();
		mBufMaterials = mCL->createBuffer( materialsCL, bytesMTL );
	}
	// BRDF: Shirley-Ashikhmin
	else if( brdf == 1 ) {
		vector<material_shirley_ashikhmin> materialsCL;

		for( int i = 0; i < materials.size(); i++ ) {
			material_shirley_ashikhmin mtl;
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

		bytesMTL = sizeof( material_shirley_ashikhmin ) * materialsCL.size();
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
