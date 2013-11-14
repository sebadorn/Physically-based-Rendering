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

	mKernelRays = mCL->createKernel( "generateRays" );
	mKernelIntersections = mCL->createKernel( "findIntersectionsKdTree" );
	mKernelColors = mCL->createKernel( "accumulateColors" );

	mFOV = Cfg::get().value<cl_float>( Cfg::PERS_FOV );
	mNumBounces = Cfg::get().value<cl_uint>( Cfg::RENDER_BOUNCES );
	mSampleCount = 0;
	mTimeSinceStart = boost::posix_time::microsec_clock::local_time();
}


/**
 * Deconstructor.
 */
PathTracer::~PathTracer() {
	delete mCL;
}


/**
 * OpenCL: Accumulate all colors of the hit surface.
 * @param {cl_float} timeSinceStart Time in seconds since start of rendering.
 */
void PathTracer::clAccumulateColors( cl_float timeSinceStart ) {
	cl_uint i = 0;
	cl_float textureWeight = mSampleCount / (cl_float) ( mSampleCount + 1 );

	mCL->setKernelArg( mKernelColors, i, sizeof( cl_mem ), &mBufOrigins );
	mCL->setKernelArg( mKernelColors, ++i, sizeof( cl_mem ), &mBufNormals );
	mCL->setKernelArg( mKernelColors, ++i, sizeof( cl_mem ), &mBufAccColors );
	mCL->setKernelArg( mKernelColors, ++i, sizeof( cl_mem ), &mBufColorMasks );
	mCL->setKernelArg( mKernelColors, ++i, sizeof( cl_float ), &textureWeight );
	mCL->setKernelArg( mKernelColors, ++i, sizeof( cl_float ), &timeSinceStart );
	mCL->setKernelArg( mKernelColors, ++i, sizeof( cl_mem ), &mBufTextureIn );
	mCL->setKernelArg( mKernelColors, ++i, sizeof( cl_mem ), &mBufTextureOut );

	mCL->execute( mKernelColors );
	mCL->finish();
}


/**
 * OpenCL: Find intersections of rays with scene.
 * @param {cl_float} timeSinceStart Time in seconds since start of rendering.
 */
void PathTracer::clFindIntersections( cl_float timeSinceStart ) {
	cl_uint i = 0;

	mCL->setKernelArg( mKernelIntersections, i, sizeof( cl_mem ), &mBufOrigins );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufRays );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufNormals );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufScVertices );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufScFaces );
	// mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufScNormals );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufKdNodeData1 );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufKdNodeData2 );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufKdNodeData3 );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufKdNodeRopes );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_uint ), &mKdRootNodeIndex );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_float ), &timeSinceStart );

	mCL->execute( mKernelIntersections );
	mCL->finish();
}


/**
 * OpenCL: Compute the initial rays into the scene.
 */
void PathTracer::clInitRays() {
	cl_uint i = 0;
	cl_float fovRad = utils::degToRad( mFOV );

	mCL->setKernelArg( mKernelRays, i, sizeof( cl_mem ), &mBufEye );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_float ), &fovRad );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufOrigins );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufRays );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufAccColors );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufColorMasks );

	mCL->execute( mKernelRays );
	mCL->finish();
}


/**
 * Generate the path traced image, which is basically just a 2D texture.
 * @return {std::vector<cl_float>} Float vector representing a 2D image.
 */
