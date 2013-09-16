#include <assimp/postprocess.h>
#include <cmath>
#include <GL/glew.h>
#include <GL/glut.h>
#include <iostream>
#include <QtGui>
#include <unistd.h>

#include "../CL.h"
#include "../utils.h"
#include "Window.h"
#include "GLWidget.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

#define CAM_MOVE_SPEED 0.5f
#define RENDER_INTERVAL 16.666f
#define SHADER "phong_"


/**
 * Constructor.
 * @param {QWidget*} parent Parent QWidget this QWidget is contained in.
 */
GLWidget::GLWidget( QWidget* parent ) : QGLWidget( QGLFormat( QGL::SampleBuffers ), parent ) {
	CL* mCl = new CL();
	Assimp::Importer mImporter;

	mScene = NULL;
	mDoRendering = false;
	mFrameCount = 0;
	mPreviousTime = 0;
	this->cameraReset();

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
 * Move the camera position backward.
 */
void GLWidget::cameraMoveBackward() {
	mCamera.eyeX += sin( utils::degToRad( mCamera.rotX ) ) * cos( utils::degToRad( mCamera.rotY ) ) * CAM_MOVE_SPEED;
	mCamera.eyeY -= sin( utils::degToRad( mCamera.rotY ) ) * CAM_MOVE_SPEED;
	mCamera.eyeZ -= cos( utils::degToRad( mCamera.rotX ) ) * cos( utils::degToRad( mCamera.rotY ) ) * CAM_MOVE_SPEED;
}


/**
 * Move the camera position downward.
 */
void GLWidget::cameraMoveDown() {
	mCamera.eyeY -= CAM_MOVE_SPEED;
}


/**
 * Move the camera position forward.
 */
void GLWidget::cameraMoveForward() {
	mCamera.eyeX -= sin( utils::degToRad( mCamera.rotX ) ) * cos( utils::degToRad( mCamera.rotY ) ) * CAM_MOVE_SPEED;
	mCamera.eyeY += sin( utils::degToRad( mCamera.rotY ) ) * CAM_MOVE_SPEED;
	mCamera.eyeZ += cos( utils::degToRad( mCamera.rotX ) ) * cos( utils::degToRad( mCamera.rotY ) ) * CAM_MOVE_SPEED;
}


/**
 * Move the camera position to the left.
 */
void GLWidget::cameraMoveLeft() {
	mCamera.eyeX += cos( utils::degToRad( mCamera.rotX ) ) * CAM_MOVE_SPEED;
	mCamera.eyeZ += sin( utils::degToRad( mCamera.rotX ) ) * CAM_MOVE_SPEED;
}


/**
 * Move the camera position to the right.
 */
void GLWidget::cameraMoveRight() {
	mCamera.eyeX -= cos( utils::degToRad( mCamera.rotX ) ) * CAM_MOVE_SPEED;
	mCamera.eyeZ -= sin( utils::degToRad( mCamera.rotX ) ) * CAM_MOVE_SPEED;
}


/**
 * Move the camera position upward.
 */
void GLWidget::cameraMoveUp() {
	mCamera.eyeY += CAM_MOVE_SPEED;
}


/**
 * Reset the camera position and rotation.
 */
void GLWidget::cameraReset() {
	mCamera.eyeX = 1.0f;
	mCamera.eyeY = 0.0f;
	mCamera.eyeZ = 1.0f;
	mCamera.centerX = 0.0f;
	mCamera.centerY = 0.0f;
	mCamera.centerZ = 0.0f;
	mCamera.upX = 0.0f;
	mCamera.upY = 1.0f;
	mCamera.upZ = 0.0f;
	mCamera.rotX = 0.0f;
	mCamera.rotY = 0.0f;
}


/**
 * Draw an axis.
 */
void GLWidget::drawAxis() {
	glBegin( GL_LINES );
	glColor3d( 255, 0, 0 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 500, 0, 0 );

	glColor3d( 0, 255, 0 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 0, 500, 0 );

	glColor3d( 0, 0, 255 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 0, 0, 500 );
	glEnd();
}


/**
 * Draw the main objects of the scene.
 */
void GLWidget::drawScene() {
	if( mScene == NULL ) {
		return;
	}

	glEnableClientState( GL_VERTEX_ARRAY );
	// glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	for( uint i = 0; i < mScene->mNumMeshes; i++ ) {
		aiMesh* mesh = mScene->mMeshes[i];
		aiMaterial* material = mScene->mMaterials[mesh->mMaterialIndex];

		glVertexPointer( 3, GL_FLOAT, 0, &mesh->mVertices[0] );

		if( mesh->mNormals->Length() > 0 ) {
			glNormalPointer( GL_FLOAT, 0, &mesh->mNormals[0] );
			glEnableClientState( GL_NORMAL_ARRAY );
		}

		aiColor4D aiAmbient( 0.0f, 0.0f, 0.0f, 1.0f );
		material->Get( AI_MATKEY_COLOR_AMBIENT, aiAmbient );

		aiColor4D aiDiffuse( 0.0f, 0.0f, 0.0f, 1.0f );
		material->Get( AI_MATKEY_COLOR_DIFFUSE, aiDiffuse );

		aiColor4D aiSpecular( 0.0f, 0.0f, 0.0f, 1.0f );
		material->Get( AI_MATKEY_COLOR_SPECULAR, aiSpecular );

		float ambient[4] = { aiAmbient[0], aiAmbient[1], aiAmbient[2], aiAmbient[3] };
		float diffuse[4] = { aiDiffuse[0], aiDiffuse[1], aiDiffuse[2], aiDiffuse[3] };
		float specular[4] = { aiSpecular[0], aiSpecular[1], aiSpecular[2], aiSpecular[3] };

		glUniform4fv( glGetUniformLocation( mGLProgram, "ambient" ), 1, ambient );
		glUniform4fv( glGetUniformLocation( mGLProgram, "diffuse" ), 1, diffuse );
		glUniform4fv( glGetUniformLocation( mGLProgram, "specular" ), 1, specular );

		glDrawElements( GL_TRIANGLES, mMeshFacesData[i].size(), GL_UNSIGNED_INT, &(mMeshFacesData[i])[0] );
		glDisableClientState( GL_NORMAL_ARRAY );
	}

	// glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glDisableClientState( GL_VERTEX_ARRAY );
}


