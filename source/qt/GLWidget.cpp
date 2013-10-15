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
	mCL->createKernel( "pathTracing" );

	mModelMatrix = glm::mat4( 1.0f );
	mProjectionMatrix = glm::perspective(
		Cfg::get().value<float>( Cfg::PERS_FOV ),
		Cfg::get().value<float>( Cfg::WINDOW_WIDTH ) / Cfg::get().value<float>( Cfg::WINDOW_HEIGHT ),
		Cfg::get().value<float>( Cfg::PERS_ZNEAR ),
		Cfg::get().value<float>( Cfg::PERS_ZFAR )
	);

	mDoRendering = false;
	mFrameCount = 0;
	mPreviousTime = 0;
	mCamera = new Camera( this );
	mSampleCount = 0;
	mSelectedLight = -1;

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
	glDeleteFramebuffers( 1, &mFramebuffer );
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
	// printf( "%g %g %g\n", mViewMatrix[0][0], mViewMatrix[0][1], mViewMatrix[0][2] );
	// printf( "%g %g %g\n", mViewMatrix[1][0], mViewMatrix[1][1], mViewMatrix[1][2] );
	// printf( "%g %g %g\n-----\n", mViewMatrix[2][0], mViewMatrix[2][1], mViewMatrix[2][2] );
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
 * Get a ray that originates from the eye position.
 * @param  {glm::mat4} matrix Should be the inversed model-view-projection matrix (maybe with additional jittering).
 * @param  {glm::vec3} eye    Camera eye position.
 * @param  {float}     x      Direction of the ray: X coordinate.
 * @param  {float}     y      Direction of the ray: Y coordinate.
 * @param  {float}     z      Direction of the ray: Z coordinate.
 * @return {glm::vec3}        Resulting eye ray.
 */
glm::vec3 GLWidget::getEyeRay( glm::mat4 matrix, glm::vec3 eye, float x, float y, float z ) {
	glm::vec4 target = matrix * glm::vec4( x, y, z, 1.0f );
	glm::vec3 r(
		target[0] / target[3],
		target[1] / target[3],
		target[2] / target[3]
	);

	return r - eye;
}


/**
 * Get a jitter 4x4 matrix based on the model-view-projection matrix in order to
 * slightly alter the eye rays. This results in anti-aliasing.
 * @return {glm::mat4} Jitter Model-view-projection matrix.
 */