vector<cl_float> PathTracer::generateImage() {
	cl_float timeSinceStart = this->getTimeSinceStart();

	this->updateEyeBuffer();
	mCL->updateImageReadOnly( mBufTextureIn, mWidth, mHeight, &mTextureOut[0] );
	this->clInitRays();

	for( cl_uint bounce = 0; bounce < mNumBounces; bounce++ ) {
		this->clFindIntersections( timeSinceStart + bounce );
		this->clAccumulateColors( timeSinceStart );
	}

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
 * Init the needed OpenCL buffers: Faces, vertices, camera eye and rays.
 * @param {std::vector<cl_float>*} vertices  Vertices of the model.
 * @param {std::vector<cl_uint>*}  faces     Faces (triangles) of the model.
 * @param {std::vector<kdNode_t>}  kdNodes   The nodes of the kd-tree.
 * @param {cl_uint}                rootIndex Index of the root kd node.
 */
void PathTracer::initOpenCLBuffers(
	vector<cl_float> vertices, vector<cl_uint> faces,
	vector<kdNode_t> kdNodes, cl_uint rootIndex
) {
	cl_uint pixels = mWidth * mHeight;
	mKdRootNodeIndex = rootIndex;

	vector<cl_float> kdData1; // [x, y, z, bbMin(x,y,z), bbMax(x,y,z)]
	vector<cl_int> kdData2;   // [left, right, axis, facesIndex, ropesIndex]
	vector<cl_int> kdData3;   // [numFaces, a, b, c ...]
	vector<cl_int> kdRopes;   // [left, right, bottom, top, back, front]
	this->kdNodesToVectors( kdNodes, &kdData1, &kdData2, &kdData3, &kdRopes );

	mBufKdNodeData1 = mCL->createBuffer( kdData1, sizeof( cl_float ) * kdData1.size() );
	mBufKdNodeData2 = mCL->createBuffer( kdData2, sizeof( cl_int ) * kdData2.size() );
	mBufKdNodeData3 = mCL->createBuffer( kdData3, sizeof( cl_int ) * kdData3.size() );
	mBufKdNodeRopes = mCL->createBuffer( kdRopes, sizeof( cl_int ) * kdRopes.size() );

	mBufScFaces = mCL->createBuffer( faces, sizeof( cl_uint ) * faces.size() );
	mBufScVertices = mCL->createBuffer( vertices, sizeof( cl_float ) * vertices.size() );
	// mBufScNormals = mCL->createBuffer( mNormals, sizeof( cl_float ) * mNormals.size() );

	mBufEye = mCL->createEmptyBuffer( sizeof( cl_float ) * 12, CL_MEM_READ_ONLY );

	mBufOrigins = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels, CL_MEM_READ_WRITE );
	mBufRays = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels, CL_MEM_READ_WRITE );
	mBufNormals = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels, CL_MEM_READ_WRITE );
	mBufAccColors = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels, CL_MEM_READ_WRITE );
	mBufColorMasks = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels, CL_MEM_READ_WRITE );

	mTextureOut = vector<cl_float>( pixels * 4, 0.0f );
	mBufTextureIn = mCL->createImageReadOnly( mWidth, mHeight, &mTextureOut[0] );
	mBufTextureOut = mCL->createImageWriteOnly( mWidth, mHeight );
}


/**
 * Store the kd-nodes data in seperate lists for each data type
 * in order to pass it to OpenCL.
 * @param {std::vector<KdNode_t>}  kdNodes The nodes to extract the data from.
 * @param {std::vector<cl_float>*} kdData1 Data: [x, y, z, bbMin(x,y,z), bbMax(x,y,z)]
 * @param {std::vector<cl_int>*}   kdData2 Data: [left, right, axis, facesIndex, ropesIndex]
 * @param {std::vector<cl_int>*}   kdData3 Data: [numFaces, a, b, c ...]
 * @param {std::vector<cl_int>*}   kdRopes Data: [left, right, bottom, top, back, front]
 */
void PathTracer::kdNodesToVectors(
	vector<kdNode_t> kdNodes,
	vector<cl_float>* kdData1, vector<cl_int>* kdData2, vector<cl_int>* kdData3,
	vector<cl_int>* kdRopes
) {
	for( cl_uint i = 0; i < kdNodes.size(); i++ ) {
		// Vertice coordinates
		kdData1->push_back( kdNodes[i].pos[0] );
		kdData1->push_back( kdNodes[i].pos[1] );
		kdData1->push_back( kdNodes[i].pos[2] );

		// Bounding box
		kdData1->push_back( kdNodes[i].bbMin[0] );
		kdData1->push_back( kdNodes[i].bbMin[1] );
		kdData1->push_back( kdNodes[i].bbMin[2] );
		kdData1->push_back( kdNodes[i].bbMax[0] );
		kdData1->push_back( kdNodes[i].bbMax[1] );
		kdData1->push_back( kdNodes[i].bbMax[2] );

		// Index of self and children
		kdData2->push_back( kdNodes[i].left );
		kdData2->push_back( kdNodes[i].right );
		kdData2->push_back( kdNodes[i].axis );

		// Index of faces of this node in kdData3
		kdData2->push_back( ( kdNodes[i].faces.size() > 0 ) ? kdData3->size() : -1 );

		// Faces
		kdData3->push_back( kdNodes[i].faces.size() );

		for( cl_uint j = 0; j < kdNodes[i].faces.size(); j++ ) {
			kdData3->push_back( kdNodes[i].faces[j] );
		}

		// Ropes
		if( kdNodes[i].left < 0 && kdNodes[i].right < 0 ) {
			kdData2->push_back( kdRopes->size() );

			for( cl_uint j = 0; j < kdNodes[i].ropes.size(); j++ ) {
				kdRopes->push_back( kdNodes[i].ropes[j] );
			}
		}
		else {
			kdData2->push_back( -1 );
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

	// Jittering for anti-aliasing
	// TODO: probably wrong or at least not very good
	if( Cfg::get().value<bool>( Cfg::RENDER_ANTIALIAS ) ) {
		eye += this->getJitter();
	}

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
