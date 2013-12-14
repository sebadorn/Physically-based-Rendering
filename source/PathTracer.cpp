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
	mBounces = Cfg::get().value<cl_uint>( Cfg::RENDER_BOUNCES );

	mCL = new CL();
	mCL->loadProgram( Cfg::get().value<string>( Cfg::OPENCL_PROGRAM ) );

	mKernelRays = mCL->createKernel( "initRays" );
	mKernelPathTracing = mCL->createKernel( "pathTracing" );
	mKernelShadowTest = mCL->createKernel( "shadowTest" );
	mKernelSetColors = mCL->createKernel( "setColors" );

	mFOV = Cfg::get().value<cl_float>( Cfg::PERS_FOV );
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
 * OpenCL: Compute the initial rays into the scene.
 * @param {cl_float} timeSinceStart Time since start of the program in seconds.
 */
void PathTracer::clInitRays( cl_float timeSinceStart ) {
	mCL->setKernelArg( mKernelRays, 5, sizeof( cl_float ), &timeSinceStart );

	mCL->execute( mKernelRays );
	mCL->finish();
}


/**
 * OpenCL: Find the paths in the scene and accumulate the colors of hit surfaces.
 * @param {cl_float} timeSinceStart Time since start of the program in seconds.
 */
void PathTracer::clPathTracing( cl_float timeSinceStart ) {
	mCL->setKernelArg( mKernelPathTracing, 1, sizeof( cl_float ), &timeSinceStart );

	mCL->execute( mKernelPathTracing );
	mCL->finish();
}


void PathTracer::clSetColors( cl_float timeSinceStart ) {
	cl_float textureWeight = mSampleCount / (cl_float) ( mSampleCount + 1 );

	mCL->setKernelArg( mKernelSetColors, 1, sizeof( cl_float ), &timeSinceStart );
	mCL->setKernelArg( mKernelSetColors, 2, sizeof( cl_float ), &textureWeight );

	mCL->execute( mKernelSetColors );
	mCL->finish();
}


/**
 * OpenCL: Test if a hit surface point is in the shadow.
 * @param {cl_float} timeSinceStart Time since start of the program in seconds.
 */
void PathTracer::clShadowTest( cl_float timeSinceStart ) {
	mCL->setKernelArg( mKernelShadowTest, 1, sizeof( cl_float ), &timeSinceStart );

	mCL->execute( mKernelShadowTest );
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
	this->clShadowTest( timeSinceStart );
	this->clSetColors( timeSinceStart );

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
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufLights );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufOrigins );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufRays );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufScNormals );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufScFacesVN );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufKdNonLeaves );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufKdLeaves );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_float8 ), &mKdRootBB );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufKdNodeFaces );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_int ), &mKdRootNodeIndex );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufHits );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufHitNormals );
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
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_float4 ), &initRayParts );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufEye );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufOrigins );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufRays );
	++i; // 5: timeSinceStart
}


void PathTracer::initArgsKernelSetColors() {
	cl_uint i = 0;

	++i; // 1: timeSinceStart
	++i; // 2: textureWeight
	mCL->setKernelArg( mKernelSetColors, ++i, sizeof( cl_mem ), &mBufHits );
	mCL->setKernelArg( mKernelSetColors, ++i, sizeof( cl_mem ), &mBufHitNormals );
	mCL->setKernelArg( mKernelSetColors, ++i, sizeof( cl_mem ), &mBufLights );
	mCL->setKernelArg( mKernelSetColors, ++i, sizeof( cl_mem ), &mBufMaterialToFace );
	mCL->setKernelArg( mKernelSetColors, ++i, sizeof( cl_mem ), &mBufMaterialsColorDiffuse );
	mCL->setKernelArg( mKernelSetColors, ++i, sizeof( cl_mem ), &mBufTextureIn );
	mCL->setKernelArg( mKernelSetColors, ++i, sizeof( cl_mem ), &mBufTextureOut );
}


/**
 * Init the kernel arguments for the OpenCL kernel to do the shadow tests.
 */
void PathTracer::initArgsKernelShadowTest() {
	cl_uint i = 0;

	++i; // 1: timeSinceStart
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_mem ), &mBufLights );
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_mem ), &mBufKdNonLeaves );
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_mem ), &mBufKdLeaves );
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_float8 ), &mKdRootBB );
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_mem ), &mBufKdNodeFaces );
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_int ), &mKdRootNodeIndex );
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_mem ), &mBufHits );
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_mem ), &mBufHitNormals );
}


/**
 * Set the OpenCL kernel arguments that either don't change or point to a CL buffer.
 */
