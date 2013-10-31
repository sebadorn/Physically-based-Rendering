#include "GLWidget.h"

using namespace std;


/**
 * Constructor.
 * @param {QWidget*} parent Parent QWidget this QWidget is contained in.
 */
GLWidget::GLWidget( QWidget* parent ) : QGLWidget( parent ) {
	srand( (unsigned) time( 0 ) );

	mCL = new CL();
	mCL->loadProgram( Cfg::get().value<string>( Cfg::OPENCL_PROGRAM ) );
	mKernelRays = mCL->createKernel( "generateRays" );
	mKernelIntersections = mCL->createKernel( "findIntersectionsKdTree" );
	mKernelColors = mCL->createKernel( "accumulateColors" );

	mFOV = Cfg::get().value<cl_float>( Cfg::PERS_FOV );
	mModelMatrix = glm::mat4( 1.0f );

	mKdTree = NULL;

	mDoRendering = false;
	mFrameCount = 0;
	mPreviousTime = 0;
	mCamera = new Camera( this );
	mSampleCount = 0;
	mSelectedLight = -1;

	mViewBoundingBox = false;
	mViewKdTree = false;
	mViewOverlay = false;
	mViewTracer = true;

	mTimer = new QTimer( this );
	connect( mTimer, SIGNAL( timeout() ), this, SLOT( update() ) );

	mTimeSinceStart = boost::posix_time::microsec_clock::local_time();

	this->startRendering();
}


/**
 * Destructor.
 */
GLWidget::~GLWidget() {
	this->stopRendering();
	this->deleteOldModel();
	glDeleteTextures( mTargetTextures.size(), &mTargetTextures[0] );

	if( mKdTree ) {
		delete mKdTree;
	}
}


/**
 * Calculate the matrices for view, model, model-view-projection and normals.
 */
void GLWidget::calculateMatrices() {
	if( !mDoRendering ) {
		return;
	}

	mSampleCount = 0;

	glm::vec3 e = mCamera->getEye_glmVec3();
	glm::vec3 c = mCamera->getAdjustedCenter_glmVec3();
	glm::vec3 u = mCamera->getUp_glmVec3();

	mViewMatrix = glm::lookAt( e, c, u );
	mModelViewProjectionMatrix = mProjectionMatrix * mViewMatrix * mModelMatrix;
}


/**
 * The camera has changed. Handle it.
 */
void GLWidget::cameraUpdate() {
	this->calculateMatrices();
}


/**
 * Check if an error occured in the last executed OpenGL function.
 */
void GLWidget::checkGLForErrors() {
	if( glGetError() != 0 ) {
		Logger::logDebug( (const char*) gluErrorString( glGetError() ) );
	}
}


/**
 * Check if there is an error in the current status of the framebuffer.
 */
void GLWidget::checkFramebufferForErrors() {
	GLenum err = glCheckFramebufferStatus( GL_FRAMEBUFFER );

	if( err != GL_FRAMEBUFFER_COMPLETE ) {
		string errMsg( "[OpenGL] Error configuring framebuffer: " );

		switch( err ) {
			case GL_FRAMEBUFFER_UNDEFINED:
				errMsg.append( "GL_FRAMEBUFFER_UNDEFINED" );
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				errMsg.append( "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT" );
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				errMsg.append( "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT" );
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
				errMsg.append( "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER" );
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
				errMsg.append( "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER" );
				break;
			case GL_FRAMEBUFFER_UNSUPPORTED:
				errMsg.append( "GL_FRAMEBUFFER_UNSUPPORTED" );
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
				errMsg.append( "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" );
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
				errMsg.append( "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS" );
				break;
			default:
				errMsg.append( "unknown error code" );
		}

		Logger::logError( errMsg );
		exit( EXIT_FAILURE );
	}
}


