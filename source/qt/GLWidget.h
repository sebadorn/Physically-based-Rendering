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
#include "../KdTree.h"
#include "../Logger.h"
#include "../ModelLoader.h"
#include "../utils.h"
#include "Window.h"

#ifndef GL_MULTISAMPLE
	#define GL_MULTISAMPLE 0x809D
#endif

// Number of vertex arrays
#define NUM_VA 3
// Vertex array for path tracer texture
#define VA_TRACER 0
// Vertex array for model overlay (for position comparison)
#define VA_OVERLAY 1
// Vertex array for bounding box of the model
#define VA_BOUNDINGBOX 2


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

		static const GLuint ATTRIB_POINTER_VERTEX = 0;
		Camera* mCamera;

	protected:
		void calculateMatrices();
		void checkGLForErrors();
		void checkFramebufferForErrors();
		void deleteOldModel();
		glm::vec3 getEyeRay( glm::mat4 matrix, glm::vec3 eye, float x, float y, float z );
		glm::vec3 getJitter();
		void initGlew();
		void initializeGL();
		void initOpenCLBuffers();
		void initShaders();
		void initTargetTexture();
		void loadShader( GLuint program, GLuint shader, std::string path );
		void paintGL();
		void paintScene();
		void resizeGL( int width, int height );
		void setShaderBuffersForOverlay( std::vector<GLfloat> vertices, std::vector<GLuint> indices );
		void setShaderBuffersForBoundingBox( std::vector<GLfloat> bbox );
		void setShaderBuffersForTracer();
		void showFPS();

	private:
		bool mDoRendering;
		int mSelectedLight;
		cl_float mFOV;

		GLuint mFrameCount;
		GLuint mGLProgramTracer;
		GLuint mGLProgramSimple;
		GLuint mIndexBuffer;
		GLuint mPreviousTime;
		GLuint mSampleCount;

		CL* mCL;
		QTimer* mTimer;
		KdTree* mKdTree;

		std::vector<light_t> mLights;
		std::vector<GLuint> mNumIndices;
		std::map<GLuint, GLuint> mTextureIDs;
		std::vector<GLuint> mVA;
		std::vector<GLuint> mTargetTextures;

		glm::mat4 mModelMatrix;
		glm::mat4 mModelViewProjectionMatrix;
		glm::mat4 mProjectionMatrix;
		glm::mat4 mViewMatrix;

		boost::posix_time::ptime mTimeSinceStart;

		std::vector<GLfloat> mVertices;
		std::vector<GLuint> mIndices;
		std::vector<GLfloat> mNormals;

		std::vector<float> mTextureOut;

		cl_mem mBufferIndices;
		cl_mem mBufferVertices;
		cl_mem mBufferNormals;
		cl_mem mBufferEye;
		cl_mem mBufferVecW;
		cl_mem mBufferVecU;
		cl_mem mBufferVecV;
		cl_mem mKernelArgTextureIn;
		cl_mem mKernelArgTextureOut;

};

#endif
