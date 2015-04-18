#include "GLWidget.h"

using std::map;
using std::string;
using std::vector;


/**
 * Constructor.
 * @param {QWidget*} parent Parent QWidget this QWidget is contained in.
 */
GLWidget::GLWidget( QWidget* parent ) : QGLWidget( parent ) {
	mFOV = glm::radians( Cfg::get().value<cl_float>( Cfg::PERS_FOV ) );
	mModelMatrix = glm::mat4( 1.0f );

	mDoRendering = false;
	mFrameCount = 0;
	mPreviousTime = 0;

	mMoveSun = false;
	mViewBVH = false;
	mViewDebug = false;
	mViewOverlay = false;
	mViewTracer = true;

	mInfoWindow = NULL;
	mPathTracer = new PathTracer( this );
	mCamera = new Camera( this );
	mTimer = new QTimer( this );

	mPathTracer->setCamera( mCamera );
	connect( mTimer, SIGNAL( timeout() ), this, SLOT( update() ) );
}


/**
 * Destructor.
 */
GLWidget::~GLWidget() {
	this->stopRendering();
	this->deleteOldModel();

	glDeleteTextures( 1, &mTargetTexture );
	glDeleteTextures( 1, &mDebugTexture );
	glDeleteProgram( mGLProgramTracer );
	glDeleteProgram( mGLProgramDebug );
	glDeleteProgram( mGLProgramSimple );

	delete mTimer;
	delete mCamera;
	delete mPathTracer;

	if( mInfoWindow != NULL ) {
		delete mInfoWindow;
	}
}


/**
 * Calculate the matrices for view, model, model-view-projection and normals.
 */