void GLWidget::clAccumulateColors( float timeSinceStart ) {
	uint i = 0;
	cl_float textureWeight = mSampleCount / (cl_float) ( mSampleCount + 1 );

	mCL->setKernelArg( mKernelColors, i, sizeof( cl_mem ), &mBufOrigins );
	mCL->setKernelArg( mKernelColors, ++i, sizeof( cl_mem ), &mBufNormals );
	mCL->setKernelArg( mKernelColors, ++i, sizeof( cl_mem ), &mBufAccColors );
	mCL->setKernelArg( mKernelColors, ++i, sizeof( cl_mem ), &mBufColorMasks );
	mCL->setKernelArg( mKernelColors, ++i, sizeof( cl_float ), &textureWeight );
	mCL->setKernelArg( mKernelColors, ++i, sizeof( cl_float ), &timeSinceStart );
	mCL->setKernelArg( mKernelColors, ++i, sizeof( cl_mem ), &mKernelArgTextureIn );
	mCL->setKernelArg( mKernelColors, ++i, sizeof( cl_mem ), &mKernelArgTextureOut );

	mCL->execute( mKernelColors );
	mCL->finish();
}


/**
 * OpenCL: Find intersections of rays with scene.
 */
void GLWidget::clFindIntersections( float timeSinceStart ) {
	uint i = 0;
	cl_uint numFaces = mFaces.size();

	mCL->setKernelArg( mKernelIntersections, i, sizeof( cl_mem ), &mBufOrigins );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufRays );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufNormals );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufScNormals );

	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufBoundingBox );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufKdNodeData1 );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufKdNodeData2 );
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_mem ), &mBufKdNodeData3 );

	cl_uint rootNode = mKdTree->getRootNode()->index;
	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_uint ), &rootNode );

	mCL->setKernelArg( mKernelIntersections, ++i, sizeof( cl_float ), &timeSinceStart );

	mCL->execute( mKernelIntersections );
	mCL->finish();
}


/**
 * OpenCL: Compute the initial rays into the scene.
 */
void GLWidget::clInitRays() {
	uint i = 0;
	cl_float fovRad = utils::degToRad( mFOV );

	mCL->setKernelArg( mKernelRays, i, sizeof( cl_mem ), &mBufEye );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufVecW );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufVecU );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufVecV );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_float ), &fovRad );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufOrigins );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufRays );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufAccColors );
	mCL->setKernelArg( mKernelRays, ++i, sizeof( cl_mem ), &mBufColorMasks );

	mCL->execute( mKernelRays );
	mCL->finish();
}


/**
 * Delete data (buffers, textures) of the old model.
 */
void GLWidget::deleteOldModel() {
	// Delete old vertex array buffers
	if( mVA.size() > 0 ) {
		glDeleteBuffers( mVA.size(), &mVA[0] );
		glDeleteBuffers( 1, &mIndexBuffer );

		map<GLuint, GLuint>::iterator texIter = mTextureIDs.begin();
		while( texIter != mTextureIDs.end() ) {
			glDeleteTextures( 1, &((*texIter).second) );
			texIter++;
		}
	}
}


/**
 * Get a jitter vector in order to slightly alter the eye ray.
 * This results in anti-aliasing.
 * @return {glm::vec3} Jitter.
 */
glm::vec3 GLWidget::getJitter() {
	glm::vec3 v = glm::vec3(
		rand() / (float) RAND_MAX * 1.0f - 1.0f,
		rand() / (float) RAND_MAX * 1.0f - 1.0f,
		0.0f
	);

	v[0] *= ( 1.0f / (float) width() );
	v[1] *= ( 1.0f / (float) height() );

	return v;
}


/**
 * Initialize OpenGL and start rendering.
 */
void GLWidget::initializeGL() {
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

	// glEnable( GL_DEPTH_TEST );
	// glEnable( GL_MULTISAMPLE );

	glEnable( GL_ALPHA_TEST );
	glAlphaFunc( GL_ALWAYS, 0.0f );

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	this->initGlew();

	mVA = vector<GLuint>( NUM_VA );

	Logger::logInfo( string( "[OpenGL] Version " ).append( (char*) glGetString( GL_VERSION ) ) );
	Logger::logInfo( string( "[OpenGL] GLSL " ).append( (char*) glGetString( GL_SHADING_LANGUAGE_VERSION ) ) );

	this->initTargetTexture();
}


/**
 * Init GLew.
 */
