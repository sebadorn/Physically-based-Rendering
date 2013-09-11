#include <QtGui>
#include <GL/glew.h>
#include <GL/glut.h>

#include <cmath>
#include <iostream>
#include <unistd.h>

#include "../CL.h"
#include "../tinyobjloader/tiny_obj_loader.h"
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

	mFrameCount = 0;
	mPreviousTime = 0;
	this->cameraReset();

	mTimer = new QTimer( this );
	connect( mTimer, SIGNAL( timeout() ), this, SLOT( update() ) );
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
	glEnableClientState( GL_VERTEX_ARRAY );
	// glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	for( uint i = 0; i < mLoadedShapes.size(); i++ ) {
		tinyobj::mesh_t* shapeMesh = &mLoadedShapes[i].mesh;
		tinyobj::material_t* shapeMtl = &mLoadedShapes[i].material;

		glVertexPointer( 3, GL_FLOAT, 0, &shapeMesh->positions[0] );

		if( shapeMesh->normals.size() > 0 ) {
			glNormalPointer( GL_FLOAT, 0, &shapeMesh->normals[0] );
			glEnableClientState( GL_NORMAL_ARRAY );
		}

		float ambient[4] = { shapeMtl->ambient[0], shapeMtl->ambient[1], shapeMtl->ambient[2], 1.0 };
		float diffuse[4] = { shapeMtl->diffuse[0], shapeMtl->diffuse[1], shapeMtl->diffuse[2], 1.0 };
		float specular[4] = { shapeMtl->specular[0], shapeMtl->specular[1], shapeMtl->specular[2], 1.0 };

		glUniform4fv( glGetUniformLocation( mGLProgram, "ambient" ), 1, ambient );
		glUniform4fv( glGetUniformLocation( mGLProgram, "diffuse" ), 1, diffuse );
		glUniform4fv( glGetUniformLocation( mGLProgram, "specular" ), 1, specular );

		glDrawElements( GL_TRIANGLES, shapeMesh->indices.size(), GL_UNSIGNED_INT, &shapeMesh->indices[0] );
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
	this->startRendering();
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


/**
 * Load an OBJ model and its materials.
 * @param {string} filepath Path to the OBJ and MTL file.
 * @param {string} filename Name of the OBJ file, including extension.
 */
void GLWidget::loadModel( std::string filepath, std::string filename ) {
	std::vector<tinyobj::shape_t> shapes;
	std::string err = tinyobj::LoadObj( shapes, ( filepath + filename ).c_str(), filepath.c_str() );

	if( !err.empty() ) {
		std::cerr << "* [GLWidget] " << err << std::endl;
		exit( 1 );
	}

	std::cout << "* [GLWidget] Model \"" << filename << "\" loaded." << std::endl;

	mLoadedShapes = shapes;
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
	mDoRendering = true;
	mTimer->start( RENDER_INTERVAL );
}


/**
 * Stop rendering.
 */
void GLWidget::stopRendering() {
	mDoRendering = false;
	mTimer->stop();
	( (Window*) parentWidget() )->updateStatus( "Stopped." );
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
