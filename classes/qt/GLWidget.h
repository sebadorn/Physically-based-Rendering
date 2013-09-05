#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>

#include "../tinyobjloader/tiny_obj_loader.h"


class GLWidget : public QGLWidget {

	Q_OBJECT

	public:
		GLWidget( QWidget *parent = 0 );
		~GLWidget();

		void calculateFPS();
		std::vector<tinyobj::shape_t> loadModel();
		QSize minimumSizeHint() const;
		QSize sizeHint() const;

	protected:
		void initializeGL();
		void paintGL();
		void resizeGL( int width, int height );

	private:
		uint mFrameCount;
		uint mPreviousTime;
		std::vector<tinyobj::shape_t> mLoadedShapes;
		QTimer *mTimer;

};

#endif