void PathTracer::initKernelArgs() {
	this->initArgsKernelRays();
	this->initArgsKernelPathTracing();
	this->initArgsKernelShadowTest();
	this->initArgsKernelSetColors();
}


/**
 * Init the needed OpenCL buffers: Faces, vertices, camera eye and rays.
 * @param {std::vector<cl_float>*} vertices  Vertices of the model.
 * @param {std::vector<cl_uint>*}  faces     Faces (triangles) of the model.
 * @param {std::vector<kdNode_t>}  kdNodes   The nodes of the kd-tree.
 * @param {cl_uint}                rootIndex Index of the root kd node.
 */
void PathTracer::initOpenCLBuffers(
	vector<cl_float> vertices, vector<cl_uint> faces, vector<cl_float> normals,
	vector<cl_uint> facesVN, vector<cl_int> facesMtl, vector<material_t> materials,
	vector<light_t> lights,
	vector<kdNode_t> kdNodes, cl_uint rootIndex
) {
	cl_uint pixels = mWidth * mHeight;
	mKdRootNodeIndex = rootIndex;

	vector<kdNonLeaf_cl> kdNonLeaves;
	vector<kdLeaf_cl> kdLeaves;
	vector<cl_float> kdFaces; // [a, b, c, ..., faceIndex]

	this->kdNodesToVectors( kdNodes, &kdFaces, &kdNonLeaves, &kdLeaves, faces, vertices );

	cl_float8 kdRootBB = {
		kdNodes[rootIndex].bbMin[0],
		kdNodes[rootIndex].bbMin[1],
		kdNodes[rootIndex].bbMin[2],
		0.0f,
		kdNodes[rootIndex].bbMax[0],
		kdNodes[rootIndex].bbMax[1],
		kdNodes[rootIndex].bbMax[2],
		0.0f
	};
	mKdRootBB = kdRootBB;

	mBufKdNonLeaves = mCL->createBuffer( kdNonLeaves, sizeof( kdNonLeaf_cl ) * kdNonLeaves.size() );
	mBufKdLeaves = mCL->createBuffer( kdLeaves, sizeof( kdLeaf_cl ) * kdLeaves.size() );
	mBufKdNodeFaces = mCL->createBuffer( kdFaces, sizeof( cl_float ) * kdFaces.size() );

	vector<cl_float4> diffuseColors; // [r, g, b]
	for( int i = 0; i < materials.size(); i++ ) {
		cl_float4 d = { materials[i].Kd[0], materials[i].Kd[1], materials[i].Kd[2], 0.0f };
		diffuseColors.push_back( d );
	}
	mBufMaterialsColorDiffuse = mCL->createBuffer( diffuseColors, sizeof( cl_float4 ) * diffuseColors.size() );
	mBufMaterialToFace = mCL->createBuffer( facesMtl, sizeof( cl_int ) * facesMtl.size() );


	vector<cl_float4> normalsFloat4;

	for( int i = 0; i < normals.size(); i += 3 ) {
		cl_float4 n = {
			normals[i],
			normals[i + 1],
			normals[i + 2],
			0.0f
		};
		normalsFloat4.push_back( n );
	}

	mBufScNormals = mCL->createBuffer( normalsFloat4, sizeof( cl_float4 ) * normalsFloat4.size() );
	mBufScFacesVN = mCL->createBuffer( facesVN, sizeof( cl_uint ) * facesVN.size() );


	mBufEye = mCL->createEmptyBuffer( sizeof( cl_float ) * 12, CL_MEM_READ_ONLY );
	mBufOrigins = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels, CL_MEM_READ_WRITE );
	mBufRays = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels, CL_MEM_READ_WRITE );
	mBufNormals = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels, CL_MEM_READ_WRITE );

	mBufHits = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels * mBounces, CL_MEM_READ_WRITE );
	mBufHitNormals = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels * mBounces, CL_MEM_READ_WRITE );

	mTextureOut = vector<cl_float>( pixels * 4, 0.0f );
	mBufTextureIn = mCL->createImage2DReadOnly( mWidth, mHeight, &mTextureOut[0] );
	mBufTextureOut = mCL->createImage2DWriteOnly( mWidth, mHeight );

	mBufLights = mCL->createEmptyBuffer( sizeof( cl_float4 ) * lights.size(), CL_MEM_READ_ONLY );
	this->updateLights( lights );

	this->initKernelArgs();
}


