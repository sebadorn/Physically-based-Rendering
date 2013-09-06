#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>

#include "../tinyobjloader/tiny_obj_loader.h"


typedef struct {
	int rotX, rotY;
	float eyeX, eyeY, eyeZ;
	float centerX, centerY, centerZ;
	float upX, upY, upZ;
} camera_t;


class GLWidget : public QGLWidget {

	Q_OBJECT

	public:
		camera_t mCamera;

		GLWidget( QWidget *parent = 0 );
		~GLWidget();
		void cameraMoveBackward();
		void cameraMoveForward();
		void cameraMoveLeft();
		void cameraMoveRight();
		bool isRendering();
		QSize minimumSizeHint() const;
		QSize sizeHint() const;

	protected:
		void calculateFPS();
		void drawAxis();
		void drawScene();
		void initializeGL();
		std::vector<tinyobj::shape_t> loadModel();
		void paintGL();
		void resizeGL( int width, int height );

	private:
		uint mFrameCount;
		uint mPreviousTime;
		std::vector<tinyobj::shape_t> mLoadedShapes;
		QTimer *mTimer;

};

#endif