/**
 * Initialize OpenGL and start rendering.
 */
void GLWidget::initializeGL() {
	glClearColor( 0.9f, 0.9f, 0.9f, 0.0f );

	glEnable( GL_DEPTH_TEST );
	glEnable( GL_MULTISAMPLE );
	glEnable( GL_LINE_SMOOTH );

	this->initShader();
}


/**
 * Load and compile the shader.
 */
void GLWidget::initShader() {
	GLenum err = glewInit();

	if( err != GLEW_OK ) {
		std::cerr << "* [GLEW] Init failed: " << glewGetErrorString( err ) << std::endl;
		exit( 1 );
	}

	std::cout << "* [GLEW] Using version " << glewGetString( GLEW_VERSION ) << std::endl;

	std::string shaderString;

	mGLProgram = glCreateProgram();
	GLuint vertexShader = glCreateShader( GL_VERTEX_SHADER );
	GLuint fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );

	std::string path = "source/shader/";
	path.append( SHADER );

	shaderString = utils::loadFileAsString( ( path + "vertex.glsl" ).c_str() );
	const GLchar* shaderSourceVertex = shaderString.c_str();
	const GLint shaderLengthVertex = shaderString.size();
	glShaderSource( vertexShader, 1, &shaderSourceVertex, &shaderLengthVertex );
	glCompileShader( vertexShader );
	glAttachShader( mGLProgram, vertexShader );

	shaderString = utils::loadFileAsString( ( path + "fragment.glsl" ).c_str() );
	const GLchar* shaderSourceFragment = shaderString.c_str();
	const GLint shaderLengthFragment = shaderString.size();
	glShaderSource( fragmentShader, 1, &shaderSourceFragment, &shaderLengthFragment );
	glCompileShader( fragmentShader );
	glAttachShader( mGLProgram, fragmentShader );

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


