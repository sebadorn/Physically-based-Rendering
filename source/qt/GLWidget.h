#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <ctime>
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
		void moveCamera( const int key );
		QSize sizeHint() const;
		void selectNextLight();
		void selectPreviousLight();
		void startRendering();
		void stopRendering();
		void toggleLightControl();

		Camera* mCamera;

	protected:
		void calculateMatrices();
		void deleteOldModel();
		glm::vec3 getEyeRay( glm::mat4 matrix, float x, float y );
		glm::mat4 getJitterMatrix( glm::vec3 v );
		void initGlew();
		void initializeGL();
		void initShaders();
		void initTargetTexture();
		void loadShader( GLuint shader, std::string path );
		void paintGL();
		void paintScene();
		void resizeGL( int width, int height );
		std::string shaderReplacePlaceholders( std::string shaderString );
		void showFPS();

	private:
		bool mDoRendering;
		int mSelectedLight;

		GLuint mFrameCount;
		GLuint mGLProgramRender;
		GLuint mGLProgramTracer;
		GLuint mIndexBuffer;
		GLuint mPreviousTime;
		GLuint mSampleCount;

		CL* mCl;
		QTimer* mTimer;

		std::vector<light_t> mLights;
		std::vector<GLuint> mNumIndices;
		std::map<GLuint, GLuint> mTextureIDs;
		std::vector<GLuint> mVA;
		std::vector<GLuint> mTargetTextures;

		glm::mat3 mNormalMatrix;
		glm::mat4 mModelMatrix;
		glm::mat4 mModelViewProjectionMatrix;
		glm::mat4 mProjectionMatrix;
		glm::mat4 mViewMatrix;

		boost::posix_time::ptime mTimeSinceStart;

		std::vector<GLfloat> mVertices;
		std::vector<GLint> mIndices;
		std::vector<GLfloat> mNormals;

		GLuint mFramebuffer;
		GLuint mShaderVert;
		GLuint mShaderFrag;
		GLuint mVertexAttribute;
		GLuint mVertexBuffer;

};

#endif
