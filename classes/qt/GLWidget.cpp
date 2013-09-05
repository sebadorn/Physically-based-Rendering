#include <QtGui>
#include <QtOpenGL>
#include <GL/glut.h>

#include <iostream>
#include <unistd.h>
#include <assert.h>

#include "../config.h"
#include "../tinyobjloader/tiny_obj_loader.h"
#include "Window.h"
#include "GLWidget.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif


GLWidget::GLWidget( QWidget *parent ) : QGLWidget( QGLFormat( QGL::SampleBuffers ), parent ) {
	mFrameCount = 0;
	mPreviousTime = 0;

	GLWidget::loadModel();

	mTimer = new QTimer( this );
	connect( mTimer, SIGNAL( timeout() ), this, SLOT( update() ) );
}


void GLWidget::loadModel() {
	std::string inputfile = "resources/tinyobjloader-master/cornell_box.obj";
	std::vector<tinyobj::shape_t> shapes;

	std::string err = tinyobj::LoadObj(shapes, inputfile.c_str(), "resources/tinyobjloader-master/" );

	if (!err.empty()) {
		std::cerr << err << std::endl;
		exit(1);
	}

	std::cout << "# of shapes : " << shapes.size() << std::endl;

	for (size_t i = 0; i < shapes.size(); i++) {
		printf("shape[%ld].name = %s\n", i, shapes[i].name.c_str());
		printf("shape[%ld].indices: %ld\n", i, shapes[i].mesh.indices.size());
		assert((shapes[i].mesh.indices.size() % 3) == 0);
		for (size_t f = 0; f < shapes[i].mesh.indices.size(); f++) {
			printf("  idx[%ld] = %d\n", f, shapes[i].mesh.indices[f]);
		}

		printf("shape[%ld].vertices: %ld\n", i, shapes[i].mesh.positions.size());
		assert((shapes[i].mesh.positions.size() % 3) == 0);
		for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
			printf("  v[%ld] = (%f, %f, %f)\n", v,
			shapes[i].mesh.positions[3*v+0],
			shapes[i].mesh.positions[3*v+1],
			shapes[i].mesh.positions[3*v+2]);
		}

		printf("shape[%ld].material.name = %s\n", i, shapes[i].material.name.c_str());
		printf("  material.Ka = (%f, %f ,%f)\n", shapes[i].material.ambient[0], shapes[i].material.ambient[1], shapes[i].material.ambient[2]);
		printf("  material.Kd = (%f, %f ,%f)\n", shapes[i].material.diffuse[0], shapes[i].material.diffuse[1], shapes[i].material.diffuse[2]);
		printf("  material.Ks = (%f, %f ,%f)\n", shapes[i].material.specular[0], shapes[i].material.specular[1], shapes[i].material.specular[2]);
		printf("  material.Tr = (%f, %f ,%f)\n", shapes[i].material.transmittance[0], shapes[i].material.transmittance[1], shapes[i].material.transmittance[2]);
		printf("  material.Ke = (%f, %f ,%f)\n", shapes[i].material.emission[0], shapes[i].material.emission[1], shapes[i].material.emission[2]);
		printf("  material.Ns = %f\n", shapes[i].material.shininess);
		printf("  material.map_Ka = %s\n", shapes[i].material.ambient_texname.c_str());
		printf("  material.map_Kd = %s\n", shapes[i].material.diffuse_texname.c_str());
		printf("  material.map_Ks = %s\n", shapes[i].material.specular_texname.c_str());
		printf("  material.map_Ns = %s\n", shapes[i].material.normal_texname.c_str());
		std::map<std::string, std::string>::iterator it(shapes[i].material.unknown_parameter.begin());
		std::map<std::string, std::string>::iterator itEnd(shapes[i].material.unknown_parameter.end());
		for (; it != itEnd; it++) {
			printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
		}
		printf("\n");
	}
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
