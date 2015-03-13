#ifndef GLWIDGET_H
#define GLWIDGET_H

#define GLM_FORCE_RADIANS

#include <algorithm>
#include <ctime>
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <map>
#include <vector>
#include <QGLWidget>

#include "../accelstructures/BVH.h"
#include "../Camera.h"
#include "../CL.h"
#include "../Cfg.h"
#include "../Logger.h"
#include "../ModelLoader.h"
#include "../PathTracer.h"
#include "../utils.h"
#include "InfoWindow.h"
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
// Vertex array for Bounding Volume Hierarchy visualization
#define VA_BVH 2

using std::map;
using std::string;
using std::vector;


class Camera;
class InfoWindow;
class PathTracer;


class GLWidget : public QGLWidget {

	Q_OBJECT

	public:
		GLWidget( QWidget* parent );
		~GLWidget();
		void cameraUpdate();
		void createKernelWindow( CL* cl );
		void destroyKernelWindow();
		bool isRendering();
		void loadModel( string filepath, string filename );
		QSize minimumSizeHint() const;
		void moveCamera( const int key );
		void showKernelWindow();
		QSize sizeHint() const;
		void startRendering();
		void stopRendering();
		void toggleSunMovement();

		static const GLuint ATTRIB_POINTER_VERTEX = 0;
		Camera* mCamera;

	protected:
		void calculateMatrices();
		void checkGLForErrors();
		void checkFramebufferForErrors();
		void deleteOldModel();
		glm::vec3 getEyeRay( glm::mat4 matrix, glm::vec3 eye, float x, float y, float z );
		void initGlew();
		void initializeGL();
		void initShaders();
		void initTargetTexture();
		void loadShader( GLuint program, GLuint shader, string path );
		void paintGL();
		void paintScene();
		void resizeGL( int width, int height );
		void setShaderBuffersForOverlay( vector<GLfloat> vertices, vector<GLuint> indices );
		void setShaderBuffersForBVH( vector<GLfloat> vertices, vector<GLuint> indices );
		void setShaderBuffersForTracer();
		void showFPS();

	protected slots:
		void toggleViewBVH();
		void toggleViewOverlay();
		void toggleViewTracer();

	private:
		bool mDoRendering;
		bool mMoveSun;
		bool mViewBVH;
		bool mViewOverlay;
		bool mViewTracer;

		cl_float mFOV;

		GLuint mBVHNumIndices;
		GLuint mFrameCount;
		GLuint mGLProgramTracer;
		GLuint mGLProgramSimple;
		GLuint mIndexBuffer;
		GLuint mPreviousTime;
		GLuint mRenderStartTime;

		InfoWindow* mInfoWindow;
		QTimer* mTimer;
		PathTracer* mPathTracer;

		vector<GLuint> mNumIndices;
		map<GLuint, GLuint> mTextureIDs;
		vector<GLuint> mVA;
		GLuint mTargetTexture;

		glm::mat4 mModelMatrix;
		glm::mat4 mModelViewProjectionMatrix;
		glm::mat4 mProjectionMatrix;
		glm::mat4 mViewMatrix;

		vector<cl_uint> mFaces;
		vector<cl_float> mNormals;
		vector<cl_float> mTextureOut;
		vector<cl_float> mVertices;

};

#endif