void GLWidget::initGlew() {
	GLenum err = glewInit();

	if( err != GLEW_OK ) {
		Logger::logError( string( "[GLEW] Init failed: " ).append( (char*) glewGetErrorString( err ) ) );
		exit( EXIT_FAILURE );
	}
	Logger::logInfo( string( "[GLEW] Version " ).append( (char*) glewGetString( GLEW_VERSION ) ) );
}


/**
 * Init the needed OpenCL buffers: Faces, vertices, camera eye and rays.
 */
void GLWidget::initOpenCLBuffers() {
	int pixels = width() * height();

	vector<kdNode_t> kdNodes = mKdTree->getNodes();
	vector<cl_float> kdData1;
	vector<cl_int> kdData2;
	vector<cl_int> kdData3;

	for( uint i = 0; i < kdNodes.size(); i++ ) {
		// Vertice coordinates
		kdData1.push_back( kdNodes[i].pos[0] );
		kdData1.push_back( kdNodes[i].pos[1] );
		kdData1.push_back( kdNodes[i].pos[2] );

		// Index of self and children
		kdData2.push_back( kdNodes[i].index );
		kdData2.push_back( kdNodes[i].left );
		kdData2.push_back( kdNodes[i].right );

		// Index of faces of this node in kdData3
		kdData2.push_back( kdData3.size() );

		// Faces
		kdData3.push_back( kdNodes[i].faces.size() / 2 );
		for( uint j = 0; j < kdNodes[i].faces.size(); j += 2 ) {
			kdData3.push_back( kdNodes[i].faces[j] );
			kdData3.push_back( kdNodes[i].faces[j + 1] );
		}
	}

	mBufKdNodeData1 = mCL->createBuffer( kdData1, sizeof( cl_float ) * kdData1.size() );
	mBufKdNodeData2 = mCL->createBuffer( kdData2, sizeof( cl_int ) * kdData2.size() );
	mBufKdNodeData3 = mCL->createBuffer( kdData3, sizeof( cl_int ) * kdData3.size() );

	mBufScFaces = mCL->createBuffer( mFaces, sizeof( cl_uint ) * mFaces.size() );
	mBufScVertices = mCL->createBuffer( mVertices, sizeof( cl_float ) * mVertices.size() );
	mBufScNormals = mCL->createBuffer( mNormals, sizeof( cl_float ) * mNormals.size() );
	mBufBoundingBox = mCL->createBuffer( mBoundingBox, sizeof( cl_float ) * 6 );

	mBufEye = mCL->createEmptyBuffer( sizeof( cl_float ) * 3, CL_MEM_READ_ONLY );
	mBufVecW = mCL->createEmptyBuffer( sizeof( cl_float ) * 3, CL_MEM_READ_ONLY );
	mBufVecU = mCL->createEmptyBuffer( sizeof( cl_float ) * 3, CL_MEM_READ_ONLY );
	mBufVecV = mCL->createEmptyBuffer( sizeof( cl_float ) * 3, CL_MEM_READ_ONLY );

	mBufOrigins = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels, CL_MEM_READ_WRITE );
	mBufRays = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels, CL_MEM_READ_WRITE );
	mBufNormals = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels, CL_MEM_READ_WRITE );
	mBufAccColors = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels, CL_MEM_READ_WRITE );
	mBufColorMasks = mCL->createEmptyBuffer( sizeof( cl_float4 ) * pixels, CL_MEM_READ_WRITE );
}


/**
 * Load and compile the shader.
 */
