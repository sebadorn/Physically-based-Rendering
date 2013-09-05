#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>


class GLWidget : public QGLWidget {

	Q_OBJECT

	public:
		GLWidget( QWidget *parent = 0 );
		~GLWidget();

		void calculateFPS();
		void loadModel();
		QSize minimumSizeHint() const;
		QSize sizeHint() const;

	protected:
		void initializeGL();
		void paintGL();
		void resizeGL( int width, int height );

	private:
		uint mFrameCount;
		uint mPreviousTime;
		QTimer *mTimer;

};

#endif
