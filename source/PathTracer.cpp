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

	mKernelRays = mCL->createKernel( "initRays" );
	mKernelPathTracing = mCL->createKernel( "pathTracing" );

	mGLWidget->createKernelWindow( mCL );

	this->initKernelArgs();
}


/**
 * Init OpenCL buffers for the BVH.
 * @param {BVH*} bvh The generated Bounding Volume Hierarchy.
 */
size_t PathTracer::initOpenCLBuffers_BVH( BVH* bvh ) {
	vector<BVHNode*> stNodes = bvh->getNodes();
	vector<bvhNode_cl> sphereNodes;

	for( cl_uint i = 0; i < stNodes.size(); i++ ) {
		cl_float4 bbMin = { stNodes[i]->bbMin[0], stNodes[i]->bbMin[1], stNodes[i]->bbMin[2], 0.0f };
		cl_float4 bbMax = { stNodes[i]->bbMax[0], stNodes[i]->bbMax[1], stNodes[i]->bbMax[2], 0.0f };

		bvhNode_cl sn;
		sn.bbMin = bbMin;
		sn.bbMax = bbMax;
		sn.bbMin.w = ( stNodes[i]->leftChild == NULL ) ? -1.0f : (cl_float) stNodes[i]->leftChild->id;
		sn.bbMax.w = ( stNodes[i]->rightChild == NULL ) ? -1.0f : (cl_float) stNodes[i]->rightChild->id;

		vector<cl_uint4> facesVec = stNodes[i]->faces;
		cl_int4 faces;
		faces.x = ( facesVec.size() >= 1 ) ? facesVec[0].w : -1;
		faces.y = ( facesVec.size() >= 2 ) ? facesVec[1].w : -1;
		faces.z = ( facesVec.size() >= 3 ) ? facesVec[2].w : -1;
		faces.w = ( facesVec.size() >= 4 ) ? facesVec[3].w : -1;
		sn.faces = faces;

		sphereNodes.push_back( sn );
	}

	size_t bytes = sizeof( bvhNode_cl ) * sphereNodes.size();
	mBufBVH = mCL->createBuffer( sphereNodes, bytes );

	return bytes;
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

	size_t bytes = sizeof( face_cl ) * faceStructs.size();
	mBufFaces = mCL->createBuffer( faceStructs, bytes );

	return bytes;
}


/**
 * Init OpenCL buffers for the materials, including spectral power distributions.
 * @param {ModelLoader*} ml Model loader already holding the needed model data.
 */
size_t PathTracer::initOpenCLBuffers_Materials( ModelLoader* ml ) {
	vector<material_t> materials = ml->getMaterials();
	map<string, string> mtl2spd = ml->getMaterialToSPD();
	map<string, vector<cl_float> > spectra = ml->getSpectralPowerDistributions();


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
	}

	size_t bytesSPD = sizeof( cl_float ) * spectraCL.size();
	mBufSPDs = mCL->createBuffer( spectraCL, bytesSPD );


	// Materials

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

	size_t bytesMTL = sizeof( material_cl_t ) * materialsCL.size();
	mBufMaterials = mCL->createBuffer( materialsCL, bytesMTL );

	return bytesSPD + bytesMTL;
}


/**
 * Init OpenCL buffers of the rays.
 */
size_t PathTracer::initOpenCLBuffers_Rays() {
	cl_uint pixels = mWidth * mHeight;
	mBufEye = mCL->createEmptyBuffer( sizeof( cl_float ) * 12, CL_MEM_READ_ONLY );
	mBufRays = mCL->createEmptyBuffer( sizeof( rayBase ) * pixels, CL_MEM_READ_WRITE );

	return sizeof( cl_float ) * 12 + sizeof( rayBase ) * pixels;
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