void GLWidget::initShaders() {
	string shaderPath;
	GLuint shaderFrag, shaderVert;


	// Shaders for path tracing

	shaderPath = Cfg::get().value<string>( Cfg::SHADER_PATH );
	shaderPath.append( Cfg::get().value<string>( Cfg::SHADER_NAME ) );

	glDeleteProgram( mGLProgramTracer );

	mGLProgramTracer = glCreateProgram();
	shaderVert = glCreateShader( GL_VERTEX_SHADER );
	shaderFrag = glCreateShader( GL_FRAGMENT_SHADER );

	this->loadShader( mGLProgramTracer, shaderVert, shaderPath + string( ".vert" ) );
	this->loadShader( mGLProgramTracer, shaderFrag, shaderPath + string( ".frag" ) );

	glLinkProgram( mGLProgramTracer );
	glDetachShader( mGLProgramTracer, shaderVert );
	glDetachShader( mGLProgramTracer, shaderFrag );
	glDeleteShader( shaderVert );
	glDeleteShader( shaderFrag );


	// Shaders for drawing simple geometry with just one color

	shaderPath = Cfg::get().value<string>( Cfg::SHADER_PATH );
	shaderPath.append( "simple" );

	glDeleteProgram( mGLProgramSimple );

	mGLProgramSimple = glCreateProgram();
	GLuint shaderVertSimple = glCreateShader( GL_VERTEX_SHADER );
	GLuint shaderFragSimple = glCreateShader( GL_FRAGMENT_SHADER );

	this->loadShader( mGLProgramSimple, shaderVertSimple, shaderPath + string( ".vert" ) );
	this->loadShader( mGLProgramSimple, shaderFragSimple, shaderPath + string( ".frag" ) );

	glLinkProgram( mGLProgramSimple );
	glDetachShader( mGLProgramSimple, shaderVert );
	glDetachShader( mGLProgramSimple, shaderFrag );
	glDeleteShader( shaderVert );
	glDeleteShader( shaderFrag );
}


/**
 * Init the target textures for the generated image.
 */
void GLWidget::initTargetTexture() {
	size_t w = width();
	size_t h = height();

	mTextureOut = vector<cl_float>( w * h * 4 );

	mTargetTextures.clear();

	for( uint i = 0; i < 2; i++ ) {
		GLuint texture;
		glGenTextures( 1, &texture );
		glBindTexture( GL_TEXTURE_2D, texture );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, &mTextureOut[0] );
		mTargetTextures.push_back( texture );
	}
	glBindTexture( GL_TEXTURE_2D, 0 );

	mKernelArgTextureIn = mCL->createImageReadOnly( w, h, &mTextureOut[0] );
	mKernelArgTextureOut = mCL->createImageWriteOnly( w, h );
}


/**
 * Check, if QGLWidget is currently rendering.
 * @return {bool} True, if is rendering, false otherwise.
 */
bool GLWidget::isRendering() {
	return ( mDoRendering && mTimer->isActive() );
}


/**
 * Load 3D model and start rendering it.
 * @param {string} filepath Path to the file, without file name.
 * @param {string} filename Name of the file.
 */
void GLWidget::loadModel( string filepath, string filename ) {
	this->deleteOldModel();

	ModelLoader* ml = new ModelLoader();
	ml->loadModel( filepath, filename );
	mFaces = ml->mFaces;
	mVertices = ml->mVertices;
	mNormals = ml->mNormals;

	vector<cl_float> bbox = ml->mBoundingBox;
	mBoundingBox = &bbox[0];

	cl_float bbMin[3] = { mBoundingBox[0], mBoundingBox[1], mBoundingBox[2] };
	cl_float bbMax[3] = { mBoundingBox[3], mBoundingBox[4], mBoundingBox[5] };

	// CLEAN UP
	boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();
	mKdTree = new KdTree( mVertices, mFaces, bbMin, bbMax );
	boost::posix_time::time_duration msdiff = boost::posix_time::microsec_clock::local_time() - start;

	vector<GLfloat> verticesKdTree;
	vector<GLuint> indicesKdTree;
	mKdTree->visualize( &verticesKdTree, &indicesKdTree );
	mKdTreeNumIndices = indicesKdTree.size();

	char msg[80];
	snprintf(
		msg, 80, "[KdTree] Generated kd-tree in %g ms. %lu nodes.",
		(float) msdiff.total_milliseconds(), mKdTree->getNodes().size()
	);
	Logger::logInfo( msg );

	this->setShaderBuffersForOverlay( mVertices, mFaces );
	this->setShaderBuffersForBoundingBox( ml->mBoundingBox );
	this->setShaderBuffersForKdTree( verticesKdTree, indicesKdTree );
	this->setShaderBuffersForTracer();
	this->initOpenCLBuffers();
	this->initShaders();

	delete ml;

	// Ready
	this->startRendering();
}


