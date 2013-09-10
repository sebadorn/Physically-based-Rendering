#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>
#include <unistd.h>

#include "../CL.h"
#include "../tinyobjloader/tiny_obj_loader.h"


typedef struct {
	float eyeX, eyeY, eyeZ;
	float centerX, centerY, centerZ;
	float upX, upY, upZ;
	float rotX, rotY;
} camera_t;


class GLWidget : public QGLWidget {

	Q_OBJECT

	public:
		GLWidget( QWidget* parent );
		~GLWidget();
		void cameraMoveBackward();
		void cameraMoveDown();
		void cameraMoveForward();
		void cameraMoveLeft();
		void cameraMoveRight();
		void cameraMoveUp();
		void cameraReset();
		bool isRendering();
		void loadModel( std::string filepath, std::string filename );
		QSize minimumSizeHint() const;
		QSize sizeHint() const;
		void startRendering();
		void stopRendering();
		void updateCameraRot( int moveX, int moveY );

	protected:
		void drawAxis();
		void drawScene();
		void initializeGL();
		void initShader();
		void paintGL();
		void resizeGL( int width, int height );
		void showFPS();

	private:
		bool mDoRendering;
		uint mFrameCount;
		uint mPreviousTime;
		GLuint mGLProgram;
		camera_t mCamera;
		std::vector<tinyobj::shape_t> mLoadedShapes;
		QTimer* mTimer;
		CL* mCl;

};

#endif
