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
#define NUM_VA 4
// Vertex array for path tracer texture
#define VA_TRACER 0
// Vertex array for model overlay (for position comparison)
#define VA_OVERLAY 1
// Vertex array for bounding box of the model
#define VA_BOUNDINGBOX 2
// Vertex array for kd-tree visualization
#define VA_KDTREE 3


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
		void clInitRays();
		void clFindIntersections( float timeSinceStart );
		void clAccumulateColors( float timeSinceStart );
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
		void setShaderBuffersForKdTree( std::vector<GLfloat> vertices, std::vector<GLuint> indices );
		void setShaderBuffersForTracer();
		void showFPS();

	protected slots:
		void toggleViewBoundingBox();
		void toggleViewKdTree();
		void toggleViewOverlay();
		void toggleViewTracer();

	private:
		bool mDoRendering;
		int mSelectedLight;
		cl_float mFOV;
		cl_float* mBoundingBox;

		bool mViewBoundingBox;
		bool mViewKdTree;
		bool mViewOverlay;
		bool mViewTracer;

		GLuint mFrameCount;
		GLuint mGLProgramTracer;
		GLuint mGLProgramSimple;
		GLuint mIndexBuffer;
		GLuint mPreviousTime;
		GLuint mSampleCount;

		CL* mCL;
		QTimer* mTimer;
		KdTree* mKdTree;
		GLuint mKdTreeNumIndices;

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
		std::vector<GLuint> mFaces;
		std::vector<GLfloat> mNormals;

		std::vector<float> mTextureOut;

		cl_mem mBufScFaces;
		cl_mem mBufScVertices;
		cl_mem mBufScNormals;
		cl_mem mBufBoundingBox;

		cl_mem mBufKdNodes;
		cl_mem mBufKdNodeData1;
		cl_mem mBufKdNodeData2;
		cl_mem mBufKdNodeData3;

		cl_mem mBufEye;
		cl_mem mBufVecW;
		cl_mem mBufVecU;
		cl_mem mBufVecV;

		cl_mem mBufOrigins;
		cl_mem mBufRays;
		cl_mem mBufNormals;
		cl_mem mBufAccColors;
		cl_mem mBufColorMasks;

		cl_mem mKernelArgTextureIn;
		cl_mem mKernelArgTextureOut;

		cl_kernel mKernelColors;
		cl_kernel mKernelIntersections;
		cl_kernel mKernelRays;

};

#endif
