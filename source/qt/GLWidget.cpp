#include "GLWidget.h"

using namespace std;


/**
 * Constructor.
 * @param {QWidget*} parent Parent QWidget this QWidget is contained in.
 */
GLWidget::GLWidget( QWidget* parent ) : QGLWidget( parent ) {
	CL* mCl = new CL();

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

	mTimer = new QTimer( this );
	connect( mTimer, SIGNAL( timeout() ), this, SLOT( update() ) );

	this->startRendering();
}


/**
 * Destructor.
 */
GLWidget::~GLWidget() {
	this->stopRendering();
}


/**
 * Calculate the matrices for view, model, model-view-projection and normals.
 */
void GLWidget::calculateMatrices() {
	if( !mDoRendering ) {
		return;
	}

	mViewMatrix = glm::lookAt(
		mCamera->getEye_glmVec3(),
		mCamera->getAdjustedCenter_glmVec3(),
		mCamera->getUp_glmVec3()
	);

	// mModelMatrix = glm::mat4( 1.0f );

	// If no scaling is involved:
	mNormalMatrix = glm::mat3( mModelMatrix );
	// else:
	// mNormalMatrix = glm::inverseTranspose( glm::mat3( mModelMatrix ) );

	mModelViewProjectionMatrix = mProjectionMatrix * mViewMatrix * mModelMatrix;
}


/**
 * The camera has changed. Handle it.
 */
void GLWidget::cameraUpdate() {
	this->calculateMatrices();
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
 * Initialize OpenGL and start rendering.
 */
void GLWidget::initializeGL() {
	glClearColor( 0.9f, 0.9f, 0.9f, 0.0f );

	glEnable( GL_DEPTH_TEST );
	glEnable( GL_MULTISAMPLE );

	glEnable( GL_ALPHA_TEST );
	glAlphaFunc( GL_ALWAYS, 0.0f );

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	this->initShaders();

	Logger::logInfo( string( "[OpenGL] Version " ).append( (char*) glGetString( GL_VERSION ) ) );
	Logger::logInfo( string( "[OpenGL] GLSL " ).append( (char*) glGetString( GL_SHADING_LANGUAGE_VERSION ) ) );
}


/**
 * Load and compile the shader.
 */
void GLWidget::initShaders() {
	GLenum err = glewInit();

	if( err != GLEW_OK ) {
		Logger::logError( string( "[GLEW] Init failed: " ).append( (char*) glewGetErrorString( err ) ) );
		exit( 1 );
	}
	Logger::logInfo( string( "[GLEW] Version " ).append( (char*) glewGetString( GLEW_VERSION ) ) );

	mGLProgram = glCreateProgram();
	GLuint vertexShader = glCreateShader( GL_VERTEX_SHADER );
	GLuint fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );

	string shaderPath = Cfg::get().value<string>( Cfg::SHADER_PATH );
	shaderPath.append( Cfg::get().value<string>( Cfg::SHADER_NAME ) );

	this->loadShader( vertexShader, shaderPath + string( ".vert" ) );
	this->loadShader( fragmentShader, shaderPath +string( ".frag" ) );

	glLinkProgram( mGLProgram );
	glUseProgram( mGLProgram );
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

	mVA = ml->loadModel( filepath, filename );
	mIndexBuffer = ml->getIndexBuffer();
	mNumIndices = ml->getNumIndices();
	mTextureIDs = ml->getTextureIDs();
	mLights = ml->getLights();

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
		Logger::logError( string( "[Shader]\n" ).append( logBuffer ) );
		exit( 1 );
	}

	glAttachShader( mGLProgram, shader );
}


/**
 * Set a minimum width and height for the QWidget.
 * @return {QSize} Minimum width and height.
 */
QSize GLWidget::minimumSizeHint() const {
	return QSize( 50, 50 );
}


/**
 * Draw the scene.
 */
void GLWidget::paintGL() {
	if( !mDoRendering ) {
		return;
	}

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


	glUniformMatrix4fv(
		glGetUniformLocation( mGLProgram, "modelViewProjectionMatrix" ), 1, GL_FALSE, &mModelViewProjectionMatrix[0][0]
	);
	glUniformMatrix4fv(
		glGetUniformLocation( mGLProgram, "modelMatrix" ), 1, GL_FALSE, &mModelMatrix[0][0]
	);
	glUniformMatrix4fv(
		glGetUniformLocation( mGLProgram, "viewMatrix" ), 1, GL_FALSE, &mViewMatrix[0][0]
	);
	glUniformMatrix3fv(
		glGetUniformLocation( mGLProgram, "normalMatrix" ), 1, GL_FALSE, &mNormalMatrix[0][0]
	);
	glUniform3fv(
		glGetUniformLocation( mGLProgram, "cameraPosition" ), 3, &(mCamera->getEye())[0]
	);


	// Light(s)

	glUniform1i( glGetUniformLocation( mGLProgram, "numLights" ), mLights.size() );
	char lightName1[20];
	char lightName2[20];

	for( uint i = 0; i < mLights.size(); i++ ) {
		snprintf( lightName1, 20, "light%uData1", i );
		snprintf( lightName2, 20, "light%uData2", i );

		float lightData1[16] = {
			mLights[i].position[0], mLights[i].position[1], mLights[i].position[2], mLights[i].position[3],
			mLights[i].diffuse[0], mLights[i].diffuse[1], mLights[i].diffuse[2], mLights[i].diffuse[3],
			mLights[i].specular[0], mLights[i].specular[1], mLights[i].specular[2], mLights[i].specular[3],
			mLights[i].constantAttenuation, mLights[i].linearAttenuation, mLights[i].quadraticAttenuation, mLights[i].spotCutoff
		};
		float lightData2[4] = {
			mLights[i].spotExponent, mLights[i].spotDirection[0], mLights[i].spotDirection[1], mLights[i].spotDirection[2]
		};

		glUniformMatrix4fv( glGetUniformLocation( mGLProgram, lightName1 ), 1, GL_FALSE, &lightData1[0] );
		glUniform4fv( glGetUniformLocation( mGLProgram, lightName2 ), 1, &lightData2[0] );
	}


	this->paintScene();
	this->showFPS();
}


/**
 * Draw the main objects of the scene.
 */
void GLWidget::paintScene() {
	for( GLuint i = 0; i < mVA.size(); i++ ) {
		GLfloat useTexture = 1.0f;
		if( mTextureIDs.find( mVA[i] ) != mTextureIDs.end() ) {
			glBindTexture( GL_TEXTURE_2D, mTextureIDs[mVA[i]] );
		}
		else {
			glBindTexture( GL_TEXTURE_2D, 0 );
			useTexture = 0.0f;
		}

		glUniform1f( glGetUniformLocation( mGLProgram, "useTexture_vert" ), useTexture );

		glBindVertexArray( mVA[i] );
		glDrawElements( GL_TRIANGLES, mNumIndices[i], GL_UNSIGNED_INT, 0 );
	}

	glBindVertexArray( 0 );
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