/**
 * Load a GLSL shader and attach it to the program.
 * @param {GLuint}     program ID of the shader program.
 * @param {GLuint}     shader  ID of the shader.
 * @param {std:string} path    File path and name.
 */
void GLWidget::loadShader( GLuint program, GLuint shader, string path ) {
	string shaderString = utils::loadFileAsString( path.c_str() );

	const GLchar* shaderSource = shaderString.c_str();
	const GLint shaderLength = shaderString.size();

	glShaderSource( shader, 1, &shaderSource, &shaderLength );
	glCompileShader( shader );

	GLint status;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &status );

	if( status != GL_TRUE ) {
		char logBuffer[1000];
		glGetShaderInfoLog( shader, 1000, 0, logBuffer );
		Logger::logError( path + string( "\n" ).append( logBuffer ) );
		exit( EXIT_FAILURE );
	}

	glAttachShader( program, shader );
}


/**
 * Set a minimum width and height for the QWidget.
 * @return {QSize} Minimum width and height.
 */
QSize GLWidget::minimumSizeHint() const {
	return QSize( 50, 50 );
}


/**
 * Move the camera or if selected a light.
 * @param {const int} key Key code.
 */
void GLWidget::moveCamera( const int key ) {
	if( !this->isRendering() ) {
		return;
	}

	switch( key ) {

		case Qt::Key_W:
			if( mSelectedLight == -1 ) { mCamera->cameraMoveForward(); }
			else { mLights[mSelectedLight].position[0] += 0.5f; }
			break;

		case Qt::Key_S:
			if( mSelectedLight == -1 ) { mCamera->cameraMoveBackward(); }
			else { mLights[mSelectedLight].position[0] -= 0.5f; }
			break;

		case Qt::Key_A:
			if( mSelectedLight == -1 ) { mCamera->cameraMoveLeft(); }
			else { mLights[mSelectedLight].position[2] += 0.5f; }
			break;

		case Qt::Key_D:
			if( mSelectedLight == -1 ) { mCamera->cameraMoveRight(); }
			else { mLights[mSelectedLight].position[2] -= 0.5f; }
			break;

		case Qt::Key_Q:
			if( mSelectedLight == -1 ) { mCamera->cameraMoveUp(); }
			else { mLights[mSelectedLight].position[1] += 0.5f; }
			break;

		case Qt::Key_E:
			if( mSelectedLight == -1 ) { mCamera->cameraMoveDown(); }
			else { mLights[mSelectedLight].position[1] -= 0.5f; }
			break;

		case Qt::Key_R:
			mCamera->cameraReset();
			break;

	}
}


/**
 * Draw the scene.
 */
void GLWidget::paintGL() {
	if( !mDoRendering || mVertices.size() <= 0 ) {
		return;
	}

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Light(s)

	// char lightName1[20];
	// char lightName2[20];

	// for( uint i = 0; i < mLights.size(); i++ ) {
	// 	snprintf( lightName1, 20, "light%uData1", i );
	// 	snprintf( lightName2, 20, "light%uData2", i );

	// 	float lightData1[16] = {
	// 		mLights[i].position[0], mLights[i].position[1], mLights[i].position[2], mLights[i].position[3],
	// 		mLights[i].diffuse[0], mLights[i].diffuse[1], mLights[i].diffuse[2], mLights[i].diffuse[3],
	// 		mLights[i].specular[0], mLights[i].specular[1], mLights[i].specular[2], mLights[i].specular[3],
	// 		mLights[i].constantAttenuation, mLights[i].linearAttenuation, mLights[i].quadraticAttenuation, mLights[i].spotCutoff
	// 	};
	// 	float lightData2[4] = {
	// 		mLights[i].spotExponent, mLights[i].spotDirection[0], mLights[i].spotDirection[1], mLights[i].spotDirection[2]
	// 	};
	// }


	if( mViewTracer ) {
		mSampleCount++;

		glm::vec3 c = mCamera->getAdjustedCenter_glmVec3();
		glm::vec3 eye = mCamera->getEye_glmVec3();
		glm::vec3 up = mCamera->getUp_glmVec3();

		// Jittering for anti-aliasing
		// TODO: probably wrong or at least not very good
		if( Cfg::get().value<bool>( Cfg::RENDER_ANTIALIAS ) ) {
			glm::vec3 jitter = this->getJitter();
			eye += jitter;
		}

		glm::vec3 w = glm::normalize( glm::vec3( c[0] - eye[0], c[1] - eye[1], c[2] - eye[2] ) );
		glm::vec3 u = glm::normalize( glm::cross( w, up ) );
		glm::vec3 v = glm::normalize( glm::cross( u, w ) );

		mCL->updateBuffer( mBufEye, sizeof( cl_float ) * 3, &eye[0] );
		mCL->updateBuffer( mBufVecW, sizeof( cl_float ) * 3, &w[0] );
		mCL->updateBuffer( mBufVecU, sizeof( cl_float ) * 3, &u[0] );
		mCL->updateBuffer( mBufVecV, sizeof( cl_float ) * 3, &v[0] );

		boost::posix_time::time_duration msdiff = boost::posix_time::microsec_clock::local_time() - mTimeSinceStart;
		cl_float timeSinceStart = msdiff.total_milliseconds() * 0.001f;

		this->clInitRays();
		mCL->updateImageReadOnly( width(), height(), &mTextureOut[0] );

		for( uint bounce = 0; bounce < 3; bounce++ ) {
			this->clFindIntersections( timeSinceStart + bounce );
			this->clAccumulateColors( timeSinceStart );
		}

		mCL->readImageOutput( width(), height(), &mTextureOut[0] );
		mCL->finish();
	}


	this->paintScene();
	this->showFPS();
}


