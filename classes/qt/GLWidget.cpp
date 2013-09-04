#include <QtGui>
#include <QtOpenGL>
#include <GL/glut.h>

#include <iostream>
#include <math.h>

#include "../config.h"
#include "Window.h"
#include "GLWidget.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif


GLWidget::GLWidget( QWidget *parent ) : QGLWidget( QGLFormat( QGL::SampleBuffers ), parent ) {
	mFrameCount = 0;
	mPreviousTime = 0;
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


QSize GLWidget::minimumSizeHint() const {
	return QSize( 50, 50 );
}


QSize GLWidget::sizeHint() const {
	return QSize( 1000, 600 );
}


void GLWidget::initializeGL() {
	glClearColor( 0.9f, 0.9f, 0.9f, 0.0f );

	glEnable( GL_DEPTH_TEST );
	glEnable( GL_MULTISAMPLE );
	glEnable( GL_LINE_SMOOTH );

	// glEnable( GL_LIGHTING );
	// glEnable( GL_LIGHT0 );
	// GLfloat lightPosition[4] = { 0.5, 5.0, 7.0, 1.0 };
	// glLightfv( GL_LIGHT0, GL_POSITION, lightPosition );

	mTimer->start( CFG_GL_TIMER );
}


void GLWidget::paintGL() {
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	glPushMatrix();
	gluLookAt( -2, 0, -2,  0, 0, 0,  0, 1, 0 );
	glBegin( GL_LINES );
	glLineWidth( 10 );

	glColor3f( 255, 0, 0 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 500, 0, 0 );

	glColor3f( 0, 255, 0 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 0, 500, 0 );

	glColor3f( 0, 0, 255 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 0, 0, 500 );

	glEnd();
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