glm::mat4 GLWidget::getJitterMatrix() {
	glm::mat4 jitter = glm::mat4( 1.0f );
	glm::vec3 v = glm::vec3(
		rand() / (float) RAND_MAX * 1.0f - 1.0f,
		rand() / (float) RAND_MAX * 1.0f - 1.0f,
		0.0f
	);

	v[0] *= ( 1.0f / (float) width() );
	v[1] *= ( 1.0f / (float) height() );

	jitter[0][3] = v[0];
	jitter[1][3] = v[1];
	jitter[2][3] = v[2];

	return glm::inverse( jitter * mModelViewProjectionMatrix );
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

	glGenFramebuffers( 1, &mFramebuffer );

	this->initVertexBuffer();


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
 * Init the needed OpenCL buffers: Indices, vertices, camera eye and rays.
 */
void GLWidget::initOpenCLBuffers() {
	mBufferIndices = mCL->createBuffer( mIndices, sizeof( cl_uint ) * mIndices.size() );
	mBufferVertices = mCL->createBuffer( mVertices, sizeof( cl_float ) * mVertices.size() );

	mBufferEye = mCL->createEmptyBuffer( sizeof( cl_float ) * 3 );
	mBufferRay00 = mCL->createEmptyBuffer( sizeof( cl_float ) * 3 );
	mBufferRay01 = mCL->createEmptyBuffer( sizeof( cl_float ) * 3 );
	mBufferRay10 = mCL->createEmptyBuffer( sizeof( cl_float ) * 3 );
	mBufferRay11 = mCL->createEmptyBuffer( sizeof( cl_float ) * 3 );
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

	this->loadShader( shaderVert, shaderPath + string( ".vert" ) );
	this->loadShader( shaderFrag, shaderPath + string( ".frag" ) );

	glAttachShader( mGLProgramTracer, shaderVert );
	glAttachShader( mGLProgramTracer, shaderFrag );

	glLinkProgram( mGLProgramTracer );
	glUseProgram( mGLProgramTracer );

	glDetachShader( mGLProgramTracer, shaderVert );
	glDeleteShader( shaderVert );

	glDetachShader( mGLProgramTracer, shaderFrag );
	glDeleteShader( shaderFrag );
	glUseProgram( 0 );


	// Shaders for drawing lines
	shaderPath = Cfg::get().value<string>( Cfg::SHADER_PATH );
	shaderPath.append( "lines" );

	glDeleteProgram( mGLProgramLines );

	mGLProgramLines = glCreateProgram();
	GLuint shaderVertLines = glCreateShader( GL_VERTEX_SHADER );
	GLuint shaderFragLines = glCreateShader( GL_FRAGMENT_SHADER );

	this->loadShader( shaderVertLines, shaderPath + string( ".vert" ) );
	this->loadShader( shaderFragLines, shaderPath + string( ".frag" ) );

	glAttachShader( mGLProgramLines, shaderVertLines );
	glAttachShader( mGLProgramLines, shaderFragLines );

	glLinkProgram( mGLProgramLines );
	glUseProgram( mGLProgramLines );

	glDetachShader( mGLProgramLines, shaderVertLines );
	glDeleteShader( shaderVertLines );

	glDetachShader( mGLProgramLines, shaderFragLines );
	glDeleteShader( shaderFragLines );
	glUseProgram( 0 );
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
 * Init vertex buffer for the generated OpenCL texture.
 */
void GLWidget::initVertexBuffer() {
	mVA = vector<GLuint>( 2 );

	GLuint vaTracer;
	glGenVertexArrays( 1, &vaTracer );
	glBindVertexArray( vaTracer );

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
	glVertexAttribPointer( GLWidget::ATTRIB_POINTER_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, 0 );
	glEnableVertexAttribArray( GLWidget::ATTRIB_POINTER_VERTEX );

	glBindVertexArray( 0 );

	mVA[0] = vaTracer;


	// GLuint vaLines;
	// glGenVertexArrays( 1, &vaLines );
	// glBindVertexArray( vaLines );

	// GLfloat verticesLines[24] = {
	// 	0.0f, 0.0f, 0.0f,
	// 	1.0f, 0.0f, 0.0f,
	// 	0.0f, 1.0f, 0.0f,
	// 	1.0f, 1.0f, 0.0f,
	// 	0.0f, 0.0f, 1.0f,
	// 	1.0f, 0.0f, 1.0f,
	// 	0.0f, 1.0f, 1.0f,
	// 	1.0f, 1.0f, 1.0f
	// };
	// GLushort indicesLines[24] = {
	// 	0, 1, 1, 3, 3, 2, 2, 0,
	// 	4, 5, 5, 7, 7, 6, 6, 4,
	// 	0, 4, 1, 5, 2, 6, 3, 7
	// };

	// GLuint vertexBufferLines;
	// glGenBuffers( 1, &vertexBufferLines );
	// glBindBuffer( GL_ARRAY_BUFFER, vertexBufferLines );
	// glBufferData( GL_ARRAY_BUFFER, sizeof( verticesLines ), &verticesLines, GL_STATIC_DRAW );
	// glVertexAttribPointer( GLWidget::ATTRIB_POINTER_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0 );
	// glEnableVertexAttribArray( GLWidget::ATTRIB_POINTER_VERTEX );

	// GLuint indexBufferLines;
	// glGenBuffers( 1, &indexBufferLines );
	// glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBufferLines );
	// glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indicesLines ), &indicesLines, GL_STATIC_DRAW );

	// glBindVertexArray( 0 );
	// glBindBuffer( GL_ARRAY_BUFFER, 0 );
	// glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	// mVA.push_back( vaLines );
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
	mIndices = ml->mIndices;
	mVertices = ml->mVertices;
	// mNormals = ml->mNormals;


	GLuint vaLines;
	glGenVertexArrays( 1, &vaLines );
	glBindVertexArray( vaLines );

	GLuint vertexBufferLines;
	glGenBuffers( 1, &vertexBufferLines );
	glBindBuffer( GL_ARRAY_BUFFER, vertexBufferLines );
	glBufferData( GL_ARRAY_BUFFER, sizeof( GLfloat ) * mVertices.size(), &mVertices[0], GL_STATIC_DRAW );
	glVertexAttribPointer( GLWidget::ATTRIB_POINTER_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0 );
	glEnableVertexAttribArray( GLWidget::ATTRIB_POINTER_VERTEX );

	GLuint indexBufferLines;
	glGenBuffers( 1, &indexBufferLines );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBufferLines );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( GLuint ) * mIndices.size(), &mIndices, GL_STATIC_DRAW );

	glBindVertexArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	mVA[1] = vaLines;


	this->initOpenCLBuffers();
	this->initShaders();

	// Ready
	this->startRendering();
}


