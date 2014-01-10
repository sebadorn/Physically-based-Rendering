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
	mCL->setKernelArg( mKernelRays, 1, sizeof( cl_float ), &timeSinceStart );

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
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufScVertices );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufScFaces );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufScNormals );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufScFacesVN );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufKdNonLeaves );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufKdLeaves );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufKdFaces );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_float8 ), &mKdRootBB );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufRays );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufFaceToMaterial );
	mCL->setKernelArg( mKernelPathTracing, ++i, sizeof( cl_mem ), &mBufMaterials );
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


void PathTracer::initArgsKernelSetColors() {
	cl_uint i = 0;

	++i; // 1: timeSinceStart
	++i; // 2: textureWeight
	mCL->setKernelArg( mKernelSetColors, ++i, sizeof( cl_mem ), &mBufRays );
	mCL->setKernelArg( mKernelSetColors, ++i, sizeof( cl_mem ), &mBufLights );
	mCL->setKernelArg( mKernelSetColors, ++i, sizeof( cl_mem ), &mBufFaceToMaterial );
	mCL->setKernelArg( mKernelSetColors, ++i, sizeof( cl_mem ), &mBufMaterials );
	mCL->setKernelArg( mKernelSetColors, ++i, sizeof( cl_mem ), &mBufSPDs );
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
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_mem ), &mBufScVertices );
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_mem ), &mBufScFaces );
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_mem ), &mBufKdNonLeaves );
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_mem ), &mBufKdLeaves );
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_float8 ), &mKdRootBB );
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_mem ), &mBufKdFaces );
	mCL->setKernelArg( mKernelShadowTest, ++i, sizeof( cl_mem ), &mBufRays );
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
 * @param {std::vector<cl_float>} vertices Vertices of the model.
 * @param {std::vector<cl_uint>}  faces    Faces (triangles) of the model.
 * @param {std::vector<cl_float>} normals  Normals of the model.
 * @param {ModelLoader*}          ml       Model loader already holding the needed model data.
 * @param {std::vector<light_t>}  lights   Lights of the scene.
 * @param {std::vector<kdNode_t>} kdNodes  Nodes of the kd-tree.
 * @param {kdNode_t*}             rootNode Root node of the kd-tree.
 */
void PathTracer::initOpenCLBuffers(
	vector<cl_float> vertices, vector<cl_uint> faces, vector<cl_float> normals,
	ModelLoader* ml, vector<light_t> lights, vector<kdNode_t> kdNodes, kdNode_t* rootNode
) {
	this->initOpenCLBuffers_KdTree( vertices, faces, kdNodes, rootNode );
	this->initOpenCLBuffers_Materials( ml );
	this->initOpenCLBuffers_Normals( normals, ml );
	this->initOpenCLBuffers_Rays();
	this->initOpenCLBuffers_Textures();
	this->initOpenCLBuffers_Lights( lights );

	this->initKernelArgs();
}


/**
 * Init OpenCL buffers for the kd-tree.
 * @param {std::vector<cl_float>} vertices Vertices of the model.
 * @param {std::vector<cl_uint>}  faces    Faces (triangles) of the model.
 * @param {std::vector<kdNode_t>} kdNodes  Nodes of the kd-tree.
 * @param {kdNode_t*}             rootNode Root node of the kd-tree.
 */
void PathTracer::initOpenCLBuffers_KdTree(
	vector<cl_float> vertices, vector<cl_uint> faces,
	vector<kdNode_t> kdNodes, kdNode_t* rootNode
) {
	vector<cl_float4> vertices4;
	vector<cl_uint4> faces4;

	for( int i = 0; i < faces.size(); i += 3 ) {
		cl_uint4 f = {
			faces[i],
			faces[i + 1],
			faces[i + 2],
			0
		};
		faces4.push_back( f );
	}
	for( int i = 0; i < vertices.size(); i += 3 ) {
		cl_float4 v = {
			vertices[i],
			vertices[i + 1],
			vertices[i + 2],
			0.0f
		};
		vertices4.push_back( v );
	}

	mBufScVertices = mCL->createBuffer( vertices4, sizeof( cl_float4 ) * vertices4.size() );
	mBufScFaces = mCL->createBuffer( faces4, sizeof( cl_uint4 ) * faces4.size() );


	vector<kdNonLeaf_cl> kdNonLeaves;
	vector<kdLeaf_cl> kdLeaves;
	vector<cl_uint> kdFaces;

	this->kdNodesToVectors( vertices4, faces4, kdNodes, &kdFaces, &kdNonLeaves, &kdLeaves );

	cl_float8 kdRootBB = {
		rootNode->bbMin[0], rootNode->bbMin[1], rootNode->bbMin[2], 0.0f,
		rootNode->bbMax[0], rootNode->bbMax[1], rootNode->bbMax[2], 0.0f
	};
	mKdRootBB = kdRootBB;

	mBufKdNonLeaves = mCL->createBuffer( kdNonLeaves, sizeof( kdNonLeaf_cl ) * kdNonLeaves.size() );
	mBufKdLeaves = mCL->createBuffer( kdLeaves, sizeof( kdLeaf_cl ) * kdLeaves.size() );
	mBufKdFaces = mCL->createBuffer( kdFaces, sizeof( cl_uint ) * kdFaces.size() );
}