void GLWidget::calculateMatrices() {
	if( !mDoRendering ) {
		return;
	}

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
	mPathTracer->resetSampleCount();
	mRenderStartTime = glutGet( GLUT_ELAPSED_TIME );
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
 * Create the kernel info window.
 * @param {CL*} cl
 */
void GLWidget::createKernelWindow( CL* cl ) {
	if( mInfoWindow == NULL ) {
		mInfoWindow = new InfoWindow( dynamic_cast<QWidget*>( this->parent() ), cl );
	}
	else {
		Logger::logWarning( "[GLWidget] InfoWindow already exists, won't create a new one. @see GLWidget::createKernelWindow()." );
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
 * Destroy the kernel info window.
 */
void GLWidget::destroyKernelWindow() {
	if( mInfoWindow != NULL ) {
		delete mInfoWindow;

		mInfoWindow = NULL;
	}
}


/**
 * Initialize OpenGL and start rendering.
 */
void GLWidget::initializeGL() {
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

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
	Logger::logDebug( string( "[GLEW] Version " ).append( (char*) glewGetString( GLEW_VERSION ) ) );
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


	// Shaders for debug

	shaderPath = Cfg::get().value<string>( Cfg::SHADER_PATH );
	shaderPath.append( "debug" );

	glDeleteProgram( mGLProgramDebug );

	mGLProgramDebug = glCreateProgram();
	shaderVert = glCreateShader( GL_VERTEX_SHADER );
	shaderFrag = glCreateShader( GL_FRAGMENT_SHADER );

	this->loadShader( mGLProgramDebug, shaderVert, shaderPath + string( ".vert" ) );
	this->loadShader( mGLProgramDebug, shaderFrag, shaderPath + string( ".frag" ) );

	glLinkProgram( mGLProgramDebug );
	glDetachShader( mGLProgramDebug, shaderVert );
	glDetachShader( mGLProgramDebug, shaderFrag );
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


	Logger::logInfo( "[GLWidget] Initialized OpenGL shaders." );
}


/**
 * Init the target textures for the generated image.
 */
void GLWidget::initTargetTexture() {
	size_t w = width();
	size_t h = height();

	mTextureOut = vector<cl_float>( w * h * 4 );

	glGenTextures( 1, &mTargetTexture );
	glBindTexture( GL_TEXTURE_2D, mTargetTexture );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, &mTextureOut[0] );
	glBindTexture( GL_TEXTURE_2D, 0 );


	mTextureDebug = vector<cl_float>( w * h * 4 );

	glGenTextures( 1, &mDebugTexture );
	glBindTexture( GL_TEXTURE_2D, mDebugTexture );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, &mTextureDebug[0] );
	glBindTexture( GL_TEXTURE_2D, 0 );
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
	this->destroyKernelWindow();
	this->deleteOldModel();

	ModelLoader* ml = new ModelLoader();
	ml->loadModel( filepath, filename );

	ObjParser* op = ml->getObjParser();

    mFaces = op->getFacesV();
	mNormals = op->getNormals();
	mVertices = op->getVertices();

	const short usedAccelStruct = Cfg::get().value<short>( Cfg::ACCEL_STRUCT );
	AccelStructure* accelStruct;

	if( usedAccelStruct == ACCELSTRUCT_BVH ) {
		accelStruct = new BVH( op->getObjects(), mVertices, mNormals );
	}
	else if( usedAccelStruct == ACCELSTRUCT_KDTREE ) {
		accelStruct = new KdTree( mFaces, op->getFacesVN(), mVertices, mNormals );
	}

	// Visualization of the acceleration structure
	vector<GLfloat> visVertices;
	vector<GLuint> visIndices;
	accelStruct->visualize( &visVertices, &visIndices );
	mAccelStructNumIndices = visIndices.size();

	// Shader buffers
	this->setShaderBuffersForOverlay( mVertices, mFaces );
	this->setShaderBuffersForBVH( visVertices, visIndices );
	this->setShaderBuffersForTracer();
	this->initShaders();

	// OpenCL buffers
	mPathTracer->initOpenCLBuffers( mVertices, mFaces, mNormals, ml, accelStruct );

	delete ml;
	delete accelStruct;

	// Ready
	this->startRendering();
	this->calculateMatrices();
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
		char logBuffer[1024];
		glGetShaderInfoLog( shader, 1024, 0, logBuffer );
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
 * Move the camera.
 * @param {const int} key Key code.
 */
void GLWidget::moveCamera( const int key ) {
	if( !this->isRendering() ) {
		return;
	}

	if( mMoveSun ) {
		mPathTracer->moveSun( key );
		return;
	}

	switch( key ) {

		case Qt::Key_W:
			mCamera->cameraMoveForward();
			break;

		case Qt::Key_S:
			mCamera->cameraMoveBackward();
			break;

		case Qt::Key_A:
			mCamera->cameraMoveLeft();
			break;

		case Qt::Key_D:
			mCamera->cameraMoveRight();
			break;

		case Qt::Key_Q:
			mCamera->cameraMoveUp();
			break;

		case Qt::Key_E:
			mCamera->cameraMoveDown();
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

	if( mViewTracer ) {
		mTextureOut = mPathTracer->generateImage( &mTextureDebug );
	}

	this->paintScene();
	this->showFPS();
}


/**
 * Draw the main objects of the scene.
 */
void GLWidget::paintScene() {
	// Path tracing result
	if( mViewTracer && !mViewDebug ) {
		glUseProgram( mGLProgramTracer );

		glUniform1i( glGetUniformLocation( mGLProgramTracer, "width" ), width() );
		glUniform1i( glGetUniformLocation( mGLProgramTracer, "height" ), height() );

		glBindTexture( GL_TEXTURE_2D, mTargetTexture );
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA, width(), height(),
			0, GL_RGBA, GL_FLOAT, &mTextureOut[0]
		);
		glBindVertexArray( mVA[VA_TRACER] );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

		glBindTexture( GL_TEXTURE_2D, 0 );
		glBindVertexArray( 0 );
		glUseProgram( 0 );
	}


	// Debug texture
	if( mViewDebug ) {
		glUseProgram( mGLProgramDebug );

		glUniform1i( glGetUniformLocation( mGLProgramDebug, "width" ), width() );
		glUniform1i( glGetUniformLocation( mGLProgramDebug, "height" ), height() );

		glBindTexture( GL_TEXTURE_2D, mTargetTexture );
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA, width(), height(),
			0, GL_RGBA, GL_FLOAT, &mTextureDebug[0]
		);
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
		GLfloat color[4] = { 0.6f, 1.0f, 0.3f, 0.4f };
		glUniform4fv( glGetUniformLocation( mGLProgramSimple, "fragColor" ), 1, color );

		GLfloat translate[3] = { 0.0f, 0.0f, 0.0f };
		glUniform3fv( glGetUniformLocation( mGLProgramSimple, "translate" ), 1, translate );

		glBindVertexArray( mVA[VA_OVERLAY] );
		glDrawElements( GL_TRIANGLES, mFaces.size(), GL_UNSIGNED_INT, NULL );

		glBindVertexArray( 0 );
		glUseProgram( 0 );
	}


	// Bounding box
	if( mViewBVH ) {
		glUseProgram( mGLProgramSimple );
		glUniformMatrix4fv(
			glGetUniformLocation( mGLProgramSimple, "mvp" ),
			1, GL_FALSE, &mModelViewProjectionMatrix[0][0]
		);
		GLfloat color[4] = { 1.0f, 1.0f, 1.0f, 0.6f };
		glUniform4fv( glGetUniformLocation( mGLProgramSimple, "fragColor" ), 1, color );

		GLfloat translate[3] = { 0.0f, 0.0f, 0.0f };
		glUniform3fv( glGetUniformLocation( mGLProgramSimple, "translate" ), 1, translate );

		glBindVertexArray( mVA[VA_BVH] );
		glDrawElements( GL_LINES, mAccelStructNumIndices, GL_UNSIGNED_INT, NULL );

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

	mProjectionMatrix = glm::perspectiveFov(
		mFOV, (GLfloat) width, (GLfloat) height,
		Cfg::get().value<GLfloat>( Cfg::PERS_ZNEAR ),
		Cfg::get().value<GLfloat>( Cfg::PERS_ZFAR )
	);

	mPathTracer->setWidthAndHeight( width, height );
	this->calculateMatrices();
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
 * Set the vertex array for the BVH.
 * @param {std::vector<GLfloat>} vertices
 * @param {std::vector<GLuint>}  indices
 */
void GLWidget::setShaderBuffersForBVH( vector<GLfloat> vertices, vector<GLuint> indices ) {
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

	mVA[VA_BVH] = vaID;
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
 * Show the kernel info window.
 */
void GLWidget::showKernelWindow() {
	if( mInfoWindow == NULL ) {
		Logger::logError( "[GLWidget] InfoWindow has not been created yet. @see GLWidget::createKernelWindow()." );
		return;
	}

	if( !mInfoWindow->isVisible() ) {
		mInfoWindow->show();
	}
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

		glm::vec3 e = mCamera->getEye_glmVec3();
		glm::vec3 c = mCamera->getCenter_glmVec3();

		GLuint elapsedTime = ( currentTime - mRenderStartTime ) / 1000;

		char statusText[256];
		snprintf(
			statusText, 256,
			"%02u:%02u - %.2f FPS (%d\u00D7%dpx) (eye: %.2f/%.2f/%.2f) (center: %.2f/%.2f/%.2f)",
			elapsedTime / 60, elapsedTime % 60, fps, width(), height(), e[0], e[1], e[2], c[0], c[1], c[2]
		);
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
		mPathTracer->resetSampleCount();
		mDoRendering = true;
		mRenderStartTime = glutGet( GLUT_ELAPSED_TIME );
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
 * Toggle movement of the sun position.
 */
void GLWidget::toggleSunMovement() {
	mMoveSun = !mMoveSun;
}


/**
 * Toggle rendering of the BVH.
 */
void GLWidget::toggleViewBVH() {
	mViewBVH = !mViewBVH;
}


/**
 * Toggle rendering of the debug image.
 */
void GLWidget::toggleViewDebug() {
	mViewDebug = !mViewDebug;
};


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