void GLWidget::loadModel( std::string filepath, std::string filename ) {
	const uint flags = aiProcess_JoinIdenticalVertices |
	                   aiProcess_Triangulate |
	                   aiProcess_GenNormals |
	                   aiProcess_SortByPType |
	                   aiProcess_FindInvalidData |
	                   aiProcess_FindDegenerates |
	                   aiProcess_OptimizeMeshes |
	                   aiProcess_SplitLargeMeshes;
	// Doc: http://assimp.sourceforge.net/lib_html/postprocess_8h.html
	// more: aiProcess_GenSmoothNormals |
	//       aiProcess_SplitLargeMeshes |
	//       aiProcess_FixInfacingNormals |
	//       aiProcess_FindDegenerates |
	//       aiProcess_FindInvalidData |
	//       aiProcess_OptimizeMeshes |
	//       aiProcess_OptimizeGraph
	mScene = mImporter.ReadFile( filepath + filename, flags );

	if( !mScene ) {
		std::cerr << "* [GLWidget] Error importing model: " << mImporter.GetErrorString() << std::endl;
		exit( 1 );
	}

	mMeshFacesData.clear();

	for( uint i = 0; i < mScene->mNumMeshes; i++ ) {
		aiMesh* mesh = mScene->mMeshes[i];
		mMeshFacesData.push_back( std::vector<uint>( mesh->mNumFaces * 3 ) );

		for( uint j = 0; j < mesh->mNumFaces; j++ ) {
			const aiFace* face = &mesh->mFaces[j];
			mMeshFacesData[i].push_back( face->mIndices[0] );
			mMeshFacesData[i].push_back( face->mIndices[1] );
			mMeshFacesData[i].push_back( face->mIndices[2] );
		}
	}

	std::cout << "* [GLWidget] Imported model \"" << filename << "\" of " << mScene->mNumMeshes << " meshes." << std::endl;

	this->startRendering();
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
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	glPushMatrix();
	gluLookAt(
		mCamera.eyeX, mCamera.eyeY, mCamera.eyeZ,
		mCamera.eyeX - mCamera.centerX, mCamera.eyeY + mCamera.centerY, mCamera.eyeZ + mCamera.centerZ,
		mCamera.upX, mCamera.upY, mCamera.upZ
	);

	this->drawAxis();
	this->drawScene();
	glPopMatrix();

	this->showFPS();
}


/**
 * Handle resizing of the QWidget by updating the viewport and perspective.
 * @param {int} width  New width of the QWidget.
 * @param {int} height New height of the QWidget.
 */
void GLWidget::resizeGL( int width, int height ) {
	glViewport( 0, 0, width, height );

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( 50, width / (float) height, 0.1, 2000 );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
}


/**
 * Calculate the current framerate and show it in the status bar.
 */
void GLWidget::showFPS() {
	mFrameCount++;

	uint currentTime = glutGet( GLUT_ELAPSED_TIME );
	uint timeInterval = currentTime - mPreviousTime;

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
	return QSize( 1000, 600 );
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


/**
 * Update the viewing direction of the camera, triggered by mouse movement.
 * @param moveX {int} moveX Movement (of the mouse) in 2D X direction.
 * @param moveY {int} moveY Movement (of the mouse) in 2D Y direction.
 */
void GLWidget::updateCameraRot( int moveX, int moveY ) {
	mCamera.rotX -= moveX;
	mCamera.rotY += moveY;

	if( mCamera.rotX >= 360.0f ) {
		mCamera.rotX = 0.0f;
	}
	else if( mCamera.rotX < 0.0f ) {
		mCamera.rotX = 360.0f;
	}

	if( mCamera.rotY > 90 ) {
		mCamera.rotY = 90.0f;
	}
	else if( mCamera.rotY < -90.0f ) {
		mCamera.rotY = -90.0f;
	}

	mCamera.centerX = sin( utils::degToRad( mCamera.rotX ) )
		- fabs( sin( utils::degToRad( mCamera.rotY ) ) )
		* sin( utils::degToRad( mCamera.rotX ) );
	mCamera.centerY = sin( utils::degToRad( mCamera.rotY ) );
	mCamera.centerZ = cos( utils::degToRad( mCamera.rotX ) )
		- fabs( sin( utils::degToRad( mCamera.rotY ) ) )
		* cos( utils::degToRad( mCamera.rotX ) );

	if( mCamera.centerY == 1.0f ) {
		mCamera.upX = sin( utils::degToRad( mCamera.rotX ) );
	}
	else if( mCamera.centerY == -1.0f ) {
		mCamera.upX = -sin( utils::degToRad( mCamera.rotX ) );
	}
	else {
		mCamera.upX = 0.0f;
	}

	if( mCamera.centerY == 1.0f || mCamera.centerY == -1.0f ) {
		mCamera.upY = 0.0f;
	}
	else {
		mCamera.upY = 1.0f;
	}

	if( mCamera.centerY == 1.0f ) {
		mCamera.upZ = -cos( utils::degToRad( mCamera.rotX ) );
	}
	else if( mCamera.centerY == -1.0f ) {
		mCamera.upZ = cos( utils::degToRad( mCamera.rotX ) );
	}
	else {
		mCamera.upZ = 0.0f;
	}
}