/**
 * Load a GLSL shader and attach it to the program.
 * @param {GLuint}     shader ID of the shader.
 * @param {std:string} path   File path and name.
 */
void GLWidget::loadShader( GLuint shader, string path ) {
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


	boost::posix_time::time_duration msdiff = boost::posix_time::microsec_clock::local_time() - mTimeSinceStart;
	cl_float timeSinceStart = msdiff.total_milliseconds() * 0.001f;


	// Jittering for anti-aliasing
	glm::mat4 jitter = glm::inverse( mModelViewProjectionMatrix ); //this->getJitterMatrix();

	uint i = 0;
	vector<cl_mem> clBuffers;

	cl_float textureWeight = mSampleCount / (cl_float) ( mSampleCount + 1 );
	mSampleCount++;


	glm::vec3 c = mCamera->getCenter_glmVec3();
	glm::vec3 eye = mCamera->getEye_glmVec3();
	// glm::vec3 eyeMVP = glm::vec3( mModelViewProjectionMatrix * glm::vec4( eye, 1.0f ) );

	glm::vec3 w = glm::normalize( glm::vec3( c[0] + eye[0], c[1] - eye[1], c[2] - eye[2] ) );
	glm::vec3 u = glm::cross( w, mCamera->getUp_glmVec3() );
	glm::vec3 v = glm::cross( u, w );

	// glm::vec3 ray00 = this->getEyeRay( jitter, eyeMVP, c[0] - 1.0f, c[1] - 1.0f, c[2] );
	// glm::vec3 ray01 = this->getEyeRay( jitter, eyeMVP, c[0] - 1.0f, c[1] + 1.0f, c[2] );
	// glm::vec3 ray10 = this->getEyeRay( jitter, eyeMVP, c[0] + 1.0f, c[1] - 1.0f, c[2] );
	// glm::vec3 ray11 = this->getEyeRay( jitter, eyeMVP, c[0] + 1.0f, c[1] + 1.0f, c[2] );

	mCL->setKernelArg( i, sizeof( cl_mem ), &mBufferIndices );
	mCL->setKernelArg( ++i, sizeof( cl_mem ), &mBufferVertices );

	mCL->updateBuffer( mBufferEye, sizeof( cl_float ) * 3, &eye[0] );
	mCL->updateBuffer( mBufferRay00, sizeof( cl_float ) * 3, &c[0] );
	mCL->updateBuffer( mBufferRay01, sizeof( cl_float ) * 3, &w[0] );
	mCL->updateBuffer( mBufferRay10, sizeof( cl_float ) * 3, &u[0] );
	mCL->updateBuffer( mBufferRay11, sizeof( cl_float ) * 3, &v[0] );
	// mCL->updateBuffer( mBufferEye, sizeof( cl_float ) * 3, &eye[0] );
	// mCL->updateBuffer( mBufferRay00, sizeof( cl_float ) * 3, &ray00[0] );
	// mCL->updateBuffer( mBufferRay01, sizeof( cl_float ) * 3, &ray01[0] );
	// mCL->updateBuffer( mBufferRay10, sizeof( cl_float ) * 3, &ray10[0] );
	// mCL->updateBuffer( mBufferRay11, sizeof( cl_float ) * 3, &ray11[0] );

	mCL->setKernelArg( ++i, sizeof( cl_mem ), &mBufferEye );
	mCL->setKernelArg( ++i, sizeof( cl_mem ), &mBufferRay00 );
	mCL->setKernelArg( ++i, sizeof( cl_mem ), &mBufferRay01 );
	mCL->setKernelArg( ++i, sizeof( cl_mem ), &mBufferRay10 );
	mCL->setKernelArg( ++i, sizeof( cl_mem ), &mBufferRay11 );

	mCL->setKernelArg( ++i, sizeof( cl_float ), &textureWeight );
	mCL->setKernelArg( ++i, sizeof( cl_float ), &timeSinceStart );

	cl_uint numIndices = mIndices.size();
	mCL->setKernelArg( ++i, sizeof( cl_uint ), &numIndices );

	mCL->updateImageReadOnly( width(), height(), &mTextureOut[0] );

	mCL->setKernelArg( ++i, sizeof( cl_mem ), &mKernelArgTextureIn );
	mCL->setKernelArg( ++i, sizeof( cl_mem ), &mKernelArgTextureOut );

	mCL->execute();
	mCL->readImageOutput( width(), height(), &mTextureOut[0] );
	mCL->finish();


	this->paintScene();

	this->showFPS();
}


