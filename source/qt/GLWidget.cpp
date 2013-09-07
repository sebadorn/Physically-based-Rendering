#include <QtGui>
#include <QtOpenGL>
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


/**
 * Constructor.
 * @param {QWidget*} parent Parent QWidget this QWidget is contained in.
 */
GLWidget::GLWidget( QWidget *parent ) : QGLWidget( QGLFormat( QGL::SampleBuffers ), parent ) {
	mFrameCount = 0;
	mPreviousTime = 0;

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

	CL *cl = new CL();

	mLoadedShapes = GLWidget::loadModel( "resources/models/cornell-box/", "CornellBox-Glossy.obj" );

	mTimer = new QTimer( this );
	connect( mTimer, SIGNAL( timeout() ), this, SLOT( update() ) );
}


/**
 * Destructor.
 */
GLWidget::~GLWidget() {
	mTimer->stop();
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
	glEnableClientState( GL_NORMAL_ARRAY );
	glEnableClientState( GL_COLOR_ARRAY );

	for( uint i = 0; i < mLoadedShapes.size(); i++ ) {
		float colors[mLoadedShapes[i].mesh.positions.size()];
		for( uint j = 0; j < sizeof( colors ) / sizeof( *colors ); j += 3 ) {
			colors[j] = mLoadedShapes[i].material.ambient[0];
			colors[j+1] = mLoadedShapes[i].material.ambient[1];
			colors[j+2] = mLoadedShapes[i].material.ambient[2];
		}

		glVertexPointer( 3, GL_FLOAT, 0, &mLoadedShapes[i].mesh.positions[0] );
		glNormalPointer( GL_FLOAT, 0, &mLoadedShapes[i].mesh.normals[0] );
		glColorPointer( 3, GL_FLOAT, 0, colors );

		glDrawElements(
			GL_TRIANGLES,
			mLoadedShapes[i].mesh.indices.size(),
			GL_UNSIGNED_INT,
			&mLoadedShapes[i].mesh.indices[0]
		);
	}

	glDisableClientState( GL_COLOR_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
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

	mTimer->start( RENDER_INTERVAL );
}


/**
 * Check, if QGLWidget is currently rendering.
 * @return {bool} True, if is rendering, false otherwise.
 */
bool GLWidget::isRendering() {
	return mTimer->isActive();
}


/**
 * Load an OBJ model and its materials.
 * @param  {const char*}              File path and name.
 * @return {vector<tinyobj::shape_t>} The loaded model.
 */
std::vector<tinyobj::shape_t> GLWidget::loadModel( std::string filepath, std::string filename ) {
	// std::string inputfile = "resources/models/cornell-box/CornellBox-Glossy.obj";
	std::vector<tinyobj::shape_t> shapes;

	std::string err = tinyobj::LoadObj( shapes, ( filepath + filename ).c_str(), filepath.c_str() );

	if( !err.empty() ) {
		std::cerr << err << std::endl;
		exit( 1 );
	}

	std::cout << "* [GLWidget] Model \"" << filename << "\" loaded." << std::endl;

	return shapes;
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
 * @param width  {int} width  New width of the QWidget.
 * @param height {int} height New height of the QWidget.
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

		char statusText[20];
		snprintf( statusText, 20, "%.2f FPS", fps );
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
