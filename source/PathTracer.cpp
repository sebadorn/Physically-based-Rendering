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

	mStructCam.focusPoint.x = -1;
	mStructCam.focusPoint.y = -1;
	mStructCam.lense.x = Cfg::get().value<cl_float>( Cfg::CAM_LENSE_FOCALLENGTH );
	mStructCam.lense.y = Cfg::get().value<cl_float>( Cfg::CAM_LENSE_APERTURE );
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
	mCL->setKernelArg( mKernelPathTracing, 3, sizeof( camera_cl ), &mStructCam );

	mCL->execute( mKernelPathTracing );
	mCL->finish();
}


/**
 * Generate the path traced image, which is basically just a 2D texture.
 * @return {std::vector<cl_float>} Float vector representing a 2D image.
 */
vector<cl_float> PathTracer::generateImage( vector<cl_float>* textureDebug ) {
	this->updateEyeBuffer();
	mCL->updateImageReadOnly( mBufTextureIn, mWidth, mHeight, &mTextureOut[0] );

	cl_float timeSinceStart = this->getTimeSinceStart();
	this->clPathTracing( timeSinceStart );

	mCL->readImageOutput( mBufTextureOut, mWidth, mHeight, &mTextureOut[0] );
	mCL->readImageOutput( mBufTextureDebug, mWidth, mHeight, &(*textureDebug)[0] );
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
 */
void PathTracer::initKernelArgs() {
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
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( camera_cl ), &mStructCam );

	switch( Cfg::get().value<int>( Cfg::ACCEL_STRUCT ) ) {

		case ACCELSTRUCT_BVH:
			mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufBVH );
			break;

		default:
			Logger::logError( "[PathTracer] Unknown acceleration structure." );
			exit( EXIT_FAILURE );

	}

	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufFaces );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufVertices );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufNormals );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufMaterials );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufLights );

	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufTextureIn );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufTextureOut );
	mCL->setKernelArg( mKernelPathTracing, i++, sizeof( cl_mem ), &mBufTextureDebug );
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

	#define MSG_LENGTH 128
	char msg[MSG_LENGTH];

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
	snprintf( msg, MSG_LENGTH, "[PathTracer] Created faces buffer in %g ms -- %.2f %s.", timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	// Buffer: Acceleration Structure
	timerStart = boost::posix_time::microsec_clock::local_time();
	const short usedAccelStruct = Cfg::get().value<short>( Cfg::ACCEL_STRUCT );
	string accelName;

	if( usedAccelStruct == ACCELSTRUCT_BVH ) {
		bytes = this->initOpenCLBuffers_BVH( (BVH*) accelStruc, ml, faces );
		accelName = "BVH";
	}

	timerEnd = boost::posix_time::microsec_clock::local_time();
	timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	utils::formatBytes( bytes, &bytesFloat, &unit );
	snprintf( msg, MSG_LENGTH, "[PathTracer] Created %s buffer in %g ms -- %.2f %s.", accelName.c_str(), timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	// Buffer: Material(s)
	timerStart = boost::posix_time::microsec_clock::local_time();
	bytes = this->initOpenCLBuffers_Materials( ml );
	timerEnd = boost::posix_time::microsec_clock::local_time();
	timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	utils::formatBytes( bytes, &bytesFloat, &unit );
	snprintf( msg, MSG_LENGTH, "[PathTracer] Created material buffer in %g ms -- %.2f %s.", timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	// Buffer: Light(s)
	timerStart = boost::posix_time::microsec_clock::local_time();
	bytes = this->initOpenCLBuffers_Lights( ml );
	timerEnd = boost::posix_time::microsec_clock::local_time();
	timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	utils::formatBytes( bytes, &bytesFloat, &unit );
	snprintf( msg, MSG_LENGTH, "[PathTracer] Created light buffer in %g ms -- %.2f %s.", timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	snprintf( msg, MSG_LENGTH, "%lu", mLights.size() );
	mCL->setReplacement( string( "#NUM_LIGHTS#" ), string( msg ) );

	// Buffer: Textures
	timerStart = boost::posix_time::microsec_clock::local_time();
	bytes = this->initOpenCLBuffers_Textures();
	timerEnd = boost::posix_time::microsec_clock::local_time();
	timeDiff = ( timerEnd - timerStart ).total_milliseconds();
	utils::formatBytes( bytes, &bytesFloat, &unit );
	snprintf( msg, MSG_LENGTH, "[PathTracer] Created texture buffer in %g ms -- %.2f %s.", timeDiff, bytesFloat, unit.c_str() );
	Logger::logInfo( msg );

	Logger::indent( 0 );
	Logger::logInfo( "[PathTracer] ... Done." );


	mCL->loadProgram( Cfg::get().value<string>( Cfg::OPENCL_PROGRAM ) );
	mKernelPathTracing = mCL->createKernel( "pathTracing" );
	mGLWidget->createKernelWindow( mCL );

	this->initKernelArgs();
}


/**
 * Init OpenCL buffers for the BVH.
 * @param  {BVH*}   bvh The generated Bounding Volume Hierarchy.
 * @return {size_t}     Buffer size.
 */
size_t PathTracer::initOpenCLBuffers_BVH( BVH* bvh, ModelLoader* ml, vector<cl_uint> faces ) {
	vector<BVHNode*> bvhNodes = bvh->getNodes();
	vector<bvhNode_cl> bvhNodesCL;

	vector<cl_uint> facesVN = ml->getObjParser()->getFacesVN();
	vector<cl_int> facesMtl = ml->getObjParser()->getFacesMtl();
	vector<face_cl> faceStructs;

	for( cl_uint i = 0; i < bvhNodes.size(); i++ ) {
		BVHNode* node = bvhNodes[i];

		cl_float4 bbMin = { node->bbMin[0], node->bbMin[1], node->bbMin[2], 0.0f };
		cl_float4 bbMax = { node->bbMax[0], node->bbMax[1], node->bbMax[2], 0.0f };

		bvhNode_cl sn;
		sn.bbMin = bbMin;
		sn.bbMax = bbMax;

		vector<Tri> facesVec = node->faces;
		cl_uint fvecLen = facesVec.size();
		sn.bbMin.w = ( fvecLen > 0 ) ? (cl_float) faceStructs.size() + 0 : -1.0f;
		sn.bbMax.w = ( fvecLen > 1 ) ? (cl_float) faceStructs.size() + 1 : -1.0f;

		// Set the flag to skip the next left child node.
		if( fvecLen == 0 && node->skipNextLeft ) {
			sn.bbMin.w = -2.0f;
		}


		// No parent means it's the root node.
		// Otherwise it is some other node, including leaves.
		// Also for leaf nodes the next node to visit is given by the position in memory.
		if( node->parent != NULL && fvecLen == 0 ) {
			bool isLeftNode = ( node->parent->leftChild == node );

			if( !isLeftNode ) {
				if( node->parent->parent != NULL ) {
					BVHNode* dummy = new BVHNode();
					dummy->parent = node->parent;

					// As long as we are on the right side of a (sub)tree,
					// skip parents until we either are at the root or
					// our parent has a true sibling again.
					while( dummy->parent->parent->rightChild == dummy->parent ) {
						dummy->parent = dummy->parent->parent;

						if( dummy->parent->parent == NULL ) {
							break;
						}
					}

					// Reached a parent with a true sibling.
					if( dummy->parent->parent != NULL ) {
						sn.bbMax.w = dummy->parent->parent->rightChild->id;
					}
				}
			}
			// Node on the left, go to the right sibling.
			else {
				sn.bbMax.w = node->parent->rightChild->id;
			}
		}

		bvhNodesCL.push_back( sn );

		// Faces
		for( int i = 0; i < fvecLen; i++) {
			Tri tri = facesVec[i];
			face_cl face;

			face.vertices.x = faces[tri.face.w * 3];
			face.vertices.y = faces[tri.face.w * 3 + 1];
			face.vertices.z = faces[tri.face.w * 3 + 2];
			// Material of face
			face.vertices.w = facesMtl[tri.face.w];

			face.normals.x = facesVN[tri.normals.w * 3];
			face.normals.y = facesVN[tri.normals.w * 3 + 1];
			face.normals.z = facesVN[tri.normals.w * 3 + 2];
			face.normals.w = 0;

			faceStructs.push_back( face );
		}
	}

	size_t bytesBVH = sizeof( bvhNode_cl ) * bvhNodesCL.size();
	mBufBVH = mCL->createBuffer( bvhNodesCL, bytesBVH );

	char msg[16];
	snprintf( msg, 16, "%lu", bvhNodesCL.size() );
	mCL->setReplacement( string( "#BVH_NUM_NODES#" ), string( msg ) );

	size_t bytesF = sizeof( face_cl ) * faceStructs.size();
	mBufFaces = mCL->createBuffer( faceStructs, bytesF );

	return bytesBVH + bytesF;
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
	vector<cl_float4> vertices4;
	vector<cl_float4> normals4;
	size_t bytesF = 0;

	// BVH creates the faces buffer alongside the BVH buffer so the faces
	// can be ordered by appearance in leaf nodes.
	if( Cfg::get().value<short>( Cfg::ACCEL_STRUCT ) != ACCELSTRUCT_BVH ) {
		vector<cl_uint> facesVN = ml->getObjParser()->getFacesVN();
		vector<cl_int> facesMtl = ml->getObjParser()->getFacesMtl();
		vector<face_cl> faceStructs;

		// Convert array of face indices to own face struct.
		for( int i = 0; i < faces.size(); i += 3 ) {
			face_cl face;

			face.vertices.x = faces[i];
			face.vertices.y = faces[i + 1];
			face.vertices.z = faces[i + 2];
			// Material of face
			face.vertices.w = facesMtl[i / 3];

			face.normals.x = facesVN[i];
			face.normals.y = facesVN[i + 1];
			face.normals.z = facesVN[i + 2];
			face.normals.w = 0;

			faceStructs.push_back( face );
		}

		bytesF = sizeof( face_cl ) * faceStructs.size();

		mBufFaces = mCL->createBuffer( faceStructs, bytesF );
	}

	for( int i = 0; i < vertices.size(); i += 3 ) {
		cl_float4 v = { vertices[i], vertices[i + 1], vertices[i + 2], 0.0f };
		vertices4.push_back( v );
	}

	for( int i = 0; i < normals.size(); i += 3 ) {
		cl_float4 n = { normals[i], normals[i + 1], normals[i + 2], 0.0f };
		normals4.push_back( n );
	}

	size_t bytesV = sizeof( cl_float4 ) * vertices4.size();
	size_t bytesN = sizeof( cl_float4 ) * normals4.size();

	mBufVertices = mCL->createBuffer( vertices4, bytesV );
	mBufNormals = mCL->createBuffer( normals4, bytesN );

	return bytesF + bytesV + bytesN;
}


/**
 * Init OpenCL buffers for the lights.
 * @param {ModelLoader*} ml Model loader already holding the needed model data.
 */
size_t PathTracer::initOpenCLBuffers_Lights( ModelLoader* ml ) {
	vector<light_t> lights = ml->getObjParser()->getLights();

	for( int i = 0; i < lights.size(); i++ ) {
		light_cl light;
		light.pos = lights[i].pos;
		light.rgb = lights[i].rgb;
		light.data.x = lights[i].type;

		// Point light
		if( light.data.x == 1 ) {
			light.data.y = 0.0f;
			light.data.z = 0.0f;
			light.data.w = 0.0f;
		}
		// Orb
		if( light.data.x == 2 ) {
			light.data.y = lights[i].radius;
			light.data.z = 0.0f;
			light.data.w = 0.0f;
		}

		mLights.push_back( light );
	}

	bool noLights = false;

	if( mLights.size() == 0 ) {
		noLights = true;
		light_cl light;
		mLights.push_back( light );
	}

	size_t bytes = sizeof( light_cl ) * mLights.size();
	mBufLights = mCL->createBuffer( mLights, bytes );

	if( noLights ) {
		mLights.clear();
	}

	return bytes;
}


/**
 * Init OpenCL buffers for the materials, including spectral power distributions.
 * @param {ModelLoader*} ml Model loader already holding the needed model data.
 */
size_t PathTracer::initOpenCLBuffers_Materials( ModelLoader* ml ) {
	vector<material_t> materials = ml->getObjParser()->getMaterials();
	size_t bytesMTL = this->initOpenCLBuffers_MaterialsRGB( materials );

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
			mtl.data.s0 = materials[i].d;
			mtl.data.s1 = materials[i].Ni;
			mtl.data.s2 = materials[i].p;
			mtl.data.s3 = materials[i].rough;
			mtl.rgbDiff = materials[i].Kd;
			mtl.rgbSpec = materials[i].Ks;

			materialsCL.push_back( mtl );

			if( materials[i].mtlName == "sky_light" ) {
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
			mtl.data.s0 = materials[i].d;
			mtl.data.s1 = materials[i].Ni;
			mtl.data.s2 = materials[i].nu;
			mtl.data.s3 = materials[i].nv;
			mtl.data.s4 = materials[i].Rs;
			mtl.data.s5 = materials[i].Rd;
			mtl.rgbDiff = materials[i].Kd;
			mtl.rgbSpec = materials[i].Ks;

			materialsCL.push_back( mtl );

			if( materials[i].mtlName == "sky_light" ) {
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
 * Init OpenCL buffers of the textures.
 */
size_t PathTracer::initOpenCLBuffers_Textures() {
	mTextureOut = vector<cl_float>( mWidth * mHeight * 4, 0.0f );

	mBufTextureIn = mCL->createImage2DReadOnly( mWidth, mHeight, &mTextureOut[0] );
	mBufTextureOut = mCL->createImage2DWriteOnly( mWidth, mHeight );
	mBufTextureDebug = mCL->createImage2DWriteOnly( mWidth, mHeight );

	return sizeof( cl_float ) * mTextureOut.size() * 3.0f;
}


/**
 * Move the position of the sun. This will also reset the sample count.
 * @param {const int} key Pressed key.
 */
void PathTracer::moveSun( const int key ) {
	// switch( key ) {

	// 	case Qt::Key_W:
	// 		mSunPos.x += 0.25f;
	// 		break;

	// 	case Qt::Key_S:
	// 		mSunPos.x -= 0.25f;
	// 		break;

	// 	case Qt::Key_A:
	// 		mSunPos.z -= 0.25f;
	// 		break;

	// 	case Qt::Key_D:
	// 		mSunPos.z += 0.25f;
	// 		break;

	// 	case Qt::Key_Q:
	// 		mSunPos.y += 0.25f;
	// 		break;

	// 	case Qt::Key_E:
	// 		mSunPos.y -= 0.25f;
	// 		break;

	// }

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
 * Set the camera focus to the given pixel position.
 * Negative coordinates mean no point focus.
 * @param {int} x X coordinate.
 * @param {int} y Y coordinate.
 */
void PathTracer::setFocus( int x, int y ) {
	mStructCam.focusPoint.x = x;
	mStructCam.focusPoint.y = y;

	this->resetSampleCount();
	mGLWidget->resetRenderTime();
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

	mStructCam.eye.x = eye[0];
	mStructCam.eye.y = eye[1];
	mStructCam.eye.z = eye[2];

	mStructCam.w.x = w[0];
	mStructCam.w.y = w[1];
	mStructCam.w.z = w[2];

	mStructCam.u.x = u[0];
	mStructCam.u.y = u[1];
	mStructCam.u.z = u[2];

	mStructCam.v.x = v[0];
	mStructCam.v.y = v[1];
	mStructCam.v.z = v[2];
}
