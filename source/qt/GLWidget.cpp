#include <QtGui>
#include <QtOpenGL>
#include <GL/glut.h>

#include <iostream>
#include <unistd.h>
#include <assert.h>

#include "../config.h"
#include "../tinyobjloader/tiny_obj_loader.h"
#include "../utils.h"
#include "Window.h"
#include "GLWidget.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif


GLWidget::GLWidget( QWidget *parent ) : QGLWidget( QGLFormat( QGL::SampleBuffers ), parent ) {
	mFrameCount = 0;
	mPreviousTime = 0;

	mCamera.eyeX = 0.0f;
	mCamera.eyeY = 0.8f;
	mCamera.eyeZ = 3.0f;
	mCamera.centerX =  0.0f;
	mCamera.centerY =  0.8f;
	mCamera.centerZ = -1.0f;
	mCamera.upX = 0.0f;
	mCamera.upY = 1.0f;
	mCamera.upZ = 0.0f;
	mCamera.rotX = 0;
	mCamera.rotY = 0;

	mLoadedShapes = GLWidget::loadModel();

	mTimer = new QTimer( this );
	connect( mTimer, SIGNAL( timeout() ), this, SLOT( update() ) );
}


GLWidget::~GLWidget() {
	mTimer->stop();
}


void GLWidget::calculateFPS() {
	mFrameCount++;

	uint currentTime = glutGet( GLUT_ELAPSED_TIME );
	uint timeInterval = currentTime - mPreviousTime;

	if( timeInterval > 1000 ) {
		float fps = (float) mFrameCount / (float) timeInterval * 1000.0f;
		mPreviousTime = currentTime;
		mFrameCount = 0;
		char statusText[20];
		snprintf( statusText, 20, "%.2f FPS", fps );
		( (Window*) parentWidget() )->updateStatus( statusText );
	}
}


void GLWidget::cameraMoveBackward() {
	float speed = 1.0f;
	mCamera.eyeX += sin( utils::degToRad( mCamera.rotX ) ) * cos( utils::degToRad( mCamera.rotY ) ) * speed;
	mCamera.eyeY -= sin( utils::degToRad( mCamera.rotY ) ) * speed;
	mCamera.eyeZ -= cos( utils::degToRad( mCamera.rotX ) ) * cos( utils::degToRad( mCamera.rotY ) ) * speed;
}


void GLWidget::cameraMoveForward() {
	float speed = 1.0f;
	mCamera.eyeX -= sin( utils::degToRad( mCamera.rotX ) ) * cos( utils::degToRad( mCamera.rotY ) ) * speed;
	mCamera.eyeY += sin( utils::degToRad( mCamera.rotY ) ) * speed;
	mCamera.eyeZ += cos( utils::degToRad( mCamera.rotX ) ) * cos( utils::degToRad( mCamera.rotY ) ) * speed;
}


void GLWidget::cameraMoveLeft() {
	float speed = 1.0f;
	mCamera.eyeX += cos( utils::degToRad( mCamera.rotX ) ) * speed;
	mCamera.eyeZ += sin( utils::degToRad( mCamera.rotX ) ) * speed;
}


void GLWidget::cameraMoveRight() {
	float speed = 1.0f;
	mCamera.eyeX -= cos( utils::degToRad( mCamera.rotX ) ) * speed;
	mCamera.eyeZ -= sin( utils::degToRad( mCamera.rotX ) ) * speed;
}


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


void GLWidget::initializeGL() {
	glClearColor( 0.9f, 0.9f, 0.9f, 0.0f );

	glEnable( GL_DEPTH_TEST );
	glEnable( GL_MULTISAMPLE );
	glEnable( GL_LINE_SMOOTH );

	mTimer->start( CFG_GL_TIMER );
}


bool GLWidget::isRendering() {
	return mTimer->isActive();
}


std::vector<tinyobj::shape_t> GLWidget::loadModel() {
	std::string inputfile = "resources/models/cornell-box/CornellBox-Glossy.obj";
	std::vector<tinyobj::shape_t> shapes;

	std::string err = tinyobj::LoadObj( shapes, inputfile.c_str(), "resources/models/cornell-box/" );

	if( !err.empty() ) {
		std::cerr << err << std::endl;
		exit( 1 );
	}

	return shapes;
}


QSize GLWidget::minimumSizeHint() const {
	return QSize( 50, 50 );
}


void GLWidget::paintGL() {
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	glPushMatrix();
	gluLookAt(
		mCamera.eyeX, mCamera.eyeY, mCamera.eyeZ,
		mCamera.eyeX + mCamera.centerX, mCamera.eyeY + mCamera.centerY, mCamera.eyeZ + mCamera.centerZ,
		mCamera.upX, mCamera.upY, mCamera.upZ
	);
	this->drawAxis();
	this->drawScene();
	glPopMatrix();

	this->calculateFPS();
}


void GLWidget::resizeGL( int width, int height ) {
	glViewport( 0, 0, width, height );

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( 50, width / (float) height, 0.1, 2000 );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
}


QSize GLWidget::sizeHint() const {
	return QSize( 1000, 600 );
}