/**
 * Draw the main objects of the scene.
 */
void GLWidget::paintScene() {
	// Path tracing result
	glUseProgram( mGLProgramTracer );

	glBindTexture( GL_TEXTURE_2D, mTargetTextures[0] );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width(), height(), 0, GL_RGBA, GL_FLOAT, &mTextureOut[0] );

	glBindVertexArray( mVA[0] );
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

	glBindTexture( GL_TEXTURE_2D, 0 );
	glBindVertexArray( 0 );
	glUseProgram( 0 );


	// Overlay for the path tracing image to highlight/outline elements with a box
	glUseProgram( mGLProgramLines );
	glUniformMatrix4fv(
		glGetUniformLocation( mGLProgramLines, "mvp" ),
		1, GL_FALSE, &mModelViewProjectionMatrix[0][0]
	);

	glBindVertexArray( mVA[1] );
	// glDrawElements( GL_LINES, 24, GL_UNSIGNED_SHORT, 0 );
	glDrawArrays( GL_TRIANGLES, 0, mIndices.size() );

	glBindVertexArray( 0 );
	glUseProgram( 0 );


	// reverse( mTargetTextures.begin(), mTargetTextures.end() );

	// glBindFramebuffer( GL_FRAMEBUFFER, mFramebuffer );
	// glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTargetTextures[0], 0 );
	// glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}


/**
 * Handle resizing of the QWidget by updating the viewport and perspective.
 * @param {int} width  New width of the QWidget.
 * @param {int} height New height of the QWidget.
 */
void GLWidget::resizeGL( int width, int height ) {
	glViewport( 0, 0, width, height );

	mProjectionMatrix = glm::perspective(
		Cfg::get().value<float>( Cfg::PERS_FOV ),
		width / (float) height,
		Cfg::get().value<float>( Cfg::PERS_ZNEAR ),
		Cfg::get().value<float>( Cfg::PERS_ZFAR )
	);

	this->calculateMatrices();
}


/**
 * Calculate the current framerate and show it in the status bar.
 */
void GLWidget::showFPS() {
	mFrameCount++;

	GLuint currentTime = glutGet( GLUT_ELAPSED_TIME );
	GLuint timeInterval = currentTime - mPreviousTime;

	if( timeInterval > 1000 ) {
		float fps = mFrameCount / (float) timeInterval * 1000.0f;
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
 * Start or resume rendering.
 */
void GLWidget::startRendering() {
	if( !mDoRendering ) {
		mDoRendering = true;
		float fps = Cfg::get().value<float>( Cfg::RENDER_INTERVAL );
		mTimer->start( fps );
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