/**
 * Store the kd-nodes data in seperate lists for each data type
 * in order to pass it to OpenCL.
 * @param {std::vector<KdNode_t>}  kdNodes  The nodes to extract the data from.
 * @param {std::vector<cl_float>*} kdSplits Data: [x, y, z]
 * @param {std::vector<cl_float>*} kdBB     Data: [x, y, z, x, y, z]
 * @param {std::vector<cl_int>*}   kdMeta   Data: [left, right, axis, facesIndex, numFaces, ropesIndex]
 * @param {std::vector<cl_int>*}   kdFaces  Data: [a, b, c ..., faceIndex]
 * @param {std::vector<cl_int>*}   kdRopes  Data: [left, right, bottom, top, back, front]
 * @param {std::vector<cl_uint>*}  faces    Faces.
 * @param {std::vector<cl_float>*} vertices Vertices.
 */
void PathTracer::kdNodesToVectors(
	vector<kdNode_t> kdNodes, vector<cl_float>* kdFaces,
	vector<kdNonLeaf_cl>* kdNonLeaves, vector<kdLeaf_cl>* kdLeaves,
	vector<cl_uint> faces, vector<cl_float> vertices
) {
	cl_uint a, b, c, pos;

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
				( kdNodes[i].left->axis < 0 ), // is left node a leaf
				( kdNodes[i].right->axis < 0 ) // is right node a leaf
			};
			node.children = children;

			kdNonLeaves->push_back( node );
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

			// Set highest bit as flag for being a leaf node.
			// The -1 isn't going to be a problem, because the entryDistance < exitDistance condition
			// in the kernel will stop the loop, because the wrong index can cause any harm.
			ropes.s0 = ( ropes.s0 != 0 && nodeRopes[0]->axis < 0 ) ? -ropes.s0 : ropes.s0;
			ropes.s1 = ( ropes.s1 != 0 && nodeRopes[1]->axis < 0 ) ? -ropes.s1 : ropes.s1;
			ropes.s2 = ( ropes.s2 != 0 && nodeRopes[2]->axis < 0 ) ? -ropes.s2 : ropes.s2;
			ropes.s3 = ( ropes.s3 != 0 && nodeRopes[3]->axis < 0 ) ? -ropes.s3 : ropes.s3;
			ropes.s4 = ( ropes.s4 != 0 && nodeRopes[4]->axis < 0 ) ? -ropes.s4 : ropes.s4;
			ropes.s5 = ( ropes.s5 != 0 && nodeRopes[5]->axis < 0 ) ? -ropes.s5 : ropes.s5;

			// Index of faces of this node in kdFaces
			ropes.s6 = ( kdNodes[i].faces.size() > 0 ) ? kdFaces->size() : -1;

			// Number of faces in this node
			ropes.s7 = kdNodes[i].faces.size() / 3 * 10;

			node.ropes = ropes;

			// Bounding box
			cl_float4 bbMin = { kdNodes[i].bbMin[0], kdNodes[i].bbMin[1], kdNodes[i].bbMin[2], 0.0f };
			cl_float4 bbMax = { kdNodes[i].bbMax[0], kdNodes[i].bbMax[1], kdNodes[i].bbMax[2], 0.0f };
			node.bbMin = bbMin;
			node.bbMax = bbMax;

			kdLeaves->push_back( node );
		}

		// Faces
		for( cl_uint j = 0; j < kdNodes[i].faces.size(); j += 3 ) {
			a = kdNodes[i].faces[j] * 3;
			kdFaces->push_back( vertices[a] );
			kdFaces->push_back( vertices[a + 1] );
			kdFaces->push_back( vertices[a + 2] );

			b = kdNodes[i].faces[j + 1] * 3;
			kdFaces->push_back( vertices[b] );
			kdFaces->push_back( vertices[b + 1] );
			kdFaces->push_back( vertices[b + 2] );

			c = kdNodes[i].faces[j + 2] * 3;
			kdFaces->push_back( vertices[c] );
			kdFaces->push_back( vertices[c + 1] );
			kdFaces->push_back( vertices[c + 2] );

			// Find index of face in original (complete) face list.
			// This index will be needed to assign the right material to the face.
			pos = 0;

			for( cl_uint k = 0; k < faces.size(); k += 3 ) {
				if( faces[k] == a / 3 && faces[k + 1] == b / 3 && faces[k + 2] == c / 3 ) {
					pos = k / 3;
					break;
				}
			}

			kdFaces->push_back( (cl_float) pos );
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


/**
 * Update the lights buffer.
 * @param {std::vector<light_t>} lights List of all lights.
 */
void PathTracer::updateLights( vector<light_t> lights ) {
	vector<cl_float4> clLights;
	for( int i = 0; i < lights.size(); i++ ) {
		cl_float4 l;
		l.x = lights[i].position[0];
		l.y = lights[i].position[1];
		l.z = lights[i].position[2];
		l.w = 0.0f;
		clLights.push_back( l );
	}
	mCL->updateBuffer( mBufLights, sizeof( cl_float4 ) * clLights.size(), &clLights[0] );
	mSampleCount = 0;
}