/**
 * Draw the main objects of the scene.
 */
void GLWidget::paintScene() {
	// Path tracing result
	if( mViewTracer ) {
		glUseProgram( mGLProgramTracer );

		glBindTexture( GL_TEXTURE_2D, mTargetTextures[0] );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width(), height(), 0, GL_RGBA, GL_FLOAT, &mTextureOut[0] );

		glBindVertexArray( mVA[VA_TRACER] );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

		glBindTexture( GL_TEXTURE_2D, 0 );
		glBindVertexArray( 0 );
		glUseProgram( 0 );
	}


	// Overlay for the path tracing image to highlight/outline elements with a box
	if( mViewOverlay ) {
		glUseProgram( mGLProgramSimple );
		glUniformMatrix4fv(
			glGetUniformLocation( mGLProgramSimple, "mvp" ),
			1, GL_FALSE, &mModelViewProjectionMatrix[0][0]
		);
		float color[4] = { 0.6f, 1.0f, 0.3f, 0.4f };
		glUniform4fv( glGetUniformLocation( mGLProgramSimple, "fragColor" ), 1, color );

		glBindVertexArray( mVA[VA_OVERLAY] );
		glDrawElements( GL_TRIANGLES, mFaces.size(), GL_UNSIGNED_INT, NULL );

		glBindVertexArray( 0 );
		glUseProgram( 0 );
	}


	// Bounding box
	if( mViewBoundingBox ) {
		glUseProgram( mGLProgramSimple );
		glUniformMatrix4fv(
			glGetUniformLocation( mGLProgramSimple, "mvp" ),
			1, GL_FALSE, &mModelViewProjectionMatrix[0][0]
		);
		float color[4] = { 1.0f, 1.0f, 1.0f, 0.6f };
		glUniform4fv( glGetUniformLocation( mGLProgramSimple, "fragColor" ), 1, color );

		glBindVertexArray( mVA[VA_BOUNDINGBOX] );
		glDrawElements( GL_LINES, 24, GL_UNSIGNED_INT, NULL );

		glBindVertexArray( 0 );
		glUseProgram( 0 );
	}


	// kd-tree
	if( mViewKdTree ) {
		glUseProgram( mGLProgramSimple );
		glUniformMatrix4fv(
			glGetUniformLocation( mGLProgramSimple, "mvp" ),
			1, GL_FALSE, &mModelViewProjectionMatrix[0][0]
		);
		float color[4] = { 0.6f, 0.85f, 1.0f, 0.8f };
		glUniform4fv( glGetUniformLocation( mGLProgramSimple, "fragColor" ), 1, color );

		glBindVertexArray( mVA[VA_KDTREE] );
		glDrawElements( GL_LINES, mKdTreeNumIndices, GL_UNSIGNED_INT, NULL );

		glBindVertexArray( 0 );
		glUseProgram( 0 );
	}
}


