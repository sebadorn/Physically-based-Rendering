#include "GLWidget.h"

using namespace std;


/**
 * Constructor.
 * @param {QWidget*} parent Parent QWidget this QWidget is contained in.
 */
GLWidget::GLWidget( QWidget* parent ) : QGLWidget( parent ) {
	CL* mCl = new CL();

	mProjectionMatrix = glm::perspective( FOV, WIDTH / HEIGHT, ZNEAR, ZFAR );

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

	mModelMatrix = glm::mat4( 1.0f );

	mModelViewMatrix = mViewMatrix * mModelMatrix;

	// mNormalMatrix = glm::inverseTranspose( glm::mat3( mModelViewMatrix ) );
	// If no scaling is involved:
	mNormalMatrix = glm::mat3( mModelViewMatrix );

	mModelViewProjectionMatrix = mProjectionMatrix * mViewMatrix * mModelMatrix;
}


/**
 * The camera has changed. Handle it.
 */
void GLWidget::cameraUpdate() {
	this->calculateMatrices();
}


/**
 * Draw the main objects of the scene.
 */
void GLWidget::drawScene() {
	for( GLuint i = 0; i < mVA.size(); i++ ) {
		GLfloat useTexture = 1.0f;
		if( mTextureIDs.count( mVA[i] ) > 0 ) {
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
 * Initialize OpenGL and start rendering.
 */
void GLWidget::initializeGL() {
	glClearColor( 0.9f, 0.9f, 0.9f, 0.0f );

	glEnable( GL_DEPTH_TEST );

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

	this->loadShader( vertexShader, "source/shader/phong.vert" );
	this->loadShader( fragmentShader, "source/shader/phong.frag" );

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
	ModelLoader* ml = new ModelLoader();

	mVA = ml->loadModelIntoBuffers( filepath, filename );
	mNumIndices = ml->getNumIndices();
	mTextureIDs = ml->getTextureIDs();

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
	char logBuffer[1000];
	glGetShaderiv( shader, GL_COMPILE_STATUS, &status );

	if( status != GL_TRUE ) {
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

	vector<float> lightPosition = mCamera->getEye();

	GLuint matrixMVP = glGetUniformLocation( mGLProgram, "mModelViewProjectionMatrix" );
	GLuint matrixModelView = glGetUniformLocation( mGLProgram, "mModelViewMatrix" );
	GLuint matrixNormal = glGetUniformLocation( mGLProgram, "mNormalMatrix" );
	GLuint light0 = glGetUniformLocation( mGLProgram, "lightPosition" );

	glUniformMatrix4fv( matrixMVP, 1, GL_FALSE, &mModelViewProjectionMatrix[0][0] );
	glUniformMatrix4fv( matrixModelView, 1, GL_FALSE, &mModelViewMatrix[0][0] );
	glUniformMatrix3fv( matrixNormal, 1, GL_FALSE, &mNormalMatrix[0][0] );
	glUniform3fv( light0, 3, &lightPosition[0] );

	this->drawScene();
	this->showFPS();
}


/**
 * Handle resizing of the QWidget by updating the viewport and perspective.
 * @param {int} width  New width of the QWidget.
 * @param {int} height New height of the QWidget.
 */
void GLWidget::resizeGL( int width, int height ) {
	glViewport( 0, 0, width, height );
	mProjectionMatrix = glm::perspective( FOV, width / (float) height, ZNEAR, ZFAR );
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
	return QSize( WIDTH, HEIGHT );
}


/**
 * Start or resume rendering.
 */
void GLWidget::startRendering() {
	if( !mDoRendering ) {
		mDoRendering = true;
		mTimer->start( RENDER_INTERVAL );
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
