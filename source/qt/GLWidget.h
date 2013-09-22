#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <algorithm>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <map>
#include <vector>
#include <QGLWidget>

#include "../Camera.h"
#include "../CL.h"
#include "../Cfg.h"
#include "../Logger.h"
#include "../ModelLoader.h"
#include "../utils.h"
#include "Window.h"

#ifndef GL_MULTISAMPLE
	#define GL_MULTISAMPLE 0x809D
#endif


class Camera;


class GLWidget : public QGLWidget {

	Q_OBJECT

	public:
		GLWidget( QWidget* parent );
		~GLWidget();
		void cameraUpdate();
		bool isRendering();
		void loadModel( std::string filepath, std::string filename );
		QSize minimumSizeHint() const;
		QSize sizeHint() const;
		void startRendering();
		void stopRendering();

		Camera* mCamera;

	protected:
		void calculateMatrices();
		void deleteOldModel();
		void drawScene();
		void initializeGL();
		void initShaders();
		void loadShader( GLuint shader, std::string path );
		void paintGL();
		void resizeGL( int width, int height );
		void showFPS();

	private:
		bool mDoRendering;
		GLuint mFrameCount;
		GLuint mGLProgram;
		GLuint mIndexBuffer;
		GLuint mPreviousTime;
		CL* mCl;
		QTimer* mTimer;
		std::vector<GLuint> mNumIndices;
		std::map<GLuint, GLuint> mTextureIDs;
		std::vector<GLuint> mVA;
		glm::mat3 mNormalMatrix;
		glm::mat4 mModelMatrix;
		glm::mat4 mModelViewMatrix;
		glm::mat4 mModelViewProjectionMatrix;
		glm::mat4 mProjectionMatrix;
		glm::mat4 mViewMatrix;

};

#endif