/**
 * Handle resizing of the QWidget by updating the viewport and perspective.
 * @param {int} width  New width of the QWidget.
 * @param {int} height New height of the QWidget.
 */
void GLWidget::resizeGL( int width, int height ) {
	glViewport( 0, 0, width, height );

	mProjectionMatrix = glm::perspective(
		mFOV,
		width / (GLfloat) height,
		Cfg::get().value<GLfloat>( Cfg::PERS_ZNEAR ),
		Cfg::get().value<GLfloat>( Cfg::PERS_ZFAR )
	);

	this->calculateMatrices();
}


/**
 * Select the next light in the list.
 */
void GLWidget::selectNextLight() {
	if( mSelectedLight > -1 ) {
		mSelectedLight = ( mSelectedLight + 1 ) % mLights.size();
	}
}


/**
 * Select the previous light in the list.
 */
void GLWidget::selectPreviousLight() {
	if( mSelectedLight > -1 ) {
		mSelectedLight = ( mSelectedLight == 0 ) ? mLights.size() - 1 : mSelectedLight - 1;
	}
}


/**
 * Set the vertex array for the model overlay.
 * @param {std::vector<GLfloat>} vertices Vertices of the model.
 * @param {std::vector<GLuint>}  indices  Indices of the vertices of the faces.
 */
void GLWidget::setShaderBuffersForOverlay( vector<GLfloat> vertices, vector<GLuint> indices ) {
	GLuint vaID;
	glGenVertexArrays( 1, &vaID );
	glBindVertexArray( vaID );

	GLuint vertexBuffer;
	glGenBuffers( 1, &vertexBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( GLfloat ) * vertices.size(), &vertices[0], GL_STATIC_DRAW );
	glVertexAttribPointer( GLWidget::ATTRIB_POINTER_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0 );
	glEnableVertexAttribArray( GLWidget::ATTRIB_POINTER_VERTEX );

	GLuint indexBuffer;
	glGenBuffers( 1, &indexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( GLuint ) * indices.size(), &indices[0], GL_STATIC_DRAW );

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );

	mVA[VA_OVERLAY] = vaID;
}


/**
 * Set the vertex array for the model bounding box.
 * @param {std::vector<GLfloat>} bbox Min and max vertices of the bounding box.
 */
void GLWidget::setShaderBuffersForBoundingBox( vector<GLfloat> bbox ) {
	GLfloat bbVertices[24] = {
		// bottom
		bbox[0], bbox[1], bbox[2],
		bbox[3], bbox[1], bbox[2],
		bbox[0], bbox[1], bbox[5],
		bbox[3], bbox[1], bbox[5],
		// top
		bbox[0], bbox[4], bbox[2],
		bbox[3], bbox[4], bbox[2],
		bbox[0], bbox[4], bbox[5],
		bbox[3], bbox[4], bbox[5]
	};
	GLuint bbIndices[24] = {
		// bottom
		0, 1, 1, 3, 3, 2, 2, 0,
		// left
		0, 4, 2, 6,
		// top
		6, 4, 4, 5, 5, 7, 7, 6,
		// right
		1, 5, 3, 7
	};

	GLuint vaID;
	glGenVertexArrays( 1, &vaID );
	glBindVertexArray( vaID );

	GLuint vertexBuffer;
	glGenBuffers( 1, &vertexBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( bbVertices ), &bbVertices, GL_STATIC_DRAW );
	glVertexAttribPointer( GLWidget::ATTRIB_POINTER_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0 );
	glEnableVertexAttribArray( GLWidget::ATTRIB_POINTER_VERTEX );

	GLuint indexBuffer;
	glGenBuffers( 1, &indexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( bbIndices ), &bbIndices, GL_STATIC_DRAW );

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );

	mVA[VA_BOUNDINGBOX] = vaID;
}


/**
 * Set the vertex array for the generated kd-tree.
 * @param {std::vector<GLfloat>} vertices
 * @param {std::vector<GLuint>}  indices
 */