/**
 * Init OpenCL buffers for the lights.
 * @param {std::vector<light_t>} lights Lights of the scene.
 */
void PathTracer::initOpenCLBuffers_Lights( vector<light_t> lights ) {
	mBufLights = mCL->createEmptyBuffer( sizeof( cl_float4 ) * lights.size(), CL_MEM_READ_ONLY );
	this->updateLights( lights );
}


/**
 * Init OpenCL buffers for the materials, including spectral power distributions.
 * @param {ModelLoader*} ml Model loader already holding the needed model data.
 */
void PathTracer::initOpenCLBuffers_Materials( ModelLoader* ml ) {
	vector<cl_int> facesMtl = ml->getFacesMtl();
	vector<material_t> materials = ml->getMaterials();
	map<string, string> mtl2spd = ml->getMaterialToSPD();
	map<string, vector<cl_float> > spectra = ml->getSpectralPowerDistributions();

	vector<cl_float> spd, spectraCL;
	map<string, cl_uint> specID;
	map<string, vector<cl_float> >::iterator it;
	int specCounter = 0;

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
		mtl.Ks = materials[i].Ks;
		mtl.d = materials[i].d;
		mtl.Ni = materials[i].Ni;
		mtl.Ns = materials[i].Ns;
		mtl.gloss = materials[i].gloss;
		mtl.illum = materials[i].illum;

		string spdName = mtl2spd[materials[i].mtlName];
		mtl.spdDiffuse = specID[spdName];

		materialsCL.push_back( mtl );
	}


	mBufFaceToMaterial = mCL->createBuffer( facesMtl, sizeof( cl_int ) * facesMtl.size() );
	mBufMaterials = mCL->createBuffer( materialsCL, sizeof( material_cl_t ) * materialsCL.size() );
	mBufSPDs = mCL->createBuffer( spectraCL, sizeof( cl_float ) * spectraCL.size() );
}


/**
 * Init OpenCL buffers of the normals.
 * @param {std::vector<cl_float>} normals Normals of the model.
 * @param {ModelLoader*}          ml      Model loader already holding the needed model data.
 */
void PathTracer::initOpenCLBuffers_Normals( vector<cl_float> normals, ModelLoader* ml ) {
	vector<cl_uint> facesVN = ml->getFacesVN();
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
}


/**
 * Init OpenCL buffers of the rays.
 */
void PathTracer::initOpenCLBuffers_Rays() {
	cl_uint pixels = mWidth * mHeight;
	mBufEye = mCL->createEmptyBuffer( sizeof( cl_float ) * 12, CL_MEM_READ_ONLY );
	mBufNormals = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels, CL_MEM_READ_WRITE );
	mBufRays = mCL->createEmptyBuffer( sizeof( ray4 ) * pixels * mBounces, CL_MEM_READ_WRITE );
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
 * @param {std::vector<cl_float>*}     vertices    Vertices.
 * @param {std::vector<cl_uint>*}      faces       Faces.
 * @param {std::vector<KdNode_t>}      kdNodes     All nodes of the kd-tree.
 * @param {std::vector<cl_float>*}     kdFaces     Data: [a, b, c ..., faceIndex]
 * @param {std::vector<kdNonLeaf_cl>*} kdNonLeaves Output: Nodes of the kd-tree, that aren't leaves.
 * @param {std::vector<kdLeaf_cl>*}    kdLeaves    Output: Leaf nodes of the kd-tree.
 */
void PathTracer::kdNodesToVectors(
	vector<cl_float4> vertices4, vector<cl_uint4> faces4,
	vector<kdNode_t> kdNodes, vector<cl_uint>* kdFaces,
	vector<kdNonLeaf_cl>* kdNonLeaves, vector<kdLeaf_cl>* kdLeaves
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
				( kdNodes[i].left->axis < 0 ) ? 1 : 0, // is left node a leaf
				( kdNodes[i].right->axis < 0 ) ? 1 : 0 // is right node a leaf
			};
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

			// Set highest bit as flag for being a leaf node.
			// The -1 isn't going to be a problem, because the entryDistance < exitDistance condition
			// in the kernel will stop the loop.
			ropes.s0 *= ( ropes.s0 != 0 && nodeRopes[0]->axis < 0 ) ? -1 : 1;
			ropes.s1 *= ( ropes.s1 != 0 && nodeRopes[1]->axis < 0 ) ? -1 : 1;
			ropes.s2 *= ( ropes.s2 != 0 && nodeRopes[2]->axis < 0 ) ? -1 : 1;
			ropes.s3 *= ( ropes.s3 != 0 && nodeRopes[3]->axis < 0 ) ? -1 : 1;
			ropes.s4 *= ( ropes.s4 != 0 && nodeRopes[4]->axis < 0 ) ? -1 : 1;
			ropes.s5 *= ( ropes.s5 != 0 && nodeRopes[5]->axis < 0 ) ? -1 : 1;

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
				cl_uint4 f = {
					kdNodes[i].faces[j],
					kdNodes[i].faces[j + 1],
					kdNodes[i].faces[j + 2],
					0
				};

				pos = -1;

				for( cl_uint k = 0; k < faces4.size(); k++ ) {
					if( faces4[k].x == f.x && faces4[k].y == f.y && faces4[k].z == f.z ) {
						pos = k;
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