void GLWidget::setShaderBuffersForKdTree( vector<GLfloat> vertices, vector<GLuint> indices ) {
	GLuint vaID;
	glGenVertexArrays( 1, &vaID );
	glBindVertexArray( vaID );

	GLuint vertexBuffer;
	glGenBuffers( 1, &vertexBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( GLfloat ) * vertices.size(), &vertices[0], GL_STATIC_DRAW );
	glVertexAttribPointer( GLWidget::ATTRIB_POINTER_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0 );
	glEnableVertexAttribArray( GLWidget::ATTRIB_POINTER_VERTEX );

	GLuint indexBuffer;
	glGenBuffers( 1, &indexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( GLuint ) * indices.size(), &indices[0], GL_STATIC_DRAW );

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );

	mVA[VA_KDTREE] = vaID;
}


/**
 * Init vertex buffer for the rendering of the OpenCL generated texture.
 */
void GLWidget::setShaderBuffersForTracer() {
	GLuint vaID;
	glGenVertexArrays( 1, &vaID );
	glBindVertexArray( vaID );

	GLfloat vertices[8] = {
		-1.0f, -1.0f,
		-1.0f, +1.0f,
		+1.0f, -1.0f,
		+1.0f, +1.0f
	};

	GLuint vertexBuffer;
	glGenBuffers( 1, &vertexBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), &vertices, GL_STATIC_DRAW );
	glVertexAttribPointer( GLWidget::ATTRIB_POINTER_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, (void*) 0 );
	glEnableVertexAttribArray( GLWidget::ATTRIB_POINTER_VERTEX );

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );

	mVA[VA_TRACER] = vaID;
}


/**
 * Calculate the current framerate and show it in the status bar.
 */
void GLWidget::showFPS() {
	mFrameCount++;

	GLuint currentTime = glutGet( GLUT_ELAPSED_TIME );
	GLuint timeInterval = currentTime - mPreviousTime;

	if( timeInterval > 1000 ) {
		GLfloat fps = mFrameCount / (GLfloat) timeInterval * 1000.0f;
		mPreviousTime = currentTime;
		mFrameCount = 0;

		char statusText[40];
		snprintf( statusText, 40, "%.2f FPS (%d\u00D7%dpx)", fps, width(), height() );
		( (Window*) parentWidget() )->updateStatus( statusText );
	}
}


/**
 * Set size of the QWidget.
 * @return {QSize} Width and height of the QWidget.
 */
QSize GLWidget::sizeHint() const {
	return QSize(
		Cfg::get().value<uint>( Cfg::WINDOW_WIDTH ),
		Cfg::get().value<uint>( Cfg::WINDOW_HEIGHT )
	);
}


/**
 * Start or resume rendering.
 */
void GLWidget::startRendering() {
	if( !mDoRendering ) {
		mDoRendering = true;
		mTimer->start( Cfg::get().value<float>( Cfg::RENDER_INTERVAL ) );
	}
}


/**
 * Stop rendering.
 */
void GLWidget::stopRendering() {
	if( mDoRendering ) {
		mDoRendering = false;
		mTimer->stop();
		( (Window*) parentWidget() )->updateStatus( "Stopped." );
	}
}


/**
 * Switch between controlling the camera and the lights.
 */
void GLWidget::toggleLightControl() {
	if( mSelectedLight == -1 ) {
		if( mLights.size() > 0 ) {
			mSelectedLight = 0;
		}
	}
	else {
		mSelectedLight = -1;
	}
}


/**
 * Toggle rendering of the bounding box.
 */
void GLWidget::toggleViewBoundingBox() {
	mViewBoundingBox = !mViewBoundingBox;
}


/**
 * Toggle rendering of the kd-tree visualization.
 */
void GLWidget::toggleViewKdTree() {
	mViewKdTree = !mViewKdTree;
}


/**
 * Toggle rendering of the translucent overlay over the traced model.
 */
void GLWidget::toggleViewOverlay() {
	mViewOverlay = !mViewOverlay;
}


/**
 * Toggle rendering of the path tracing.
 */
void GLWidget::toggleViewTracer() {
	mViewTracer = !mViewTracer;
}
